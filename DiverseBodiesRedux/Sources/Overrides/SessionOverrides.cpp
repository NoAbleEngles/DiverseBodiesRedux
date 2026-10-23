#include "PCH.h"
#include "Overrides/SessionOverrides.h"
#include "Overrides/SavedFormId.h"

#include <boost/json.hpp>

namespace json = boost::json;

namespace
{
	// FormID в co-save хранится не сырым числом, а как "плагин + локальный ID": порядок плагинов
	// между сессиями может поменяться. Временные формы (0xFF......) плагина не имеют - они как есть.
	struct StableId
	{
		std::string   plugin;
		std::uint32_t local{ 0 };
	};

	// 0 - плагина нет в этой сборке (оверрайд пропадает).
	std::uint32_t FromStable(const StableId& a_id)
	{
		if (a_id.plugin.empty()) {
			return a_id.local;
		}
		auto* data = RE::TESDataHandler::GetSingleton();
		if (!data) {
			return 0;
		}
		const auto [file, full] = data->LookupLoadedFile(a_id.plugin);
		if (!file) {
			return 0;
		}
		return full ? (static_cast<std::uint32_t>(file->compileIndex) << 24) | (a_id.local & 0x00FFFFFF) :
		              0xFE000000 | (static_cast<std::uint32_t>(file->smallFileCompileIndex) << 12) | (a_id.local & 0xFFF);
	}

	json::array TypesToJson(const std::set<PresetType>& a_types)
	{
		json::array result;
		for (const auto type : a_types) {
			result.push_back(json::string(GetPresetTypeString(type)));
		}
		return result;
	}

	std::set<PresetType> TypesFromJson(const json::value* a_value)
	{
		std::set<PresetType> result;
		if (a_value && a_value->is_array()) {
			for (const auto& item : a_value->as_array()) {
				if (item.is_string()) {
					const auto type = GetPresetTypeFromString(std::string(item.as_string()));
					if (type != PresetType::NONE) {
						result.insert(type);
					}
				}
			}
		}
		return result;
	}
}

SessionOverrides& SessionOverrides::get()
{
	static SessionOverrides instance;
	return instance;
}

void SessionOverrides::refreshAny()
{
	m_any.store(!m_exclusions.empty() || !m_disabledPresets.empty(), std::memory_order_relaxed);
}

SessionOverrides::ExclusionOverride SessionOverrides::exclusionFor(std::uint32_t a_key) const
{
	std::lock_guard lock(m_mutex);
	const auto      it = m_exclusions.find(a_key);
	return it != m_exclusions.end() ? it->second : ExclusionOverride{};
}

bool SessionOverrides::isPresetDisabled(const std::string& a_id) const
{
	if (!any()) {
		return false;
	}
	std::lock_guard lock(m_mutex);
	return m_disabledPresets.contains(a_id);
}

std::map<std::uint32_t, SessionOverrides::ExclusionOverride> SessionOverrides::exclusions() const
{
	std::lock_guard lock(m_mutex);
	return m_exclusions;
}

std::set<std::string> SessionOverrides::disabledPresets() const
{
	std::lock_guard lock(m_mutex);
	return m_disabledPresets;
}

void SessionOverrides::toggleExclusion(std::uint32_t a_key, PresetType a_type, bool a_baseExcluded)
{
	std::lock_guard lock(m_mutex);
	auto&           entry = m_exclusions[a_key];
	if (a_baseExcluded) {
		// Исключён правилом JSON: переключаем "разрешён в этой сессии".
		if (!entry.allow.erase(a_type)) {
			entry.allow.insert(a_type);
		}
	} else {
		// В JSON не исключён: переключаем "исключён в этой сессии".
		if (!entry.deny.erase(a_type)) {
			entry.deny.insert(a_type);
		}
	}
	if (entry.empty()) {
		m_exclusions.erase(a_key);
	}
	refreshAny();
}

void SessionOverrides::addExclusion(std::uint32_t a_key)
{
	std::lock_guard lock(m_mutex);
	auto&           entry = m_exclusions[a_key];
	for (auto type = PresetType::BODYMORPHS; type < PresetType::END; type = static_cast<PresetType>(static_cast<int>(type) + 1)) {
		entry.allow.erase(type);
		entry.deny.insert(type);
	}
	refreshAny();
}

void SessionOverrides::resetExclusion(std::uint32_t a_key)
{
	std::lock_guard lock(m_mutex);
	m_exclusions.erase(a_key);
	refreshAny();
}

void SessionOverrides::setPresetDisabled(const std::string& a_id, bool a_disabled)
{
	std::lock_guard lock(m_mutex);
	if (a_disabled) {
		m_disabledPresets.insert(a_id);
	} else {
		m_disabledPresets.erase(a_id);
	}
	refreshAny();
}

void SessionOverrides::resetAll()
{
	clear();
}

void SessionOverrides::clear()
{
	std::lock_guard lock(m_mutex);
	m_exclusions.clear();
	m_disabledPresets.clear();
	refreshAny();
}

void SessionOverrides::Serialize(const F4SE::SerializationInterface* a_intfc) const
{
	if (!a_intfc) {
		return;
	}
	json::object root;
	root["v"] = kRecordVersion;
	{
		std::lock_guard lock(m_mutex);
		if (m_exclusions.empty() && m_disabledPresets.empty()) {
			return;  // нечего писать: в сейве записи не будет
		}
		json::array exclusions;
		for (const auto& [key, entry] : m_exclusions) {
			json::object item;
			item["form"] = key;
			item["allow"] = TypesToJson(entry.allow);
			item["deny"] = TypesToJson(entry.deny);
			exclusions.push_back(std::move(item));
		}
		root["exclusions"] = std::move(exclusions);
		json::array presets;
		for (const auto& id : m_disabledPresets) {
			presets.push_back(json::string(id));
		}
		root["disabledPresets"] = std::move(presets);
	}

	const auto text = json::serialize(root);
	if (!a_intfc->OpenRecord(kRecordType, kRecordVersion)) {
		logger::error("SessionOverrides: can't open the co-save record");
		return;
	}
	if (!a_intfc->WriteRecordData(text.data(), static_cast<std::uint32_t>(text.size()))) {
		logger::warn("SessionOverrides: failed to write the record");
	}
}

void SessionOverrides::Deserialize(const F4SE::SerializationInterface* a_intfc, std::uint32_t a_version, std::uint32_t a_length)
{
	clear();
	if (!a_intfc || a_length == 0 || a_length > 16 * 1024 * 1024) {
		return;
	}
	if (a_version > kRecordVersion) {
		logger::warn("SessionOverrides: record version {} is newer than supported {} -- ignored", a_version, kRecordVersion);
		return;
	}
	std::string text(a_length, '\0');
	if (a_intfc->ReadRecordData(text.data(), a_length) != a_length) {
		logger::warn("SessionOverrides: failed to read the record");
		return;
	}

	try {
		const auto value = json::parse(text);
		if (!value.is_object()) {
			return;
		}
		std::lock_guard lock(m_mutex);
		if (const auto* list = value.as_object().if_contains("exclusions"); list && list->is_array()) {
			for (const auto& item : list->as_array()) {
				if (!item.is_object()) {
					continue;
				}
				const auto& object = item.as_object();
				std::uint32_t key = 0;
				if (const auto* form = object.if_contains("form"); form && form->is_number()) {
					key = ResolveSavedFormID(static_cast<std::uint32_t>(form->to_number<std::uint64_t>()));
				} else {
					// первые тестовые сборки: плагин + локальный ID
					StableId stable;
					if (const auto* plugin = object.if_contains("plugin"); plugin && plugin->is_string()) {
						stable.plugin = std::string(plugin->as_string());
					}
					if (const auto* id = object.if_contains("id"); id && id->is_number()) {
						stable.local = static_cast<std::uint32_t>(id->to_number<std::uint64_t>());
					}
					key = FromStable(stable);
				}
				if (!key) {
					logger::warn("SessionOverrides: the form of an exclusion override isn't in this load order -- dropped");
					continue;
				}
				ExclusionOverride entry;
				entry.allow = TypesFromJson(object.if_contains("allow"));
				entry.deny = TypesFromJson(object.if_contains("deny"));
				if (!entry.empty()) {
					m_exclusions[key] = std::move(entry);
				}
			}
		}
		if (const auto* list = value.as_object().if_contains("disabledPresets"); list && list->is_array()) {
			for (const auto& item : list->as_array()) {
				if (item.is_string()) {
					m_disabledPresets.insert(std::string(item.as_string()));
				}
			}
		}
		refreshAny();
		logger::info("SessionOverrides: loaded {} exclusion override(s), {} disabled preset(s)", m_exclusions.size(), m_disabledPresets.size());
	} catch (const std::exception& e) {
		logger::warn("SessionOverrides: unreadable record: {}", e.what());
	}
}
