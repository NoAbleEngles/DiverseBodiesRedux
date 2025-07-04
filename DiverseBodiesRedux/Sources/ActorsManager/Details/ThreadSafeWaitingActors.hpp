#pragma	once
#include "ActorsPresetHashEquals.hpp"
#include <vector>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <functional>


class ActorsManager;

/**
 * @brief Потокобезопасный контейнер для хранения и обработки актёров, ожидающих применения пресетов.
 *
 * Класс предназначен для хранения идентификаторов актёров, для которых требуется применить пресеты,
 * с обеспечением потокобезопасного доступа. Позволяет добавлять, очищать, проверять наличие и обрабатывать
 * ожидающих актёров с помощью заданного метода применения пресетов.
 */
class ThreadSafeWaitingActors
{
public:
	using ApplyPreset = std::function<bool(const ActorsManagerDefs::ActorPreset&)>;

	/**
	 * @brief Конструктор с указанием контейнера пресетов и метода применения. Рекомендовано использовать только этот конструктор.
	 * @param presets Указатель на контейнер пресетов.
	 * @param applyMethod Метод для применения пресетов к актёру.
	 */
	inline explicit ThreadSafeWaitingActors(std::unordered_map<uint32_t, ActorsManagerDefs::Presets>* presets, ApplyPreset applyMethod) : m_presets(presets), applyActorPresets(applyMethod) {}
	
	/**
	 * @brief Конструктор по умолчанию. Оставлен только для совместимости с другими частями кода. Не рекомендуется использовать вручную.
	 */
	inline ThreadSafeWaitingActors() = default;

	/**
	 * @brief Копирующий конструктор. Оставлен только для совместимости с другими частями кода. Не рекомендуется использовать вручную.
	 */
	inline ThreadSafeWaitingActors(const ThreadSafeWaitingActors&) = default;

	/**
	 * @brief Перемещающий конструктор. Оставлен только для совместимости с другими частями кода. Не рекомендуется использовать вручную.
	 */
	inline ThreadSafeWaitingActors(ThreadSafeWaitingActors&&) = default;

	/**
	 * @brief Оператор копирующего присваивания. Оставлен только для совместимости с другими частями кода. Не рекомендуется использовать вручную.
	 */
	inline ThreadSafeWaitingActors& operator=(const ThreadSafeWaitingActors&) = default;

	/**
	 * @brief Оператор перемещающего присваивания. Оставлен только для совместимости с другими частями кода. Не рекомендуется использовать вручную.
	 */
	inline ThreadSafeWaitingActors& operator=(ThreadSafeWaitingActors&&) = default;

	/**
	 * @brief Добавляет идентификатор актёра в очередь ожидания.
	 * @param formID Идентификатор актёра.
	 */
	inline void push_back(uint32_t formID) {
		std::lock_guard lock(m_mutex);
		m_waitingActors.insert(formID);
	}

	/**
	 * @brief Очищает очередь ожидающих актёров.
	 */
	inline void clear() {
		std::lock_guard lock(m_mutex);
		m_waitingActors.clear();
	}

	/**
	 * @brief Проверяет, пуста ли очередь ожидающих актёров.
	 * @return true, если очередь пуста.
	 */
	inline bool empty() const noexcept {
		std::lock_guard lock(m_mutex);
		return m_waitingActors.empty();
	}

	/**
	 * @brief Применяет пресеты ко всем ожидающим актёрам и очищает очередь.
	 */
	inline void flush() {
		if (!m_presets || !applyActorPresets) {
			return;
		}

		while (true) {
			std::set<uint32_t> copy;
			{
				std::lock_guard lock(m_mutex);
				if (m_waitingActors.empty()) {
					break;
				}
				copy.swap(m_waitingActors);
			}
			for (auto& formID : copy) {
				if (auto preset = m_presets->find(formID); preset != m_presets->end()) {
					applyActorPresets(*preset);
				}
			}
		}
	}

private:
	std::set<uint32_t> m_waitingActors{};
	mutable std::mutex m_mutex;
	std::unordered_map<uint32_t, ActorsManagerDefs::Presets>* m_presets;
	ApplyPreset applyActorPresets = nullptr;
};