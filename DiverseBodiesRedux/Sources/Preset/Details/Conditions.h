#pragma once
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#include <boost/json.hpp>
#include "PresetEnums.h"

using namespace boost::json;
namespace logger = F4SE::log;

/**
 * @brief Флаги уровня совпадения условий для объекта Actor.
 * Используется для определения степени соответствия объекта Actor заданным условиям.
 * NONE - не совпадает ни с одним условием, KEYWORDS - совпадает по ключевым словам,
 * FACTIONS - совпадает по фракциям, FULL - полное совпадение (например, по formID или editorID). KEYWORDS и FACTIONS добавляются как флаги к результату. has и hasNot имеют один флаг для каждого из них, но проваленная проверка hasNot всё равно вернёт NONE, даже если есть совпадение по has.
 */
enum class CoincidenceLevel : int {
	NONE = 0, // Не совпадает ни с одним условием  
	GENDER = 1 << 0,
	KEYWORDS = 1 << 1,
	FACTIONS = 1 << 2,
	FULL = static_cast<int>(0x7FFFFFFF) // Полное совпадение, например, по formID или editorID  
};

CoincidenceLevel operator|(CoincidenceLevel a, CoincidenceLevel b) noexcept;
CoincidenceLevel& operator|=(CoincidenceLevel& a, CoincidenceLevel b) noexcept;
bool operator<(CoincidenceLevel lhs, CoincidenceLevel rhs) noexcept;

enum class Filter : int {
	None = 0,				// Нет фильтра
	Gender = 1 << 0,		// Пол
	HasKeyword = 1 << 1,	// Имеет ключевое слово
	HasNotKeyword = 1 << 2, // Не имеет ключевого слова
	InFaction = 1 << 3,		// В фракции
	NotInFaction = 1 << 4,  // Не в фракции
	EditorID = 1 << 5,		// По editorID
	FormID = 1 << 6			// По formID
};

Filter operator|(Filter a, Filter b) noexcept;
Filter& operator|=(Filter& a, Filter b) noexcept;
Filter operator&(Filter a, Filter b) noexcept;
Filter& operator&=(Filter& a, Filter b) noexcept;
Filter operator~(Filter a) noexcept;

const Filter AllFilters = Filter::Gender | Filter::HasKeyword | Filter::HasNotKeyword | Filter::InFaction | Filter::NotInFaction | Filter::EditorID | Filter::FormID;

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
	 * @param filter параметр для указания, какие условия проверять. По умолчанию проверяются все условия.
	 * @return 0, если actor не удовлетворяет любому из условий. Совпадение по условиям возвращает уровень совпадения :
	 * Максимальный уровень совпадения - 10, если совпадает по formID, или editorID.
	 * наличие/отсутствие кейворда считается более точным совпадением, чем наличие/отсутствие фракции, поэтому за наличие/отсутствие кейворда начисляется 3 очка.
	 * наличие/отсутствие во фракции начисляет 2 очка.
	 * Иными словами - если не указан точно
     */
	CoincidenceLevel check(const RE::Actor* actor, Filter filter
		= Filter::HasKeyword | Filter::HasNotKeyword | Filter::InFaction | Filter::NotInFaction | Filter::FormID | Filter::EditorID)  const noexcept;

	/**
     * @brief Проверяет на пустоту.
     */
	bool empty() const noexcept;

	/**
     * @brief Сбросить все условия.
     */
	void clear() noexcept;

	/**
	 * @brief Печать информации об условиях в строку.
	 * @return Строка с информацией об условиях.
	 */
	std::string print() const;

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
