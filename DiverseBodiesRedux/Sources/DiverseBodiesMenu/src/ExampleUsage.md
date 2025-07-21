# Примеры использования DirectApply Menu

## Базовый пример интеграции

### 1. Инициализация в конструкторе

```cpp
DirectApply::DirectApply() {
    logger::info("DirectApply: Constructor called");
    
    menuName = MENU_NAME;
    menuFlags.set(
        RE::UI_MENU_FLAGS::kUsesCursor,
        RE::UI_MENU_FLAGS::kDisablePauseMenu,
        RE::UI_MENU_FLAGS::kUsesMovementToDirection
    );
    depthPriority.set(RE::UI_DEPTH_PRIORITY::kTerminal);

    // Загрузка SWF файла
    const auto ScaleformManager = RE::BSScaleformManager::GetSingleton();
    if (!ScaleformManager->LoadMovieEx(*this, SWF_PATH, "root.menu_mc", 
                                       RE::Scaleform::GFx::Movie::ScaleModeType::kShowAll)) {
        RE::SendHUDMessage::ShowHUDMessage("Failed to load menu.", "UIMenuCancel", false, true);
        return;
    }

    // Настройка фильтров
    filterHolder = RE::msvc::make_unique<RE::BSGFxShaderFXTarget>(*uiMovie, "root.menu_mc");
    if (filterHolder) {
        filterHolder->CreateAndSetFiltersToHUD(RE::HUDColorTypes::kGameplayHUDColor);
        shaderFXObjects.push_back(filterHolder.get());
    }

    isInitialized = false; // Инициализация произойдет в ProcessMessage
    logger::info("DirectApply: Constructor completed");
}
```

### 2. Инициализация menuObj при первом показе

```cpp
RE::UI_MESSAGE_RESULTS DirectApply::ProcessMessage(RE::UIMessage& a_message) {
    switch (*a_message.type) {
    case RE::UI_MESSAGE_TYPE::kShow:
        if (!isInitialized) {
            logger::info("DirectApply: Initializing menuObj for the first time");
            
            if (uiMovie) {
                // Получаем _root (Main класс в AS3)
                bool success = uiMovie->GetVariable(&menuObj, "_root");
                if (success && menuObj.IsObject()) {
                    isInitialized = true;
                    
                    // Настройка макета
                    RE::Scaleform::GFx::Value marginArgs[3];
                    marginArgs[0] = 20.0; // right margin
                    marginArgs[1] = 100.0; // top margin
                    marginArgs[2] = 100.0; // bottom margin
                    
                    RE::Scaleform::GFx::Value result;
                    menuObj.Invoke("setMenuMargins", &result, marginArgs, 3);
                    menuObj.Invoke("updateMenuScale", &result, nullptr, 0);
                    
                    logger::info("DirectApply: Menu initialization complete");
                }
            }
        }
        
        // Настройка контроллера
        auto ControlMap = RE::ControlMap::GetSingleton();
        ControlMap->PushInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kBasicMenuNav);
        ControlMap->PushInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kThumbNav);
        ControlMap->PushInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kVirtualController);
        RE::SendHUDMessage::PushHUDMode("SpecialMode");
        RE::PlayerControls::GetSingleton()->blockPlayerInput = true;
        
        // Очистка и показ меню
        clear();
        showActors(); // Или любой другой контент
        
        return RE::UI_MESSAGE_RESULTS::kPassOn;
        
    // ... остальные случаи
    }
}
```

## Простые примеры использования

### Создание главного меню

```cpp
void DirectApply::showMainMenu() {
    if (!isInitialized) return;
    
    clear();
    setTitle("Главное меню");
    
    push(MENU_OBJECT::BUTTON, "Новая игра");
    push(MENU_OBJECT::BUTTON, "Загрузить игру");
    push(MENU_OBJECT::BUTTON, "Настройки");
    push(MENU_OBJECT::BUTTON, "Выход");
}
```

### Создание меню настроек

```cpp
void DirectApply::showSettingsMenu() {
    if (!isInitialized) return;
    
    clear();
    setTitle("Настройки");
    
    // Разделитель
    push(MENU_OBJECT::LABEL, "--- Графика ---");
    
    // Чекбокс полноэкранного режима
    push(MENU_OBJECT::CHECKBOX, "Полноэкранный режим", isFullscreen ? 1 : 0);
    
    // Свитчер качества графики
    push(MENU_OBJECT::SWITCHER, "Низкое,Среднее,Высокое,Ультра", graphicsQuality);
    
    // Разделитель
    push(MENU_OBJECT::LABEL, "--- Звук ---");
    
    // Чекбокс звука
    push(MENU_OBJECT::CHECKBOX, "Включить звук", soundEnabled ? 1 : 0);
    
    // Кнопка возврата
    push(MENU_OBJECT::BUTTON, "Назад");
}
```

### Создание меню инвентаря

```cpp
void DirectApply::showInventoryMenu() {
    if (!isInitialized) return;
    
    clear();
    setTitle("Инвентарь");
    
    auto player = RE::PlayerCharacter::GetSingleton();
    auto inventory = player->GetInventory();
    
    push(MENU_OBJECT::LABEL, "Выберите предмет:");
    
    for (const auto& [item, data] : inventory) {
        if (item && data.first > 0) {
            std::string itemName = item->GetDisplayFullName();
            if (itemName.empty()) itemName = "<Unknown Item>";
            
            // Добавляем количество
            std::string label = fmt::format("{} ({})", itemName, data.first);
            push(MENU_OBJECT::BUTTON, label);
        }
    }
    
    push(MENU_OBJECT::BUTTON, "Закрыть");
}
```

## Обработка событий

### Колбэки кнопок

```cpp
extern "C" void pushButtonComponent(const char* labelText) {
    std::string label(labelText);
    
    if (label == "Новая игра") {
        // Начать новую игру
        startNewGame();
        DirectApply::getSingleton()->hide();
        
    } else if (label == "Загрузить игру") {
        // Показать меню загрузки
        DirectApply::getSingleton()->showLoadMenu();
        
    } else if (label == "Настройки") {
        // Показать настройки
        DirectApply::getSingleton()->showSettingsMenu();
        
    } else if (label == "Назад") {
        // Вернуться к главному меню
        DirectApply::getSingleton()->showMainMenu();
        
    } else if (label == "Выход" || label == "Закрыть") {
        // Закрыть меню
        DirectApply::getSingleton()->hide();
    }
}
```

### Колбэки чекбоксов

```cpp
extern "C" void changeCheckboxComponent(const char* labelText, bool checked) {
    std::string label(labelText);
    
    if (label == "Полноэкранный режим") {
        setFullscreenMode(checked);
        
    } else if (label == "Включить звук") {
        setSoundEnabled(checked);
        
    } else if (label == "Показывать подсказки") {
        setShowTooltips(checked);
    }
    
    // Сохраняем настройки
    saveSettings();
}
```

### Колбэки свитчеров

```cpp
extern "C" void changeSwitcherComponent(const char* labelText, int selectedIndex) {
    std::string label(labelText);
    
    if (label == "Низкое,Среднее,Высокое,Ультра") {
        setGraphicsQuality(selectedIndex);
        
    } else if (label == "Русский,English,Français") {
        setLanguage(selectedIndex);
        
    } else if (label == "Легко,Нормально,Сложно") {
        setDifficulty(selectedIndex);
    }
    
    // Сохраняем настройки
    saveSettings();
}
```

## Массовое добавление элементов

### Создание меню из массива

```cpp
void DirectApply::showQuickMenu() {
    if (!isInitialized) return;
    
    clear();
    setTitle("Быстрое меню");
    
    // Создаем массив элементов
    RE::Scaleform::GFx::Value menuArray;
    uiMovie->CreateArray(&menuArray);
    
    // Массив данных для меню
    struct MenuData {
        int type;
        std::string label;
        int state;
    };
    
    std::vector<MenuData> menuItems = {
        {0, "--- Действия ---", 0},
        {1, "Быстрое сохранение", 0},
        {1, "Быстрая загрузка", 0},
        {0, "--- Настройки ---", 0},
        {2, "Автосохранение", autoSaveEnabled ? 1 : 0},
        {3, "Сложность,Легко,Нормально,Сложно", currentDifficulty},
        {0, "", 0},
        {1, "Закрыть", 0}
    };
    
    // Добавляем элементы в массив
    for (const auto& item : menuItems) {
        RE::Scaleform::GFx::Value menuItem;
        uiMovie->CreateArray(&menuItem);
        
        menuItem.PushBack(item.type);
        menuItem.PushBack(item.label.c_str());
        if (item.type == 2 || item.type == 3) {
            menuItem.PushBack(item.state);
        }
        
        menuArray.PushBack(menuItem);
    }
    
    // Добавляем весь массив одним вызовом
    RE::Scaleform::GFx::Value result;
    menuObj.Invoke("pushArray", &result, &menuArray, 1);
}
```

## Интеграция с хотками

### Регистрация хотки

```cpp
void DirectApply::registerHotkey() {
    // Настройка обработчика ввода
    DBMenuToggleHandler::setInputEventType(RE::INPUT_EVENT_TYPE::kButton);
    DBMenuToggleHandler::setButtonCode(RE::BS_BUTTON_CODE::kF1); // F1 ключ
    
    // Установка функции-обработчика
    DBMenuToggleHandler::install(OnToggleKey);
}

// Обработчик нажатия хотки
void DirectApply::OnToggleKey(RE::PlayerControls* controls, const RE::InputEvent* inputEvent) {
    auto uiManager = RE::UI::GetSingleton();
    
    if (uiManager->IsMenuOpen(MENU_NAME)) {
        // Если меню открыто - закрываем
        uiManager->GetMenu(MENU_NAME)->Close();
    } else {
        // Если меню закрыто - открываем
        uiManager->GetMenu(MENU_NAME)->Show();
    }
}
```

## Полезные утилиты

### Поиск и замена элементов

```cpp
void DirectApply::replaceMenuItem(const std::string& oldLabel, const std::string& newLabel) {
    if (!isInitialized) return;
    
    // Находим элемент
    RE::Scaleform::GFx::Value findLabel(oldLabel.c_str());
    RE::Scaleform::GFx::Value result;
    menuObj.Invoke("find", &result, &findLabel, 1);
    
    int index = result.GetInt();
    if (index >= 0) {
        // Удаляем старый элемент
        RE::Scaleform::GFx::Value deleteLabel(oldLabel.c_str());
        menuObj.Invoke("deleteComponent", &result, &deleteLabel, 1);
        
        // Вставляем новый на то же место
        RE::Scaleform::GFx::Value insertArgs[3];
        insertArgs[0] = index;
        insertArgs[1] = 1; // BUTTON
        insertArgs[2] = newLabel.c_str();
        menuObj.Invoke("insert", &result, insertArgs, 3);
    }
}
```

### Получение информации о меню

```cpp
void DirectApply::printMenuInfo() {
    if (!isInitialized) return;
    
    // Получаем информацию о масштабировании
    RE::Scaleform::GFx::Value scaleInfo;
    menuObj.Invoke("getMenuScaleInfo", &scaleInfo, nullptr, 0);
    
    // Получаем отступы
    RE::Scaleform::GFx::Value margins;
    menuObj.Invoke("getMenuMargins", &margins, nullptr, 0);
    
    logger::info("DirectApply: Menu info retrieved");
}
```

---

Эти примеры показывают основные паттерны использования системы меню в реальных проектах.
