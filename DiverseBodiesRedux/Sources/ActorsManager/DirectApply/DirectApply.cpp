#include "DirectApply.h"
#include "../../PresetsManager/PresetsManager.h"
#include "globals.h"
#include "Hooks/Hooks.h"


void Hooked_PerformInputProcessing(RE::PlayerControls* self, const RE::InputEvent* a_queueHead);
// Получи адрес виртуальной функции PerformInputProcessing
using PerformInputProcessing_t = void(__fastcall*)(RE::PlayerControls*, const RE::InputEvent*);


void Hooked_PerformInputProcessing(RE::PlayerControls* self, const RE::InputEvent* a_queueHead)
{
    // Перебираем цепочку событий
    if (!a_queueHead) {
        return static_cast<Hooks<PerformInputProcessing_t>*>(DirectApply::m_performInputProcessingHook)->GetOriginal()(self, a_queueHead);
	}
    for (auto event = a_queueHead; event; event = event->next) {
        if (event->eventType.get() == RE::INPUT_EVENT_TYPE::kButton && static_cast<const RE::ButtonEvent*>(event)->GetBSButtonCode() == RE::BS_BUTTON_CODE::kNumpad_Divide) {
            DirectApply::m_buttonEventHandler(static_cast<const RE::ButtonEvent*>(event));
        }
    }
    // Вызов оригинала
    static_cast<Hooks<PerformInputProcessing_t>*>(DirectApply::m_performInputProcessingHook)->GetOriginal()(self, a_queueHead);
}

DirectApply::DirectApply(
    const std::unordered_map<uint32_t, ActorsManagerDefs::Presets>* const presets,
    const std::mutex* mutex,
    const ApplyPreset& applyFunc,
    const UpdateInStorage& updateInStorage
) : m_presets(presets), m_mutex(mutex), m_applyPresetFunc(applyFunc), m_updateInStorageFunc(updateInStorage),
m_movieDef(nullptr), m_movie(nullptr), m_state(MENU_STATE::NONE), m_options{}, m_maxActorsCount(10), m_initialized(false)
{
    ++m_counter;

    constexpr const char* swfPath = "Data\\Interface\\DiverseBodiesMenu.swf";
    if (!std::filesystem::exists(swfPath)) { 
        logger::error("SWF file not found: {}", swfPath);
        return;
	}

    m_buttonEventHandler = [this](const RE::ButtonEvent* a_event) {
        if (m_state == MENU_STATE::NONE && isMenuInitialized()) {
            showActors();
        }
        else if (m_state != MENU_STATE::NONE && isMenuInitialized()) {
            hide();
        }
        };

    if (!m_performInputProcessingHook) {
        uintptr_t vtbl = *(uintptr_t*)RE::PlayerControls::GetSingleton();
        uintptr_t funcAddr = *(uintptr_t*)(vtbl + 0x0 * sizeof(uintptr_t)); // 0-й метод в vtable
        m_performInputProcessingHook = new Hooks<PerformInputProcessing_t>(funcAddr, Hooked_PerformInputProcessing);
        static_cast<Hooks<PerformInputProcessing_t>*>(m_performInputProcessingHook)->Install();
    }

    RE::Scaleform::GFx::Loader loader{};
    
	logger::info("Trying to load SWF: {}", swfPath);
    m_movieDef = loader.CreateMovie(swfPath);
    if (!m_movieDef) {
        logger::error("MovieDef is not initialized.");
        m_movie = nullptr;
        return;
    }

    m_movie = m_movieDef->CreateInstance();
    if (!m_movie) {
        logger::error("Movie is not initialized.");
        m_movieDef = nullptr;
        return;
    }

    RE::Scaleform::GFx::Value root;
    m_movie->GetVariable(&root, "root");
    root.SetMember("Call", RE::Scaleform::GFx::Value(this));

    m_initialized = true;
}

DirectApply::~DirectApply() {
    --m_counter;
    if (m_counter == 0 && m_performInputProcessingHook) {
		static_cast<Hooks<PerformInputProcessing_t>*>(m_performInputProcessingHook)->Uninstall();
        delete static_cast<Hooks<PerformInputProcessing_t>*>(m_performInputProcessingHook);
        m_performInputProcessingHook = nullptr;
	}
    
    if (m_buttonEventHandler) {
		m_buttonEventHandler = nullptr; // Сброс обработчика событий кнопок
    }
    if (m_movie) {
        m_movie->Release();
        m_movie = nullptr;
    }
    if (m_movieDef) {
        m_movieDef->Release();
        m_movieDef = nullptr;
    }

	m_initialized = false;
}

bool DirectApply::isMenuInitialized() const noexcept {
    return m_initialized;
}

void DirectApply::showActors() {
    if (!m_movie) {
        logger::error("Movie is not initialized, cannot show actors.");
        return;
    }

    RE::Scaleform::GFx::Value menuDataArray;
    m_movie->CreateArray(&menuDataArray);
    auto actorsVector = getActorsInRange(RE::PlayerCharacter::GetSingleton(), 1000, m_maxActorsCount);

    auto count = 0;
    auto active = 0;
    for (const auto& actor : actorsVector) {
        if (!actor) {
            logger::error("Invalid ActorObject in menuDataVector");
            continue;
        }

        if (actor == m_options.actor) {
            active = count;
            m_options.actor = nullptr;
        }

        RE::Scaleform::GFx::Value actorObj;
        m_movie->CreateObject(&actorObj);
        actorObj.SetMember("type", MENU_OBJECT::BUTTON);
        actorObj.SetMember("labelText", fmt::format("[{}] {}", actor->formID, actor->GetDisplayFullName()).c_str());
        menuDataArray.PushBack(actorObj);
        ++count;
    }

    m_state = MENU_STATE::ACTORS;

    // ПРАВИЛЬНЫЙ ВЫЗОВ:
    RE::Scaleform::GFx::Value args[2];
    args[0] = menuDataArray;
    args[1] = RE::Scaleform::GFx::Value(active);

    RE::Scaleform::GFx::Value result;
    m_movie->asMovieRoot->Invoke("showMenu", &result, args, 2);
}

void DirectApply::showOptions() {
    RE::Scaleform::GFx::Value menuDataArray;
    m_movie->CreateArray(&menuDataArray);

    // Лейбл актёра
    {
        RE::Scaleform::GFx::Value labelObj;
        m_movie->CreateObject(&labelObj);
        labelObj.SetMember("type", MENU_OBJECT::LABEL);
        labelObj.SetMember("labelText", m_options.actor ?
            fmt::format("[{}] {}", 
                m_options.actor->formID, 
                m_options.actor->GetDisplayFullName()).c_str()
            : "Unknown actor");
        menuDataArray.PushBack(labelObj);
    }

    // Свитчер для выбора типа пресета
    {
        std::string bodyTypeObjects;
        for (auto type = PresetType::BODYMORPHS; type < PresetType::END; type = static_cast<PresetType>(static_cast<int>(type) + 1)) {
            if (!bodyTypeObjects.empty())
                bodyTypeObjects += ",";
            bodyTypeObjects += GetPresetTypeString(type);
        }
        RE::Scaleform::GFx::Value switcherObj;
        m_movie->CreateObject(&switcherObj);
        switcherObj.SetMember("type", MENU_OBJECT::SWITCHER);
        switcherObj.SetMember("labelText", bodyTypeObjects.c_str());
        switcherObj.SetMember("index", static_cast<int>(m_options.presetType));
        menuDataArray.PushBack(switcherObj);
    }

    // Вспомогательная лямбда для чекбоксов
    auto pushCheckbox = [&menuDataArray, this](const char* label, Filter filterOption) {
        RE::Scaleform::GFx::Value checkBox;
        m_movie->CreateObject(&checkBox);
        checkBox.SetMember("type", MENU_OBJECT::CHECKBOX);
        checkBox.SetMember("labelText", label);
        checkBox.SetMember("selected", static_cast<bool>(m_options.filter & filterOption));
        menuDataArray.PushBack(checkBox);
        };

    pushCheckbox("FormID", Filter::FormID);
    pushCheckbox("EditorID", Filter::EditorID);
    pushCheckbox("HasKeyword", Filter::HasKeyword);
    pushCheckbox("HasNotKeyword", Filter::HasNotKeyword);
    pushCheckbox("InFaction", Filter::InFaction);
    pushCheckbox("NotInFaction", Filter::NotInFaction);

    // Кнопка "Apply"
    {
        RE::Scaleform::GFx::Value applyButton;
        m_movie->CreateObject(&applyButton);
        applyButton.SetMember("type", MENU_OBJECT::BUTTON);
        applyButton.SetMember("labelText", "Apply");
        menuDataArray.PushBack(applyButton);
    }

    m_state = MENU_STATE::OPTIONS;

    RE::Scaleform::GFx::Value args[2];
    args[0] = menuDataArray;
    args[1] = RE::Scaleform::GFx::Value(1); // активен switcher

    RE::Scaleform::GFx::Value result;
    m_movie->asMovieRoot->Invoke("updateMenu", &result, args, 2);
}

void DirectApply::showPresets(int active) {
    if (!m_options.actor) {
        logger::error("Current actor is not set, cannot show presets menu.");
        return;
    }

    if (static_cast<int>(m_options.filter & Filter::Gender) == 0) {
        m_options.filter |= Filter::Gender; // Пол должен быть всегда включён
    }

    auto filter = [this](const RE::Actor* actor, const Preset& preset)->bool {
        return static_cast<bool>(preset.check(actor, m_options.filter));
        };

    auto& manager = PresetsManager::get();
    static auto presetsArray = manager.getPresets(m_options.actor, filter);

    RE::Scaleform::GFx::Value menuDataArray;
    m_movie->CreateArray(&menuDataArray);

    for (const auto& preset : presetsArray) {
        if (!preset) {
            continue;
        }
        RE::Scaleform::GFx::Value presetButton;
        m_movie->CreateObject(&presetButton);
        presetButton.SetMember("type", MENU_OBJECT::BUTTON);
        presetButton.SetMember("labelText", preset->id().c_str());
        menuDataArray.PushBack(presetButton);
    }

    RE::Scaleform::GFx::Value args[2];
    args[0] = menuDataArray;
    args[1] = RE::Scaleform::GFx::Value(active);

    RE::Scaleform::GFx::Value result;
    m_movie->asMovieRoot->Invoke("updateMenu", &result, args, 2);
}

void DirectApply::hide() {
    if (m_movie) {
        RE::Scaleform::GFx::Value result;
        m_movie->asMovieRoot->Invoke("hideMenu", &result, nullptr, 0);
    }
    else {
        logger::error("Movie is not initialized, cannot hide menu.");
    }

    m_state = MENU_STATE::NONE;
    m_options = Options{};
}

void DirectApply::Call(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    if (!m_movie || !m_movieDef) {
        logger::error("Movie or MovieDef is not initialized, cannot process Call.");
        return;
    }

    if (m_state == MENU_STATE::NONE) {
        logger::error("Menu is not active, cannot process Call.");
        return;
    }

    if (params.argCount < 1) {
        logger::error("Invalid number of parameters in Call.");
        return;
    }

    if (params.args[0].GetType() != RE::Scaleform::GFx::Value::ValueType::kConvertString) {
        logger::error("First argument of Call must be a string.");
        return;
    }

    std::string signal_name = params.args[0].GetString();

    if (signal_name == "push") {
        if (m_state == MENU_STATE::ACTORS) {
            actorsPushCall(params);
        } else if (m_state == MENU_STATE::OPTIONS) {
            optionsPushCall(params);
        } else if (m_state == MENU_STATE::PRESETS) {
            presetsPushCall(params);
        } else {
			logger::error("Invalid state for push call: {}", static_cast<int>(m_state));
        }
    } else if (signal_name == "back") {
        if (m_state == MENU_STATE::ACTORS) {
            actorsBackCall(params);
        }
        else if (m_state == MENU_STATE::OPTIONS) {
            optionsBackCall(params);
        }
        else if (m_state == MENU_STATE::PRESETS) {
            presetsBackCall(params);
		} else {
			logger::error("Invalid state for back call: {}", static_cast<int>(m_state));
        }
    } else if (signal_name == "hover") {
        if  (m_state == MENU_STATE::PRESETS) {
            presetsHoverCall(params);
        } 
    } else {
        logger::error("Unknown signal name: {}", signal_name);
        return;
    }
}

void DirectApply::actorsPushCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    if (params.argCount < 2) {
        logger::error("Invalid number of parameters in Call.");
        return;
    }

    if (params.args[1].GetType() != RE::Scaleform::GFx::Value::ValueType::kConvertString) {
        logger::error("Second argument of Call must be a number.");
        return;
    }

    std::string element = params.args[1].GetString();
    // Получим значение элемента менжду символами [ и ]
    auto start = element.find('[');
    auto end = element.find(']');
    if (start == std::string::npos || end == std::string::npos || start >= end) {
        logger::error("Invalid actor element format: {}", element);
        return;
    }
    std::string actorIdStr = element.substr(start + 1, end - start - 1);
    try {
        auto actorId = std::stoul(actorIdStr, nullptr, 16); // Преобразуем в число
        m_options.actor = functions::getFormByFormID<RE::Actor>(actorId);
    }
    catch (const std::exception& e) {
        logger::error("Failed to parse actor ID from element '{}': {}", element, e.what());
        return;
    }
    catch (...) {
        logger::error("Unknown error while parsing actor ID from element '{}'.", element);
        return;
    }
    if (!m_options.actor) {
        logger::error("Actor with ID {} not found.", element);
        return;
    }

    showOptions();
}

void DirectApply::optionsPushCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    if (params.argCount < 2) {
        logger::error("Invalid number of parameters in Call.");
        return;
    }

    if (params.args[1].GetType() != RE::Scaleform::GFx::Value::ValueType::kConvertString) {
        logger::error("Second argument of Call must be a number.");
        return;
    }

    std::string element = params.args[1].GetString();
    if (auto presetType = getPresetTypeFromString(element); presetType != PresetType::NONE) {
        m_options.presetType = presetType;
    }
    else if (element == "FormID") {
        m_options.filter |= Filter::FormID;
    }
    else if (element == "EditorID") {
        m_options.filter |= Filter::EditorID;
    }
    else if (element == "HasKeyword") {
        m_options.filter |= Filter::HasKeyword;
    }
    else if (element == "HasNotKeyword") {
        m_options.filter |= Filter::HasNotKeyword;
    }
    else if (element == "InFaction") {
        m_options.filter |= Filter::InFaction;
    }
    else if (element == "NotInFaction") {
        m_options.filter |= Filter::NotInFaction;
    }
    else if (element == "Apply") {
        // Применяем пресет
        if (!m_options.actor) {
            logger::error("No actor selected, cannot apply preset.");
            return;
        }
        showPresets();
    }
}

void DirectApply::presetsPushCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    if (m_options.actor == nullptr) {
        logger::error("No valid actor selected, cannot apply preset.");
        return;
    }
    
    if (params.argCount < 2) {
        logger::error("Invalid number of parameters in Call.");
        return;
    }

    if (params.args[1].GetType() != RE::Scaleform::GFx::Value::ValueType::kConvertString) {
        logger::error("Second argument of Call must be a number.");
        return;
    }
    
    std::string element = params.args[1].GetString();
    auto& pmanager = PresetsManager::get();
    auto preset = pmanager.getPreset(element);

    if (!preset) {
        logger::error("Preset '{}' not found.", element);
        return;
    }

    m_updateInStorageFunc(m_options.actor, preset);
}

void DirectApply::actorsBackCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    hide();
}

void DirectApply::optionsBackCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    if (params.argCount < 2) {
        logger::error("Invalid number of parameters in Call.");
        return;
    }

    if (params.args[1].GetType() != RE::Scaleform::GFx::Value::ValueType::kConvertString) {
        logger::error("Second argument of Call must be a number.");
        return;
    }
    
    std::string element = params.args[1].GetString();
    if (auto presetType = getPresetTypeFromString(element); presetType != PresetType::NONE) {
		m_options.presetType = presetType;
    } else if (element == "FormID") {
        m_options.filter &= ~Filter::FormID;
    }
    else if (element == "EditorID") {
        m_options.filter &= ~Filter::EditorID;
    }
    else if (element == "HasKeyword") {
        m_options.filter &= ~Filter::HasKeyword;
    }
    else if (element == "HasNotKeyword") {
        m_options.filter &= ~Filter::HasNotKeyword;
    }
    else if (element == "InFaction") {
        m_options.filter &= ~Filter::InFaction;
    }
    else if (element == "NotInFaction") {
        m_options.filter &= ~Filter::NotInFaction;
    }
    else {
        showActors();
    }
}

void DirectApply::presetsBackCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    showOptions();
}

PresetType DirectApply::getPresetTypeFromString(const std::string& str) {
    for (auto i = 1; i < static_cast<int>(PresetType::END); ++i) {
		if (GetPresetTypeString(static_cast<PresetType>(i)) == str)
		    return static_cast<PresetType>(i);
	}
	return PresetType::NONE; // Возвращаем NONE, если не найдено совпадение
}

void DirectApply::presetsHoverCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    if (m_options.actor == nullptr) {
        logger::error("No valid actor selected, cannot apply preset.");
        return;
    }
    
    if (params.argCount < 2) {
        logger::error("Invalid number of parameters in Call.");
        return;
    }

    if (params.args[1].GetType() != RE::Scaleform::GFx::Value::ValueType::kConvertString) {
        logger::error("Second argument of Call must be a number.");
        return;
    }

    std::string element = params.args[1].GetString();
    auto& pmanager = PresetsManager::get();
    auto preset = pmanager.getPreset(element);

    if (!preset) {
        logger::error("Preset '{}' not found.", element);
        return;
    }

    m_applyPresetFunc(m_options.actor, preset);
}

std::vector<RE::Actor*> DirectApply::getActorsInRange(RE::TESObjectREFR* a_ref,
    std::uint32_t a_maxDistance, int a_maxActorsCount, bool a_includeDead)
{
    std::vector<RE::Actor*> result;

    auto originPos{ a_ref->GetPosition() };

    const auto processLists = RE::ProcessLists::GetSingleton();

    for (const auto& actorHandle : processLists->highActorHandles) {
        const auto actorPtr = actorHandle.get();
        const auto currentActor = actorPtr.get();

        if (!currentActor) {
            continue;
        }

        if (!a_includeDead && currentActor->IsDead(true)) {
            continue;
        }

        if (currentActor == RE::PlayerCharacter::GetSingleton()) {
            continue; // Пропускаем игрока
		}

        if (a_ref->GetPosition().GetDistance(currentActor->GetPosition()) <= a_maxDistance) {
            result.push_back(currentActor);
        }
    }

    if (result.size() < a_maxActorsCount)
        return result;
    else {
        return std::vector<RE::Actor*>(result.begin(), result.begin() + a_maxActorsCount);
    }
}

