/*
 * Пример использования ThemeManager из C++ для управления цветовыми схемами
 * 
 * Файл демонстрирует интеграцию нового класса ThemeManager в DirectApply.cpp
 * и показывает, как можно использовать все доступные методы для настройки цветов.
 */

// Пример интеграции в DirectApply.cpp:

/*
// В методе ProcessMessage при типе kShow можно применить цветовую тему:

case RE::UI_MESSAGE_TYPE::kShow:
{
    // ... существующий код инициализации ...

    // Применяем цветовую тему через новый ThemeManager
    if (menuObj.IsObject() && uiMovie && uiMovie->asMovieRoot) {
        
        // Пример 1: Применение полной темы Glass
        RE::Scaleform::GFx::Value glassTheme;
        uiMovie->CreateObject(&glassTheme);
        
        // ScrollPane цвета
        glassTheme.SetMember("scrollPaneBorder", 0x80404040);
        glassTheme.SetMember("scrollPaneBackground", 0x60101010);
        
        // Label цвета
        glassTheme.SetMember("labelNormal", 0x00000000);
        glassTheme.SetMember("labelText", 0xFFE0E0E0);
        glassTheme.SetMember("labelHover", 0x40606060);
        glassTheme.SetMember("labelSelected", 0x80808080);
        
        // Button цвета
        glassTheme.SetMember("buttonNormal", 0x804080A0);
        glassTheme.SetMember("buttonText", 0xFFFFFFFF);
        glassTheme.SetMember("buttonHover", 0x906090C0);
        glassTheme.SetMember("buttonSelected", 0xA080A0D0);
        
        // Checkbox цвета
        glassTheme.SetMember("checkboxNormal", 0x603060A0);
        glassTheme.SetMember("checkboxText", 0xFFE0E0E0);
        glassTheme.SetMember("checkboxHover", 0x804080B0);
        glassTheme.SetMember("checkboxSelected", 0xA060A0E0);
        
        // Switcher цвета
        glassTheme.SetMember("switcherNormal", 0x70507090);
        glassTheme.SetMember("switcherText", 0xFFFFFFFF);
        glassTheme.SetMember("switcherHover", 0x906080B0);
        glassTheme.SetMember("switcherSelected", 0xB080A0D0);
        
        // Применяем тему
        RE::Scaleform::GFx::Value args[1];
        args[0] = glassTheme;
        RE::Scaleform::GFx::Value result;
        uiMovie->asMovieRoot->Invoke("root.setColorsTheme", &result, args, 1);
        
        logger::info("DirectApply: Применена цветовая тема Glass через ThemeManager");
        
        // Пример 2: Применение цветов к отдельным типам компонентов
        // Установка цветов только для кнопок:
        setComponentColorsRGBA(MENU_OBJECT::BUTTON, 0xFF0080FF, 0xFF000000, 0xFF00AAFF, 0xFF0066CC);
        
        // Пример 3: Установка цветов ScrollPane
        setScrollableMenuColorsRGBA(0xFF666666, 0x80222222);
        
        // Пример 4: Сброс к цветам по умолчанию
        // resetColorsToDefault(); // Вернет все к теме Glass
    }
    
    return RE::UI_MESSAGE_RESULTS::kPassOn;
}
*/

/*
// Дополнительные методы ThemeManager, доступные через C++:

// 1. Применение полной цветовой темы
void applyCustomTheme() {
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        return;
    }
    
    // Создаем объект темы
    RE::Scaleform::GFx::Value customTheme;
    uiMovie->CreateObject(&customTheme);
    
    // Настраиваем кастомные цвета
    customTheme.SetMember("scrollPaneBorder", 0xFF333333);
    customTheme.SetMember("scrollPaneBackground", 0x80111111);
    customTheme.SetMember("buttonNormal", 0xFF4080C0);
    customTheme.SetMember("buttonText", 0xFFFFFFFF);
    // ... другие цвета
    
    RE::Scaleform::GFx::Value args[1];
    args[0] = customTheme;
    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.setColorsTheme", &result, args, 1);
}

// 2. Установка цветов отдельного типа компонентов
void setButtonColors(uint32_t normal, uint32_t text, uint32_t hover, uint32_t selected) {
    setComponentColorsRGBA(MENU_OBJECT::BUTTON, normal, text, hover, selected);
}

// 3. Получение текущих цветов компонента
void logCurrentButtonColors() {
    auto colors = getComponentColors(MENU_OBJECT::BUTTON);
    if (colors.IsObject()) {
        RE::Scaleform::GFx::Value normalColor, textColor, hoverColor, selectedColor;
        colors.GetMember("normal", &normalColor);
        colors.GetMember("text", &textColor);
        colors.GetMember("hover", &hoverColor);
        colors.GetMember("selected", &selectedColor);
        
        logger::info("Button colors - Normal: 0x{:08X}, Text: 0x{:08X}, Hover: 0x{:08X}, Selected: 0x{:08X}",
            static_cast<uint32_t>(normalColor.GetNumber()),
            static_cast<uint32_t>(textColor.GetNumber()),
            static_cast<uint32_t>(hoverColor.GetNumber()),
            static_cast<uint32_t>(selectedColor.GetNumber()));
    }
}

// 4. Применение цветов из INI файла с переопределениями
void applyThemeFromINI() {
    // Загружаем базовую тему
    setColorsTheme(Glass); // или Neon, Vintage
    
    // Применяем переопределения из INI
    ini::map colorsIni = *globals::g_ini;
    if (colorsIni.update()) {
        // Переопределение цветов кнопок
        if (auto buttonColors = colorsIni.at("COLORS/sButton", ""s); !buttonColors.empty()) {
            auto parsed = parseComponentColors(buttonColors);
            applyComponentColorOverride(MENU_OBJECT::BUTTON, parsed, Glass);
        }
        
        // Переопределение цветов ScrollPane
        if (auto scrollPaneColors = colorsIni.at("COLORS/sScrollPane", ""s); !scrollPaneColors.empty()) {
            auto parsed = parseScrollPaneColors(scrollPaneColors);
            if (!parsed.isEmpty()) {
                uint32_t borderColor = parsed.border.empty() ? Glass.scrollPaneBorder : parseHexColor(parsed.border);
                uint32_t bgColor = parsed.background.empty() ? Glass.scrollPaneBackground : parseHexColor(parsed.background);
                setScrollableMenuColorsRGBA(borderColor, bgColor);
            }
        }
    }
}
*/

/*
// Пример INI конфигурации для цветов:

[COLORS]
; Общая тема (Glass, Neon, Vintage)
sTheme=Glass

; Переопределения цветов компонентов (формат: normal,text,hover,selected)
; Можно указать только нужные цвета, остальные останутся из темы
sButton=0xFF4080C0,0xFFFFFFFF,0xFF60A0E0,0xFF8060C0
sCheckbox=0xFF606060,,0xFF808080,  ; Только normal и hover, text и selected из темы
sLabel=,0xFF00FF00,,              ; Только зеленый цвет текста
sSwitcher=0xFF805020,0xFFFFE0C0,0xFFA06040,0xFFC08060

; Переопределение ScrollPane (формат: border,background)
sScrollPane=0xFF404040,0x60101010

; Отладка
[DEBUG]
bScaleformLog=true
*/

/*
// Возможные улучшения ThemeManager:

1. Поддержка анимированных переходов между цветовыми схемами
2. Сохранение пользовательских цветовых настроек
3. Поддержка градиентов для фонов компонентов
4. Интеграция с модами для автоматической загрузки цветовых схем
5. Поддержка сезонных/праздничных тем
6. Возможность создания цветовых схем через внутриигровой интерфейс

// Методы для будущего расширения:
- saveCurrentThemeToFile(filename)
- loadThemeFromFile(filename)
- createAnimatedTransition(fromTheme, toTheme, duration)
- setSeasonalTheme(season)
- enableDynamicColors(timeOfDay)
*/
