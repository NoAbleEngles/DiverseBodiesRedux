#pragma once
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#include <boost/json.hpp>

using namespace boost::json;
namespace logger = F4SE::log;

/**
 * @brief Класс для хранения и проверки условий, применяемых к объекту Actor.
 */
class ConditionSettings
{
public:
	/**
     * @brief Конструктор по умолчанию.
     */
	ConditionSettings() noexcept = default;

	/**
     * @brief Конструктор из JSON-значения.
     * @param item JSON-объект с параметрами условий.
     */
	ConditionSettings(boost::json::object& item);

	/**
     * @brief Конструктор из пути к файлу.
     * @param path Путь к JSON-файлу с условиями.
     */
	ConditionSettings(const std::filesystem::path& path);

	/**
     * @brief Конструктор из строки пути.
     * @param path Строка с путем к JSON-файлу.
     */
	ConditionSettings(const std::string& path);

	/**
     * @brief Конструктор для задания пола (костыль для Bodymorphs).
     * @param gender Пол.
     */
	ConditionSettings(RE::Actor::Sex gender) noexcept;

	/**
     * @brief Деструктор.
     */
	~ConditionSettings() noexcept = default;

	ConditionSettings(const ConditionSettings&) = default;
	ConditionSettings(ConditionSettings&&) = default;

	/**
     * @brief Проверяет, удовлетворяет ли actor условиям.
     * @param actor Указатель на объект Actor.
     * @return true, если actor удовлетворяет условиям.
     */
	bool check(const RE::Actor* actor) const noexcept;

	/**
     * @brief Проверяет на пустоту.
     */
	bool empty() const noexcept;

	/**
     * @brief Сбросить все условия.
     */
	void clear() noexcept;

	bool operator==(const ConditionSettings& other) const noexcept = default;
	bool operator<(const ConditionSettings& other) const noexcept;
	ConditionSettings& operator=(const ConditionSettings&) noexcept = default;
	ConditionSettings& operator=(ConditionSettings&&) noexcept = default;

private:
	RE::Actor::Sex m_gender{ RE::Actor::Sex::None };
	std::optional<int> m_formID{};
	std::string m_editorID{};
	bool m_editorIdContains{};
	std::vector<RE::BGSKeyword*> m_hasKeyword{};
	std::vector<RE::BGSKeyword*> m_hasNotKeyword{};
	std::vector<RE::TESFaction*> m_inFaction{};
	std::vector<RE::TESFaction*> m_notInFaction{};

	/**
     * @brief Загрузить условия из файла.
     * @param path Путь к JSON-файлу.
     * @return true, если загрузка успешна.
     */
	bool loadFromFile(const std::filesystem::path& path);
};

/**
 * @brief Получить TESForm по строковому FormID и имени плагина.
 * @param xFormID Строковый FormID.
 * @param plugin Имя плагина.
 * @return Указатель на TESForm или nullptr.
 */
RE::TESForm* getFormFromString(const std::string& xFormID, const std::string& plugin);
