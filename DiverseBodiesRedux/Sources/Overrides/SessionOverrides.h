#pragma once

#include "Preset/Details/PresetEnums.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>

/**
 * @brief Оверрайды правил из JSON внутри одной игровой сессии (сохранения).
 *
 * JSON-файлы мода не трогаются: здесь лежит только то, что игрок поменял для этого сейва, и оно
 * сериализуется в co-save отдельной записью 'DBRO'. Что можно:
 *  - исключения (Exclusions/*.json): разрешить типы пресетов, исключённые правилом (allow), и
 *    добавить исключение, которого в JSON нет (deny). Ключ - FormID актёра или его базового NPC,
 *    как в самих правилах;
 *  - пресеты: отключить пресет (disabled) - автоматическая раздача его не выбирает.
 *
 * Обратная совместимость: запись пишется после основной (DBR2), поэтому старая версия мода, которая
 * читает только первую запись, её не видит и не ломается; в сейве без записи оверрайдов нет.
 * Потокобезопасно.
 */
class SessionOverrides
{
public:
	static constexpr std::uint32_t kRecordType = 'DBRO';
	static constexpr std::uint32_t kRecordVersion = 1;

	struct ExclusionOverride
	{
		std::set<PresetType> allow;  // типы, исключённые правилом JSON, но разрешённые в этой сессии
		std::set<PresetType> deny;   // типы, исключённые только в этой сессии

		bool empty() const noexcept { return allow.empty() && deny.empty(); }
	};

	static SessionOverrides& get();

	// Есть ли хоть один оверрайд (быстрая проверка для горячих путей).
	bool any() const noexcept { return m_any.load(std::memory_order_relaxed); }

	ExclusionOverride exclusionFor(std::uint32_t a_key) const;
	bool              isPresetDisabled(const std::string& a_id) const;

	std::map<std::uint32_t, ExclusionOverride> exclusions() const;
	std::set<std::string>                      disabledPresets() const;

	// Переключает тип: если он сейчас исключён - разрешает, если нет - исключает. a_baseExcluded -
	// исключён ли он правилом JSON (без оверрайдов).
	void toggleExclusion(std::uint32_t a_key, PresetType a_type, bool a_baseExcluded);
	// Исключает ключ целиком (все типы), если в JSON его нет.
	void addExclusion(std::uint32_t a_key);
	void resetExclusion(std::uint32_t a_key);
	void setPresetDisabled(const std::string& a_id, bool a_disabled);
	void resetAll();

	// Сброс без записи в сейв (загрузка сохранения / новая игра).
	void clear();

	// Co-save. Serialize пишет запись, если есть что писать; Deserialize вызывается, когда
	// GetNextRecordInfo вернул запись kRecordType.
	void Serialize(const F4SE::SerializationInterface* a_intfc) const;
	void Deserialize(const F4SE::SerializationInterface* a_intfc, std::uint32_t a_version, std::uint32_t a_length);

private:
	SessionOverrides() = default;

	void refreshAny();  // m_mutex уже взят

	mutable std::mutex                         m_mutex;
	std::map<std::uint32_t, ExclusionOverride> m_exclusions;
	std::set<std::string>                      m_disabledPresets;
	std::atomic_bool                           m_any{ false };
};
