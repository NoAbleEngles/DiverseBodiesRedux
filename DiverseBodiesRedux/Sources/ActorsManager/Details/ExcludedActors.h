#pragma once
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include "globals.h"
#include "Preset/Details/PresetEnums.h"
#include <filesystem>
#include <map>
#include <boost/json.hpp>
#include <memory>
#include <optional>

class ExcludedActors {
public:

	/**
	* @brief обновляет список исключенных актёров и их баз.
	*/
	void refreshExclusionList() noexcept;

	/**
	* @brief Возвращает указатель на набор типов пресетов, по которым актёр исключён.
	* @param actor Указатель на объект Actor.
	* @return Указатель на набор типов пресетов, по которым актёр исключён, если набор пуст - значит актёр полностью исключён. std::nullopt, если актёр не в списке исключённых.
	*/
	std::optional<std::shared_ptr<std::unordered_set<PresetType>>> getExcludedTypes(const RE::Actor* actor) const noexcept;

private:
	/// FormID актёра и его базовой формы (NPC), флаги PresetType.
	std::map<uint32_t, std::shared_ptr<std::unordered_set<PresetType>>> m_excludedForms;

	bool loadExclusionsFromFolder(const std::string& folder) noexcept;
	bool loadExclusionsFromJsonFile(const std::filesystem::path& filePath) noexcept;
	bool loadExclusionsFromJsonObject(boost::json::object& jsonObject) noexcept;
};