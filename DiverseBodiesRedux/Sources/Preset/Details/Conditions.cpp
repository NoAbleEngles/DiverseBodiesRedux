#include "Conditions.h"
#include <Utils/utility.h>
#include <filesystem>
#include <fstream>
#include "globals.h"
#include "LooksMenu/ParseLooksMenuPreset.h"

Filter getFilterFromString(const std::string& filterString) noexcept {
	if (filterString.empty()) {
		return Filter::None;
	}

	for (
			Filter filter = getHighestBit(AllFilters); 
			filter != Filter::None; 
			filter = static_cast<Filter>(static_cast<int>(filter) >> 1)
		) 
	{
		if (getFilterString(filter) == filterString) {
			return filter;
		}
	}
	return Filter::None;
}

void for_each_filter(const std::function<void(Filter)>& func) noexcept {
	Filter filter; 
	for (
			filter = getHighestBit(AllFilters); 
			filter != Filter::None; 
			filter = static_cast<Filter>(static_cast<int>(filter) >> 1)
		) 
	{
		func(filter);
	}
}

std::string_view getFilterString(Filter filter) noexcept {
	switch (filter) {
	case Filter::Gender:
		return "Gender";
	case Filter::HasKeyword:
		return "HasKeyword";
	case Filter::HasNotKeyword:
		return "HasNotKeyword";
	case Filter::InFaction:
		return "InFaction";
	case Filter::NotInFaction:
		return "NotInFaction";
	/*case Filter::EditorIDs:
		return "EditorIDs";
	case Filter::NotEditorIDs:
		return "NotEditorIDs";*/
	case Filter::FormIDs:
		return "FormIDs";
	case Filter::NotFormIDs:
		return "NotFormIDs";
	default:
		logger::error("Unknown filter: {}", static_cast<int>(filter));
		return "";
	}
}

constexpr Filter getHighestBit(Filter flags) noexcept {
	int value = static_cast<int>(flags);
	if (value == 0)
		return Filter::None;
	int highest = 1;
	while (value >>= 1)
		highest <<= 1;
	return static_cast<Filter>(highest);
}

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
	auto fault = false;

	auto it = find_ci(obj, "gender");
	if (it != end && it->value().is_int64()) {
		int i = static_cast<int>(it->value().as_int64());
		if (i < 0)
			m_gender = RE::Actor::Sex::None;
		else
			m_gender = i ? RE::Actor::Sex::Female : RE::Actor::Sex::Male;
	}

	// Convert all strings to lowercase before inserting into the set
	auto fillEditorIdStringSet = [](const boost::json::array& arr) {
		std::unordered_set<std::string> set{};
		for (const auto& val : arr) {
			if (val.is_string()) {
				std::string str{ val.as_string().c_str() };
				std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
				set.insert(std::move(str));
			}
		}
		return set;
	};

	auto getFormIDsFromJsonArray = [&fault](boost::json::array& arr)->std::unordered_set<std::uint32_t> {
		std::unordered_set<std::uint32_t> set{};
		for (const auto& formID : arr) {
			if (formID.is_string()) {
				std::string str = formID.as_string().c_str();

				auto pair = utils::string::split(str, "|");
				if (pair.empty())
					continue;
				try {
					if (pair.size() >= 2) {
						auto form = functions::getFormFromString(pair[0], pair[1]);
						if (form && form->As<RE::Actor>() || form->As<RE::TESNPC>()) {
							set.insert(form->GetFormID());
						}
						else {
							fault = true;
							logger::info("Invalid formID: {} in plugin: {}",
								pair[0], pair[1]);
						}
					}
					else {
						auto fid = std::stoul(pair[0], nullptr, 16);
						if (auto form = RE::TESForm::GetFormByID(fid); form && (form->As<RE::Actor>() || form->As<RE::TESNPC>())) {
							set.insert(form->GetFormID());
						}
						else {
							fault = true;
							logger::info("Invalid formID: {} (no plugin set)", pair[0]);
						}
					}
				}
				catch (const std::exception& e) {
					fault = true;
					logger::info("Error parsing formID {} in plugin {}: {}",
						pair[0], (pair.size() < 2) ? "" : pair[1], e.what());
				}
				catch (...) {
					fault = true;
					logger::info("Unknown error parsing formID {} in plugin {}",
						pair[0], (pair.size() < 2) ? "" : pair[1]);
				}
			}
		}
		return set;
	};

	auto getTemplateSetJsonArray = [&fault]<typename T>(boost::json::array& arr)->std::unordered_set<T> {
		std::unordered_set<T> set{};
		for (const auto& arr_element : arr) {
			if (arr_element.is_string()) {
				auto pair = utils::string::split(arr_element.as_string(), "|");
				if (pair.empty()) {
					continue;
				}
				else if (pair.size() >= 2) {
					if (auto form = RE::fallout_cast<T>(functions::getFormFromString(pair[0], pair[1])); form) {
						set.insert(form);
					}
					else {
						fault = true;
						logger::info("Invalid formID: {}|{}", pair[0], pair[1]);
					}
				}
				else if (pair.size() == 1) {
					try {
						auto id = std::stoul(pair[0], nullptr, 16);
						auto tesForm = RE::TESForm::GetFormByID(id);
						if (!tesForm) {
							fault = true;
							logger::info("Invalid formID: {}", pair[0]);
							continue;
						}
						if (auto form = RE::fallout_cast<T>(tesForm); form) {
							set.insert(form);
						}
						else {
							fault = true;
							logger::info("Invalid formID: {}", pair[0]);
						}
					}
					catch (...) {
						auto tesForm = RE::TESForm::GetFormByEditorID(pair[0]);
						if (!tesForm) {
							fault = true;
							logger::info("Invalid formID: {}", pair[0]);
							continue;
						}
						if (auto form = RE::fallout_cast<T>(tesForm); form) {
							set.insert(form);
						}
						else {
							fault = true;
							logger::info("Invalid formID: {}", pair[1]);
						}
					}
				}
			}
		}
		return set;
	};

	static constexpr const char* faultMessageWarn = "In this case, it is just a warning that can be ignored. It implies that you wanted to add additional conditions for a modification that is currently unavailable, but the other conditions are set correctly. Therefore the preset will be kept in the valid list.";
	static constexpr const char* faultMessageError = "In this case, it's a serious error. It implies that you wanted to make some conditions, but you did it incorrectly. To prevent the preset from being applied when it is not needed, the conditions have been reset and the preset will be removed from the valid list.";

	auto checkFault = [this, &fault](const auto& container)->bool {
		if (fault) {
			if (container.empty()) {
				logger::error("No valid formIDs found, please check your conditions file. {}", faultMessageError);
			}
			else {
				logger::warn("Some formIDs are invalid, please check your conditions file. {}", faultMessageWarn);
				fault = false;
			}
		}
		return fault;
	};

	it = find_ci(obj, "formIDs");
	if (it != end && it->value().is_array()) {
		m_formIDs = std::move(getFormIDsFromJsonArray(it->value().as_array()));
	}

	if (checkFault(m_formIDs)) { clear(); return; };

	it = find_ci(obj, "notFormIDs");
	if (it != end && it->value().is_array()) {
		m_notFormIDs = std::move(getFormIDsFromJsonArray(it->value().as_array()));
	}
	
	fault = false; // В данном случае, если formID не найден - предполагаем что это formID из мода, который отсутствует, следовательно условие будет выполнено в любом случае.

	/*it = obj.find("editorIDs");
	if (it != end && it->value().is_array()) {
		m_editorIDs = std::move(fillEditorIdStringSet(it->value().as_array()));
	}

	it = obj.find("notEditorIDs");
	if (it != end && it->value().is_array()) {
		m_notEditorIDs = std::move(fillEditorIdStringSet(it->value().as_array()));
	}

	it = obj.find("editorIDContains");
	if (it != end && it->value().is_bool()) {
		m_editorIdContains = it->value().as_bool();
	}

	it = obj.find("notEditorIDContains");
	if (it != end && it->value().is_bool()) {
		m_notEditorIdContains = it->value().as_bool();
	}*/

	it = find_ci(obj, "inFaction");
	if (it != end && it->value().is_array()) {
		m_inFaction = getTemplateSetJsonArray.operator()<RE::TESFaction*>(it->value().as_array());
	}

	if (checkFault(m_inFaction)) { clear(); return; };

	it = find_ci(obj, "notInFaction");
	if (it != end && it->value().is_array()) {
		m_notInFaction = std::move(getTemplateSetJsonArray.operator()<RE::TESFaction*>(it->value().as_array()));
	}

	fault = false; // В данном случае, если formID не найден - предполагаем что это formID из мода, который отсутствует, следовательно условие будет выполнено в любом случае.

	it = find_ci(obj, "hasKeyword");
	if (it != end && it->value().is_array()) {
		m_hasKeyword = std::move(getTemplateSetJsonArray.operator()<RE::BGSKeyword*>(it->value().as_array()));
	}

	if (checkFault(m_hasKeyword)) { clear(); return; };

	it = find_ci(obj, "hasNotKeyword");
	if (it != end && it->value().is_array()) {
		m_hasNotKeyword = std::move(getTemplateSetJsonArray.operator() < RE::BGSKeyword* > (it->value().as_array()));
	}

	fault = false; // В данном случае, если formID не найден - предполагаем что это formID из мода, который отсутствует, следовательно условие будет выполнено в любом случае.

	it = find_ci(obj, "onlyTints");
	if (it != end && it->value().is_bool()) {
		m_onlyTints = it->value().as_bool();
	}
}

CoincidenceLevel ConditionSettings::check(const RE::Actor* actor, Filter filter) const noexcept {
	CoincidenceLevel coincidenceLevel = CoincidenceLevel::NONE;
	if (!actor) {
		logger::info("ConditionSettings::check: actor is null");
		return CoincidenceLevel::NONE;
	}

	// Проверка пола
	if ((filter & Filter::Gender) != Filter::None) {
		if (m_gender != RE::Actor::Sex::None && const_cast<RE::Actor*>(actor)->GetSex() != m_gender)
			return CoincidenceLevel::NONE;
		coincidenceLevel |= CoincidenceLevel::GENDER;
	}

	/*auto getLeveledForm = [](RE::Actor* actor) -> RE::TESForm* {
		if (actor->extraList == nullptr)
			return nullptr;
		auto& extraData = actor->extraList->extraData;
		auto leveledCreatureExtra = extraData.GetByType(RE::EXTRA_DATA_TYPE::kLeveledCreature);
		if (leveledCreatureExtra) {
			return *reinterpret_cast<RE::TESForm**>(reinterpret_cast<uintptr_t>(leveledCreatureExtra) + 0x18);
		}
		return nullptr;
	};*/

	auto actorNPC = actor->GetNPC();
	std::string editorIDBase{};
	uint32_t formIDBase = 0;
	if (auto leveled = functions::getLeveledForm(const_cast<RE::Actor*>(actor))) {
		editorIDBase = leveled->GetFormEditorID();
		formIDBase = leveled->GetFormID();
	}
	else {
		editorIDBase = actorNPC->GetFormEditorID();
		formIDBase = actorNPC->GetFormID();
	}
	auto formID = actor->GetFormID();

	// Проверка formID
	if ((filter & Filter::FormIDs) != Filter::None && !m_formIDs.empty()) {
		if (m_formIDs.contains(formID) || m_formIDs.contains(formIDBase))
			return CoincidenceLevel::FULL;
		else
			return CoincidenceLevel::NONE;
	}

	// Проверка notFormID
	if ((filter & Filter::NotFormIDs) != Filter::None && !m_notFormIDs.empty()) {
		if (m_notFormIDs.contains(formID) || m_notFormIDs.contains(formIDBase))
			return CoincidenceLevel::NONE;
	}
	
	// EditorID
	std::string editorID = actor->GetFormEditorID();
	std::transform(editorID.begin(), editorID.end(), editorID.begin(), [](unsigned char c) { return std::tolower(c); });
	std::transform(editorIDBase.begin(), editorIDBase.end(), editorIDBase.begin(), [](unsigned char c) { return std::tolower(c); });

	/*if ((filter & Filter::EditorIDs) != Filter::None && !m_editorIDs.empty()) {
		if (!m_editorIdContains) {
			if (!m_editorIDs.contains(editorID) && !m_editorIDs.contains(editorIDBase))
				return CoincidenceLevel::NONE;
			coincidenceLevel |= CoincidenceLevel::FULL;
		}
		else {
			bool found = false;
			for (const auto& id : m_editorIDs) {
				if ((!editorID.empty() && editorID.find(id) != std::string::npos) ||
					(!editorIDBase.empty() && editorIDBase.find(id) != std::string::npos)) {
					found = true;
					break;
				}
			}
			if (!found)
				return CoincidenceLevel::NONE;
		}
	}

	if ((filter & Filter::NotEditorIDs) != Filter::None && !m_notEditorIDs.empty()) {
		if (!m_notEditorIdContains) {
			if (m_notEditorIDs.contains(editorID) || m_notEditorIDs.contains(editorIDBase))
				return CoincidenceLevel::NONE;
			coincidenceLevel |= CoincidenceLevel::FULL;
		}
		else {
			for (const auto& id : m_notEditorIDs) {
				if ((!editorID.empty() && editorID.find(id) != std::string::npos) ||
					(!editorIDBase.empty() && editorIDBase.find(id) != std::string::npos))
					return CoincidenceLevel::NONE;
			}
		}
	}*/

	// Ключевые слова
	auto HasKeywordInList = [](const RE::Actor* actor, const std::unordered_set<RE::BGSKeyword*>& keywords) {
		for (auto kwd : keywords)
			if (actor->HasKeyword(kwd))
				return true;
		return false;
		};

	if ((filter & Filter::HasKeyword) != Filter::None && !m_hasKeyword.empty()) {
		if (!HasKeywordInList(actor, m_hasKeyword))
			return CoincidenceLevel::NONE;
		coincidenceLevel |= CoincidenceLevel::KEYWORDS;
	}

	if ((filter & Filter::HasNotKeyword) != Filter::None && !m_hasNotKeyword.empty()) {
		if (HasKeywordInList(actor, m_hasNotKeyword))
			return CoincidenceLevel::NONE;
	}

	// Фракции
	auto IsInFaction = [](const RE::Actor* actor, const std::unordered_set<RE::TESFaction*>& factions) {
		for (auto f : factions)
			if (actor->IsInFaction(f))
				return true;
		return false;
		};

	if ((filter & Filter::InFaction) != Filter::None && !m_inFaction.empty()) {
		if (!IsInFaction(actor, m_inFaction))
			return CoincidenceLevel::NONE;
		coincidenceLevel |= CoincidenceLevel::FACTIONS;
	}

	if ((filter & Filter::NotInFaction) != Filter::None && !m_notInFaction.empty()) {
		if (IsInFaction(actor, m_notInFaction))
			return CoincidenceLevel::NONE;
	}

	return coincidenceLevel;
}

bool ConditionSettings::empty() const noexcept {
	return *this == ConditionSettings{};
}

void ConditionSettings::clear() noexcept {
	*this = std::move(ConditionSettings{});
}

RE::Actor::Sex ConditionSettings::gender() const noexcept {
	return m_gender;
}

bool ConditionSettings::onlyTints() const noexcept {
	return m_onlyTints;
}

void ConditionSettings::setGender(RE::Actor::Sex sex) noexcept {
	m_gender = sex;
}

bool ConditionSettings::operator<(const ConditionSettings& other) const noexcept
{
	return static_cast<int>(m_gender) < static_cast<int>(other.m_gender);
}

constexpr inline std::string_view genderToString(int gender) {
	switch (gender)
	{
	case 0:
		return "Male";
	case 1:
		return "Female";
	default:
		return "Unknown";
	}
}

std::string ConditionSettings::print() const {
	std::ostringstream oss;
	oss << "[";
	
	oss << "Gender=" << genderToString(m_gender) << "; ";

	if (!m_formIDs.empty()) {
		oss << "FormIDs=";
		for (const auto& id : m_formIDs) {
			oss << std::hex << id << ", ";
		}
		oss << "; ";
	}

	if (!m_notFormIDs.empty()) {
		oss << "NotFormIDs=";
		for (const auto& id : m_notFormIDs) {
			oss << std::hex << id << ", ";
		}
		oss << "; ";
	}

	/*if (!m_editorIDs.empty()) {
		oss << "EditorIDs=";
		for (const auto& id : m_editorIDs) {
			oss << id << ", ";
		}
		oss << "; ";
	}

	if (!m_notEditorIDs.empty()) {
		oss << "NotEditorIDs=";
		for (const auto& id : m_notEditorIDs) {
			oss << id << ", ";
		}
		oss << "; ";
	}*/

	if (!m_hasKeyword.empty()) oss << "Keywords=" << m_hasKeyword.size() << "; ";
	if (!m_hasNotKeyword.empty()) oss << "NotKeywords=" << m_hasNotKeyword.size() << "; ";
	if (!m_inFaction.empty()) oss << "InFaction=" << m_inFaction.size() << "; ";
	if (!m_notInFaction.empty()) oss << "NotInFaction=" << m_notInFaction.size() << "; ";
	oss << "]\n";
	return oss.str();
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

inline CoincidenceLevel operator|(CoincidenceLevel a, CoincidenceLevel b) noexcept {
	return static_cast<CoincidenceLevel>(static_cast<int>(a) | static_cast<int>(b));
}
inline CoincidenceLevel& operator|=(CoincidenceLevel& a, CoincidenceLevel b) noexcept {
	a = a | b;
	return a;
}

inline bool operator<(CoincidenceLevel lhs, CoincidenceLevel rhs) noexcept {
	return static_cast<int>(lhs) < static_cast<int>(rhs);
}

Filter operator|(Filter a, Filter b) noexcept {
	return static_cast<Filter>(static_cast<int>(a) | static_cast<int>(b));
}

Filter& operator|=(Filter& a, Filter b) noexcept {
	a = a | b;
	return a;
}

Filter operator&(Filter a, Filter b) noexcept {
	return static_cast<Filter>(static_cast<int>(a) & static_cast<int>(b));
}

Filter& operator&=(Filter& a, Filter b) noexcept {
	a = a & b;
	return a;
}

Filter operator~(Filter a) noexcept
{
	return static_cast<Filter>(~static_cast<std::underlying_type_t<Filter>>(a));
}

Filter operator^(Filter a, Filter b) noexcept {
	return static_cast<Filter>(static_cast<int>(a) ^ static_cast<int>(b));
}

Filter& operator^=(Filter& a, Filter b) noexcept {
	a = a ^ b;
	return a;
}

//CoincidenceLevel ConditionSettings::check(const RE::Actor* actor, Filter filter) const noexcept {
//	
//	CoincidenceLevel сoincidenceLevel = CoincidenceLevel::NONE;
//	if (!actor) {
//		logger::info("ConditionSettings::check: actor is null");
//		return CoincidenceLevel::NONE;
//	} 
//
//	// Если пол не совпадает пресет не сможет примениться, поэтому проверка на пол должна быть первой, и при несовпадении сразу возвращать NONE.
//	if ((static_cast<int>(filter & Filter::Gender))) {
//		if (m_gender != RE::Actor::Sex::None && const_cast<RE::Actor*>(actor)->GetSex() != m_gender)
//			return CoincidenceLevel::NONE; // Пол всегда должен совпадать, иначе нет смысла проверять дальше
//		else
//			сoincidenceLevel |= CoincidenceLevel::GENDER;
//	}
//
//	auto actorBase = functions::getLeveledTESNPC(actor->GetNPC());
//	auto formID = actor->GetFormID();
//	auto formIDBase = actorBase ? actorBase->GetFormID() : 0;
//
//	// Проверка на formID приоритетнее всего остального, если он указан. Ноль считается как "не указан". Если указан formID, то всё остальное игнорируется. FormID проверяется как для Actor, так и для его базы.
//	if (static_cast<int>(filter & Filter::FormID) && !m_formIDs.empty()) {
//		if (m_formIDs.contains(formID) || m_formIDs.contains(formIDBase))
//			return CoincidenceLevel::FULL; // Если указано formID, то оно должно совпадать
//		else
//			return CoincidenceLevel::NONE; // Максимальный уровень совпадения за FormID
//	}
//
//	// проверка на notFormID, является негативным фильтром, т.е. если указано notFormID, и оно совпадает, то возвращается NONE, иначе проходит дальше и происходят прочие проверки.
//	if (static_cast<int>(filter & Filter::NotFormID) && !m_notFormIDs.empty()) {
//		if (m_notFormIDs.contains(formID) || m_notFormIDs.contains(formIDBase))
//			return CoincidenceLevel::NONE; // Если указано notFormID, то оно не должно совпадать
//	}
//
//	// Если флаг m_editorIdContains не установлен, то проверка editorID должна быть точной, иначе - частичной. Проверка на formID приоритетнее всего остального (кроме formID), по этому если editorID указан и совпадает, то всё остальное игнорируется. Если editorID не совпадает, то всегда вовзращается NONE. Пустой editorID считается как "не указан", и проверка на него не выполняется. EditorID проверяется как для Actor, так и для его базы.
//	if (static_cast<int>(filter & Filter::EditorID)) {
//		if (!m_editorIdContains) {
//			if (!m_editorIDs.empty()) {
//				if (!m_editorIDs.contains(editorID) || !m_editorIDs.contains(editorIDBase))
//					return CoincidenceLevel::NONE; // Если указано editorID, то оно должно совпадать
//				else
//					сoincidenceLevel |= CoincidenceLevel::FULL; // Максимальный уровень совпадения за совпадения по ID редактора
//			}
//		}
//		else {
//			// Является только негативным фильтром, если есть флаг совпадения, т.е. если указан editorID, и он не совпадает частично, то возвращается NONE, иначе проходит дальше и происходят прочие проверки.
//			if (!m_editorIDs.empty()) {
//				if (!editorID.empty()) {
//					for (const auto& id : m_editorIDs) {
//						if (editorID.find(id) == std::string::npos || editorIDBase.find(id) == std::string::npos)
//							return CoincidenceLevel::NONE; // Если указано editorID, то оно не должно совпадать частично
//					}
//				}
//			}
//		}
//	}
//
//	if (static_cast<int>(filter & Filter::NotEditorID)) {
//		if (!m_notEditorIdContains) {
//			if (!m_notEditorIDs.empty()) {
//				if (m_notEditorIDs.contains(editorID) || m_notEditorIDs.contains(editorIDBase))
//					return CoincidenceLevel::NONE; // Если указано editorID, то оно должно совпадать
//				else
//					сoincidenceLevel |= CoincidenceLevel::FULL; // Максимальный уровень совпадения за совпадения по ID редактора
//			}
//		}
//		else {
//			// Является только негативным фильтром, если есть флаг совпадения, т.е. если указан editorID, и он не совпадает частично, то возвращается NONE, иначе проходит дальше и происходят прочие проверки.
//			if (!m_notEditorIDs.empty()) {
//				for (const auto& id : m_editorIDs) {
//					if (editorID.find(id) != std::string::npos || editorIDBase.find(id) != std::string::npos)
//						return CoincidenceLevel::NONE; // Если указано editorID, то оно не должно совпадать частично
//				}
//			}
//		}
//	}
//
//	auto HasKeywordInList = [&actorBase](const RE::Actor* actor, const std::unordered_set<RE::BGSKeyword*>& keywords) {
//		
//		for (auto kwd : keywords) {
//			if (actor->HasKeyword(kwd))
//				return true;
//		}
//		return false;
//	};
//
//	// Если ключевые слова указаны, то добавляется уровень совпадения за наличие ключевого слова, если оно есть в списке. Если ключевое слово не указано, то проверка не выполняется. Если указано ключевое слово, и его нет в списке, то возвращается NONE. 
//	if (static_cast<int>(filter & Filter::HasKeyword)) {
//		if (m_hasKeyword.size() && !HasKeywordInList(actor, m_hasKeyword))
//			return CoincidenceLevel::NONE;
//		else if (m_hasKeyword.size())
//			сoincidenceLevel |= CoincidenceLevel::KEYWORDS;  // Уровень совпадения за наличие ключевого слова
//	}
//
//	// Если ключевые слова указаны, то добавляется уровень совпадения за наличие ключевого слова, если оно есть в списке. Если ключевое слово не указано, то проверка не выполняется. Если указано ключевое слово, и оно есть в списке, то возвращается NONE. 
//	if (static_cast<int>(filter & Filter::HasNotKeyword)) {
//		if (m_hasNotKeyword.size() && HasKeywordInList(actor, m_hasNotKeyword))
//			return CoincidenceLevel::NONE;
//		else if (m_hasNotKeyword.size())
//			сoincidenceLevel |= CoincidenceLevel::KEYWORDS;  // Уровень совпадения за отсутствие ключевого слова
//	}
//
//	auto IsInFaction = [](const RE::Actor* actor, const std::unordered_set<RE::TESFaction*>& factions) {
//		for (auto f : factions) {
//			if (actor->IsInFaction(f))
//				return true;
//		}
//		return false;
//	};
//
//	if (static_cast<int>(filter & Filter::InFaction)) {
//		if (m_inFaction.size() && !IsInFaction(actor, m_inFaction))
//			return CoincidenceLevel::NONE;
//		else if (m_inFaction.size())
//			сoincidenceLevel |= CoincidenceLevel::FACTIONS;  // Уровень совпадения за нахождение во фракции
//	}
//
//	if (static_cast<int>(filter & Filter::NotInFaction)) {
//		if (m_notInFaction.size() && IsInFaction(actor, m_notInFaction))
//			return CoincidenceLevel::NONE;
//		else if (m_notInFaction.size())
//			сoincidenceLevel |= CoincidenceLevel::FACTIONS;  // Уровень совпадения за не нахождение во фракции
//	}
//
//	return сoincidenceLevel;
//}