#include "Tint.h"
#include "globals.h"
#include "LooksMenu/ParseLooksMenuPreset.h"
#include <type_traits>
#include <cstring>  // Для memset

namespace logger = F4SE::log;

Tint::Tint(uint32_t id, uint8_t percent) :
	m_id(id), m_percent(percent) {
}

Tint::Tint(uint32_t uniqueID, const boost::json::object& jsonObject) {
    if (!loadFromJsonObject(uniqueID, jsonObject)) {
        logger::error("Tint::Tint:{}: Failed to create from JSON object for uniqueID {}", 
            uniqueID != 0 ? "loadFromLooksMenuPresetObject" : "loadFromJsonObject", 
            uniqueID
        );
	}
}

Tint::~Tint() noexcept {}; // Реализация чистого виртуального деструктора

bool Tint::operator==(const Tint& other) const noexcept {
    if (getType() != other.getType()) { return false; }
    return m_id == other.m_id && m_percent == other.m_percent;
}

bool Tint::operator<(const Tint& other) const noexcept {
	if (getType() != other.getType()) return getType() < other.getType();
    return m_id != other.m_id ? m_id < other.m_id : m_percent < other.m_percent;
}

uint32_t Tint::id() const noexcept {
	return m_id;
}

uint8_t Tint::percent() const noexcept {
	return m_percent;
}

void Tint::setId(uint32_t id) noexcept {
	m_id = id;
}

void Tint::setPercent(uint8_t percent) noexcept {
	m_percent = percent;
}

bool Tint::empty() const noexcept {
    return m_id == 0;
}
void Tint::clear() noexcept {
    m_id = 0;
    m_percent = 0;
}

bool Tint::apply(RE::TESNPC* npc) const {
	if (!npc || !npc->tintingData) {
        return false;
    }

    return AddTint(npc, *this) ? true : false;
}

bool Tint::remove(RE::TESNPC* npc) const {
	return RemoveTint(npc, m_id);
}

RE::BGSCharacterTint::Entry* Tint::CreateTintEntry(const Tint& tint, RE::BGSCharacterTint::Template::Entry* templateEntry) {
    if (!templateEntry) return nullptr;

    RE::BGSCharacterTint::Entry* newTintEntry = nullptr;

    // Определяем тип шаблона через RTTI
    auto rttiName = std::string{ typeid(*templateEntry).name() };

    // Функция для создания MaskEntry с правильными vtable и инициализацией
    auto createMaskEntry = [](void* memory, RE::BGSCharacterTint::Template::Entry* templateEntry, const Tint& tint) -> RE::BGSCharacterTint::MaskEntry* {
        // Очищаем память
        memset(memory, 0, sizeof(RE::BGSCharacterTint::MaskEntry));
        
        // Устанавливаем VTABLE из игры вручную
        REL::Relocation<uintptr_t> gameVTable{RE::VTABLE::BGSCharacterTint__MaskEntry[0]};
        *reinterpret_cast<uintptr_t*>(memory) = gameVTable.get();
        
        // Инициализируем поля как в игре
        auto* entry = static_cast<RE::BGSCharacterTint::MaskEntry*>(memory);
        entry->templateEntry = templateEntry;
        entry->percent = tint.percent();
        entry->tintIndex = static_cast<uint16_t>(tint.id() & 0xFFFF);
        entry->pad13 = 0;
        entry->pad14 = 0;
        
        return entry;
    };

    // Функция для создания PaletteEntry с правильными vtable и инициализацией
    auto createPaletteEntry = [](void* memory, RE::BGSCharacterTint::Template::Entry* templateEntry, const Tint& tint) -> RE::BGSCharacterTint::PaletteEntry* {
        // Очищаем память
        memset(memory, 0, sizeof(RE::BGSCharacterTint::PaletteEntry));
        
        // Устанавливаем VTABLE из игры вручную
        REL::Relocation<uintptr_t> gameVTable{RE::VTABLE::BGSCharacterTint__PaletteEntry[0]};
        *reinterpret_cast<uintptr_t*>(memory) = gameVTable.get();
        
        auto* entry = static_cast<RE::BGSCharacterTint::PaletteEntry*>(memory);
        entry->templateEntry = templateEntry;
        entry->percent = tint.percent();
        entry->tintIndex = static_cast<uint16_t>(tint.id() & 0xFFFF);
        entry->pad13 = 0;
        entry->pad14 = 0;
        
        // Специальная инициализация для PaletteEntry (как в дизассемблированном коде)
        #pragma warning(push)
        #pragma warning(disable : 4309)
        entry->color.bgra = 0xFFFFFFFF;  // как в игре
        entry->colorID = 0xFFFF;         // как в игре
        entry->pad1E = 0;
        #pragma warning(pop)
        
        // Если есть данные цвета из tint
        if (tint.getType() == Tint::Type::kPalette) {
            const auto* paletteTint = static_cast<const PaletteTint*>(&tint);
            entry->color.bgra = paletteTint->getColor().bgra;
            entry->colorID = paletteTint->colorID();
        }
        
        return entry;
    };

    // Функция для создания TextureSetEntry с правильными vtable и инициализацией
    auto createTextureSetEntry = [](void* memory, RE::BGSCharacterTint::Template::Entry* templateEntry, const Tint& tint) -> RE::BGSCharacterTint::TextureSetEntry* {
        // Очищаем память
        memset(memory, 0, sizeof(RE::BGSCharacterTint::TextureSetEntry));
        
        // Устанавливаем VTABLE из игры вручную
        REL::Relocation<uintptr_t> gameVTable{RE::VTABLE::BGSCharacterTint__TextureSetEntry[0]};
        *reinterpret_cast<uintptr_t*>(memory) = gameVTable.get();
        
        // Инициализируем поля как в игре
        auto* entry = static_cast<RE::BGSCharacterTint::TextureSetEntry*>(memory);
        entry->templateEntry = templateEntry;
        entry->percent = tint.percent();
        entry->tintIndex = static_cast<uint16_t>(tint.id() & 0xFFFF);
        entry->pad13 = 0;
        entry->pad14 = 0;
        
        return entry;
    };

    try {
        if (rttiName.find("BGSCharacterTint::Template::Mask") != std::string::npos) {
            void* memory = tints_memory::AllocateScrapHeap<RE::BGSCharacterTint::MaskEntry>();
            if (!memory) {
                logger::error("CreateTintEntry: Failed to allocate memory for MaskEntry");
                return nullptr;
            }
            
            logger::info("CreateTintEntry: Creating MaskEntry for tint ID {}", tint.id());
            newTintEntry = createMaskEntry(memory, templateEntry, tint);
        }
        else if (rttiName.find("BGSCharacterTint::Template::Palette") != std::string::npos) {
            void* memory = tints_memory::AllocateScrapHeap<RE::BGSCharacterTint::PaletteEntry>();
            if (!memory) {
                logger::error("CreateTintEntry: Failed to allocate memory for PaletteEntry");
                return nullptr;
            }
            
            logger::info("CreateTintEntry: Creating PaletteEntry for tint ID {}", tint.id());
            newTintEntry = createPaletteEntry(memory, templateEntry, tint);
        }
        else if (rttiName.find("BGSCharacterTint::Template::TextureSet") != std::string::npos) {
            void* memory = tints_memory::AllocateScrapHeap<RE::BGSCharacterTint::TextureSetEntry>();
            if (!memory) {
                logger::error("CreateTintEntry: Failed to allocate memory for TextureSetEntry");
                return nullptr;
            }
            
            logger::info("CreateTintEntry: Creating TextureSetEntry for tint ID {}", tint.id());
            newTintEntry = createTextureSetEntry(memory, templateEntry, tint);
        }
        else {
            logger::error("CreateTintEntry: Unknown template type: {}", rttiName);
            return nullptr;
        }

        if (newTintEntry) {
            logger::info("CreateTintEntry: Successfully created {} with vtable 0x{:X}", 
                newTintEntry->GetTypeString(), 
                *reinterpret_cast<uintptr_t*>(newTintEntry));
        }
    }
    catch (const std::exception& e) {
        logger::error("CreateTintEntry: Exception while creating tint: {}", e.what());
        if (newTintEntry) {
            // НЕ вызываем деструктор, так как объект создан без конструктора
            tints_memory::DeallocateScrapHeap(newTintEntry);
            newTintEntry = nullptr;
        }
    }
    catch (...) {
        logger::error("CreateTintEntry: Unknown exception while creating tint");
        if (newTintEntry) {
            // НЕ вызываем деструктор, так как объект создан без конструктора
            tints_memory::DeallocateScrapHeap(newTintEntry);
            newTintEntry = nullptr;
        }
    }

    return newTintEntry;
}

bool Tint::updateTint(RE::BGSCharacterTint::Entry* entry, const Tint& tint) {
    if (!entry) return false;

    entry->percent = tint.percent();

    // Для цветных тинтов также обновляем цвет и colorID
    if (tint.getType() == Tint::Type::kPalette &&
        entry->GetType() == RE::BGSCharacterTint::Entry::kTypePalette) {
        const auto* paletteTint = static_cast<const PaletteTint*>(&tint);
        auto paletteEntry = static_cast<RE::BGSCharacterTint::PaletteEntry*>(entry);
        paletteEntry->color.bgra = paletteTint->getColor().bgra;
        paletteEntry->colorID = paletteTint->colorID();
    }

    return true;
}

RE::BGSCharacterTint::Entry* Tint::AddTint(RE::TESNPC* npc, const Tint& tint) {
    if (!npc->tintingData) {
        logger::error("AddTint: Invalid NPC or tintingData is null");
        return nullptr;
	}
    auto& tints = *npc->tintingData;

	auto entry = tints.GetEntryByID(static_cast<uint16_t>(tint.id()));
    if (updateTint(entry, tint)) {
        return entry;
	}

    auto templateEntry = tint.m_templateEntry;
    if (!templateEntry) {
        templateEntry = tryToFindTemplateEntry(tint.id());
    }

    if (!templateEntry) {
        logger::error("AddTint: Template entry not found for uniqueID {}", tint.id());
        return nullptr;
	}

	entry = CreateTintEntry(tint, templateEntry);

    if (!entry) {
        logger::error("AddTint: Failed to create tint entry for uniqueID {}", tint.id());
        return nullptr;
    }

    if (entry) {
        tints.Add(entry, nullptr);
		return entry;
    }
    
    return nullptr;
}

bool Tint::RemoveTint(RE::TESNPC* npc, uint32_t tintUniqueID) {
    if (!npc || !npc->tintingData) {
        logger::error("RemoveTint: Invalid NPC or tintingData is null");
        return false;
    }

    // Находим индекс тинта для удаления
    for (uint32_t i = 0; i < npc->tintingData->size; ++i) {
        auto* entry = npc->tintingData->data[i];
        if (entry && entry->templateEntry && entry->templateEntry->uniqueID == tintUniqueID) {
            // Удаляем тинт через новый API
            npc->tintingData->RemoveAt(i);
            
            logger::info("RemoveTint: Successfully removed tint with uniqueID {}", tintUniqueID);
            
            return true;
        }
    }

    logger::warn("RemoveTint: Tint with uniqueID {} not found", tintUniqueID);
    return false;
}

RE::BGSCharacterTint::Template::Entry* Tint::tryToFindTemplateEntry(uint32_t uniqueID) {
    // Получаем шаблон HumanRace для поиска template entry
    static auto HumanRace = static_cast<RE::TESRace*>(RE::TESForm::GetFormByID(0x13746));

    if (!HumanRace || !HumanRace->faceRelatedData) {
        logger::error("FindTemplateEntry: HumanRace or faceRelatedData is null");
        return nullptr;
    }

    // Ищем шаблонную запись в обоих полах: 0 = Male, 1 = Female
    for (int gender = 0; gender < 2; ++gender) {
        auto* frd = HumanRace->faceRelatedData[gender];
        if (!frd || !frd->tintingTemplate) continue;

        for (const auto* group : frd->tintingTemplate->groups) {
            if (!group) continue;

            for (const auto* entry : group->entries) {
                if (entry && entry->uniqueID == uniqueID) {
                    return const_cast<RE::BGSCharacterTint::Template::Entry*>(entry);
                }
            }
        }
    }

    // Если шаблон не найден в HumanRace, ищем в тинтах игрока
    static auto player = RE::PlayerCharacter::GetSingleton();
    auto playerNPC = player ? player->GetNPC() : nullptr;

    if (playerNPC) {
        auto playerFace = functions::getFaceTESNPC(playerNPC);
        if (playerFace && playerFace->tintingData) {
            auto& playerTintingData = *playerFace->tintingData;
            auto entry = playerTintingData.GetEntryByID(static_cast<uint16_t>(uniqueID & 0xFFFF));

            if (entry && entry->templateEntry) {
                return entry->templateEntry;
            }
        }
    }

    logger::error("FindTemplateEntry: Template entry with uniqueID {} not found", uniqueID);
    return nullptr;
}

// ============================================================================================
// РЕАЛИЗАЦИЯ МЕТОДОВ LOADFROMJSONOBJECT
// ============================================================================================

bool Tint::loadFromJsonObject(uint32_t uniqueID, const boost::json::object& obj) noexcept {
    if (!this->empty()) {
        this->clear();
    }

    if (uniqueID == 0) {
        logger::error("Tint::loadFromLMPresetObject: Tint ID is null.");
        return false; // ID obrigatorio
    }

    logger::info("Tint::loadFromLMPresetObject: uniqueID {}: Starting to load base tint data", uniqueID);
    
    setId(uniqueID);
    
    auto it = find_ci(obj, "Percent");
    if (it != obj.end()) {
        logger::info("Tint::loadFromLMPresetObject: uniqueID {}: Processing Percent field", uniqueID);
        
        if (it->value().is_int64()) {
            auto val = it->value().as_int64();
            logger::info("Tint::loadFromLMPresetObject: uniqueID {{}}: Percent value (int64): {}", uniqueID, val);
            
            if (val < 0 || val > 255) {
                logger::error("Tint::loadFromLMPresetObject: uniqueID {}: 'Percent' field is not in range (0 - 255): {}", uniqueID, val);
                return false;
			}
            m_percent = static_cast<uint8_t>(val);
        } else if (it->value().is_double()) {
            auto val = it->value().as_double();
            logger::info("Tint::loadFromLMPresetObject: uniqueID {}: Percent value (double): {}", uniqueID, val);
            
            // BGSCharacterTint::Entry::percent хранится в диапазоне 0..100 (движок умножает на 2.55f в GetBodyTintColor).
            auto scaledVal = (val >= 0.0 && val <= 1.0) ? (val * 100.0) : val;
            if (scaledVal < 0.0 || scaledVal > 255.0) {
                logger::error("Tint::loadFromLMPresetObject: uniqueID {}: 'Percent' field (scaled) is out of range: {} -> {}", uniqueID, val, scaledVal);
                return false;
            }
            m_percent = static_cast<uint8_t>(scaledVal + 0.5);
        } else {
            logger::error("Tint::loadFromLMPresetObject: uniqueID {}: 'Percent' field is not a valid number, type: {}", uniqueID, static_cast<int>(it->value().kind()));
            return false;
        }
        
        logger::info("Tint::loadFromLMPresetObject: uniqueID {}: Final Percent value: {}", uniqueID, m_percent);
	} else {
        logger::info("Tint::loadFromLMPresetObject: uniqueID {}: No Percent field found, using default (0)", uniqueID);
        m_percent = 0;
    }

    logger::info("Tint::loadFromLMPresetObject: uniqueID {}: Successfully loaded base tint with ID: {}, Percent: {}", uniqueID, m_id, m_percent);
    return true;
}

// ============================================================================================
// MASKTINT РЕАЛИЗАЦИЯ
// ============================================================================================

MaskTint::MaskTint(uint32_t id, uint8_t percent) : Tint { id, percent } {}

MaskTint::MaskTint(uint32_t uniqueID, const boost::json::object& jsonObject) : Tint{ uniqueID, jsonObject } {
    if (this->Tint::empty()) {
        logger::error("MaskTint::MaskTint: Tint is empty, cannot create MaskTint from JSON object.");
        return;
	}

    if (!loadFromJsonObject(uniqueID, jsonObject)) {
        MaskTint::clear();
        logger::error("MaskTint::MaskTint:{}: Failed to create from JSON object for uniqueID {}",
            uniqueID != 0 ? "loadFromLooksMenuPresetObject" : "loadFromJsonObject",
            uniqueID
        );
    }
}

MaskTint::~MaskTint() noexcept {};

Tint::Type MaskTint::getType() const noexcept { return TYPE; }

bool MaskTint::operator==(const Tint& other) const noexcept {
    if (!Tint::operator==(other)) { return false; }
    // MaskTint больше не имеет дополнительных полей для сравнения
    return true;
}

bool MaskTint::operator<(const Tint& other) const noexcept {
    // MaskTint больше не имеет дополнительных полей для сравнения
    return Tint::operator<(other);
}

bool MaskTint::loadFromJsonObject(uint32_t uniqueID, const boost::json::object& obj) noexcept {
    // MaskTint больше не имеет дополнительных полей, только базовые из Tint
    return Tint::loadFromJsonObject(uniqueID, obj);
}

// ============================================================================================
// PALETTETINT РЕАЛИЗАЦИЯ
// ============================================================================================

PaletteTint::PaletteTint(uint32_t id, uint8_t percent) : Tint{ id, percent } {}

PaletteTint::PaletteTint(uint32_t uniqueID, const boost::json::object& jsonObject) : Tint{ uniqueID, jsonObject } {
    if (empty()) {
        logger::error("PaletteTint::PaletteTint: Tint is empty, cannot create PaletteTint from JSON object.");
        return;
	}

    if (!loadFromJsonObject(uniqueID, jsonObject)) {
        clear();
        logger::error("PaletteTint::PaletteTint:{}: Failed to create from JSON object for uniqueID {}",
            uniqueID != 0 ? "loadFromLooksMenuPresetObject" : "loadFromJsonObject",
            uniqueID
        );
    }
}

PaletteTint::~PaletteTint() noexcept {};

Tint::Type PaletteTint::getType() const noexcept { return TYPE; }

bool PaletteTint::operator==(const Tint& other) const noexcept {
    if (!Tint::operator==(other)) { return false; }
    const auto* otherPalette = static_cast<const PaletteTint*>(&other);
    return m_color.bgra == otherPalette->m_color.bgra 
        && m_colorID == otherPalette->m_colorID;
}

bool PaletteTint::operator<(const Tint& other) const noexcept {
    if (!Tint::operator==(other)) { return Tint::operator<(other); }
    const auto* otherPalette = static_cast<const PaletteTint*>(&other);
    if (m_color.bgra == otherPalette->m_color.bgra) { return m_colorID < otherPalette->m_colorID; }
    return (m_color.bgra < otherPalette->m_color.bgra);
}

Tint::Color PaletteTint::getColor() const noexcept { return m_color; }

void PaletteTint::setColor(const Color& color) noexcept { m_color = color; }

uint16_t PaletteTint::colorID() const noexcept { return m_colorID; }

void PaletteTint::setColorID(uint16_t colorID) noexcept { m_colorID = colorID; }

bool PaletteTint::loadFromJsonObject(uint32_t uniqueID, const boost::json::object& jsonObject) noexcept {
    if (!Tint::loadFromJsonObject(uniqueID, jsonObject)) return false;
    
    m_color.bgra = 0;
    m_colorID = 0;
    
    auto it = find_ci(jsonObject, "colorID");

    if (it != jsonObject.end()) {
        logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: Processing colorID field", uniqueID);

        if (it->value().is_int64()) {
            auto val = it->value().as_int64();
            logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: colorID value (int64): {}", uniqueID, val);

            if (val < 0) {
                logger::error("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: 'colorID' field is negative: {}", uniqueID, val);
                return false;
            }

            // Расширяем диапазон до uint32_t и обрезаем до uint16_t
            if (val > std::numeric_limits<uint32_t>::max()) {
                logger::warn("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: 'colorID' field {} exceeds uint32_t range, clamping", uniqueID, val);
                m_colorID = std::numeric_limits<uint16_t>::max();
            }
            else if (val > std::numeric_limits<uint16_t>::max()) {
                logger::warn("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: 'colorID' field {} exceeds uint16_t range, truncating to lower 16 bits", uniqueID, val);
                m_colorID = static_cast<uint16_t>(val & 0xFFFF);
            }
            else {
                m_colorID = static_cast<uint16_t>(val);
            }

            logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: Final colorID value: {}", uniqueID, m_colorID);
        }
        else if (it->value().is_double()) {
            auto val = it->value().as_double();
            logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: colorID value (double): {}", uniqueID, val);

            if (val < 0) {
                logger::error("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: 'colorID' field is negative: {}", uniqueID, val);
                return false;
            }

            // Расширяем диапазон до uint32_t и обрезаем до uint16_t
            if (val > std::numeric_limits<uint32_t>::max()) {
                logger::warn("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: 'colorID' field {} exceeds uint32_t range, clamping", uniqueID, val);
                m_colorID = std::numeric_limits<uint16_t>::max();
            }
            else if (val > std::numeric_limits<uint16_t>::max()) {
                logger::warn("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: 'colorID' field {} exceeds uint16_t range, truncating to lower 16 bits", uniqueID, static_cast<uint32_t>(val));
                m_colorID = static_cast<uint16_t>(static_cast<uint32_t>(val) & 0xFFFF);
            }
            else {
                m_colorID = static_cast<uint16_t>(val);
            }

            logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: Final colorID value: {}", uniqueID, m_colorID);
        }
        else {
            logger::error("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: 'colorID' field is not a valid number, type: {}", uniqueID, static_cast<int>(it->value().kind()));
            return false;
        }
    }
    else {
        logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: No colorID field found", uniqueID);
    }

    it = find_ci(jsonObject, "Color");
    if (it != jsonObject.end()) {
        logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: Processing Color field", uniqueID);

        if (it->value().is_int64()) {
            auto val = it->value().as_int64();
            logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: Color value (int64): {}", uniqueID, val);

            if (val < 0) {
                logger::error("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: 'Color' field is negative: {}", uniqueID, val);
                return false;
            }

            if (val > std::numeric_limits<uint32_t>::max()) {
                logger::warn("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: 'Color' field {} exceeds uint32_t range, clamping", uniqueID, val);
                m_color.bgra = std::numeric_limits<uint32_t>::max();
            }
            else {
                m_color.bgra = static_cast<uint32_t>(val);
            }

            logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: Final Color value: {} (R:{}, G:{}, B:{}, A:{})",
                uniqueID, m_color.bgra,
                (m_color.bgra >> 16) & 0xFF, (m_color.bgra >> 8) & 0xFF, m_color.bgra & 0xFF, (m_color.bgra >> 24) & 0xFF);
        }
        else if (it->value().is_double()) {
            auto val = it->value().as_double();
            logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: Color value (double): {}", uniqueID, val);

            if (val < 0) {
                logger::error("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: 'Color' field is negative: {}", uniqueID, val);
                return false;
            }

            if (val > std::numeric_limits<uint32_t>::max()) {
                logger::warn("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: 'Color' field {} exceeds uint32_t range, clamping", uniqueID, val);
                m_color.bgra = std::numeric_limits<uint32_t>::max();
            }
            else {
                m_color.bgra = static_cast<uint32_t>(val);
            }

            logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: Final Color value: {} (R:{}, G:{}, B:{}, A:{})",
                uniqueID, m_color.bgra,
                (m_color.bgra >> 16) & 0xFF, (m_color.bgra >> 8) & 0xFF, m_color.bgra & 0xFF, (m_color.bgra >> 24) & 0xFF);
        }
        else {
            logger::error("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: 'Color' field is not a valid number, type: {}", uniqueID, static_cast<int>(it->value().kind()));
            return false;
        }
    }
    else {
        logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: No Color field found", uniqueID);
    }

    logger::info("PaletteTint::loadFromLooksMenuPresetObject: uniqueID {}: Successfully loaded PaletteTint with colorID: {}, Color: {}",
        uniqueID, m_colorID, m_color.bgra);
    return true;
}

TextureSetTint::TextureSetTint(uint32_t id, uint8_t percent) : Tint{ id, percent } {}

TextureSetTint::TextureSetTint(uint32_t uniqueID, const boost::json::object& jsonObject) : Tint{ uniqueID, jsonObject } {
    if (empty()) {
        logger::error("TextureSetTint::TextureSetTint: Tint is empty, cannot create TextureSetTint from JSON object.");
        return;
    }
}

TextureSetTint::~TextureSetTint() noexcept {};

Tint::Type TextureSetTint::getType() const noexcept { return TYPE; }

bool TextureSetTint::loadFromJsonObject(uint32_t uniqueID, const boost::json::object& jsonObject) noexcept {
    // TextureSetTint наследует от Tint, поэтому используем родительскую реализацию
    return Tint::loadFromJsonObject(uniqueID, jsonObject);
}