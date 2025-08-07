package view {
    import flash.external.ExternalInterface;
    import flash.events.EventDispatcher;
    import flash.events.Event;
    import fl.containers.ScrollPane;
    import view.ScrollableMenu;
    import view.MenuScaler;
    import view.ThemeManager;
    import view.CustomEvent;

    /**
     * @brief Центральный менеджер меню - единый управляющий класс для всей системы меню
     * 
     * Основные обязанности:
     * - Управление жизненным циклом всех компонентов меню (ScrollableMenu, MenuScaler, ThemeManager)
     * - Централизованная обработка всех событий от компонентов
     * - Единый интерфейс для взаимодействия с C++ через ExternalInterface
     * - Координация масштабирования и цветовых схем
     * - Управление отображением и скрытием меню
     */
    public class MenuManager extends EventDispatcher {
        // ===== КОМПОНЕНТЫ СИСТЕМЫ =====
        private var menuContainer:ScrollPane;      // ScrollPane контейнер из FLA
        private var _scrollableMenu:ScrollableMenu; // Прокручиваемое меню с элементами
        private var menuScaler:MenuScaler;        // Менеджер масштабирования
        private var themeManager:ThemeManager;    // Менеджер цветовых схем
        
        // ===== СОСТОЯНИЕ МЕНЮ =====
        private var isVisible:Boolean = false;
        private var isInitialized:Boolean = false;

        
        /**
         * @brief Конструктор - инициализирует MenuManager с заданным контейнером
         * @param container ScrollPane контейнер меню (scrollPane_mc из FLA)
         */
        public function MenuManager(container:ScrollPane) {
            super();
            this.menuContainer = container;
            log("MenuManager: Инициализация с контейнером " + container.name);
            
            // Инициализируем все компоненты системы
            initializeComponents();
            
            // По умолчанию скрываем меню
            hideMenu();
            
            log("MenuManager: Инициализация завершена");
        }

        /**
         * @brief Инициализирует все компоненты системы меню
         */
        private function initializeComponents():void {
            if (isInitialized) {
                log("MenuManager: Компоненты уже инициализированы");
                return;
            }
            
            log("MenuManager: Начало инициализации компонентов...");
            
            // 1. Инициализируем MenuScaler (синглтон)
            menuScaler = MenuScaler.getInstance();
            menuScaler.initialize(menuContainer);
            menuScaler.addEventListener(MenuScaler.SCALE_CHANGED, onScaleChanged);
            log("MenuManager: MenuScaler инициализирован");
            
            // 2. Инициализируем ThemeManager (синглтон)
            themeManager = ThemeManager.getInstance();
            themeManager.addEventListener(ThemeManager.THEME_CHANGED, onThemeChanged);
            themeManager.addEventListener(ThemeManager.COMPONENT_COLORS_CHANGED, onComponentColorsChanged);
            themeManager.addEventListener(ThemeManager.SCROLLABLE_MENU_COLORS_CHANGED, onScrollableMenuColorsChanged);
            log("MenuManager: ThemeManager инициализирован");
            
            // 3. Инициализируем ScrollableMenu
            _scrollableMenu = new ScrollableMenu(menuContainer);
            _scrollableMenu.addEventListener("menuItemSelected", onMenuItemSelected);
            _scrollableMenu.addEventListener("menuItemHover", onMenuItemHover);
            _scrollableMenu.addEventListener("menuCheckboxChanged", onMenuCheckboxChanged);
            _scrollableMenu.addEventListener("menuSwitcherChanged", onMenuSwitcherChanged);
            _scrollableMenu.addEventListener("menuBack", onMenuBack);
            log("MenuManager: ScrollableMenu инициализирован");
            
            // 4. Связываем компоненты
            themeManager.initialize(menuContainer, _scrollableMenu);
            
            isInitialized = true;
            log("MenuManager: Все компоненты инициализированы");
        }

        
        /**
         * @brief Логирует сообщение через trace и отправляет в C++ через Main
         * @param message Сообщение для логирования
         */
        private function log(message:String):void {
            trace(message);
            // Отправляем событие вверх, Main перехватит и отправит в C++
            dispatchEvent(new CustomEvent("log", {message: message}));
        }

        // ===== ОБРАБОТЧИКИ СОБЫТИЙ ОТ КОМПОНЕНТОВ =====

        /**
         * @brief Обработчик изменения масштаба от MenuScaler
         */
        private function onScaleChanged(event:Event):void {
            log("MenuManager: Получено событие изменения масштаба");
            
            if (_scrollableMenu) {
                _scrollableMenu.updateScale();
            }
            
            // Отправляем событие наружу
            dispatchEvent(new CustomEvent("scaleChanged", {scale: menuScaler.scale}));
        }

        /**
         * @brief Обработчик изменения темы от ThemeManager
         */
        private function onThemeChanged(event:Event):void {
            log("MenuManager: Получено событие изменения темы");
            
            if (_scrollableMenu) {
                _scrollableMenu.updateTheme();
            }
            
            // Отправляем событие наружу
            dispatchEvent(new CustomEvent("themeChanged", {}));
        }

        /**
         * @brief Обработчик изменения цветов компонентов от ThemeManager
         */
        private function onComponentColorsChanged(event:Event):void {
            log("MenuManager: Получено событие изменения цветов компонентов");
            
            if (_scrollableMenu) {
                _scrollableMenu.updateComponentColors();
            }
        }

        /**
         * @brief Обработчик изменения цветов ScrollableMenu от ThemeManager
         */
        private function onScrollableMenuColorsChanged(event:Event):void {
            log("MenuManager: Получено событие изменения цветов ScrollableMenu");
            
            if (_scrollableMenu) {
                _scrollableMenu.updateScrollPaneColors();
            }
        }

        /**
         * @brief Обработчик выбора элемента меню от ScrollableMenu
         */
        private function onMenuItemSelected(event:*):void {
            var itemIndex:int = event.data.index;
            var itemData:Object = event.data.item; // Исправлено: было event.data.data
            
            log("MenuManager: Выбран элемент " + itemIndex + ": '" + itemData.labelText + "'");
            
            // Логируем дополнительную информацию в зависимости от типа
            switch (itemData.type) {
                case 2: // Checkbox
                    log("  └── Состояние checkbox: " + itemData.checked);
                    break;
                case 3: // Switcher  
                    log("  └── Выбрано: " + itemData.selectedValue + " (индекс: " + itemData.selectedIndex + ")");
                    break;
            }
            
            // Отправляем событие наружу для Main
            dispatchEvent(new CustomEvent("menuItemSelected", {
                index: itemIndex,
                item: itemData
            }));
        }

        /**
         * @brief Обработчик события "назад" от ScrollableMenu
         */
        private function onMenuBack(event:*):void {
            log("MenuManager: Получено событие 'назад'");
            
            // Отправляем событие наружу для Main
            dispatchEvent(new CustomEvent("menuBack", {}));
        }

        /**
         * @brief Обработчик события наведения/выбора элемента от ScrollableMenu
         */
        private function onMenuItemHover(event:*):void {
            var labelText:String = event.data.labelText;
            log("MenuManager: Выбран элемент: '" + labelText + "'");
            
            // Отправляем событие наружу для Main
            dispatchEvent(new CustomEvent("menuItemHover", {
                labelText: labelText
            }));
        }

        /**
         * @brief Обработчик изменения checkbox от ScrollableMenu
         */
        private function onMenuCheckboxChanged(event:*):void {
            var labelText:String = event.data.labelText;
            var checked:Boolean = event.data.checked;
            log("MenuManager: Checkbox изменен: '" + labelText + "' = " + checked);
            
            // Отправляем событие наружу для Main
            dispatchEvent(new CustomEvent("menuCheckboxChanged", {
                labelText: labelText,
                checked: checked
            }));
        }

        /**
         * @brief Обработчик изменения switcher от ScrollableMenu
         */
        private function onMenuSwitcherChanged(event:*):void {
            var labelText:String = event.data.labelText;
            var selectedIndex:int = event.data.selectedIndex;
            log("MenuManager: Switcher изменен: '" + labelText + "' = " + selectedIndex);
            
            // Отправляем событие наружу для Main
            dispatchEvent(new CustomEvent("menuSwitcherChanged", {
                labelText: labelText,
                selectedIndex: selectedIndex
            }));
        }

        
        // ===== ПУБЛИЧНОЕ API ДЛЯ УПРАВЛЕНИЯ МЕНЮ =====

        /**
         * @brief Показывает меню с заданными параметрами
         * @param title Заголовок меню (не используется, оставлен для совместимости)
         * @param items Массив элементов меню
         * @param activeIndex Индекс активного элемента (по умолчанию 0)
         */
        public function showMenu(title:String, items:Array, activeIndex:int = 0):void {
            log("MenuManager: showMenu() НАЧАЛО - элементов: " + (items ? items.length : 0));
            
            if (!isInitialized) {
                log("MenuManager: ERROR - Компоненты не инициализированы");
                return;
            }
            
            log("MenuManager: Вызываем updateItems()");
            // Обновляем элементы меню
            updateItems(items, activeIndex);
            
            log("MenuManager: updateItems() завершен, показываем контейнер");
            
            // ПРИНУДИТЕЛЬНО ПОКАЗЫВАЕМ SCROLLPANE
            if (menuContainer) {
                menuContainer.visible = true;
                menuContainer.alpha = 1.0;
            }
            
            // Показываем контейнер
            menuContainer.visible = true;
            isVisible = true;
            
            // Устанавливаем фокус для корректной работы клавиатуры
            if (menuContainer.stage) {
                menuContainer.stage.focus = menuContainer.stage;
            }
            
            log("MenuManager: showMenu() ЗАВЕРШЕН - меню показано успешно");
        }

        /**
         * @brief Обновляет элементы меню
         * @param items Массив элементов меню
         * @param activeIndex Индекс активного элемента (по умолчанию 0)
         */
        public function updateItems(items:Array, activeIndex:int = 0):void {
            log("MenuManager: updateItems() НАЧАЛО - элементов: " + (items ? items.length : 0) + ", activeIndex: " + activeIndex);
            
            if (!_scrollableMenu) {
                log("MenuManager: ERROR - ScrollableMenu не инициализирован");
                return;
            }
            
            if (!items) {
                log("MenuManager: WARNING - Передан null массив элементов");
                items = [];
            }
            
            log("MenuManager: Вызываем _scrollableMenu.setItems()");
            _scrollableMenu.setItems(items, activeIndex);
            log("MenuManager: _scrollableMenu.setItems() завершен");
            log("MenuManager: Элементы меню обновлены (" + items.length + " элементов), активный индекс: " + activeIndex);
        }

        /**
         * @brief Скрывает меню
         */
        public function hideMenu():void {
            menuContainer.visible = false;
            isVisible = false;
            log("MenuManager: Меню скрыто");
        }

        /**
         * @brief Проверяет, видимо ли меню
         */
        public function get visible():Boolean {
            return isVisible;
        }

        /**
         * @brief Получает текущий активный индекс
         */
        public function getCurrentActiveIndex():int {
            return _scrollableMenu ? _scrollableMenu.activeIndex : -1;
        }

        /**
         * @brief Получает количество элементов в меню
         */
        public function getItemCount():int {
            return _scrollableMenu ? _scrollableMenu.getCurrentItems().length : 0;
        }

        /**
         * @brief Получает текущие элементы меню
         */
        public function getCurrentItems():Array {
            return _scrollableMenu ? _scrollableMenu.getCurrentItems() : [];
        }

        /**
         * @brief Безопасно устанавливает активный индекс элемента меню
         * @param newIndex Индекс элемента для установки как активный (0-based)
         */
        public function setActiveIndex(newIndex:int):void {
            log("MenuManager: Установка активного индекса: " + newIndex);
            
            if (!_scrollableMenu) {
                log("MenuManager: ERROR - ScrollableMenu не инициализирован");
                return;
            }
            
            _scrollableMenu.setActiveIndex(newIndex);
        }

        /**
         * @brief Возвращает ссылка на ScrollableMenu
         */
        public function get scrollableMenu():ScrollableMenu {
            return _scrollableMenu;
        }

        
        // ===== API ДЛЯ УПРАВЛЕНИЯ МАСШТАБИРОВАНИЕМ =====

        /**
         * @brief Принудительно обновляет масштабирование меню
         */
        public function updateScale():void {
            if (menuScaler) {
                menuScaler.updateScale();
            }
        }

        /**
         * @brief Принудительно обновляет масштабирование с гарантированным результатом
         */
        public function forceUpdateScale():void {
            if (menuScaler) {
                menuScaler.forceUpdateScale();
            }
        }

        /**
         * @brief Переключает режим масштабирования (пропорциональное/растягивающее)
         */
        public function setStretchMode(useStretch:Boolean):void {
            if (menuScaler) {
                if (useStretch) {
                    menuScaler.updateScaleStretch();
                } else {
                    menuScaler.updateScale();
                }
            }
        }

        /**
         * @brief Получает текущий коэффициент масштабирования
         */
        public function getCurrentScale():Number {
            return menuScaler ? menuScaler.getCurrentScale() : 1.0;
        }

        /**
         * @brief Получает информацию о базовом разрешении
         */
        public function getBaseResolution():Object {
            return menuScaler ? menuScaler.getBaseResolution() : {width: 1920, height: 1080};
        }

        /**
         * @brief Устанавливает отступы меню от краев экрана
         */
        public function setMenuMargins(horizontal:Number, vertical:Number):void {
            if (menuScaler) {
                menuScaler.setMargins(horizontal, vertical);
            }
        }

        /**
         * @brief Получает текущие отступы меню
         */
        public function getMenuMargins():Object {
            return menuScaler ? menuScaler.getMargins() : {horizontal: 5, vertical: 10};
        }

        
        // ===== API ДЛЯ УПРАВЛЕНИЯ ЦВЕТОВЫМИ СХЕМАМИ =====

        /**
         * @brief Применяет полную цветовую тему (18 аргументов из C++)
         */
        public function setColorsTheme(scrollPaneBorder:uint, scrollPaneBackground:uint,
                                      labelNormal:uint, labelText:uint, labelHover:uint, labelSelected:uint,
                                      buttonNormal:uint, buttonText:uint, buttonHover:uint, buttonSelected:uint,
                                      checkboxNormal:uint, checkboxText:uint, checkboxHover:uint, checkboxSelected:uint,
                                      switcherNormal:uint, switcherText:uint, switcherHover:uint, switcherSelected:uint):void {
            log("MenuManager: Применение цветовой темы с 18 аргументами");
            
            if (!themeManager) {
                log("MenuManager: ERROR - ThemeManager не инициализирован");
                return;
            }
            
            // Устанавливаем цвета ScrollPane
            themeManager.setScrollableMenuColorsRGBA(scrollPaneBorder, scrollPaneBackground);
            
            // Устанавливаем цвета компонентов
            themeManager.setComponentColorsRGBA(0, labelNormal, labelText, labelHover, labelSelected);       // Label
            themeManager.setComponentColorsRGBA(1, buttonNormal, buttonText, buttonHover, buttonSelected);   // Button  
            themeManager.setComponentColorsRGBA(2, checkboxNormal, checkboxText, checkboxHover, checkboxSelected); // Checkbox
            themeManager.setComponentColorsRGBA(3, switcherNormal, switcherText, switcherHover, switcherSelected); // Switcher
            
            log("MenuManager: Цветовая тема применена");
        }

        /**
         * @brief Устанавливает цвета для определенного типа компонентов (без альфа-канала)
         */
        public function setComponentColors(componentType:int, normalColor:uint, textColor:uint, hoverColor:uint, selectedColor:uint):void {
            var normalColorRGBA:uint = 0xFF000000 | normalColor;
            var textColorRGBA:uint = 0xFF000000 | textColor;
            var hoverColorRGBA:uint = 0xFF000000 | hoverColor;
            var selectedColorRGBA:uint = 0xFF000000 | selectedColor;
            
            setComponentColorsRGBA(componentType, normalColorRGBA, textColorRGBA, hoverColorRGBA, selectedColorRGBA);
        }

        /**
         * @brief Устанавливает цвета для определенного типа компонентов с поддержкой альфа-канала
         */
        public function setComponentColorsRGBA(componentType:int, normalColorRGBA:uint, 
                                             textColorRGBA:uint, hoverColorRGBA:uint, 
                                             selectedColorRGBA:uint):void {
            if (themeManager) {
                themeManager.setComponentColorsRGBA(componentType, normalColorRGBA, textColorRGBA, 
                                                   hoverColorRGBA, selectedColorRGBA);
            } else {
                log("MenuManager: ERROR - ThemeManager не инициализирован");
            }
        }

        /**
         * @brief Устанавливает цвета для ScrollPane (без альфа-канала)
         */
        public function setScrollableMenuColors(borderColor:uint, backgroundColor:uint):void {
            var borderColorRGBA:uint = 0xFF000000 | borderColor;
            var backgroundColorRGBA:uint = 0xFF000000 | backgroundColor;
            
            setScrollableMenuColorsRGBA(borderColorRGBA, backgroundColorRGBA);
        }

        /**
         * @brief Устанавливает цвета для ScrollPane с поддержкой альфа-канала
         */
        public function setScrollableMenuColorsRGBA(borderColorRGBA:uint, backgroundColorRGBA:uint):void {
            if (themeManager) {
                themeManager.setScrollableMenuColorsRGBA(borderColorRGBA, backgroundColorRGBA);
            } else {
                log("MenuManager: ERROR - ThemeManager не инициализирован");
            }
        }

        /**
         * @brief Сбрасывает все цвета к значениям по умолчанию (тема Glass)
         */
        public function resetColorsToDefault():void {
            if (themeManager) {
                themeManager.resetColorsToDefault();
            } else {
                log("MenuManager: ERROR - ThemeManager не инициализирован");
            }
        }

        /**
         * @brief Получает текущие цвета компонента
         */
        public function getComponentColors(componentType:int):Object {
            if (themeManager) {
                return themeManager.getComponentColors(componentType);
            } else {
                log("MenuManager: ERROR - ThemeManager не инициализирован");
                return null;
            }
        }

        /**
         * @brief Получает текущие цвета ScrollPane
         */
        public function getScrollableMenuColors():Object {
            if (themeManager) {
                return themeManager.getScrollableMenuColors();
            } else {
                log("MenuManager: ERROR - ThemeManager не инициализирован");
                return null;
            }
        }

        
        // ===== ДОПОЛНИТЕЛЬНЫЕ МЕТОДЫ =====

        /**
         * @brief Добавляет один элемент в меню
         * @param item Элемент меню для добавления
         */
        public function addItem(item:Object):void {
            log("MenuManager: Добавление элемента в меню - текст: '" + item.text + "', тип: " + item.type);
            
            if (!_scrollableMenu) {
                log("MenuManager: ERROR - ScrollableMenu не инициализирован");
                return;
            }
            
            try {
                // Добавляем элемент без пересоздания всех остальных
                _scrollableMenu.addSingleItem(item);
                log("MenuManager: Элемент добавлен успешно через addSingleItem()");
            } catch (error:Error) {
                log("MenuManager: ERROR - Ошибка добавления элемента: " + error.message);
                log("MenuManager: Stack trace: " + error.getStackTrace());
            }
        }

        /**
         * @brief Очищает все элементы меню
         */
        public function clearAllItems():void {
            log("MenuManager: Очистка всех элементов меню");
            
            if (!_scrollableMenu) {
                log("MenuManager: ERROR - ScrollableMenu не инициализирован");
                return;
            }
            
            try {
                // Очищаем элементы и обновляем отображение
                _scrollableMenu.setItems([], -1);
                log("MenuManager: Все элементы меню очищены");
            } catch (error:Error) {
                log("MenuManager: ERROR - Ошибка очистки элементов: " + error.message);
            }
        }

        /**
         * @brief Устанавливает состояние checkbox компонента (вызывается из C++)
         * @param labelText Текст элемента для поиска
         * @param checked Новое состояние checkbox
         */
        public function setCheckboxState(labelText:String, checked:Boolean):void {
            log("MenuManager: setCheckboxState(" + labelText + ", " + checked + ")");
            
            if (!_scrollableMenu) {
                log("MenuManager: ERROR - ScrollableMenu не инициализирован");
                return;
            }
            
            // Находим элемент и изменяем его состояние
            var items:Array = _scrollableMenu.getCurrentItems();
            for (var i:int = 0; i < items.length; i++) {
                var item:Object = items[i];
                if (item && item.labelText === labelText && item.type === 2) { // Checkbox
                    item.checked = checked;
                    // Можно обновить визуальный элемент, если нужно
                    log("MenuManager: Checkbox состояние изменено для '" + labelText + "'");
                    return;
                }
            }
            log("MenuManager: WARNING - Checkbox '" + labelText + "' не найден");
        }

        /**
         * @brief Устанавливает состояние switcher компонента (вызывается из C++)
         * @param labelText Текст элемента для поиска
         * @param selectedIndex Новый выбранный индекс switcher
         */
        public function setSwitcherState(labelText:String, selectedIndex:int):void {
            log("MenuManager: setSwitcherState(" + labelText + ", " + selectedIndex + ")");
            
            if (!_scrollableMenu) {
                log("MenuManager: ERROR - ScrollableMenu не инициализирован");
                return;
            }
            
            // Находим элемент и изменяем его состояние
            var items:Array = _scrollableMenu.getCurrentItems();
            for (var i:int = 0; i < items.length; i++) {
                var item:Object = items[i];
                if (item && item.labelText === labelText && item.type === 3) { // Switcher
                    item.selectedIndex = selectedIndex;
                    // Можно обновить визуальный элемент, если нужно
                    log("MenuManager: Switcher состояние изменено для '" + labelText + "'");
                    return;
                }
            }
            log("MenuManager: WARNING - Switcher '" + labelText + "' не найден");
        }

        /**
         * @brief Очистка ресурсов
         */
        public function dispose():void {
            destroy();
        }

        /**
         * @brief Уничтожает MenuManager и очищает все ресурсы
         */
        public function destroy():void {
            log("MenuManager: Начало уничтожения...");
            
            // Скрываем меню
            hideMenu();
            
            // Отписываемся от событий и очищаем ScrollableMenu
            if (_scrollableMenu) {
                _scrollableMenu.removeEventListener("menuItemSelected", onMenuItemSelected);
                _scrollableMenu.removeEventListener("menuBack", onMenuBack);
                _scrollableMenu.destroy();
                _scrollableMenu = null;
                log("MenuManager: ScrollableMenu уничтожен");
            }
            
            // Отписываемся от событий и очищаем MenuScaler
            if (menuScaler) {
                menuScaler.removeEventListener(MenuScaler.SCALE_CHANGED, onScaleChanged);
                menuScaler.destroy();
                menuScaler = null;
                log("MenuManager: MenuScaler уничтожен");
            }
            
            // Отписываемся от событий и очищаем ThemeManager
            if (themeManager) {
                themeManager.removeEventListener(ThemeManager.THEME_CHANGED, onThemeChanged);
                themeManager.removeEventListener(ThemeManager.COMPONENT_COLORS_CHANGED, onComponentColorsChanged);
                themeManager.removeEventListener(ThemeManager.SCROLLABLE_MENU_COLORS_CHANGED, onScrollableMenuColorsChanged);
                themeManager.destroy();
                themeManager = null;
                log("MenuManager: ThemeManager уничтожен");
            }
            
            // Очищаем ссылки
            menuContainer = null;
            isVisible = false;
            isInitialized = false;
            
            log("MenuManager: Уничтожение завершено");
        }
    }
}
