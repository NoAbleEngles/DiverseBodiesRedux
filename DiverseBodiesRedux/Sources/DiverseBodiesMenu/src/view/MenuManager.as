package view {
    // import flash.display.MovieClip;
    // import flash.text.TextField;
    // import flash.text.TextFormat;
    import flash.external.ExternalInterface;
    import fl.containers.ScrollPane;
    import view.ScrollableMenu;
    import view.MenuScaler;
    import view.ThemeManager;

    /**
     * @brief Менеджер меню - центральный класс для управления отображением и логикой меню
     * 
     * Координирует работу между различными компонентами системы:
     * - Управляет отображением и скрытием меню
     * - Инициализирует и настраивает внутренние компоненты (ScrollPane)
     * - Обрабатывает события от пользователя и передает их в C++
     * - Интегрируется с системой масштабирования (MenuScaler)
     * - Управляет цветовыми схемами через ThemeManager
     * - (legacy) Предоставляет fallback функциональность при отсутствии элементов в FLA
     * 
     * Основные обязанности:
     * - Поиск и инициализация компонентов меню в контейнере (без заголовка и background)
     * - Управление жизненным циклом ScrollableMenu
     * - Координация масштабирования через MenuScaler
     * - Управление цветовыми схемами через ThemeManager
     * - Обработка событий выбора элементов и навигации
     */
    public class MenuManager {
        private var menuContainer:ScrollPane;      // scrollPane_mc со сцены
        private var _scrollableMenu:ScrollableMenu; // Управляет ScrollPane
        private var menuScaler:MenuScaler;        // Управляет масштабированием
        private var themeManager:ThemeManager;    // Управляет цветовыми схемами
        
        private var isVisible:Boolean = false;

        /**
         * @brief Логирует сообщение через trace и дублирует в C++ через ScrollableMenu.mainInstance
         * @param message Сообщение для логирования
         */
        private function log(message:String):void {
            trace(message);
            if (ScrollableMenu.mainInstance && ScrollableMenu.mainInstance.BGSCodeObj && 
                typeof ScrollableMenu.mainInstance.BGSCodeObj.Log === "function") {
                ScrollableMenu.mainInstance.BGSCodeObj.Log(message);
            }
        }

        /**
         * @brief Конструктор - инициализирует MenuManager с заданным контейнером
         * @param container ScrollPane контейнер меню (scrollPane_mc из FLA)
         */
        public function MenuManager(container:ScrollPane) {
            this.menuContainer = container;
            log("MenuManager: Инициализация с контейнером " + container.name);
            
            initializeComponentsInternal();
            
            // Инициализируем систему масштабирования
            initializeScaling();
            
            // По умолчанию скрываем меню
            hideMenu();
        }

        /**
         * @brief Инициализирует все компоненты меню (ScrollPane, масштабирование, цветовые схемы)
         */
        private function initializeComponentsInternal():void {
            log("MenuManager: Инициализация ScrollPane...");
            initializeScrollPane();
            
            log("MenuManager: Инициализация системы цветов...");
            initializeThemeManager();
            
            log("MenuManager: Инициализация завершена");
        }


        // Заголовок и background не используются

        /**
         * Инициализирует ScrollPane
         */
        private function initializeScrollPane():void {
            _scrollableMenu = new ScrollableMenu(menuContainer);
            _scrollableMenu.addEventListener("menuItemSelected", onMenuItemSelected);
            _scrollableMenu.addEventListener("menuBack", onMenuBack);
            log("MenuManager: fl.containers.ScrollPane инициализирован");
        }

        /**
         * Инициализирует систему управления цветовыми схемами
         */
        private function initializeThemeManager():void {
            log("MenuManager: Инициализация ThemeManager...");
            
            try {
                themeManager = ThemeManager.getInstance();
                themeManager.initialize(menuContainer, _scrollableMenu);
                log("MenuManager: ThemeManager инициализирован успешно");
            } catch (error:Error) {
                log("ERROR: Ошибка инициализации ThemeManager: " + error.message);
                log("Stack: " + error.getStackTrace());
            }
        }

        // fallback ScrollPane и background не требуются
        // private function createFallbackScrollPane():void {}

        /**
         * Инициализирует систему масштабирования меню
         */
        private function initializeScaling():void {
            log("MenuManager: Инициализация системы масштабирования...");
            
            try {
                menuScaler = MenuScaler.getInstance();
                menuScaler.initialize(menuContainer);
                log("MenuManager: MenuScaler инициализирован успешно");
            } catch (error:Error) {
                log("ERROR: Ошибка инициализации MenuScaler: " + error.message);
                log("Stack: " + error.getStackTrace());
            }
        }

        /**
         * Рекурсивно ищет первый TextField в контейнере
         */
        // Поиск TextField не требуется

        // ----- Публичные методы для управления меню -----

        /**
         * Показывает меню с заданными параметрами
         */
        public function showMenu(title:String, items:Array, activeIndex:int = 0):void {
            log("MenuManager: Показать меню '" + title + "' с " + items.length + " элементами");
            

            // Заголовок и background не используются
            
            updateItems(items, activeIndex);
            
            menuContainer.visible = true;
            isVisible = true;
            
            // Устанавливаем фокус для корректной работы клавиатуры
            if (menuContainer.stage) {
                menuContainer.stage.focus = menuContainer.stage;
            }
            
            log("MenuManager: Меню показано успешно (без заголовка)");
        }


        // Методы для заголовка и background не требуются

        /**
         * Обновляет элементы меню
         */
        public function updateItems(items:Array, activeIndex:int = 0):void {
            if (_scrollableMenu) {
                _scrollableMenu.setItems(items, activeIndex);
                log("MenuManager: Элементы меню обновлены (" + items.length + " элементов)");
            } else {
                log("ERROR: Не удалось обновить элементы - scrollableMenu не инициализирован");
            }
        }

        /**
         * Скрывает меню
         */
        public function hideMenu():void {
            menuContainer.visible = false;
            isVisible = false;
            log("MenuManager: Меню скрыто");
        }

        /**
         * Проверяет, видимо ли меню
         */
        public function get visible():Boolean {
            return isVisible;
        }

        /**
         * Получает текущий активный индекс
         */
        public function getCurrentActiveIndex():int {
            return _scrollableMenu ? _scrollableMenu.activeIndex : -1;
        }

        /**
         * Получает количество элементов в меню
         */
        public function getItemCount():int {
            return _scrollableMenu ? _scrollableMenu.getCurrentItems().length : 0;
        }

        /**
         * @brief Получает текущие элементы меню
         * @return Массив текущих элементов или пустой массив
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
            
            // Получаем текущие элементы для проверки границ и типа
            var currentItems:Array = _scrollableMenu.getCurrentItems();
            if (!currentItems || currentItems.length === 0) {
                log("MenuManager: WARNING - Нет элементов в меню для установки активного индекса");
                return;
            }
            
            // Проверяем корректность индекса
            if (newIndex < 0 || newIndex >= currentItems.length) {
                log("MenuManager: ERROR - Некорректный индекс " + newIndex + ", доступно элементов: " + currentItems.length);
                return;
            }
            
            // Проверяем, что элемент интерактивный (не LabelComponent)
            var targetItem:Object = currentItems[newIndex];
            if (targetItem && targetItem.type === 0) { // LabelComponent
                log("MenuManager: WARNING - Нельзя установить активный индекс на LabelComponent (индекс " + newIndex + ")");
                return;
            }
            
            // Вызываем напрямую публичный метод setActiveIndex
            try {
                // Сохраняем старый активный индекс
                var oldActiveIndex:int = _scrollableMenu.activeIndex;
                
                // Напрямую устанавливаем активный индекс
                _scrollableMenu.setActiveIndex(newIndex);
                
                log("MenuManager: Активный индекс изменен с " + oldActiveIndex + " на " + newIndex);
            } catch (error:Error) {
                log("MenuManager: ERROR при установке активного индекса: " + error.message);
            }
        }

        /**
         * @brief Возвращает ссылку на ScrollableMenu
         */
        public function get scrollableMenu():ScrollableMenu {
            return _scrollableMenu;
        }

        // ----- Обработчики событий -----

        /**
         * Обработчик выбора элемента меню
         */
        private function onMenuItemSelected(event:*):void {
            var itemIndex:int = event.data.index;
            var itemData:Object = event.data.item;
            
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
            
            // Отправляем в C++
            if (ExternalInterface.available) {
                try {
                    ExternalInterface.call("onMenuItemSelected", itemIndex, itemData);
                    log("MenuManager: Данные отправлены в C++");
                } catch (error:Error) {
                    log("MenuManager: Ошибка отправки в C++: " + error.message);
                }
            } else {
                log("MenuManager: ExternalInterface недоступен");
            }
        }

        /**
         * Обработчик события "назад"
         */
        private function onMenuBack(event:*):void {
            log("MenuManager: Получено событие 'назад'");
            
            // Отправляем в C++
            if (ExternalInterface.available) {
                try {
                    ExternalInterface.call("onMenuBack");
                    log("MenuManager: Событие 'назад' отправлено в C++");
                } catch (error:Error) {
                    log("MenuManager: Ошибка отправки onMenuBack в C++: " + error.message);
                }
            } else {
                log("MenuManager: ExternalInterface недоступен для onMenuBack");
            }
        }

        /**
         * Очистка ресурсов
         */
        public function dispose():void {
            log("MenuManager: Очистка ресурсов...");
            
            if (_scrollableMenu) {
                _scrollableMenu.removeEventListener("menuItemSelected", onMenuItemSelected);
                _scrollableMenu.removeEventListener("menuBack", onMenuBack);
                _scrollableMenu.destroy();
                _scrollableMenu = null;
            }
            
            if (menuScaler) {
                menuScaler.destroy();
                menuScaler = null;
            }
            
            // titleLabel = null; // удалено, больше не используется
            // background_mc = null; // удалено, больше не используется
            menuContainer = null;
            
            log("MenuManager: Ресурсы очищены");
        }

        // ----- Методы управления масштабированием -----

        /**
         * Принудительно обновляет масштабирование меню
         */
        public function updateScale():void {
            if (menuScaler) {
                menuScaler.updateScale();
            }
        }

        /**
         * @brief Принудительно обновляет масштабирование с гарантированным результатом
         * 
         * Этот метод использует forceUpdateScale() из MenuScaler для обеспечения
         * корректного масштабирования независимо от состояния готовности меню
         */
        public function forceUpdateScale():void {
            if (menuScaler) {
                menuScaler.forceUpdateScale();
            }
        }

        /**
         * Переключает режим масштабирования (пропорциональное/растягивающее)
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
         * Получает текущий коэффициент масштабирования
         */
        public function getCurrentScale():Number {
            return menuScaler ? menuScaler.getCurrentScale() : 1.0;
        }

        /**
         * Получает информацию о базовом разрешении
         */
        public function getBaseResolution():Object {
            return menuScaler ? menuScaler.getBaseResolution() : {width: 1920, height: 1080};
        }

        /**
         * Устанавливает отступы меню от краев экрана
         * @param right отступ справа (пиксели)
         * @param top отступ сверху (пиксели)  
         * @param bottom отступ снизу (пиксели)
         */
        public function setMenuMargins(right:Number, top:Number, bottom:Number):void {
            if (menuScaler) {
                menuScaler.setMargins(right, top);
            }
        }

        /**
         * Получает текущие отступы меню
         */
        public function getMenuMargins():Object {
            return menuScaler ? menuScaler.getMargins() : {right: 10, top: 10, bottom: 10};
        }

        // ===== МЕТОДЫ УПРАВЛЕНИЯ ЦВЕТОВЫМИ СХЕМАМИ =====

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
            log("MenuManager: Применение цветовой темы с 18 аргументами");
            
            if (!themeManager) {
                log("MenuManager: ERROR - ThemeManager не инициализирован");
                return;
            }
            
            // Устанавливаем цвета ScrollPane через ThemeManager
            themeManager.setScrollableMenuColorsRGBA(scrollPaneBorder, scrollPaneBackground);
            
            // Устанавливаем цвета компонентов
            themeManager.setComponentColorsRGBA(0, labelNormal, labelText, labelHover, labelSelected);       // Label
            themeManager.setComponentColorsRGBA(1, buttonNormal, buttonText, buttonHover, buttonSelected);   // Button  
            themeManager.setComponentColorsRGBA(2, checkboxNormal, checkboxText, checkboxHover, checkboxSelected); // Checkbox
            themeManager.setComponentColorsRGBA(3, switcherNormal, switcherText, switcherHover, switcherSelected); // Switcher
            
            log("MenuManager: Цветовая тема применена через ThemeManager");
        }

        /**
         * @brief Устанавливает цвета для определенного типа компонентов (без альфа-канала)
         * @param componentType Тип компонента (0=Label, 1=Button, 2=Checkbox, 3=Switcher)
         * @param normalColor Обычный цвет элемента (RGB hex)
         * @param textColor Цвет текста (RGB hex)
         * @param hoverColor Цвет при наведении (RGB hex)
         * @param selectedColor Цвет выделенного элемента (RGB hex)
         */
        public function setComponentColors(componentType:int, normalColor:uint, textColor:uint, hoverColor:uint, selectedColor:uint):void {
            // Конвертируем RGB в RGBA, добавляя полную непрозрачность
            var normalColorRGBA:uint = 0xFF000000 | normalColor;
            var textColorRGBA:uint = 0xFF000000 | textColor;
            var hoverColorRGBA:uint = 0xFF000000 | hoverColor;
            var selectedColorRGBA:uint = 0xFF000000 | selectedColor;
            
            setComponentColorsRGBA(componentType, normalColorRGBA, textColorRGBA, hoverColorRGBA, selectedColorRGBA);
        }

        /**
         * @brief Устанавливает цвета для определенного типа компонентов с поддержкой альфа-канала
         * @param componentType Тип компонента (0=Label, 1=Button, 2=Checkbox, 3=Switcher)
         * @param normalColorRGBA Обычный цвет элемента (ARGB hex)
         * @param textColorRGBA Цвет текста (ARGB hex)
         * @param hoverColorRGBA Цвет при наведении (ARGB hex)
         * @param selectedColorRGBA Цвет выделенного элемента (ARGB hex)
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
         * @param borderColor Цвет границы/ободка (RGB hex)
         * @param backgroundColor Цвет фона/подложки (RGB hex)
         */
        public function setScrollableMenuColors(borderColor:uint, backgroundColor:uint):void {
            // Конвертируем RGB в RGBA, добавляя полную непрозрачность
            var borderColorRGBA:uint = 0xFF000000 | borderColor;
            var backgroundColorRGBA:uint = 0xFF000000 | backgroundColor;
            
            setScrollableMenuColorsRGBA(borderColorRGBA, backgroundColorRGBA);
        }

        /**
         * @brief Устанавливает цвета для ScrollPane с поддержкой альфа-канала
         * @param borderColorRGBA Цвет границы/ободка (ARGB hex)
         * @param backgroundColorRGBA Цвет фона/подложки (ARGB hex)
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
         * @param componentType Тип компонента (0=Label, 1=Button, 2=Checkbox, 3=Switcher)
         * @return Объект с цветами компонента или null при ошибке
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
         * @return Объект с цветами ScrollPane
         */
        public function getScrollableMenuColors():Object {
            if (themeManager) {
                return themeManager.getScrollableMenuColors();
            } else {
                log("MenuManager: ERROR - ThemeManager не инициализирован");
                return null;
            }
        }

        /**
         * @brief Публичный метод для принудительной инициализации компонентов
         * Используется из Main.as для обеспечения корректной инициализации
         */
        public function initializeComponents():void {
            log("MenuManager: Публичная инициализация компонентов...");
            
            // Вызываем приватный метод инициализации
            initializeComponentsInternal();
            
            // Дополнительно обновляем масштабирование
            updateScale();
            
            log("MenuManager: Публичная инициализация завершена");
        }

        /**
         * @brief Применяет отложенные цвета после полной инициализации
         * Вызывается после завершения всех инициализаций для повторного применения цветов
         */
        public function applyDelayedColors():void {
            if (themeManager) {
                themeManager.applyDelayedColors();
            } else {
                log("MenuManager: ERROR - ThemeManager не инициализирован для отложенных цветов");
            }
        }

        /**
         * @brief Добавляет один элемент в меню
         * @param item Элемент меню для добавления
         */
        public function addItem(item:Object):void {
            log("MenuManager: Добавление элемента в меню - " + item.text);
            
            if (!_scrollableMenu) {
                log("MenuManager: ERROR - ScrollableMenu не инициализирован");
                return;
            }
            
            try {
                // Получаем текущие элементы
                var currentItems:Array = _scrollableMenu.getCurrentItems();
                if (!currentItems) {
                    currentItems = [];
                }
                
                // Добавляем новый элемент
                currentItems.push(item);
                
                // Обновляем меню с новым списком элементов
                updateItems(currentItems, currentItems.length - 1);
                
                log("MenuManager: Элемент добавлен, всего элементов: " + currentItems.length);
            } catch (error:Error) {
                log("MenuManager: ERROR - Ошибка добавления элемента: " + error.message);
            }
        }

        /**
         * Уничтожает MenuManager и очищает все ресурсы
         */
        public function destroy():void {
            log("MenuManager: Начало уничтожения...");
            
            // Скрываем меню
            hideMenu();
            
            // Очищаем ScrollableMenu
            if (_scrollableMenu) {
                _scrollableMenu.removeEventListener("menuItemSelected", onMenuItemSelected);
                _scrollableMenu.removeEventListener("menuBack", onMenuBack);
                _scrollableMenu.destroy();
                _scrollableMenu = null;
                log("MenuManager: ScrollableMenu уничтожен");
            }
            
            // Очищаем MenuScaler
            if (menuScaler) {
                menuScaler.destroy();
                menuScaler = null;
                log("MenuManager: MenuScaler уничтожен");
            }
            
            // Очищаем ThemeManager
            if (themeManager) {
                themeManager.destroy();
                themeManager = null;
                log("MenuManager: ThemeManager уничтожен");
            }
            
            // Очищаем ссылки
            // titleLabel = null; // удалено, больше не используется
            // background_mc = null; // удалено, больше не используется
            menuContainer = null;
            isVisible = false;
            
            log("MenuManager: Уничтожение завершено");
        }
    }
}
