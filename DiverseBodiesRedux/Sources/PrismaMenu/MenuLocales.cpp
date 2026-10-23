#include "PCH.h"
#include "PrismaMenu/MenuLocales.h"

#include "globals.h"

#include <boost/json.hpp>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <ShlObj.h>
#include <sstream>
#include <Windows.h>

namespace
{
	namespace fs = std::filesystem;

	constexpr auto kLocalesDir = "Data/F4SE/Plugins/DiverseBodiesRedux/locales";

	std::string ToLower(std::string text)
	{
		for (auto& c : text) {
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		return text;
	}

	// sLanguage из ini, которые читает игра: Custom первым (он перекрывает основной). MO2 сам
	// подменяет эти пути в Documents на ini активного профиля.
	std::string LanguageFromIniFiles()
	{
		PWSTR documents = nullptr;
		if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &documents))) {
			return {};
		}
		const fs::path base = fs::path(documents) / L"My Games" / L"Fallout4";
		CoTaskMemFree(documents);

		for (const auto* file : { L"Fallout4Custom.ini", L"Fallout4.ini" }) {
			wchar_t value[32]{};
			GetPrivateProfileStringW(L"General", L"sLanguage", L"", value, 32, (base / file).c_str());
			if (value[0]) {
				std::string result;
				for (const wchar_t* c = value; *c; ++c) {
					result += static_cast<char>(*c);
				}
				return ToLower(result);
			}
		}
		return {};
	}

	const std::string& GameLanguage()
	{
		static const std::string language = [] {
			auto fromFiles = LanguageFromIniFiles();
			logger::info("MenuLocales: game sLanguage '{}'", fromFiles);
			return fromFiles.empty() ? std::string("en") : fromFiles;
		}();
		return language;
	}
}

MenuLocales& MenuLocales::get()
{
	static MenuLocales instance;
	return instance;
}

void MenuLocales::reload()
{
	m_locales.clear();
	m_names.clear();
	std::error_code ec;
	for (const auto& entry : fs::directory_iterator(kLocalesDir, ec)) {
		if (!entry.is_regular_file() || ToLower(entry.path().extension().string()) != ".json") {
			continue;
		}
		try {
			std::ifstream file(entry.path(), std::ios::binary);
			std::stringstream buffer;
			buffer << file.rdbuf();
			auto text = buffer.str();
			if (text.starts_with("\xEF\xBB\xBF")) {
				text.erase(0, 3);
			}
			const auto root = boost::json::parse(text);
			const auto* strings = root.as_object().if_contains("strings");
			if (!strings || !strings->is_object()) {
				logger::warn("MenuLocales: {} has no 'strings' -- skipped", entry.path().filename().string());
				continue;
			}
			const auto id = ToLower(entry.path().stem().string());
			if (const auto* name = root.as_object().if_contains("displayName"); name && name->is_string()) {
				m_names[id] = std::string(name->as_string());
			}
			auto& target = m_locales[id];
			for (const auto& [key, value] : strings->as_object()) {
				if (value.is_string()) {
					target[std::string(key)] = std::string(value.as_string());
				}
			}
		} catch (const std::exception& e) {
			logger::warn("MenuLocales: failed to parse {}: {}", entry.path().filename().string(), e.what());
		}
	}
	m_loaded = true;
	logger::info("MenuLocales: {} locale(s) loaded, game language '{}', active '{}'", m_locales.size(), GameLanguage(), activeId());
}

// Код языка -> код имеющегося перевода ("en", если такого нет).
std::string MenuLocales::resolve(std::string a_wanted) const
{
	// У Fallout 4 для некоторых языков встречалось больше одного кода sLanguage.
	static const std::map<std::string, std::string> kAliases = {
		{ "cn", "zhhant" },  // китайский в OG - традиционный
		{ "zh", "zhhans" },
		{ "kr", "ko" },
		{ "pt", "ptbr" },
		{ "br", "ptbr" },
		{ "mx", "esmx" },
	};
	if (!m_locales.contains(a_wanted)) {
		if (const auto alias = kAliases.find(a_wanted); alias != kAliases.end()) {
			a_wanted = alias->second;
		}
	}
	return m_locales.contains(a_wanted) ? a_wanted : "en";
}

std::string MenuLocales::chosen() const
{
	auto wanted = ToLower(globals::g_ini ? globals::g_ini->at("MENU/sLanguage", "auto"s) : "auto"s);
	return wanted.empty() ? "auto" : wanted;
}

std::string MenuLocales::gameId() const
{
	return resolve(GameLanguage());
}

std::string MenuLocales::activeId() const
{
	const auto wanted = chosen();
	return wanted == "auto" ? gameId() : resolve(wanted);
}

std::vector<MenuLocales::Entry> MenuLocales::list() const
{
	std::vector<Entry> result;
	for (const auto& [id, strings] : m_locales) {
		const auto name = m_names.find(id);
		result.push_back({ id, name != m_names.end() ? name->second : id });
	}
	std::ranges::stable_sort(result, [](const Entry& a, const Entry& b) {
		if ((a.id == "en") != (b.id == "en")) return a.id == "en";
		return a.id < b.id;
	});
	return result;
}

bool MenuLocales::setLanguage(const std::string& a_id)
{
	const auto id = ToLower(a_id);
	if (id != "auto" && !m_locales.contains(id)) {
		return false;
	}
	if (globals::g_ini && !globals::g_ini->set("MENU/sLanguage", id)) {
		logger::warn("MenuLocales: couldn't write [MENU] sLanguage to the ini; the choice applies until the game is closed");
	}
	return true;
}

const std::map<std::string, std::string>& MenuLocales::activeStrings() const
{
	static const std::map<std::string, std::string> kEmpty;
	const auto it = m_locales.find(activeId());
	return it != m_locales.end() ? it->second : kEmpty;
}
