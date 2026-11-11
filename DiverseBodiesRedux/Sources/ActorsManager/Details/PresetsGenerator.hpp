#pragma once
#include <span>
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
//#include "Ini/Ini.h"
#include "ActorsPresetHashEquals.hpp"
#include "PresetsManager/PresetsManager.h"
#include "Utils/RandomGenerator.hpp"
#include "Overrides/SessionOverrides.h"

//namespace globals {
//	extern ini::map* g_ini;
//}

/**
 * @brief Генератор случайных пресетов для актёра.
 *
 * Класс PresetsGenerator предоставляет функциональность для выбора и назначения случайных пресетов
 * определённому актёру. Для каждого типа пресета выбирается не более одного случайного варианта.
 * Взаимодействует с PresetsManager для получения доступных пресетов для заданного актёра
 * и поддерживает обновление целевого актёра во время выполнения.
 */

class PresetsGenerator {
public:

    /**
     * @brief Конструктор по умолчанию.
     */
	inline PresetsGenerator() = default;

    /**
     * @brief Копирующий конструктор.
     */
	inline PresetsGenerator(const PresetsGenerator&) = default;

    /**
     * @brief Перемещающий конструктор.
     */
	inline PresetsGenerator(PresetsGenerator&&) = default;

    /**
     * @brief Оператор копирующего присваивания.
     */
	inline PresetsGenerator& operator=(const PresetsGenerator&) = default;

    /**
     * @brief Оператор перемещающего присваивания.
     */
	inline PresetsGenerator& operator=(PresetsGenerator&&) = default;

	/**
     * @brief Конструктор с указанием актёра.
     * @param actor Указатель на объект актёра.
     *
     * Проверяет валидность переданного актёра (только на nullptr) и инициализирует генератор.
     */
	inline PresetsGenerator(RE::Actor* actor) {
        updateActor(actor);
	}

    /**
     * @brief Получить случайный набор пресетов для текущего актёра.
     * @return Пара: formID актёра и набор случайно выбранных пресетов по типам.
     *
	 * Для каждого типа пресета выбирает один случайный пресет (если есть). Пресеты выбираются из максимально подходящих по условиям.
     */
    inline ActorsManagerDefs::ActorPreset getRandomPresets() const noexcept {
        if (!m_actor) {
            logger::error("PresetsRandomizer: Actor is nullptr.");
            return {};
        }
        ActorsManagerDefs::ActorPreset actorPreset{ m_actor->formID, ActorsManagerDefs::Presets{} };

        // Перебираем все типы пресетов, кроме NONE и END
        for (int typeInt = static_cast<int>(PresetType::NONE) + 1;
            typeInt < static_cast<int>(PresetType::END);
            ++typeInt)
        {
            PresetType type = static_cast<PresetType>(typeInt);
			// Нужно пропустить оверлеи, если это не человеческая раса, т.к. у них не отображаются нормально оверлеи на коже
            if (type != PresetType::BODYMORPHS) {

                static RE::TESRace* HumanRace = RE::fallout_cast<RE::TESRace*>(RE::TESForm::GetFormByID(0x13746));
                static RE::TESRace* SynthGen2Race = RE::fallout_cast<RE::TESRace*>(RE::TESForm::GetFormByID(0x10BD65));

                auto actorRace = m_actor->GetVisualsRace();
                if (!actorRace || ((actorRace != HumanRace) && (actorRace != SynthGen2Race)))
				    continue;
            }

            // Собираем все пресеты нужного типа и вычисляем уровень совпадения
            struct ScoredPreset {
                std::shared_ptr<Preset> preset;
                int level;
            };
            std::vector<ScoredPreset> scoredPresets;
            int maxLevel = 0;

            for (const auto& preset : m_presets) {
                // Пресет, отключённый игроком в этой сессии (оверрайды), автоматически не раздаётся.
                if (preset && preset->type() == type && !SessionOverrides::get().isPresetDisabled(preset->id())) {
                    int level = static_cast<int>(preset->check(m_actor));
                    if (level > maxLevel)
                        maxLevel = level;
                    scoredPresets.push_back({ preset, level });
                }
            }
            if (scoredPresets.empty())
                continue;
            
            // Оставляем только пресеты с максимальным уровнем совпадения
            std::vector<std::shared_ptr<Preset>> filteredPresets;
            for (const auto& sp : scoredPresets) {
                if (sp.level == maxLevel)
                    filteredPresets.push_back(sp.preset);
            }
            if (filteredPresets.empty())
                continue;

            // Если один — берём его, если несколько — случайный
            if (filteredPresets.size() == 1) {
                actorPreset.second.emplace(filteredPresets.front());
            }
            else {
                utils::RandomGenerator<uint32_t> rnd{};
                uint32_t idx = rnd(0u, static_cast<uint32_t>(filteredPresets.size() - 1u));
                actorPreset.second.emplace(filteredPresets[idx]);
            }
        }

        return actorPreset;
    }

    /**
     * @brief Установить нового актёра для генератора.
     * @param actor Указатель на объект актёра.
     */
    inline void setActor(RE::Actor* actor) {
        updateActor(actor);
	}

private:
	RE::Actor* m_actor = nullptr;
	std::vector<std::shared_ptr<Preset>> m_presets;

    /**
     * @brief Обновить внутреннего актёра и связанные с ним пресеты.
     * @param actor Указатель на объект актёра.
     *
     * Проверяет валидность актёра и наличие пресетов для него.
     */
    inline void updateActor(RE::Actor* actor) {
        if (!actor) {
            logger::error("PresetsRandomizer: Actor is nullptr.");
            return;
        }
        m_actor = actor;

        PresetsManager& pmanager = PresetsManager::get();

        m_presets = pmanager[actor];

        if (m_presets.empty()) {
            logger::error("PresetsRandomizer: No presets found for actor with formID {}.", actor->formID);
            return;
        }
	}
};
