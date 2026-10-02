#pragma once

// Runs SkookumScript source from the mod. The shipping exe still has the Skookum parser: action tree tracks
// and conditions carry script text (`GameplayHelp.show(...)`) that UFG::ScriptCache::GetScript compiles when
// the trees load, and SkookumTask::Begin runs with UFG::SkookumMgr::RunExternalCodeBlock. We call the same
// two functions, on the game thread, from a hook on SkookumScript::update_delta (the scripts' own tick).

#include <cstdint>
#include <string>

namespace skookum
{
	// Finds the functions and hooks the script tick and the Skookum debug print (compile and runtime errors go
	// to our log). Before the game initializes anything; does nothing if a function is missing. With
	// `scriptPrints`, the scripts' Debug.print/println (empty in this build) are pointed at the log once the
	// scripts are loaded: ours and the game's.
	void Install(bool scriptPrints);

	// Whether Install found everything and the tick runs.
	bool Ready();

	// Whether the game's scripts are loaded (the world actor exists), so Start can run something.
	bool Loaded();

	// Called on the game thread every script tick, before the scripts update. Register during Install time.
	using TickFn = void (*)(float delta);
	void OnTick(TickFn fn);

	// One launched code block. Never freed: a running script writes mFinished when it ends, whenever that is.
	struct Run
	{
		std::string mName;
		bool mFinished = false;      // set by the game when a durational block completes
		void* mResult = nullptr;     // SSInstance* of an immediate block's last expression
		void* mWrapper = nullptr;    // SSIExternalMethodCallWrapper* while it runs, else null
		bool mCompiled = false;
		bool mReported = false;
	};

	// Compiles `source` as a code block in the scope of class World and runs it on the World actor (so `this`
	// is the world; the player is `World.c_player`). Game thread only, i.e. from an OnTick callback. Compile
	// errors are logged by the game's own error printer. Returns the run (also when it didn't compile).
	Run* Start(const char* name, const std::string& source);

	// Called for `Debug.println("<tag>", object)` instead of logging the line: lets a script hand an object (e.g. a
	// Vehicle, a UFG::TSVehicle) to the C++ side. One handler per tag, registered at Install time.
	using TagFn = void (*)(const void* instance);
	void OnPrintTag(const char* tag, TagFn fn);

	// A symbol's name from the game's loaded symbol tables, or "?<hex>".
	std::string SymbolName(uint32_t id);

	// Text for a script value: strings, numbers, booleans, nil; other objects as their class. Strings are quoted
	// unless `quoted` is false.
	std::string Describe(const void* instance, bool quoted = true);
}
