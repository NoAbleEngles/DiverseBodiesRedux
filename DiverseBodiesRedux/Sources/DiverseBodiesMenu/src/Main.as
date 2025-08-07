package {
    import flash.external.ExternalInterface;
    import flash.display.DisplayObject;
    import flash.display.MovieClip;
    import flash.events.Event;
    import flash.utils.Timer;
    import flash.events.TimerEvent;
    import view.MenuManager;
    import view.CustomEvent;
    import view.ScrollableMenu;
    import view.Logger;
    import fl.containers.ScrollPane;

    /**
     * @brief Главный класс Flash-приложения для системы адаптивного меню
     * 
     * Отвечает за:
     * - Инициализацию системы меню и MenuManager
     * - Интеграцию с C++ через BGSCodeObj
     * - Централизованную обработку логирования
     * - Трансляцию событий между MenuManager и C++
     * - Прием вызовов из C++ через ExternalInterface (legacy совместимость)
     */
    public class Main extends MovieClip {
        // ===== ПУБЛИЧНЫЕ ПОЛЯ ДЛЯ FALLOUT 4/Skyrim SE =====
        public var BGSCodeObj:Object; // Для интеграции с MapCodeObjectFunctions

        // ===== ПРИВАТНЫЕ ПОЛЯ =====
        private var menuManager:MenuManager;
        private var logger:Logger; // Централизованный логгер
        
        // Отложенное масштабирование при первом открытии
        private var initialScaleTimer:Timer;
        private var contentReadyTimer:Timer;
        private var scaleRetryCount:int = 0;
        private var maxScaleRetries:int = 10;
        private var isContentReady:Boolean = false;

        
        /**
         * @brief Конструктор - инициализирует Flash-приложение
         */
        public function Main() {
            super();
            
            // Инициализируем централизованный логгер
            logger = Logger.getInstance();
            Logger.setMainInstance(this);
            logger.info("Запуск Flash-приложения", "Main");
            
            // Инициализируем BGSCodeObj для совместимости с Fallout 4/Skyrim SE
            if (!BGSCodeObj) {
                BGSCodeObj = new Object();
                logger.info("BGSCodeObj инициализирован", "Main");
            }
            
            // (Удалено: установка mainInstance больше не нужна)
            
            // Настраиваем ExternalInterface
            setupExternalInterface();
            
            // Инициализируем систему меню
            initializeMenu();
            
            logger.info("Инициализация завершена, приложение готово к работе", "Main");
        }

        /**
         * @brief Настраивает ExternalInterface для входящих вызовов из C++
         */
        private function setupExternalInterface():void {
            if (ExternalInterface.available) {
                logger.info("ExternalInterface доступен, настройка колбэков...", "Main");
                
                // Регистрируем методы для вызова из C++ (входящие вызовы)
                try {
                    ExternalInterface.addCallback("initializeComponents", initializeComponents);
                    ExternalInterface.addCallback("showMenu", showMenu);
                    ExternalInterface.addCallback("hideMenu", hideMenu);
                    ExternalInterface.addCallback("updateItems", updateItems);
                    ExternalInterface.addCallback("setActiveIndex", setActiveIndex);
                    ExternalInterface.addCallback("addItem", addItem);
                    ExternalInterface.addCallback("clearAllItems", clearAllItems);
                    ExternalInterface.addCallback("updateScale", updateScale);
                    ExternalInterface.addCallback("forceUpdateScale", forceUpdateScale);
                    ExternalInterface.addCallback("setStretchMode", setStretchMode);
                    ExternalInterface.addCallback("setMenuMargins", setMenuMargins);
                    ExternalInterface.addCallback("setColorsTheme", setColorsTheme);
                    ExternalInterface.addCallback("setComponentColors", setComponentColors);
                    ExternalInterface.addCallback("setComponentColorsRGBA", setComponentColorsRGBA);
                    ExternalInterface.addCallback("setScrollableMenuColors", setScrollableMenuColors);
                    ExternalInterface.addCallback("setScrollableMenuColorsRGBA", setScrollableMenuColorsRGBA);
                    ExternalInterface.addCallback("resetColorsToDefault", resetColorsToDefault);
                    
                    // Legacy методы для совместимости с старым C++ кодом
                    ExternalInterface.addCallback("show", show);
                    ExternalInterface.addCallback("hide", hide);
                    ExternalInterface.addCallback("push", push);
                    ExternalInterface.addCallback("find", find);
                    ExternalInterface.addCallback("clear", clear);
                    ExternalInterface.addCallback("back", back);
                    ExternalInterface.addCallback("hover", hover);
                    ExternalInterface.addCallback("changeCheckboxComponent", changeCheckboxComponent);
                    ExternalInterface.addCallback("changeSwitcherComponent", changeSwitcherComponent);
                    
                    logger.info("ExternalInterface колбэки зарегистрированы", "Main");
                } catch (error:Error) {
                    logger.error("Ошибка регистрации ExternalInterface: " + error.message, "Main");
                }
            } else {
                logger.warning("ExternalInterface недоступен", "Main");
            }
        }

        /**
         * @brief Инициализирует систему меню
         */
        private function initializeMenu():void {
            logger.info("Поиск ScrollPane контейнера...", "Main");
            
            // Ищем scrollPane_mc на сцене
            var scrollPane:ScrollPane = findScrollPane();
            if (scrollPane) {
                logger.log("ScrollPane найден: " + scrollPane.name, "Main");
                
                // Создаем MenuManager
                menuManager = new MenuManager(scrollPane);
                
                // Подписываемся на события от MenuManager
                menuManager.addEventListener("log", onLog);
                menuManager.addEventListener("menuItemSelected", onMenuItemSelected);
                menuManager.addEventListener("menuItemHover", onMenuItemHover);
                menuManager.addEventListener("menuCheckboxChanged", onMenuCheckboxChanged);
                menuManager.addEventListener("menuSwitcherChanged", onMenuSwitcherChanged);
                menuManager.addEventListener("menuBack", onMenuBack);
                menuManager.addEventListener("scaleChanged", onScaleChanged);
                menuManager.addEventListener("themeChanged", onThemeChanged);
                
                logger.log("MenuManager создан и настроен", "Main");
            } else {
                logger.error("ScrollPane не найден на сцене", "Main");
            }
        }

        /**
         * @brief Ищет ScrollPane на сцене
         */
        private function findScrollPane():ScrollPane {
            // Сначала ищем по имени scrollPane_mc
            var scrollPane:ScrollPane = getChildByName("scrollPane_mc") as ScrollPane;
            if (scrollPane) {
                return scrollPane;
            }
            
            // Если не найден, ищем первый ScrollPane на сцене
            for (var i:int = 0; i < numChildren; i++) {
                var child:DisplayObject = getChildAt(i);
                if (child is ScrollPane) {
                    logger.log("Найден ScrollPane с именем: " + child.name, "Main");
                    return child as ScrollPane;
                }
            }
            
            return null;
        }

        // ===== ОБРАБОТЧИКИ СОБЫТИЙ ОТ MENUMANAGER =====

        /**
         * @brief Обработчик события логирования от MenuManager
         */
        private function onLog(event:CustomEvent):void {
            logger.log(event.data.message);
        }

        /**
         * @brief Обработчик выбора элемента меню от MenuManager
         */
        private function onMenuItemSelected(event:CustomEvent):void {
            try {
                var itemIndex:int = event.data.index;
                var itemData:Object = event.data.item;
                
                logger.info("Выбран элемент " + itemIndex + ": '" + itemData.labelText + "'", "Main");
                
                // Отправляем в C++ через BGSCodeObj
                if (BGSCodeObj && typeof BGSCodeObj.ItemSelected === "function") {
                    try {
                        // Отправляем только текст элемента как единственный аргумент
                        BGSCodeObj.ItemSelected(itemData.labelText);
                        logger.info("ItemSelected отправлено через BGSCodeObj", "Main");
                    } catch (error:Error) {
                        logger.error("Ошибка отправки ItemSelected через BGSCodeObj: " + error.message, "Main");
                    }
                } else {
                    logger.warning("BGSCodeObj.ItemSelected недоступен", "Main");
                }
            } catch (error:Error) {
                logger.error("Ошибка в onMenuItemSelected: " + error.message, "Main");
            }
        }

        /**
         * @brief Обработчик события "назад" от MenuManager
         */
        private function onMenuBack(event:CustomEvent):void {
            logger.info("Получено событие 'назад'", "Main");
            
            // Отправляем в C++ через BGSCodeObj
            if (BGSCodeObj && typeof BGSCodeObj.BackPressed === "function") {
                try {
                    BGSCodeObj.BackPressed();
                    logger.info("BackPressed отправлено через BGSCodeObj", "Main");
                } catch (error:Error) {
                    logger.error("Ошибка отправки BackPressed через BGSCodeObj: " + error.message, "Main");
                }
            } else {
                logger.warning("BGSCodeObj.BackPressed недоступен", "Main");
            }
        }

        /**
         * @brief Обработчик события наведения/выбора элемента от MenuManager
         */
        private function onMenuItemHover(event:CustomEvent):void {
            var labelText:String = event.data.labelText;
            logger.info("Выбран элемент (hover): '" + labelText + "'", "Main");
            
            // Отправляем в C++ через BGSCodeObj (код 2)
            if (BGSCodeObj && typeof BGSCodeObj.ItemHoverChanged === "function") {
                try {
                    BGSCodeObj.ItemHoverChanged(labelText);
                    logger.info("ItemHoverChanged (код 2) отправлено через BGSCodeObj", "Main");
                } catch (error:Error) {
                    logger.error("Ошибка отправки ItemHoverChanged через BGSCodeObj: " + error.message, "Main");
                }
            } else {
                logger.warning("BGSCodeObj.ItemHoverChanged недоступен", "Main");
            }
        }

        /**
         * @brief Обработчик изменения checkbox от MenuManager
         */
        private function onMenuCheckboxChanged(event:CustomEvent):void {
            var labelText:String = event.data.labelText;
            var checked:Boolean = event.data.checked;
            logger.log("Checkbox изменен: '" + labelText + "' = " + checked, "Main");
            
            // Отправляем в C++ через BGSCodeObj
            if (BGSCodeObj && typeof BGSCodeObj.CheckboxChanged === "function") {
                try {
                    BGSCodeObj.CheckboxChanged(labelText, checked);
                    logger.log("CheckboxChanged отправлено через BGSCodeObj", "Main");
                } catch (error:Error) {
                    logger.error("Ошибка отправки CheckboxChanged через BGSCodeObj: " + error.message, "Main");
                }
            } else {
                logger.warning("BGSCodeObj.CheckboxChanged недоступен", "Main");
            }
        }

        /**
         * @brief Обработчик изменения switcher от MenuManager
         */
        private function onMenuSwitcherChanged(event:CustomEvent):void {
            var labelText:String = event.data.labelText;
            var selectedIndex:int = event.data.selectedIndex;
            logger.log("Switcher изменен: '" + labelText + "' = " + selectedIndex, "Main");
            
            // Отправляем в C++ через BGSCodeObj
            if (BGSCodeObj && typeof BGSCodeObj.SwitcherChanged === "function") {
                try {
                    BGSCodeObj.SwitcherChanged(labelText, selectedIndex);
                    logger.log("SwitcherChanged отправлено через BGSCodeObj", "Main");
                } catch (error:Error) {
                    logger.error("Ошибка отправки SwitcherChanged через BGSCodeObj: " + error.message, "Main");
                }
            } else {
                logger.warning("BGSCodeObj.SwitcherChanged недоступен", "Main");
            }
        }

        /**
         * @brief Обработчик изменения масштаба от MenuManager
         */
        private function onScaleChanged(event:CustomEvent):void {
            logger.log("Изменен масштаб: " + event.data.scale, "Main");
        }

        /**
         * @brief Обработчик изменения темы от MenuManager
         */
        private function onThemeChanged(event:CustomEvent):void {
            logger.log("Изменена тема", "Main");
        }

        // ===== ПУБЛИЧНОЕ API ДЛЯ C++ =====

        /**
         * @brief Принудительная инициализация компонентов (вызывается из C++)
         */
        public function initializeComponents():void {
            logger.log("initializeComponents() вызван из C++", "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            // MenuManager уже инициализирован в конструкторе
            logger.log("Компоненты уже инициализированы", "Main");
        }

        /**
         * @brief Показывает меню с заданными параметрами
         */
        public function showMenu(title:String, items:Array, activeIndex:int = 0):void {
            logger.log("showMenu() вызван - заголовок: '" + title + "', элементов: " + items.length, "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.showMenu(title, items, activeIndex);
        }

        /**
         * @brief Скрывает меню
         */
        public function hideMenu():void {
            logger.log("hideMenu() вызван", "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.hideMenu();
        }

        /**
         * @brief Обновляет элементы меню
         */
        public function updateItems(items:Array, activeIndex:int = 0):void {
            logger.log("updateItems() вызван - элементов: " + items.length, "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.updateItems(items, activeIndex);
        }

        /**
         * @brief Устанавливает активный индекс
         */
        public function setActiveIndex(newIndex:int):void {
            logger.log("setActiveIndex() вызван - индекс: " + newIndex, "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.setActiveIndex(newIndex);
        }

        /**
         * @brief Добавляет элемент в меню
         */
        public function addItem(item:Object):void {
            logger.log("addItem() вызван", "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.addItem(item);
        }

        /**
         * @brief Очищает все элементы меню
         */
        public function clearAllItems():void {
            logger.log("clearAllItems() вызван", "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.clearAllItems();
        }

        // ===== МАСШТАБИРОВАНИЕ =====

        /**
         * @brief Обновляет масштабирование
         */
        public function updateScale():void {
            logger.log("updateScale() вызван", "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.updateScale();
        }

        /**
         * @brief Принудительно обновляет масштабирование
         */
        public function forceUpdateScale():void {
            logger.log("forceUpdateScale() вызван", "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.forceUpdateScale();
        }

        /**
         * @brief Переключает режим масштабирования
         */
        public function setStretchMode(useStretch:Boolean):void {
            logger.log("setStretchMode() вызван - режим: " + useStretch, "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.setStretchMode(useStretch);
        }

        /**
         * @brief Устанавливает отступы меню
         */
        public function setMenuMargins(horizontal:Number, vertical:Number):void {
            logger.log("setMenuMargins() вызван - горизонтальные: " + horizontal + ", вертикальные: " + vertical, "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.setMenuMargins(horizontal, vertical);
        }

        // ===== ЦВЕТОВЫЕ СХЕМЫ =====

        /**
         * @brief Применяет полную цветовую тему (18 аргументов из C++)
         */
        public function setColorsTheme(scrollPaneBorder:uint, scrollPaneBackground:uint,
                                      labelNormal:uint, labelText:uint, labelHover:uint, labelSelected:uint,
                                      buttonNormal:uint, buttonText:uint, buttonHover:uint, buttonSelected:uint,
                                      checkboxNormal:uint, checkboxText:uint, checkboxHover:uint, checkboxSelected:uint,
                                      switcherNormal:uint, switcherText:uint, switcherHover:uint, switcherSelected:uint):void {
            logger.log("setColorsTheme() вызван с 18 аргументами", "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.setColorsTheme(scrollPaneBorder, scrollPaneBackground,
                                     labelNormal, labelText, labelHover, labelSelected,
                                     buttonNormal, buttonText, buttonHover, buttonSelected,
                                     checkboxNormal, checkboxText, checkboxHover, checkboxSelected,
                                     switcherNormal, switcherText, switcherHover, switcherSelected);
        }

        /**
         * @brief Устанавливает цвета для компонентов меню
         */
        public function setComponentColors(componentType:int, normalColor:uint, textColor:uint, hoverColor:uint, selectedColor:uint):void {
            logger.log("setComponentColors() вызван для типа " + componentType, "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.setComponentColors(componentType, normalColor, textColor, hoverColor, selectedColor);
        }

        /**
         * @brief Устанавливает цвета для компонентов меню с поддержкой альфа-канала
         */
        public function setComponentColorsRGBA(componentType:int, normalColorRGBA:uint, textColorRGBA:uint, hoverColorRGBA:uint, selectedColorRGBA:uint):void {
            logger.log("Main: setComponentColorsRGBA() вызван для типа " + componentType);
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.setComponentColorsRGBA(componentType, normalColorRGBA, textColorRGBA, hoverColorRGBA, selectedColorRGBA);
        }

        /**
         * @brief Устанавливает цвета для ScrollableMenu
         */
        public function setScrollableMenuColors(borderColor:uint, backgroundColor:uint):void {
            logger.log("setScrollableMenuColors() вызван", "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.setScrollableMenuColors(borderColor, backgroundColor);
        }

        /**
         * @brief Устанавливает цвета для ScrollableMenu с поддержкой альфа-канала
         */
        public function setScrollableMenuColorsRGBA(borderColorRGBA:uint, backgroundColorRGBA:uint):void {
            logger.log("setScrollableMenuColorsRGBA() вызван", "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.setScrollableMenuColorsRGBA(borderColorRGBA, backgroundColorRGBA);
        }

        /**
         * @brief Сбрасывает все цвета к значениям по умолчанию
         */
        public function resetColorsToDefault():void {
            logger.log("resetColorsToDefault() вызван", "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.resetColorsToDefault();
        }

        /**
         * @brief Получает текущие цвета компонента
         */
        public function getComponentColors(componentType:int):Object {
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return null;
            }
            
            return menuManager.getComponentColors(componentType);
        }

        /**
         * @brief Получает текущие цвета ScrollableMenu
         */
        public function getScrollableMenuColors():Object {
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return null;
            }
            
            return menuManager.getScrollableMenuColors();
        }

        // ===== LEGACY API ДЛЯ СОВМЕСТИМОСТИ =====

        /**
         * @brief C++ вызывает back() для возврата/закрытия меню
         */
        public function back():void {
            this.hide();
        }

        /**
         * @brief C++ вызывает hover(labelText:String) для подсветки
         */
        public function hover(labelText:String):void {
            var idx:int = find(labelText);
            if (idx >= 0) {
                if (menuManager) {
                    menuManager.setActiveIndex(idx);
                }
            }
        }

        /**
         * @brief C++ вызывает changeCheckboxComponent(labelText:String, checked:Boolean)
         */
        public function changeCheckboxComponent(labelText:String, checked:Boolean):void {
            logger.log("changeCheckboxComponent - " + labelText + " = " + checked, "Main");

            // Изменяем состояние компонента во Flash (НЕ отправляем обратно в C++)
            if (menuManager) {
                menuManager.setCheckboxState(labelText, checked);
            }
        }

        /**
         * @brief C++ вызывает changeSwitcherComponent(labelText:String, selectedIndex:int)
         */
        public function changeSwitcherComponent(labelText:String, selectedIndex:int):void {
            logger.log("changeSwitcherComponent - " + labelText + " = " + selectedIndex, "Main");

            // Изменяем состояние компонента во Flash (НЕ отправляем обратно в C++)
            if (menuManager) {
                menuManager.setSwitcherState(labelText, selectedIndex);
            }
        }

        // ===== LEGACY МЕТОДЫ ДЛЯ СОВМЕСТИМОСТИ С СУЩЕСТВУЮЩИМ C++ КОДОМ =====
        // Эти методы вызываются из старого кода C++ и обеспечивают совместимость

        /**
         * @brief Показывает меню (legacy метод)
         */
        public function show():void {
            logger.log("show() вызван (legacy)", "Main");
            if (menuManager) {
                // Показываем меню с текущими элементами
                var currentItems:Array = menuManager.getCurrentItems();
                logger.log("Показываем меню с " + (currentItems ? currentItems.length : 0) + " элементами", "Main");
                menuManager.showMenu("", currentItems || [], 0);
            }
        }

        /**
         * @brief Скрывает меню (legacy метод)
         */
        public function hide():void {
            if (menuManager) {
                menuManager.hideMenu();
            }
        }

        /**
         * @brief Добавляет элемент в меню (legacy метод)
         */
        public function push(type:int, text:String, ...args):void {
                logger.log("push() вызван - тип: " + type + ", текст: '" + text + "'", "Main");
            
            var item:Object = {
                type: type,
                labelText: text,
                text: text
            };
            
            // Дополнительные параметры в зависимости от типа
            switch (type) {
                case 2: // Checkbox
                    item.checked = args.length > 0 ? Boolean(args[0]) : false;
                    logger.log("Checkbox с состоянием: " + item.checked, "Main");
                    break;
                case 3: // Switcher
                    // Для Switcher text содержит строку с вариантами, разделенными запятыми
                    var optionsString:String = text || "";
                    var optionsArray:Array = [];
                    
                    if (optionsString.length > 0) {
                        optionsArray = optionsString.split(",");
                        // Очищаем пробелы в опциях
                        for (var i:int = 0; i < optionsArray.length; i++) {
                            optionsArray[i] = String(optionsArray[i]).replace(/^\s+|\s+$/g, "");
                        }
                    }
                    
                    item.options = optionsArray;
                    item.selectedIndex = args.length > 0 ? int(args[0]) : 0; // presetTypeCalculated
                    
                    logger.log("Switcher создан:", "Main");
                    logger.log("  - Опции: [" + optionsArray.join(", ") + "]", "Main");
                    logger.log("  - Выбранный индекс: " + item.selectedIndex, "Main");
                    logger.log("  - Общее количество опций: " + optionsArray.length, "Main");
                    break;
                default:
                    logger.log("Стандартный элемент типа: " + type, "Main");
                    break;
            }
            
            if (menuManager) {
                menuManager.addItem(item);
                logger.log("Элемент добавлен, всего элементов: " + menuManager.getItemCount(), "Main");
            } else {
                logger.error("MenuManager не инициализирован", "Main");
            }
        }

        /**
         * @brief Ищет элемент по тексту и возвращает индекс (legacy метод)
         */
        public function find(labelText:String):int {
            if (!menuManager) return -1;
            
            var items:Array = menuManager.getCurrentItems();
            for (var i:int = 0; i < items.length; i++) {
                if (items[i] && items[i].labelText === labelText) {
                    return i;
                }
            }
            
            return -1;
        }

        /**
         * @brief Очищает все элементы меню (legacy метод для C++)
         */
        public function clear():void {
            logger.info("clear() вызван - очистка меню", "Main");
            
            if (!menuManager) {
                logger.error("MenuManager не инициализирован", "Main");
                return;
            }
            
            menuManager.clearAllItems();
        }
    }
}