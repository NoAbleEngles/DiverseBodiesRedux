#pragma once
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_set>
#include <boost/json.hpp>
#include <ActorsManager/Details/ActorsPresetHashEquals.hpp>
#include "ActorsManager/Details/ThreadSafeWaitingActors.hpp"
#include "PresetsManager/PresetsManager.h"
#include "DirectApply/DirectApply.h"

/**
 * @brief Менеджер пресетов для актёров.
 *
 * Отвечает за хранение, применение, сериализацию и десериализацию пресетов для актёров.
 * Реализует паттерн Singleton. Следит за событиями загрузки актёров, применяет пресеты,
 * обеспечивает потокобезопасность, поддерживает отложенное применение пресетов, если
 * PresetsManager или сериализация ещё не готовы.
 */
class ActorsManager :
    public RE::BSTEventSink<RE::TESObjectLoadedEvent>,
    public RE::BSTEventSource<RE::TESObjectLoadedEvent>
{
public:
    using CallbackId = size_t;

    /**
     * @brief Получить единственный экземпляр менеджера (Singleton).
     */
    static ActorsManager& get();

    /**
     * @brief Содержится ли актёр в базе. Не проверяет isReady(), по этому значение может быть неверным.
     * @param actor актёр.
     * @return true если содержится, false во всех остальных случаях.
     */
    bool containsActor(const RE::Actor* actor) const noexcept;

    /**
     * @brief Содержится ли актёр в базе. Не проверяет isReady(), по этому значение может быть неверным. Не проверяет действительно ли formId принадлежит актёру.
     * @param formId актёра.
     * @return true если содержится, false во всех остальных случаях.
     */
    bool containsActor(uint32_t formId) const noexcept;

    /**
     * @brief Проверить, готов ли менеджер к работе (пресеты валидированы и десериализация завершена).
     * @return true если готов, иначе false.
     */
    bool isReady();

    /**
     * @brief Обработка события загрузки объекта (актеров).
     * @param a_event Событие загрузки объекта.
     * @param a_source Источник события.
     * @return Контроль дальнейшей обработки событий.
     */
    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESObjectLoadedEvent& a_event,
        RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_source) override;

    /**
     * @brief Подписаться на событие завершения десериализации.
     * @param callback Функция-обработчик.
     * @param oneShot Если true, обработчик будет вызван только один раз.
     * @return Идентификатор подписки.
     */
    CallbackId subscribeOnDeserialized(const std::function<void()>& callback, bool oneShot = false);

    /**
     * @brief Отписаться от события завершения десериализации.
     * @param id Идентификатор подписки.
     * @return true если успешно отписались.
     */
    bool unsubscribeFromDeserialized(CallbackId id);

private:
    /**
     * @brief Конструктор. Инициализирует менеджер, подписывается на события и обеспечивает корректную обработку очереди ожидания.
     */
    ActorsManager();

    // Запрет копирования и перемещения
    ActorsManager(const ActorsManager&) = delete;
    ActorsManager& operator=(const ActorsManager&) = delete;

    /**
     * @brief Сериализация пресетов в co-save.
     */
    void serialize(const F4SE::SerializationInterface*);

    /**
     * @brief Десериализация пресетов из co-save.
     */
    void deserialize(const F4SE::SerializationInterface*);

    /**
     * @brief Очистить все сохранённые пресеты.
     */
    void clear() noexcept;

    /**
     * @brief Проверить актёра по formId (существует, валиден, не исключён).
     * @param formId Идентификатор формы.
     * @return Указатель на актёра или nullptr.
     */
    RE::Actor* checkActorByFormId(uint32_t formId);

    /**
     * @brief Проверить актёра (валидность, раса, исключения).
     * @param actor Указатель на актёра.
     * @return Указатель на актёра или nullptr.
     */
    RE::Actor* checkActor(RE::Actor* actor);

	/// @copydoc checkActor
    const RE::Actor* checkActor(const RE::Actor* actor);

    /**
     * @brief Применить пресет к актёру. Не обновляет m_presets.
     * @param actor Указатель на актёра.
     * @param preset Пресет для применения.
     * @return true если успешно применён.
	 */
    bool applyActorPreset(RE::Actor* actor, const std::shared_ptr<Preset>& preset);


    /**
     * @brief Обновляет пресеты актёра в хранилище. Не применяет пресеты, а только обновляет их в m_presets. Добавляет либо делает присваивание actorPresets.second. Не проверяет валидность актёра, пресета, расы или исключения.
     * @param actor Указатель на актёра.
     * @param preset Пресет для обновления, будет заменён/добавлен пресет такого же типа в наборе пресетов персонажа.
     * @return true если успешно.
     */
    bool updateActorPresetInStorage(RE::Actor* actor, const std::shared_ptr<Preset>& preset) noexcept;

    /**
	 * @brief Применить пресеты к актёру. Только применяет пресеты, не добавляет в m_presets.
     * @param actorPresets Пара formId и набор пресетов.
     * @return true если хотя бы один пресет был применён.
     */
    bool applyActorPresets(const ActorsManagerDefs::ActorPreset& actorPresets);

    /**
	 * @brief Обновляет пресеты актёра в хранилище. Не применяет пресеты, а только обновляет их в m_presets. Добавляет либо делает присваивание actorPresets.second. Не проверяет валидность актёра, пресета, расы или исключения.
     * @param actor Указатель на актёра.
     * @param presets Набор пресетов для обновления.
     * @return true если успешно.
	 */

    bool updateActorPresetsInStorage(const ActorsManagerDefs::ActorPreset& actorPresets) noexcept;

    /**
     * @brief Проверить, соответствует ли актёр допустимой расе.
     */
    bool isQualifiedRace(const RE::Actor* actor) const noexcept;

    /**
     * @brief Проверить, исключён ли актёр из применения пресетов.
     */
    bool isExcluded(const RE::Actor* actor) const noexcept;

    /**
     * @brief Вызвать все подписки на завершение десериализации.
     */
    void fireDeserialized();

    // --- Данные ---
    PresetsManager& m_pmanager = PresetsManager::get();
    std::unordered_map<uint32_t, ActorsManagerDefs::Presets> m_presets;
    mutable std::mutex m_mutex;
    ThreadSafeWaitingActors m_waitingActors;
    DirectApply m_directApply;

    std::atomic_bool m_deserialized{ false };
    std::vector<std::pair<CallbackId, std::function<void()>>> m_deserializedCallbacks;
    std::mutex m_deserializedCallbacksMutex;
    std::atomic<CallbackId> m_nextDeserializedCallbackId{ 1 };

    static void serializationCallback(const F4SE::SerializationInterface* a_intfc);
    static void deserializationCallback(const F4SE::SerializationInterface* a_intfc);
};
