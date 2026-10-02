#pragma once

// The mod's own lines (subtitles, popups, item names) in English, Traditional or Simplified Chinese. The game has
// no Chinese of its own; a Chinese translation pack (SDCNLoc or the pack's CNLocalization.asi) replaces its text
// and fonts. `Language = auto` asks the game how it translates the cut contacts' names (`UI.localize_string`,
// the pack translates them) and picks the script from that; Chinese without the pack's fonts would show boxes.

#include <string>

namespace text
{
	enum class Language
	{
		English,
		Traditional,
		Simplified,
	};

	struct Line
	{
		const char* mEnglish;
		const char* mTraditional;
		const char* mSimplified;
	};

	// Reads the configured language ("auto", "en", "zh-Hant", "zh-Hans") and registers the script tag the check
	// reports through. Call before skookum hooks run (DllMain).
	void Install(const std::string& setting);

	// Game thread: with "auto", asks the game's text until it answers in real words (the dictionary loads after
	// the scripts); runs on every script tick by itself, cheap once known.
	void Detect();

	Language Current();
	const char* Name(Language language);

	const char* Get(const Line& line);

	// `text` as a Skookum string literal: quoted, with backslashes and quotes escaped.
	std::string Literal(const std::string& text);

	// Replaces every `what` in `text` (script templates' {PLACEHOLDERS}).
	void Replace(std::string& text, const std::string& what, const std::string& with);
}
