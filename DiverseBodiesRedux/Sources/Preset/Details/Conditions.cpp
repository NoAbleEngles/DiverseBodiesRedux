#include "Conditions.h"
#include <utils/utility.h>
#include <utils/RandomGenerator.h>
#include <filesystem>
#include <fstream>

ConditionSettings::ConditionSettings(RE::Actor::Sex gender) noexcept :
	m_gender(gender){}

ConditionSettings::ConditionSettings(const std::filesystem::path& path) {
	loadFromFile(path);
}

ConditionSettings::ConditionSettings(const std::string& path) {
	loadFromFile(path);
}

ConditionSettings::ConditionSettings(boost::json::object& obj) {
	auto end = obj.end();

	auto it = obj.find("gender");
	if (it != end && it->value().is_int64()) {
		int i = static_cast<int>(it->value().as_int64());
		if (i < 0)
			m_gender = RE::Actor::Sex::None;
		else
			m_gender = i ? RE::Actor::Sex::Female : RE::Actor::Sex::Male;
	}

	it = obj.find("formID");
	if (it != end && it->value().is_string()) {
		std::string i = it->value().as_string().c_str();
		try {
			m_formID = std::stoul(i, nullptr, 16);
		} catch (const std::invalid_argument&) {
			logger::info("Invalid formID value: {}", i);
		} catch (const std::out_of_range&) {
			logger::info("Invalid formID value out of range: {}", i);
		} catch (...) {
			logger::info("Unknown error parsing formID value: {}", i);
		}
	}

	it = obj.find("editorID");
	if (it != end && it->value().is_string()) {
		m_editorID = it->value().as_string().c_str();
	}

	it = obj.find("editorIdContains");
	if (it != end && it->value().is_bool()) {
		m_editorIdContains = it->value().as_bool();
	}

	it = obj.find("inFaction");
	if (it != end && it->value().is_array()) {
		for (const auto& faction : it->value().as_array()) {
			if (faction.is_string()) {
				auto pair = utils::string::split(faction.as_string(), ":");
				if (pair.size() >= 2) {
					if (auto form = RE::fallout_cast<RE::TESFaction*>(getFormFromString(pair[1], pair[0])); form) {
						m_inFaction.emplace_back(form);
					} else {
						logger::info("Invalid faction formID: {}", pair[1]);
					}
				}
			}
		}
	}

	it = obj.find("notInFaction");
	if (it != end && it->value().is_array()) {
		for (const auto& faction : it->value().as_array()) {
			if (faction.is_string()) {
				auto pair = utils::string::split(faction.as_string(), ":");
				if (pair.size() >= 2) {
					if (auto form = RE::fallout_cast<RE::TESFaction*>(getFormFromString(pair[1], pair[0])); form) {
						m_notInFaction.emplace_back(form);
					} else {
						logger::info("Invalid faction formID: {}", pair[1]);
					}
				}
			}
		}
	}

	it = obj.find("hasKeyword");
	if (it != end && it->value().is_array()) {
		for (const auto& keyword : it->value().as_array()) {
			if (keyword.is_string()) {
				auto pair = utils::string::split(keyword.as_string(), ":");
				if (pair.size() >= 2) {
					if (auto form = RE::fallout_cast<RE::BGSKeyword*>(getFormFromString(pair[1], pair[0])); form) {
						m_hasKeyword.emplace_back(form);
					} else {
						logger::info("Invalid keyword formID: {}", pair[1]);
					}
				}
			}
		}
	}

	it = obj.find("hasNotKeyword");
	if (it != end && it->value().is_array()) {
		for (const auto& keyword : it->value().as_array()) {
			if (keyword.is_string()) {
				auto pair = utils::string::split(keyword.as_string(), ":");
				if (pair.size() >= 2) {
					if (auto form = RE::fallout_cast<RE::BGSKeyword*>(getFormFromString(pair[1], pair[0])); form) {
						m_hasNotKeyword.emplace_back(form);
					} else {
						logger::info("Invalid keyword formID: {}", pair[1]);
					}
				}
			}
		}
	}
}

bool ConditionSettings::check(const RE::Actor* actor) const noexcept {
	if (!actor) {
		logger::info("ConditionSettings::check: actor is null");
		return false;
	}

	if (m_gender != RE::Actor::Sex::None && const_cast<RE::Actor*>(actor)->GetSex() != m_gender)
		return false;

	if (m_formID && actor->GetFormID() != m_formID)
		return false;
	
	if (!m_editorID.empty()) { 
		if (!m_editorIdContains && m_editorID.compare(actor->GetFormEditorID()) != 0)
			return false;
		else if (m_editorIdContains && std::string{ actor->GetFormEditorID() }.find(m_editorID) == std::string::npos)
			return false;
	}

	auto HasKeywordInList = [](const RE::Actor* actor, const std::vector<RE::BGSKeyword*>& keywords) {
		
		for (auto kwd : keywords) {
			if (actor->HasKeyword(kwd))
				return true;
		}
		return false;
	};

	if (m_hasKeyword.size() && !HasKeywordInList(actor, m_hasKeyword))
		return false;

	if (m_hasNotKeyword.size() && HasKeywordInList(actor, m_hasNotKeyword))
		return false;

	auto IsInFaction = [](const RE::Actor* actor, const std::vector<RE::TESFaction*>& factions) {
		for (auto f : factions) {
			if (actor->IsInFaction(f))
				return true;
		}
		return false;
	};

	if (m_inFaction.size() && !IsInFaction(actor, m_inFaction))
		return false;

	if (m_notInFaction.size() && IsInFaction(actor, m_notInFaction))
		return false;

	return true;
}

bool ConditionSettings::empty() const noexcept {
	return *this == ConditionSettings{};
}

void ConditionSettings::clear() noexcept {
	*this = std::move(ConditionSettings{});
}

bool ConditionSettings::operator<(const ConditionSettings& other) const noexcept
{
	bool res = static_cast<int>(m_gender) < static_cast<int>(other.m_gender);
	if (m_gender != other.m_gender)
		return res;

	if (m_formID) {
		if (other.m_formID) {
			return m_formID < other.m_formID;
		} else {
			return false;  // this has formID, other does not
		}

	} else if (other.m_formID) {
		return true;  // this does not have formID, other does
	}

	if (!m_editorID.empty()) {
		if (!other.m_editorID.empty()) {
			return m_editorID < other.m_editorID;
		} else {
			return false;  // this has editorID, other does not
		}
	} else if (!other.m_editorID.empty()) {
		return true;  // this does not have editorID, other does
	}

	return res;
}

bool ConditionSettings::loadFromFile(const std::filesystem::path& jsonPath)
{
	auto getJson = [](const std::filesystem::path& filepath) -> std::string {
		std::ifstream file(filepath);
		if (!file) {
			throw std::runtime_error("Could not open file: " + filepath.string());
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	};

	if (!std::filesystem::exists(jsonPath)) {
		logger::info("File does not exist: {}", jsonPath.string());
		return false;
	}

	if (jsonPath.extension() != ".json") {
		return false;
	}

	try {
		std::string jsonString = getJson(jsonPath);
		auto value = boost::json::parse(jsonString);
		if (value.is_object() == false) {
			logger::info("JSON value is not an object: {}", jsonPath.string());
			return false;
		}
		auto jsonValue = std::move(value.as_object());
		*this = std::move(ConditionSettings(jsonValue));
		return true;
	} catch (const std::exception& e) {
		logger::error("Error loading conditions from file: {}", e.what());
		return false;
	}
}

RE::TESForm* getFormFromString(const std::string& xFormID, const std::string& plugin) {
	if (xFormID.empty() || plugin.empty()) {
		return nullptr;
	}
	std::string trimmedID = xFormID.length() > 6 ? xFormID.substr(xFormID.length() - 6) : xFormID;
	trimmedID.erase(trimmedID.begin(), std::find_if(trimmedID.begin(), trimmedID.end(), [](char c) { return c != '0'; }));

	if (!trimmedID.empty() && trimmedID.front() == 'x') {
		trimmedID.erase(trimmedID.begin());
	}

	if (trimmedID.empty()) {
		return nullptr;
	}

	trimmedID = "0x" + trimmedID;

	try {
		uint32_t FormID_uint = std::stoul(trimmedID, nullptr, 16);
		return RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESForm>(FormID_uint, plugin);
	} catch (const std::invalid_argument&) {
		logger::info("Invalid formID value: {}", trimmedID);
		return nullptr;
	} catch (const std::out_of_range&) {
		logger::info("FormID value out of range: {}", trimmedID);
		return nullptr;
	} catch (...) {
		logger::info("Unknown error parsing formID value: {}", trimmedID);
		return nullptr;
	}
}
