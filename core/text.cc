#include "text.hh"

#include <Windows.h>

#include <cstring>
#include <string>

#include "log.hh"
#include "skookum.hh"

namespace text
{
	static Language gLanguage = Language::English;
	static bool gKnown = false;
	static DWORD gLastTry = 0;

	// Characters written differently in the two scripts, both sides in the same order. Seen in the pack's
	// translations of the contacts' names (e.g. 聯絡人 / 联络人).
	static const char* const kTraditional[] = { "聯", "絡", "裝", "術", "車", "專", "業", "槍", "彈", "買", "賣", "這", "個", "們",
		"來", "後", "對", "時", "會", "說", "國", "開", "過", "還", "號", "電", "話", "備", "應" };
	static const char* const kSimplified[] = { "联", "络", "装", "术", "车", "专", "业", "枪", "弹", "买", "卖", "这", "个", "们",
		"来", "后", "对", "时", "会", "说", "国", "开", "过", "还", "号", "电", "话", "备", "应" };

	static int Count(const std::string& text, const char* const* chars, size_t n)
	{
		int count = 0;
		for (size_t i = 0; i < n; ++i) {
			for (size_t at = 0; (at = text.find(chars[i], at)) != std::string::npos; at += std::strlen(chars[i])) {
				++count;
			}
		}
		return count;
	}

	// "[SDUncut:lang]": the game's text for three of the cut contacts' names, joined with '|'.
	static void OnLanguageReport(const void* instance)
	{
		const std::string reply = skookum::Describe(instance, false);
		if (reply.find("PDA_CONTACT") != std::string::npos || reply.find('|') == std::string::npos) {
			LOG("text: the game's text isn't loaded yet (\"%s\"), asking again later", reply.c_str());
			return;
		}
		bool wide = false;
		for (unsigned char c : reply) {
			wide = wide || c >= 0x80;
		}
		if (!wide) {
			gLanguage = Language::English;
		}
		else {
			const int traditional = Count(reply, kTraditional, ARRAYSIZE(kTraditional));
			const int simplified = Count(reply, kSimplified, ARRAYSIZE(kSimplified));
			// The ones in the pack's two editions that both scripts write alike, if any, go to Traditional: what the
			// pack's Cantonese edition is in.
			gLanguage = simplified > traditional ? Language::Simplified : Language::Traditional;
		}
		gKnown = true;
		LOG("text: the game says \"%s\": %s", reply.c_str(), Name(gLanguage));
	}

	void Install(const std::string& setting)
	{
		if (setting == "en") {
			gLanguage = Language::English;
		}
		else if (setting == "zh-Hant" || setting == "zh-TW" || setting == "zh-HK") {
			gLanguage = Language::Traditional;
		}
		else if (setting == "zh-Hans" || setting == "zh-CN") {
			gLanguage = Language::Simplified;
		}
		else {
			if (setting != "auto") {
				LOG("text: unknown Language \"%s\", using auto", setting.c_str());
			}
			skookum::OnPrintTag("[SDUncut:lang]", &OnLanguageReport);
			skookum::OnTick([](float) { Detect(); });
			LOG("text: language from the game's text (auto)");
			return;
		}
		gKnown = true;
		LOG("text: language %s (set in the ini)", Name(gLanguage));
	}

	void Detect()
	{
		if (gKnown || !skookum::Loaded()) {
			return;
		}
		const DWORD now = GetTickCount();
		if (gLastTry && now - gLastTry < 2000) {
			return;
		}
		gLastTry = now;
		// A minute of no answer (another mod holding Debug.println, say): stay with English.
		static int tries = 0;
		if (++tries > 30) {
			gKnown = true;
			LOG("text: no answer from the game's text after %d tries, %s", tries - 1, Name(gLanguage));
			return;
		}
		skookum::Start("language check",
			"Debug.println(\"[SDUncut:lang]\", UI.localize_string(\"$PDA_CONTACT_WEAPON\") + \"|\" + "
			"UI.localize_string(\"$PDA_CONTACT_SWAT\") + \"|\" + UI.localize_string(\"$PDA_CONTACT_GUN\"))");
	}

	Language Current()
	{
		return gLanguage;
	}

	const char* Name(Language language)
	{
		switch (language) {
		case Language::Traditional:
			return "Traditional Chinese";
		case Language::Simplified:
			return "Simplified Chinese";
		default:
			return "English";
		}
	}

	const char* Get(const Line& line)
	{
		switch (gLanguage) {
		case Language::Traditional:
			return line.mTraditional;
		case Language::Simplified:
			return line.mSimplified;
		default:
			return line.mEnglish;
		}
	}

	std::string Literal(const std::string& text)
	{
		std::string literal = "\"";
		for (char c : text) {
			if (c == '\\' || c == '"') {
				literal += '\\';
			}
			literal += c;
		}
		return literal + "\"";
	}

	void Replace(std::string& text, const std::string& what, const std::string& with)
	{
		for (size_t at = 0; (at = text.find(what, at)) != std::string::npos; at += with.size()) {
			text.replace(at, what.size(), with);
		}
	}
}
