#include "skookum.hh"

#include <Windows.h>

#include <MinHook.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "hash.hh"
#include "log.hh"
#include "mem.hh"
#include "scan.hh"

namespace skookum
{
	// Layouts (SkookumScript as linked into the game; see the IDA types):
	// - SSInstance: i_class_p +0x18, i_user_data +0x20 (Integer: int32, Real: float, Boolean: bool, Symbol: ASymbol
	//   id, String: AStringRef*, Vector3: qVector3*). SSActor adds its ANamed name (ASymbol id) at +0x48.
	// - SSClass: its name (ASymbol id = qStringHash32 of the name) at +0x8.
	// - AStringRef: i_cstr_p +0x0, i_length +0x8. AString is a single AStringRef*.
	// - UFG::ScriptCache::Script: mRefCount +0x20, mpScriptCode (SSCode*) +0x28, mpClassScope +0x30.
	using GetScriptFn = void*(__fastcall*)(const char* source, const char* classScope, const char* defaultScope, const char* debugName,
		const void* sourceHash);
	using RunCodeFn = void*(__fastcall*)(void* code, void* scopeClass, void* scope, bool* finished, void** result, const void* origin);
	using UpdateDeltaFn = void(__fastcall*)(float delta);
	using PrintStringFn = void(__fastcall*)(const void* text, bool callPrintFuncs);
	using PrintCStrFn = void(__fastcall*)(const char* text, bool callPrintFuncs);
	using GetClassFn = void*(__fastcall*)(const char* name);
	using AtomicFn = void(__fastcall*)(void* scope, void** result);
	using SymbolNameFn = const char*(__fastcall*)(uint32_t id);

	static GetScriptFn gGetScript = nullptr;
	static RunCodeFn gRunCode = nullptr;
	static UpdateDeltaFn gUpdateDelta = nullptr;
	static PrintStringFn gPrintString = nullptr;
	static PrintCStrFn gPrintCStr = nullptr;
	static GetClassFn gGetClass = nullptr;  // SSBrain::get_class(const char*)
	// UFG::qSymbolLookupStringFromSymbolTableResources: a symbol's name if a loaded symbol table has it, else null.
	// Optional (names in the log only).
	static SymbolNameFn gSymbolName = nullptr;
	static void** gWorld = nullptr;         // SkookumScript::c_world_p (SSActor*), set once the scripts are loaded
	static const void* gOrigin = nullptr;   // ASymbol_origin_embedded2, what SkookumTask passes as the call's origin
	static bool gReady = false;
	static bool gScriptPrints = false;
	static bool gPrintsPatched = false;

	static std::vector<TickFn> gTicks;
	static std::vector<Run*> gRuns;         // launched and not yet reported finished
	static int gPrintedLines = 0;

	using mem::Read;
	using mem::Readable;

	// An AString's text (AStringRef: i_cstr_p +0x0, i_length +0x8), at most `max` characters.
	static std::string StringText(const void* ref, size_t max = 400)
	{
		const char* chars = Read<const char*>(ref, 0x0);
		const size_t length = (std::min)(static_cast<size_t>(Read<uint32_t>(ref, 0x8)), max);
		return chars && Readable(chars, length) ? std::string(chars, length) : std::string();
	}

	// The game's debug print carries Skookum's compile errors (SSDebug::print_error: the error, the source with a
	// marker, the position) and runtime errors. Nothing else prints in this build, but cap it in case something does.
	static void LogPrinted(const char* text)
	{
		if (!text || ++gPrintedLines > 2000) {
			if (gPrintedLines == 2001) {
				LOG("skookum: (more debug prints not logged)");
			}
			return;
		}
		for (const char* line = text; *line;) {
			const char* end = std::strchr(line, '\n');
			const size_t length = end ? static_cast<size_t>(end - line) : std::strlen(line);
			if (length > 0 && !(length == 1 && line[0] == '\r')) {
				LOG("skookum: %.*s", static_cast<int>(length), line);
			}
			line += length;
			if (*line == '\n') {
				++line;
			}
		}
	}

	static void __fastcall PrintStringHook(const void* text, bool callPrintFuncs)
	{
		LogPrinted(StringText(Read<const void*>(text, 0), 8192).c_str());
		gPrintString(text, callPrintFuncs);
	}

	static void __fastcall PrintCStrHook(const char* text, bool callPrintFuncs)
	{
		LogPrinted(text);
		gPrintCStr(text, callPrintFuncs);
	}

	static uint32_t ClassId(const void* instance)
	{
		const void* cls = instance ? Read<const void*>(instance, 0x18) : nullptr;
		return cls ? Read<uint32_t>(cls, 0x8) : 0;
	}

	// Debug.print/println/break({Object} objs_as_strs) are class methods whose SSMethodFunc::i_atomic_f (+0x20) is
	// the linker's shared empty function (ret 0) in this build. Ours log the list's items as text, a line per
	// println/break (print collects until a line break), at most kPrintsPerSecond lines a second.
	static constexpr int kPrintsPerSecond = 100;
	static std::string gPrintLine;
	static DWORD gPrintWindow = 0;
	static int gPrintsInWindow = 0;
	static int gPrintsDropped = 0;

	static void EmitScriptLine(const std::string& line)
	{
		const DWORD now = GetTickCount();
		if (now - gPrintWindow >= 1000) {
			if (gPrintsDropped) {
				LOG("script: (%d lines dropped)", gPrintsDropped);
			}
			gPrintWindow = now;
			gPrintsInWindow = 0;
			gPrintsDropped = 0;
		}
		if (++gPrintsInWindow > kPrintsPerSecond) {
			++gPrintsDropped;
			return;
		}
		LOG("script: %s", line.c_str());
	}

	// Appends a value's text to the pending print line; a List's items one by one, down to `depth` levels.
	static void AppendItems(const void* value, int depth)
	{
		if (!value) {
			return;
		}
		if (depth > 0 && ClassId(value) == hash::String32("List")) {
			const void* list = Read<const void*>(value, 0x20);
			const uint32_t items = (std::min)(Read<uint32_t>(list, 0x0), 64u);
			const void* item = Read<const void*>(list, 0x8);
			for (uint32_t i = 0; item && i < items; ++i) {
				AppendItems(Read<const void*>(item, i * sizeof(void*)), depth - 1);
			}
			return;
		}
		gPrintLine += Describe(value, false);
	}

	struct Tag
	{
		std::string mText;
		TagFn mFn;
	};
	static std::vector<Tag> gTags;

	void OnPrintTag(const char* tag, TagFn fn)
	{
		gTags.push_back({ tag, fn });
	}

	// `Debug.println("<tag>", object)`: the group argument is a List of two, a String and the object.
	static bool HandleTag(const void* arg)
	{
		if (gTags.empty() || !arg || ClassId(arg) != hash::String32("List")) {
			return false;
		}
		const void* list = Read<const void*>(arg, 0x20);
		const void* items = Read<const void*>(list, 0x8);
		if (Read<uint32_t>(list, 0x0) != 2 || !items) {
			return false;
		}
		const void* first = Read<const void*>(items, 0x0);
		if (ClassId(first) != hash::String32("String")) {
			return false;
		}
		const std::string text = StringText(Read<const void*>(first, 0x20));
		for (const Tag& tag : gTags) {
			if (text == tag.mText) {
				tag.mFn(Read<const void*>(items, sizeof(void*)));
				return true;
			}
		}
		return false;
	}

	static void ScriptPrint(void* scope, const char* prefix, bool newline)
	{
		// SSInvokedMethod: the arguments are its SSData array i_data (+0x58 count, +0x60 SSData**; SSData::i_data_p
		// +0x8). A List instance's user data (+0x20) points to its SSList (SSList::mthd_get_at): count +0x0,
		// SSInstance** +0x8.
		const uint32_t count = Read<uint32_t>(scope, 0x58);
		const void* data = Read<const void*>(scope, 0x60);
		const void* arg = count ? Read<const void*>(Read<const void*>(data, 0x0), 0x8) : nullptr;
		if (!gScriptPrints) {
			return;
		}
		gPrintLine += prefix;
		// The parameter is a group ({Object}): the compiler packs the call's arguments into a list, so a list
		// written in the call (`Debug.println({"a", b})`) arrives nested. Flatten that one level.
		AppendItems(arg, 2);
		if (newline) {
			gPrintLine += '\n';
		}
		for (size_t end; (end = gPrintLine.find('\n')) != std::string::npos;) {
			std::string line = gPrintLine.substr(0, end);
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			if (!line.empty()) {
				EmitScriptLine(line);
			}
			gPrintLine.erase(0, end + 1);
		}
		if (gPrintLine.size() > 2000) {
			EmitScriptLine(gPrintLine);
			gPrintLine.clear();
		}
	}

	// What another mod put there before us (SDTaxi does the same), called for everything that isn't our tag.
	static AtomicFn gPreviousPrint = nullptr;
	static AtomicFn gPreviousPrintln = nullptr;
	static AtomicFn gPreviousBreak = nullptr;

	static bool IsOurTag(void* scope)
	{
		const uint32_t count = Read<uint32_t>(scope, 0x58);
		const void* data = Read<const void*>(scope, 0x60);
		const void* arg = count ? Read<const void*>(Read<const void*>(data, 0x0), 0x8) : nullptr;
		return HandleTag(arg);
	}

	static void __fastcall ScriptPrintHook(void* scope, void** result)
	{
		if (gPreviousPrint) {
			gPreviousPrint(scope, result);
		}
		ScriptPrint(scope, "", false);
	}

	static void __fastcall ScriptPrintlnHook(void* scope, void** result)
	{
		if (IsOurTag(scope)) {
			return;
		}
		if (gPreviousPrintln) {
			gPreviousPrintln(scope, result);
		}
		ScriptPrint(scope, "", true);
	}

	static void __fastcall ScriptBreakHook(void* scope, void** result)
	{
		if (gPreviousBreak) {
			gPreviousBreak(scope, result);
		}
		ScriptPrint(scope, "Debug.break: ", true);
	}

	static void PatchPrints()
	{
		gPrintsPatched = true;
		uint8_t* debug = static_cast<uint8_t*>(gGetClass("Debug"));
		if (!debug) {
			LOG("skookum: no class Debug, scripts' prints stay off");
			return;
		}
		// SSClass::i_class_methods (+0xD8): APArray {count +0x0, SSMethodBase** +0x8}; SSMethodBase name +0x8.
		const uint32_t count = Read<uint32_t>(debug, 0xD8);
		const void* methods = Read<const void*>(debug, 0xE0);
		int patched = 0;
		for (uint32_t i = 0; methods && i < count; ++i) {
			uint8_t* method = Read<uint8_t*>(methods, i * sizeof(void*));
			const uint32_t name = method ? Read<uint32_t>(method, 0x8) : 0;
			AtomicFn replacement = name == hash::String32("println") ? &ScriptPrintlnHook
				: name == hash::String32("print")                    ? &ScriptPrintHook
				: name == hash::String32("break")                    ? &ScriptBreakHook
				                                                     : nullptr;
			if (!replacement) {
				continue;
			}
			const uint8_t* current = Read<const uint8_t*>(method, 0x20);
			if (!Readable(current, 3)) {
				LOG("skookum: Debug method %08X has no function, left alone", name);
				continue;
			}
			// Not the shipped empty function: another mod's (SDTaxi patches these too). Ours runs first and hands it
			// everything but our tags.
			if (!scan::Matches(current, "C2 00 00")) {
				const AtomicFn previous = reinterpret_cast<AtomicFn>(const_cast<uint8_t*>(current));
				(replacement == &ScriptPrintlnHook ? gPreviousPrintln : replacement == &ScriptPrintHook ? gPreviousPrint : gPreviousBreak) = previous;
				LOG("skookum: Debug method %08X already replaced (by another mod?), chained", name);
			}
			std::memcpy(method + 0x20, &replacement, sizeof(replacement));
			++patched;
		}
		LOG("skookum: %d of Debug.print/println/break now write to the log", patched);
	}

	static void ReportFinished()
	{
		for (size_t i = 0; i < gRuns.size();) {
			Run* run = gRuns[i];
			if (!run->mFinished) {
				++i;
				continue;
			}
			run->mWrapper = nullptr;
			run->mReported = true;
			LOG("skookum: %s finished", run->mName.c_str());
			gRuns.erase(gRuns.begin() + static_cast<ptrdiff_t>(i));
		}
	}

	static void __fastcall UpdateDeltaHook(float delta)
	{
		// Always patched: tagged printlns are how our scripts hand values back (text.cc's language check). The
		// ScriptPrints setting only decides whether the other lines are logged.
		if (!gPrintsPatched && *gWorld) {
			PatchPrints();
		}
		ReportFinished();
		for (TickFn fn : gTicks) {
			fn(delta);
		}
		gUpdateDelta(delta);
	}

	void OnTick(TickFn fn)
	{
		gTicks.push_back(fn);
	}

	bool Ready()
	{
		return gReady;
	}

	Run* Start(const char* name, const std::string& source)
	{
		Run* run = new Run;
		run->mName = name;
		void* world = gWorld ? *gWorld : nullptr;
		if (!gReady || !world) {
			LOG("skookum: %s not started: the scripts aren't loaded", name);
			run->mFinished = run->mReported = true;
			return run;
		}

		void* script = gGetScript(source.c_str(), "World", "World", name, nullptr);
		void* code = script ? Read<void*>(script, 0x28) : nullptr;
		if (!code) {
			LOG("skookum: %s did not compile (the error is above)", name);
			run->mFinished = run->mReported = true;
			return run;
		}
		run->mCompiled = true;

		// Like SkookumTask::Begin with no sim object: the world actor as `this`, in its own class.
		run->mWrapper = gRunCode(code, Read<void*>(world, 0x18), world, &run->mFinished, &run->mResult, gOrigin);
		if (run->mFinished) {
			run->mWrapper = nullptr;
			run->mReported = true;
			LOG("skookum: %s done: %s", name, Describe(run->mResult).c_str());
		}
		else {
			LOG("skookum: %s running (durational)", name);
			gRuns.push_back(run);
		}
		return run;
	}

	struct ClassName
	{
		uint32_t mId;
		const char* mName;
	};

	static constexpr ClassName kClassNames[] = {
		{ hash::String32("None"), "None" }, { hash::String32("Object"), "Object" }, { hash::String32("String"), "String" },
		{ hash::String32("Integer"), "Integer" }, { hash::String32("Real"), "Real" }, { hash::String32("Boolean"), "Boolean" },
		{ hash::String32("Symbol"), "Symbol" }, { hash::String32("List"), "List" }, { hash::String32("Vector3"), "Vector3" },
		{ hash::String32("Transform"), "Transform" }, { hash::String32("Actor"), "Actor" }, { hash::String32("Character"), "Character" },
		{ hash::String32("Player"), "Player" }, { hash::String32("Vehicle"), "Vehicle" }, { hash::String32("World"), "World" },
		{ hash::String32("SpawnPoint"), "SpawnPoint" }, { hash::String32("GameSlice"), "GameSlice" }, { hash::String32("Prop"), "Prop" },
		{ hash::String32("Class"), "Class" }, { hash::String32("Closure"), "Closure" },
	};

	std::string SymbolName(uint32_t id)
	{
		const char* name = gSymbolName ? gSymbolName(id) : nullptr;
		if (name && Readable(name, 1)) {
			return std::string(name, strnlen(name, 200));
		}
		char text[16];
		snprintf(text, sizeof(text), "?%08X", id);
		return text;
	}

	std::string Describe(const void* instance, bool quoted)
	{
		if (!instance) {
			return "(no value)";
		}
		const uint32_t id = ClassId(instance);
		const char* className = nullptr;
		for (const ClassName& known : kClassNames) {
			if (known.mId == id) {
				className = known.mName;
			}
		}

		char text[512];
		const uint64_t data = Read<uint64_t>(instance, 0x20);
		if (id == hash::String32("None")) {
			return "nil";
		}
		if (id == hash::String32("String")) {
			const std::string chars = StringText(reinterpret_cast<const void*>(data));
			return quoted ? "\"" + chars + "\"" : chars;
		}
		else if (id == hash::String32("Integer")) {
			snprintf(text, sizeof(text), "%d", static_cast<int32_t>(data));
		}
		else if (id == hash::String32("Real")) {
			float value;
			std::memcpy(&value, &data, sizeof(value));
			snprintf(text, sizeof(text), "%g", value);
		}
		else if (id == hash::String32("Boolean")) {
			snprintf(text, sizeof(text), "%s", (data & 0xFF) ? "true" : "false");
		}
		else if (id == hash::String32("Symbol")) {
			const std::string name = SymbolName(static_cast<uint32_t>(data));
			snprintf(text, sizeof(text), "'%s'", name.c_str());
		}
		else if (id == hash::String32("Vector3") && Readable(reinterpret_cast<const void*>(data), 12)) {
			const float* v = reinterpret_cast<const float*>(data);
			snprintf(text, sizeof(text), "Vector3(%.2f, %.2f, %.2f)", v[0], v[1], v[2]);
		}
		else {
			snprintf(text, sizeof(text), "a %s", className ? className : SymbolName(id).c_str());
			// Actors (SSClass::i_superclass_p +0x10 leads to Actor) carry their instance name at +0x48.
			const void* cls = Read<const void*>(instance, 0x18);
			for (int depth = 0; cls && depth < 32; ++depth, cls = Read<const void*>(cls, 0x10)) {
				if (Read<uint32_t>(cls, 0x8) == hash::String32("Actor")) {
					const std::string name = SymbolName(Read<uint32_t>(instance, 0x48));
					return std::string(text) + " '" + name + "'";
				}
			}
		}
		return text;
	}

	void Install(bool scriptPrints)
	{
		gScriptPrints = scriptPrints;
		// Literal patterns straight into FindUnique, so tools\pdb.ps1 verify checks them.
		gGetScript = reinterpret_cast<GetScriptFn>(scan::FindUnique("UFG::ScriptCache::GetScript",
			"48 8B C4 55 41 54 41 55 41 56 41 57 48 8D 68 A9 48 81 EC F0 00 00 00 48 C7 45 8F FE FF FF FF"));
		gRunCode = reinterpret_cast<RunCodeFn>(scan::FindUnique("UFG::SkookumMgr::RunExternalCodeBlock",
			"48 8B C4 56 57 41 56 48 83 EC 60 48 C7 40 A8 FE FF FF FF 48 89 58 08 48 89 68 10 49 8B F1"));
		uint8_t* updateDelta = scan::FindUnique("SkookumScript::update_delta",
			"F2 0F 10 0D ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 5A C0 33 C9 F2 0F 58 C8");
		uint8_t* printString = scan::FindUnique("ADebug::print(AString)", "48 89 5C 24 10 56 48 83 EC 20 0F B6 DA 48 8B F1");
		uint8_t* printCStr = scan::FindUnique("ADebug::print(char)",
			"40 56 57 41 56 48 83 EC 40 48 C7 44 24 30 FE FF FF FF 48 89 5C 24 60 48 89 6C 24 68 0F B6 EA");
		uint8_t* taskBegin = scan::FindUnique("SkookumTask::Begin", "48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 30 48 8B DA");
		gGetClass = reinterpret_cast<GetClassFn>(scan::FindUnique("SSBrain::get_class(char)", "48 83 EC 28 48 8B D1 48 8D 4C 24 38 41 B9 01 00 00 00"));
		gSymbolName = reinterpret_cast<SymbolNameFn>(scan::FindUnique("UFG::qSymbolLookupStringFromSymbolTableResources",
			"40 57 48 83 EC 40 48 C7 44 24 30 FE FF FF FF 48 89 5C 24 50 48 89 74 24 60 8B D9"));
		// mov rax, SkookumScript::c_world_p; ...; lea rcx, ASymbol_origin_embedded2; ...; call RunExternalCodeBlock
		const bool ok = gGetScript && gRunCode && updateDelta && printString && printCStr && taskBegin && gGetClass &&
			scan::Matches(taskBegin + 0xDD, "48 8B 05") && scan::Matches(taskBegin + 0xE8, "48 8D 0D") &&
			scan::Matches(taskBegin + 0x107, "E8") && scan::RipTarget(taskBegin + 0x108) == reinterpret_cast<void*>(gRunCode);
		if (!ok) {
			LOG("skookum: game functions MISSING or not as expected, no scripts");
			return;
		}
		gWorld = static_cast<void**>(scan::RipTarget(taskBegin + 0xE0));
		gOrigin = scan::RipTarget(taskBegin + 0xEB);

		// The prints first: they only log.
		if (MH_CreateHook(printString, reinterpret_cast<void*>(&PrintStringHook), reinterpret_cast<void**>(&gPrintString)) != MH_OK ||
			MH_EnableHook(printString) != MH_OK ||
			MH_CreateHook(printCStr, reinterpret_cast<void*>(&PrintCStrHook), reinterpret_cast<void**>(&gPrintCStr)) != MH_OK ||
			MH_EnableHook(printCStr) != MH_OK ||
			MH_CreateHook(updateDelta, reinterpret_cast<void*>(&UpdateDeltaHook), reinterpret_cast<void**>(&gUpdateDelta)) != MH_OK ||
			MH_EnableHook(updateDelta) != MH_OK) {
			LOG("skookum: hooking the script tick or the debug print failed, no scripts");
			return;
		}
		gReady = true;
		LOG("skookum: hooked the script tick and the debug print%s", gScriptPrints ? "; scripts' Debug.print/println go to the log once loaded" : "");
	}

	bool Loaded()
	{
		return gReady && gWorld && *gWorld;
	}
}
