// Builds every script the mod runs (each contact in each language, the vendors) and checks it against the rules of
// the game's Skookum parser that a template can break without anyone noticing until the call is made in game:
// - no grouping parentheses. SSParser::parse_expression takes a `(` that starts an expression for a closure's
//   parameter list ("Whitespace required" at `(UI.`, test round 1: GunBackup never compiled and the call hung on
//   "Connected"); `(` only opens an argument list, right after a name. Group with a code block `[a + b]`.
// - brackets balance, literals end, and every {PLACEHOLDER} is filled.
// argv[1] (the .asi) isn't used.

#include "../core/config.cc"
#include "../core/contacts.cc"
#include "../core/log.cc"
#include "../core/text.cc"
#include "../core/vendor.cc"

#include <cctype>
#include <cstdio>

#pragma comment(lib, "user32.lib")  // vendor.cc's key and focus checks

// What the scripts are built with, minus the game.
namespace skookum
{
	bool Ready() { return true; }
	bool Loaded() { return true; }
	void OnTick(TickFn) {}
	Run* Start(const char* name, const std::string&)
	{
		static Run run;
		run.mName = name;
		run.mFinished = true;
		return &run;
	}
	void OnPrintTag(const char*, TagFn) {}
	std::string SymbolName(uint32_t) { return {}; }
	std::string Describe(const void*, bool) { return {}; }
}

namespace phone
{
	void Add(const Contact&) {}
}

namespace items
{
	static const Gun kGun = { "PISTOL_45CAL", 36, "pistol_45cal", 0 };
	const Gun* Find(const std::string&) { return &kGun; }
	void Install() {}
	bool SetGun(const Gun&, int) { return true; }
}

// Empty if the script passes, else what's wrong and where.
static std::string Problem(const std::string& s)
{
	auto at = [&](size_t i, const char* what) {
		const size_t begin = s.rfind('\n', i) == std::string::npos ? 0 : s.rfind('\n', i) + 1;
		const size_t end = s.find('\n', i);
		return std::string(what) + " at offset " + std::to_string(i) + ": " + s.substr(begin, end - begin);
	};
	std::string open;
	for (size_t i = 0; i < s.size(); ++i) {
		const char c = s[i];
		if (c == '"' || c == '\'') {
			const size_t start = i;
			for (++i; i < s.size() && s[i] != c; ++i) {
				if (s[i] == '\\') {
					++i;
				}
			}
			if (i >= s.size()) {
				return at(start, "unterminated literal");
			}
			continue;
		}
		if (c == '/' && i + 1 < s.size() && s[i + 1] == '/') {
			i = s.find('\n', i);
			if (i == std::string::npos) {
				break;
			}
			continue;
		}
		if (c == '(') {
			const unsigned char before = i ? static_cast<unsigned char>(s[i - 1]) : ' ';
			if (!std::isalnum(before) && before != '_' && before != '!' && before != '?') {
				return at(i, "a ( that isn't an argument list (Skookum reads it as a closure)");
			}
		}
		if (c == '(' || c == '[' || c == '{') {
			open.push_back(c);
		}
		else if (c == ')' || c == ']' || c == '}') {
			const char expected = c == ')' ? '(' : c == ']' ? '[' : '{';
			if (open.empty() || open.back() != expected) {
				return at(i, "unbalanced bracket");
			}
			open.pop_back();
		}
	}
	if (!open.empty()) {
		return "unclosed bracket";
	}
	for (size_t i = s.find('{'); i != std::string::npos; i = s.find('{', i + 1)) {
		size_t j = i + 1;
		while (j < s.size() && (std::isupper(static_cast<unsigned char>(s[j])) || s[j] == '_')) {
			++j;
		}
		if (j > i + 1 && j < s.size() && s[j] == '}') {
			return at(i, "unfilled placeholder");
		}
	}
	return {};
}

static int gChecked = 0;

static bool Expect(const std::string& script, const std::string& name)
{
	++gChecked;
	const std::string problem = Problem(script);
	if (!problem.empty()) {
		std::printf("FAIL: %s: %s\n", name.c_str(), problem.c_str());
		return false;
	}
	return true;
}

int main()
{
	// The checker itself: the bug it is here for, and what's fine.
	bool ok = true;
	if (Problem("f((UI.localize_string(\"$X\") + \": a\"))").empty() || Problem("x: a + (b *= 2.0)").empty() ||
		Problem("f(\"{TAG}\")").empty() || Problem("if a [").empty()) {
		std::printf("FAIL: the checker misses a broken script\n");
		ok = false;
	}
	const std::string good = Problem("f(UI.localize_string(\"$X\") + \": (Can't) [\", '{x}', [a + b], {c}, Transform!())");
	if (!good.empty()) {
		std::printf("FAIL: the checker rejects a good script: %s\n", good.c_str());
		ok = false;
	}

	for (const char* language : { "en", "zh-Hant", "zh-Hans" }) {
		text::Install(language);
		for (int kind = 0; kind < contacts::kKinds; ++kind) {
			for (const bool hangUp : { false, true }) {
				for (const int minutes : { 0, 10 }) {
					gConfig.mBackupMinutes = minutes;
					const contacts::Service& service = contacts::gServices[kind];
					ok &= Expect(contacts::Build(static_cast<contacts::Kind>(kind), service, hangUp),
						std::string(service.mKey) + " (" + language + (hangUp ? ", hang-up" : "") + (minutes ? ", time limit" : "") + ")");
				}
			}
		}

		std::string spot = vendor::kSpot;
		text::Replace(spot, "{X}", vendor::Number(-1234.5f));
		text::Replace(spot, "{Y}", vendor::Number(670.1f));
		text::Replace(spot, "{Z}", vendor::Number(18.65f));
		text::Replace(spot, "{FX}", vendor::Number(-1233.8f));
		text::Replace(spot, "{FY}", vendor::Number(669.4f));
		ok &= Expect(vendor::Fill(spot, items::kGun, "vendor 1"), std::string("vendor spot (") + language + ")");
		ok &= Expect(vendor::Fill(vendor::kHere, items::kGun, "test vendor"), std::string("test vendor (") + language + ")");
	}

	std::printf("%d scripts checked\n%s\n", gChecked, ok ? "PASS" : "FAIL");
	return ok ? 0 : 1;
}
