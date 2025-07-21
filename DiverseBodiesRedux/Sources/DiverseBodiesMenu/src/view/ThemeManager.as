package view {
    import flash.events.EventDispatcher;
    import flash.events.Event;
    import fl.containers.ScrollPane;
    import view.ThemeEvent;
    import view.ScrollableMenu;
    
    /**
     * @brief Централизованный менеджер тем с паттерном Observer
     * 
     * Управляет цветами всех компонентов меню и уведомляет подписчиков об изменениях.
     * Инициализируется темой Glass по умолчанию, может быть переопределён через C++ методы.
     * 
     * События:
     * - ThemeManager.THEME_CHANGED: общее изменение темы
     * - ThemeManager.COMPONENT_COLORS_CHANGED: изменение цветов компонента
     * - ThemeManager.SCROLLABLE_MENU_COLORS_CHANGED: изменение цветов ScrollableMenu
     */
    public class ThemeManager extends EventDispatcher {
        // События
        public static const THEME_CHANGED:String = "themeChanged";
        public static const COMPONENT_COLORS_CHANGED:String = "componentColorsChanged";
        public static const SCROLLABLE_MENU_COLORS_CHANGED:String = "scrollableMenuColorsChanged";
        
        // Типы компонентов
        public static const LABEL:int = 0;
        public static const BUTTON:int = 1;
        public static const CHECKBOX:int = 2;
        public static const SWITCHER:int = 3;
        
        // Singleton instance
        private static var _instance:ThemeManager;
        
        // Ссылки на компоненты
        private var scrollPane:ScrollPane;
        private var scrollableMenu:ScrollableMenu;
        
        // Цвета компонентов [componentType][colorType]
        private var componentColors:Array = [];
        
        // Цвета ScrollableMenu
        private var scrollableMenuColors:Object = {
            borderColor: 0xFF000000,
            backgroundColor: 0xFF000000
        };
        
        // Флаг инициализации
        private var isInitialized:Boolean = false;
        
        /**
         * @brief Получает singleton instance ThemeManager
         */
        public static function getInstance():ThemeManager {
            if (!_instance) {
                _instance = new ThemeManager();
                _instance.initializeWithGlassTheme();
            }
            return _instance;
        }
        
        public function ThemeManager() {
            super();
            if (_instance) {
                throw new Error("ThemeManager является синглтоном. Используйте getInstance()");
            }
        }
        
        /**
         * @brief Инициализирует ThemeManager темой Glass
         */
        public function initializeWithGlassTheme():void {
            log("ThemeManager: Инициализация темой Glass");
            
            // Инициализируем массив цветов компонентов
            componentColors = [];
            for (var i:int = 0; i <= 3; i++) {
                componentColors[i] = {};
            }
            
            // Тема Glass - прозрачные элементы с голубоватым оттенком
            
            // Label (0) - светло-серый текст
            componentColors[LABEL] = {
                normal: 0x80E0E0E0,    // Полупрозрачный светло-серый
                text: 0xFFFFFFFF,      // Белый текст
                hover: 0x80E0E0E0,     // Без изменений при hover
                selected: 0x80E0E0E0   // Без изменений при выборе
            };
            
            // Button (1) - стеклянный с голубым оттенком
            componentColors[BUTTON] = {
                normal: 0x10FFFFFF,     // Полупрозрачный синевато-серый
                text: 0xCC000000,       // Белый текст
                hover: 0x95FFFFFF,      // Светлее при hover
                selected: 0xFFFFFFFF    // Ярче при выборе
            };
            
            // Checkbox (2) - стеклянный с зелёным оттенком
            componentColors[CHECKBOX] = {
                normal: 0x10FFFFFF,     // Полупрозрачный тёмно-серый
                text: 0xCC000000,       // Белый текст
                hover: 0x95FFFFFF,      // Зеленоватый при hover
                selected: 0xFFFFFFFF    // Зелёный при выборе
            };
            
            // Switcher (3) - стеклянный с оранжевым оттенком
            componentColors[SWITCHER] = {
                normal: 0x10FFFFFF,     // Полупрозрачный тёмно-серый
                text: 0xCC000000,       // Белый текст
                hover: 0x95FFFFFF,      // Оранжевый при hover
                selected: 0xFFFFFFFF    // Ярко-оранжевый при выборе
            };
            
            // ScrollableMenu - прозрачная граница, полупрозрачный фон
            scrollableMenuColors = {
                borderColor: 0x80808080,    // Полупрозрачная серая граница
                backgroundColor: 0x40000000 // Очень прозрачный чёрный фон
            };
            
            isInitialized = true;
            
            // Уведомляем всех подписчиков об изменении темы
            dispatchEvent(new ThemeEvent(THEME_CHANGED, -1));
            
            log("ThemeManager: Тема Glass инициализирована");
        }
        
        /**
         * @brief Устанавливает тему по имени (вызывается из C++)
         */
        public function setThemeByName(themeName:String):void {
            log("ThemeManager: Установка темы: " + themeName);
            
            switch (themeName.toLowerCase()) {
                case "glass":
                    initializeWithGlassTheme();
                    break;
                    
                case "dark":
                    initializeWithDarkTheme();
                    break;
                    
                case "light":
                    initializeWithLightTheme();
                    break;
                    
                default:
                    log("ThemeManager: Неизвестная тема '" + themeName + "', используется Glass");
                    initializeWithGlassTheme();
                    break;
            }
        }
        
        /**
         * @brief Применяет полную цветовую тему (18 аргументов из C++)
         * @param scrollPaneBorder Цвет границы ScrollPane (ARGB)
         * @param scrollPaneBackground Цвет фона ScrollPane (ARGB)
         * @param labelNormal Обычный цвет Label (ARGB)
         * @param labelText Цвет текста Label (ARGB)
         * @param labelHover Цвет Label при hover (ARGB)
         * @param labelSelected Цвет выделенного Label (ARGB)
         * @param buttonNormal Обычный цвет Button (ARGB)
         * @param buttonText Цвет текста Button (ARGB)
         * @param buttonHover Цвет Button при hover (ARGB)
         * @param buttonSelected Цвет выделенного Button (ARGB)
         * @param checkboxNormal Обычный цвет Checkbox (ARGB)
         * @param checkboxText Цвет текста Checkbox (ARGB)
         * @param checkboxHover Цвет Checkbox при hover (ARGB)
         * @param checkboxSelected Цвет выделенного Checkbox (ARGB)
         * @param switcherNormal Обычный цвет Switcher (ARGB)
         * @param switcherText Цвет текста Switcher (ARGB)
         * @param switcherHover Цвет Switcher при hover (ARGB)
         * @param switcherSelected Цвет выделенного Switcher (ARGB)
         */
        public function setColorsTheme(scrollPaneBorder:uint, scrollPaneBackground:uint,
                                      labelNormal:uint, labelText:uint, labelHover:uint, labelSelected:uint,
                                      buttonNormal:uint, buttonText:uint, buttonHover:uint, buttonSelected:uint,
                                      checkboxNormal:uint, checkboxText:uint, checkboxHover:uint, checkboxSelected:uint,
                                      switcherNormal:uint, switcherText:uint, switcherHover:uint, switcherSelected:uint):void {
            log("ThemeManager: Применение пользовательской цветовой темы с 18 аргументами из C++");
            
            // Инициализируем тему если нужно
            if (!isInitialized) {
                initializeWithGlassTheme();
            }
            
            // Устанавливаем цвета ScrollPane
            log("ThemeManager: Установка цветов ScrollPane...");
            setScrollableMenuColorsRGBA(scrollPaneBorder, scrollPaneBackground);
            
            // Устанавливаем цвета для каждого типа компонентов
            log("ThemeManager: Установка цветов компонентов...");
            setComponentColorsRGBA(0, labelNormal, labelText, labelHover, labelSelected);       // Label
            setComponentColorsRGBA(1, buttonNormal, buttonText, buttonHover, buttonSelected);   // Button  
            setComponentColorsRGBA(2, checkboxNormal, checkboxText, checkboxHover, checkboxSelected); // Checkbox
            setComponentColorsRGBA(3, switcherNormal, switcherText, switcherHover, switcherSelected); // Switcher
            
            log("ThemeManager: Пользовательская цветовая тема успешно применена ко всем компонентам");
        }
        
        /**
         * @brief Устанавливает цвета для определённого типа компонента
         */
        public function setComponentColorsRGBA(componentType:int, normalColorRGBA:uint, textColorRGBA:uint, hoverColorRGBA:uint, selectedColorRGBA:uint):void {
            if (!isInitialized) {
                initializeWithGlassTheme();
            }
            
            if (componentType < 0 || componentType > 3) {
                log("ThemeManager: Неверный тип компонента: " + componentType);
                return;
            }
            
            // Определяем имя типа компонента для лога
            var componentTypeName:String;
            switch (componentType) {
                case LABEL: componentTypeName = "Label"; break;
                case BUTTON: componentTypeName = "Button"; break;
                case CHECKBOX: componentTypeName = "Checkbox"; break;
                case SWITCHER: componentTypeName = "Switcher"; break;
                default: componentTypeName = "Unknown"; break;
            }
            
            componentColors[componentType] = {
                normal: normalColorRGBA,
                text: textColorRGBA,
                hover: hoverColorRGBA,
                selected: selectedColorRGBA
            };
            
            log("ThemeManager: Цвета компонента " + componentTypeName + " (" + componentType + ") обновлены - " +
                "normal: 0x" + normalColorRGBA.toString(16).toUpperCase() + ", " +
                "text: 0x" + textColorRGBA.toString(16).toUpperCase() + ", " +
                "hover: 0x" + hoverColorRGBA.toString(16).toUpperCase() + ", " +
                "selected: 0x" + selectedColorRGBA.toString(16).toUpperCase());
            
            // Уведомляем подписчиков об изменении цветов компонента
            var event:ThemeEvent = new ThemeEvent(COMPONENT_COLORS_CHANGED);
            event.componentType = componentType;
            dispatchEvent(event);
        }
        
        /**
         * @brief Устанавливает цвета ScrollableMenu
         */
        public function setScrollableMenuColorsRGBA(borderColorRGBA:uint, backgroundColorRGBA:uint):void {
            if (!isInitialized) {
                initializeWithGlassTheme();
            }
            
            scrollableMenuColors.borderColor = borderColorRGBA;
            scrollableMenuColors.backgroundColor = backgroundColorRGBA;
            
            log("ThemeManager: Цвета ScrollableMenu обновлены - " +
                "borderColor: 0x" + borderColorRGBA.toString(16).toUpperCase() + ", " +
                "backgroundColor: 0x" + backgroundColorRGBA.toString(16).toUpperCase());
            
            // Уведомляем подписчиков об изменении цветов ScrollableMenu
            dispatchEvent(new ThemeEvent(SCROLLABLE_MENU_COLORS_CHANGED, -1));
        }
        
        /**
         * @brief Сбрасывает цвета к теме Glass по умолчанию
         */
        public function resetColorsToDefault():void {
            log("ThemeManager: Сброс цветов к умолчаниям");
            initializeWithGlassTheme();
        }
        
        /**
         * @brief Получает цвета для определённого типа компонента
         */
        public function getComponentColors(componentType:int):Object {
            if (!isInitialized) {
                initializeWithGlassTheme();
            }
            
            if (componentType < 0 || componentType > 3) {
                log("ThemeManager: Неверный тип компонента: " + componentType + ", возвращаются цвета Label");
                return componentColors[LABEL];
            }
            
            return componentColors[componentType];
        }
        
        /**
         * @brief Получает цвета ScrollableMenu
         */
        public function getScrollableMenuColors():Object {
            if (!isInitialized) {
                initializeWithGlassTheme();
            }
            
            return {
                borderColor: scrollableMenuColors.borderColor,
                backgroundColor: scrollableMenuColors.backgroundColor
            };
        }
        
        /**
         * @brief Инициализирует тёмную тему
         */
        private function initializeWithDarkTheme():void {
            log("ThemeManager: Инициализация тёмной темы");
            
            componentColors[LABEL] = {
                normal: 0xFF404040,
                text: 0xFFFFFFFF,
                hover: 0xFF404040,
                selected: 0xFF404040
            };
            
            componentColors[BUTTON] = {
                normal: 0xFF202020,
                text: 0xFFFFFFFF,
                hover: 0xFF404040,
                selected: 0xFF606060
            };
            
            componentColors[CHECKBOX] = {
                normal: 0xFF202020,
                text: 0xFFFFFFFF,
                hover: 0xFF00AA00,
                selected: 0xFF00FF00
            };
            
            componentColors[SWITCHER] = {
                normal: 0xFF202020,
                text: 0xFFFFFFFF,
                hover: 0xFFAA6600,
                selected: 0xFFFF8800
            };
            
            scrollableMenuColors = {
                borderColor: 0xFF808080,
                backgroundColor: 0xFF101010
            };
            
            isInitialized = true;
            dispatchEvent(new Event(THEME_CHANGED));
        }
        
        /**
         * @brief Инициализирует светлую тему
         */
        private function initializeWithLightTheme():void {
            log("ThemeManager: Инициализация светлой темы");
            
            componentColors[LABEL] = {
                normal: 0xFFF0F0F0,
                text: 0xFF000000,
                hover: 0xFFF0F0F0,
                selected: 0xFFF0F0F0
            };
            
            componentColors[BUTTON] = {
                normal: 0xFFE0E0E0,
                text: 0xFF000000,
                hover: 0xFFD0D0D0,
                selected: 0xFFC0C0C0
            };
            
            componentColors[CHECKBOX] = {
                normal: 0xFFE0E0E0,
                text: 0xFF000000,
                hover: 0xFF90FF90,
                selected: 0xFF60FF60
            };
            
            componentColors[SWITCHER] = {
                normal: 0xFFE0E0E0,
                text: 0xFF000000,
                hover: 0xFFFFCC60,
                selected: 0xFFFFAA30
            };
            
            scrollableMenuColors = {
                borderColor: 0xFF404040,
                backgroundColor: 0xFFF8F8F8
            };
            
            isInitialized = true;
            dispatchEvent(new Event(THEME_CHANGED));
        }
        
        /**
         * @brief Проверяет, инициализирован ли ThemeManager
         */
        public function get initialized():Boolean {
            return isInitialized;
        }
        
        /**
         * @brief Логирует сообщение
         */
        private function log(message:String):void {
            trace(message);
            // Не используем mainInstance, чтобы избежать циклических зависимостей
        }
        
        /**
         * @brief Инициализирует ThemeManager с необходимыми компонентами
         * @param scrollPane Ссылка на ScrollPane
         * @param scrollableMenu Ссылка на ScrollableMenu
         */
        public function initialize(scrollPane:ScrollPane, scrollableMenu:ScrollableMenu):void {
            log("ThemeManager: Инициализация с компонентами");
            this.scrollPane = scrollPane;
            this.scrollableMenu = scrollableMenu;
            
            // Инициализируем тему Glass по умолчанию
            initializeWithGlassTheme();
            
            log("ThemeManager: Инициализация завершена");
        }
        
        /**
         * @brief Применяет цвета с задержкой после полной инициализации ScrollPane
         */
        public function applyDelayedColors():void {
            log("ThemeManager: Применение отложенных цветов");
            // Применяем текущие цвета повторно
            dispatchEvent(new ThemeEvent(THEME_CHANGED, BUTTON));
            dispatchEvent(new ThemeEvent(COMPONENT_COLORS_CHANGED, BUTTON));
            dispatchEvent(new ThemeEvent(SCROLLABLE_MENU_COLORS_CHANGED, BUTTON));
        }
        
        /**
         * @brief Уничтожает ThemeManager и очищает ресурсы
         */
        public function destroy():void {
            log("ThemeManager: Начало уничтожения...");
            
            // Очищаем ссылки
            scrollPane = null;
            scrollableMenu = null;
            componentColors = null;
            
            log("ThemeManager: Уничтожение завершено");
        }
    }
}
