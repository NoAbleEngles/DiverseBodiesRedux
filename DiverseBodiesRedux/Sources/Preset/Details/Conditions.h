#pragma once
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <vector>
#include <unordered_set>

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

// @brief Флаги фильтров для условий, применяемых к объекту Actor. Что бы всё работало корректно при добавлении новых фильтров, нужно добавлять их только в AllFilters, getFilterFromString и метод Check.
enum class Filter : int {
	None = 0,				// Нет фильтра
	Gender = 1 << 0,		// Пол
	HasKeyword = 1 << 1,	// Имеет ключевое слово
	HasNotKeyword = 1 << 2, // Не имеет ключевого слова
	InFaction = 1 << 3,		// В фракции
	NotInFaction = 1 << 4,  // Не в фракции
	FormIDs = 1 << 5,		// По formID
	NotFormIDs = 1 << 6		// Кроме formID
};

Filter operator|(Filter a, Filter b) noexcept;
Filter& operator|=(Filter& a, Filter b) noexcept;
Filter operator&(Filter a, Filter b) noexcept;
Filter& operator&=(Filter& a, Filter b) noexcept;
Filter operator~(Filter a) noexcept;
Filter operator^(Filter a, Filter b) noexcept;
Filter& operator^=(Filter& a, Filter b) noexcept;

const Filter AllFilters = Filter::Gender | Filter::HasKeyword | Filter::HasNotKeyword | Filter::InFaction | Filter::NotInFaction | Filter::FormIDs | Filter::NotFormIDs;

/**
 * @brief Преобразует строковое представление фильтра в enum Filter.
 * @param filterString Строка, представляющая фильтр.
 * @return Соответствующий enum Filter или Filter::None, если строка не распознана.
 */
Filter getFilterFromString(const std::string& filterString) noexcept;

/**
 * @brief Получает строковое представление фильтра.
 * @param filter Фильтр, для которого нужно получить строку.
 * @return Строковое представление фильтра.
 */
constexpr std::string_view getFilterString(Filter filter) noexcept;

/**
 * @brief Выполняет функцию для каждого значения Filter.
 * @param func Функция, принимающая Filter в качестве параметра.
 */
void for_each_filter(const std::function<void(Filter)>& func) noexcept;

/**
 * @brief Получает самый старший бит из флагов Filter. Обычно используется как getHighestBit(AllFilters). Для корректной работы нужно, чтобы флаги были в порядке возрастания, без "дырок".
 * @param flags Флаги, из которых нужно получить самый старший бит.
 * @return Самый старший бит из флагов Filter или Filter::None, если флаги равны 0.
 */
constexpr Filter getHighestBit(Filter flags) noexcept;

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
		= AllFilters)  const noexcept;

	/**
     * @brief Проверяет на пустоту.
     */
	bool empty() const noexcept;

	/**
     * @brief Сбросить все условия.
     */
	void clear() noexcept;

	/**
	 * @brief Получить условие пола актёра.
	 * @return Пол актёра.
	 */
	RE::Actor::Sex gender() const noexcept;

	/**
	 * @brief Параметр используется только для NPCPreset: если true, то из всех полей пресета будут использоваться только тинты, остальные игнорируются.
	 * @return true, если нужно применять только тинты.
	 */
	bool onlyTints() const noexcept;

	/**
	 * @brief Установить условие пола актёра.
	 * @param sex Пола актёра.
	 */
	void setGender(RE::Actor::Sex sex) noexcept;

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
	std::unordered_set<uint32_t> m_formIDs{};
	std::unordered_set<uint32_t> m_notFormIDs{};
	/*std::unordered_set<std::string> m_editorIDs{};
	std::unordered_set<std::string> m_notEditorIDs{};*/
	/*bool m_editorIdContains{};
	bool m_notEditorIdContains{};*/
	std::unordered_set<RE::BGSKeyword*> m_hasKeyword{};
	std::unordered_set<RE::BGSKeyword*> m_hasNotKeyword{};
	std::unordered_set<RE::TESFaction*> m_inFaction{};
	std::unordered_set<RE::TESFaction*> m_notInFaction{};

	//только для NPCPreset
	bool m_onlyTints{};

	/**
     * @brief Загрузить условия из файла.
     * @param path Путь к JSON-файлу.
     * @return true, если загрузка успешна.
     */
	bool loadFromFile(const std::filesystem::path& path);
};
