package {
    import flash.external.ExternalInterface;
    import flash.display.DisplayObject;
    import flash.display.MovieClip;
    import flash.external.ExternalInterface;
    import flash.utils.Timer;
    import flash.events.TimerEvent;
    import flash.events.Event;
    import view.MenuManager;
    import view.components.*;
    import fl.containers.ScrollPane;

    /**
     * @brief Главный класс Flash-приложения для системы адаптивного меню
     * 
     * Отвечает за:
     * - Инициализацию системы меню с автоматическим поиском или созданием fallback
     * - Интеграцию с C++ через ExternalInterface
     * - Предоставление упрощенного API для управления меню
     * - Трансляцию событий между AS3 и C++
     * 
     * Основные возможности:
     * - Адаптивное масштабирование под любое разрешение
     * - Поддержка различных типов компонентов (кнопки, чекбоксы, свитчеры)
     * - Автоматическое позиционирование справа с настраиваемыми отступами
     * - Полная интеграция с C++ через колбэки
     */
    public class Main extends MovieClip {
        // ===== ПУБЛИЧНЫЕ ПОЛЯ ДЛЯ FALLOUT 4/Skyrim SE =====
        public var BGSCodeObj:Object; // Для интеграции с MapCodeObjectFunctions

        // ===== ПРИВАТНЫЕ ПОЛЯ =====
        private var menuManager:MenuManager;
        
        // Отложенное масштабирование при первом открытии
        private var initialScaleTimer:Timer;
        private var contentReadyTimer:Timer;
        private var scaleRetryCount:int = 0;
        private var maxScaleRetries:int = 10;
        private var isContentReady:Boolean = false;

        // ===== МЕТОДЫ НАСТРОЙКИ ЦВЕТОВ =====

        /**
         * @brief Устанавливает цвета для компонентов меню
         * @param componentType Тип компонента (0=Label, 1=Button, 2=Checkbox, 3=Switcher)
         * @param normalColor Обычный цвет элемента (RGB hex)
         * @param textColor Цвет текста (RGB hex)
         * @param hoverColor Цвет при наведении (RGB hex)
         * @param selectedColor Цвет выделенного элемента (RGB hex)
         */
        public function setComponentColors(componentType:int, normalColor:uint, textColor:uint, hoverColor:uint, selectedColor:uint):void {
            log("Main: Установка цветов для компонента " + componentType);
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }
            
            menuManager.setComponentColors(componentType, normalColor, textColor, hoverColor, selectedColor);
        }

        /**
         * @brief Устанавливает цвета для компонентов меню с поддержкой альфа-канала
         * @param componentType Тип компонента (0=Label, 1=Button, 2=Checkbox, 3=Switcher)
         * @param normalColorRGBA Обычный цвет элемента (ARGB hex)
         * @param textColorRGBA Цвет текста (ARGB hex)
         * @param hoverColorRGBA Цвет при наведении (ARGB hex)
         * @param selectedColorRGBA Цвет выделенного элемента (ARGB hex)
         */
        public function setComponentColorsRGBA(componentType:int, normalColorRGBA:uint, textColorRGBA:uint, hoverColorRGBA:uint, selectedColorRGBA:uint):void {
            log("Main: Установка RGBA цветов для компонента " + componentType + 
                " - нормальный: 0x" + normalColorRGBA.toString(16).toUpperCase() + 
                ", текст: 0x" + textColorRGBA.toString(16).toUpperCase() + 
                ", наведение: 0x" + hoverColorRGBA.toString(16).toUpperCase() + 
                ", выделенный: 0x" + selectedColorRGBA.toString(16).toUpperCase());
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }
            
            menuManager.setComponentColorsRGBA(componentType, normalColorRGBA, textColorRGBA, hoverColorRGBA, selectedColorRGBA);
        }

        /**
         * @brief Устанавливает цвета для ScrollableMenu
         * @param borderColor Цвет границы/ободка (RGB hex)
         * @param backgroundColor Цвет фона/подложки (RGB hex)
         */
        public function setScrollableMenuColors(borderColor:uint, backgroundColor:uint):void {
            log("Main: Установка цветов ScrollableMenu - граница: 0x" + borderColor.toString(16).toUpperCase() + 
                ", фон: 0x" + backgroundColor.toString(16).toUpperCase());
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }
            
            menuManager.setScrollableMenuColors(borderColor, backgroundColor);
        }

        /**
         * @brief Устанавливает цвета для ScrollableMenu с поддержкой альфа-канала
         * @param borderColorRGBA Цвет границы/ободка (ARGB hex)
         * @param backgroundColorRGBA Цвет фона/подложки (ARGB hex)
         */
        public function setScrollableMenuColorsRGBA(borderColorRGBA:uint, backgroundColorRGBA:uint):void {
            log("Main: Установка RGBA цветов ScrollableMenu - граница: 0x" + borderColorRGBA.toString(16).toUpperCase() + 
                ", фон: 0x" + backgroundColorRGBA.toString(16).toUpperCase());
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }
            
            menuManager.setScrollableMenuColorsRGBA(borderColorRGBA, backgroundColorRGBA);
        }

        /**
         * @brief Сбрасывает все цвета к значениям по умолчанию
         */
        public function resetColorsToDefault():void {
            log("Main: Сброс всех цветов к значениям по умолчанию");
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }
            
            menuManager.resetColorsToDefault();
        }

        /**
         * @brief Получает текущие цвета компонента
         * @param componentType Тип компонента (0=Label, 1=Button, 2=Checkbox, 3=Switcher)
         * @return Объект с цветами компонента
         */
        public function getComponentColors(componentType:int):Object {
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return null;
            }
            
            return menuManager.getComponentColors(componentType);
        }

        /**
         * @brief Получает текущие цвета ScrollableMenu
         * @return Объект с цветами меню
         */
        public function getScrollableMenuColors():Object {
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return null;
            }
            
            return menuManager.getScrollableMenuColors();
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
            log("Main: Применение цветовой темы с 18 аргументами");
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }
            
            menuManager.setColorsTheme(scrollPaneBorder, scrollPaneBackground,
                                     labelNormal, labelText, labelHover, labelSelected,
                                     buttonNormal, buttonText, buttonHover, buttonSelected,
                                     checkboxNormal, checkboxText, checkboxHover, checkboxSelected,
                                     switcherNormal, switcherText, switcherHover, switcherSelected);
        }

        // ===== ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ =====
        // ====== PUBLIC API ДЛЯ C++ (MapCodeObjectFunctions) ======

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
            if (idx >= 0) setHoverHighlight(idx);
        }

        /**
         * @brief C++ вызывает changeCheckboxComponent(labelText:String, checked:Boolean)
         */
        public function changeCheckboxComponent(labelText:String, checked:Boolean):void {
            sendChangeCheckboxCallback(labelText, checked);
        }

        /**
         * @brief C++ вызывает changeSwitcherComponent(labelText:String, selectedIndex:int)
         */
        public function changeSwitcherComponent(labelText:String, selectedIndex:int):void {
            sendChangeSwitcherCallback(labelText, selectedIndex);
        }
        
        /**
         * @brief Конструктор - инициализирует приложение
         */
        public function Main() {
            log("Main: Приложение запущено");
            // Инициализация BGSCodeObj как в NAFMenu
            BGSCodeObj = new Object();

            // Устанавливаем ссылку на Main для колбэков из ScrollableMenu
            view.ScrollableMenu.mainInstance = this;

            // Инициализируем систему меню
            initializeMenu();

            // Уведомляем C++ что Flash готов
            registerExternalCallbacks();

            // Настраиваем отложенное масштабирование
            setupDelayedScaling();

            // Добавляем тестовые кнопки

            show();
            //addTestButtons();
        }

        /**
         * Добавляет 5 тестовых кнопок с названиями "кнопка1" ... "кнопка5"
         */
        private function addTestButtons():void {
            for (var i:int = 1; i <= 5; i++) {
                this.push(1, "кнопка" + i);
            }
        }

        // ===== ИНИЦИАЛИЗАЦИЯ =====

        /**
         * @brief Инициализирует систему меню
         * 
         * Ищет scrollablePlane_mc на сцене, если не найден - ищет альтернативные имена,
         * если и они не найдены - создает fallback меню
         */
        private function initializeMenu():void {
            log("Main: Поиск scrollablePlane_mc на главной сцене...");
            log("Main: Всего объектов на сцене: " + this.numChildren);
            
            // Выводим список всех объектов на сцене для диагностики
            for (var i:int = 0; i < this.numChildren; i++) {
                var child:DisplayObject = this.getChildAt(i);
                log("CHILD [" + i + "]: " + child.name + " (" + child + ")");
            }
            
            var menuContainer:ScrollPane = findMenuContainer();
            if (menuContainer) {
                log("Main: ✅ Контейнер меню найден: " + menuContainer.name);
                initializeWithContainer(menuContainer);
            } else {
                log("Main: ⚠️ Контейнер меню не найден");
            }
        }

        /**
         * @brief Ищет scrollPane_mc на сцене и возвращает его как ScrollPane
         * @return ScrollPane или null если не найден
         */
        private function findMenuContainer():ScrollPane {
            var scrollPane:ScrollPane = this.getChildByName("scrollPane_mc") as ScrollPane;
            if (scrollPane) {
            log("Main: Найден scrollPane_mc на сцене");
                return scrollPane;
            }
            return null;
        }

        /**
         * @brief Инициализирует MenuManager с найденным ScrollPane и настраивает размеры
         * @param container ScrollPane меню для инициализации
         */
        private function initializeWithContainer(container:ScrollPane):void {
            log("Main: Исходный размер " + container.name + ": " + container.width + "x" + container.height);
            log("Main: Исходная позиция " + container.name + ": (" + container.x + ", " + container.y + ")");
            log("Main: Количество детей в " + container.name + ": " + container.numChildren);

            // Настраиваем ScrollPane согласно требованиям:
            // - Ширина: 30% экрана минус 10px
            // - Высота: весь экран минус 20px (10px сверху и снизу)
            // - Позиция: справа с отступом 10px
            var screenWidth:Number = stage ? stage.stageWidth : 1280;
            var screenHeight:Number = stage ? stage.stageHeight : 720;
            
            var menuWidth:Number = (screenWidth * 0.3) - 10; // 30% ширины экрана минус 10px
            var menuHeight:Number = screenHeight - 20; // Высота экрана минус отступы 10px сверху и снизу
            
            container.setSize(menuWidth, menuHeight);
            container.x = screenWidth - menuWidth - 10; // 10px от правого края
            container.y = 10; // 10px от верхнего края
            
            // Отключаем полосы прокрутки согласно требованиям
            container.horizontalScrollPolicy = "off";
            container.verticalScrollPolicy = "off";

            log("Main: ScrollPane настроен: " + menuWidth + "x" + menuHeight);
            log("Main: Позиция ScrollPane: (" + container.x + ", " + container.y + ")");
            log("Main: Полосы прокрутки отключены");

            menuManager = new MenuManager(container);
            setChildIndex(container, numChildren - 1); // container:ScrollPane, это DisplayObject
            log("Main: MenuManager инициализирован с " + container.name);
        }

        /**
         * @brief Регистрирует колбэки для взаимодействия с C++
         */
        private function registerExternalCallbacks():void {
            if (!ExternalInterface.available) {
                log("Main: ExternalInterface недоступен");
                return;
            }

            try {
                // Основные методы управления меню
                ExternalInterface.addCallback("show", show);
                ExternalInterface.addCallback("hide", hide);
                ExternalInterface.addCallback("clear", clear);
                ExternalInterface.addCallback("destroy", destroy);
                ExternalInterface.addCallback("deleteComponent", deleteComponent);
                ExternalInterface.addCallback("setActiveIndex", setActiveIndex);
                
                // Методы масштабирования
                ExternalInterface.addCallback("updateMenuScale", updateMenuScale);
                ExternalInterface.addCallback("setMenuMargins", setMenuMargins);
                ExternalInterface.addCallback("getMenuMargins", getMenuMargins);
                ExternalInterface.addCallback("getMenuScaleInfo", getMenuScaleInfo);
                
                // Методы управления подсветкой
                ExternalInterface.addCallback("setHoverHighlight", setHoverHighlight);
                ExternalInterface.addCallback("clearHoverHighlight", clearHoverHighlight);
                
                // Методы настройки цветов
                ExternalInterface.addCallback("setComponentColors", setComponentColors);
                ExternalInterface.addCallback("setComponentColorsRGBA", setComponentColorsRGBA);
                ExternalInterface.addCallback("setScrollableMenuColors", setScrollableMenuColors);
                ExternalInterface.addCallback("setScrollableMenuColorsRGBA", setScrollableMenuColorsRGBA);
                ExternalInterface.addCallback("resetColorsToDefault", resetColorsToDefault);
                ExternalInterface.addCallback("getComponentColors", getComponentColors);
                ExternalInterface.addCallback("getScrollableMenuColors", getScrollableMenuColors);
                
                // Уведомляем C++ о готовности
                ExternalInterface.call("onFlashReady");
                log("Main: C++ уведомлен о готовности Flash");
            } catch (error:Error) {
                log("Main: Ошибка связи с C++: " + error.message);
            }
        }



        // ===== ОСНОВНЫЕ API МЕТОДЫ ДЛЯ C++ =====

        /**
         * @brief Показывает меню (основной метод)
         */
        public function show():void {
            log("Main: Показ меню");
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }
            
            menuManager.showMenu("", [], 0);
            menuManager.updateScale();
            ensureMenuOnTop();
            
            // Запускаем отложенное масштабирование для гарантированного правильного размера
            scheduleInitialScale();
        }

        /**
         * @brief Принудительная инициализация компонентов (вызывается из C++)
         */
        public function initializeComponents():void {
            log("Main: initializeComponents() вызван");
            
            try {
                if (!menuManager) {
                    log("Main: ОШИБКА - MenuManager не инициализирован в initializeComponents");
                    return;
                }
                
                // Вызываем принудительную инициализацию в MenuManager
                if (menuManager) {
                    try {
                        menuManager.initializeComponents();
                        log("Main: MenuManager.initializeComponents() вызван");
                    } catch (error:Error) {
                        log("Main: Ошибка при вызове MenuManager.initializeComponents(): " + error.message);
                    }
                } else {
                    log("Main: MenuManager не найден");
                }
                
                // Дополнительно принуждаем обновление масштабирования
                menuManager.updateScale();
                log("Main: updateScale() выполнен в initializeComponents");
                
            } catch (error:Error) {
                log("Main: ОШИБКА в initializeComponents: " + error.message);
            }
        }

        /**
         * @brief Скрывает меню
         */
        public function hide():void {
            log("Main: Скрытие меню");
            if (menuManager) {
                menuManager.hideMenu();
            }
        }

        /**
         * @brief Добавляет один элемент в конец меню
         * @param type Тип элемента (0=LABEL, 1=BUTTON, 2=CHECKBOX, 3=SWITCHER)
         * @param labelText Текст элемента или список опций через запятую для SWITCHER
         * @param state Состояние элемента (опционально): для CHECKBOX (0/1), для SWITCHER (индекс)
         */
        public function push(type:int, labelText:String, state:* = undefined):void {
            log("Main: Добавление элемента типа " + type + " с лейблом '" + labelText + "'");
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }

            var item:Object = createMenuItem(type, labelText, state);
            if (item) {
                // Оптимизация: используем инкрементальное добавление вместо полного пересоздания
                menuManager.addItem(item);
                var currentCount:int = getCurrentItems().length + 1; // +1 для нового элемента
                log("Main: Элемент добавлен ИНКРЕМЕНТАЛЬНО, всего элементов: " + currentCount);
                
                // Отмечаем что контент готов и запускаем отложенное масштабирование
                markContentReady();
            }
        }

        /**
         * @brief Устанавливает активный индекс элемента меню (вызывается из C++)
         * @param newIndex Индекс элемента для установки как активный (0-based)
         */
        public function setActiveIndex(newIndex:int):void {
            log("Main: Установка активного индекса: " + newIndex);

            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }

            var currentIndex:int = menuManager.scrollableMenu ? menuManager.scrollableMenu.activeIndex : -1;
            if (currentIndex === newIndex) {
                log("Main: Активный индекс уже равен " + newIndex + ", событие не отправляется");
                return;
            }

            // Просто вызываем MenuManager — все проверки внутри него
            menuManager.setActiveIndex(newIndex);

            // Если удалось установить и это ButtonComponent — отправляем ItemHoverChanged
            var currentItems:Array = getCurrentItems();
            if (currentItems && newIndex >= 0 && newIndex < currentItems.length) {
                var targetItem:Object = currentItems[newIndex];
                if (targetItem && targetItem.type === 1) {
                    log("Main: Отправляем ItemHoverChanged для ButtonComponent '" + targetItem.labelText + "'");
                    sendHoverButtonCallback(targetItem.labelText);
                }
            }
        }

        /**
         * @brief Добавляет массив элементов в конец меню
         * @param items Массив элементов в формате [тип, лейбл, состояние?]
         */
        public function pushArray(items:Array):void {
            log("Main: Добавление " + items.length + " элементов в конец меню");
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }

            var convertedItems:Array = convertMenuItems(items);
            var currentItems:Array = getCurrentItems();
            
            for (var i:int = 0; i < convertedItems.length; i++) {
                currentItems.push(convertedItems[i]);
            }
            
            menuManager.updateItems(currentItems, currentItems.length - convertedItems.length);
            log("Main: Добавлено " + convertedItems.length + " элементов, всего: " + currentItems.length);
            
            // Отмечаем что контент готов и запускаем отложенное масштабирование
            markContentReady();
        }

        /**
         * @brief Вставляет элемент перед указанным индексом
         * @param index Индекс, перед которым вставить элемент
         * @param type Тип элемента (0=LABEL, 1=BUTTON, 2=CHECKBOX, 3=SWITCHER)
         * @param labelText Текст элемента или список опций через запятую для SWITCHER
         * @param state Состояние элемента (опционально): для CHECKBOX (0/1), для SWITCHER (индекс)
         */
        public function insert(index:int, type:int, labelText:String, state:* = undefined):void {
            log("Main: Вставка элемента типа " + type + " перед индексом " + index);
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }

            var item:Object = createMenuItem(type, labelText, state);
            if (item) {
                var currentItems:Array = getCurrentItems();
                
                if (index < 0) index = 0;
                if (index > currentItems.length) index = currentItems.length;
                
                currentItems.splice(index, 0, item);
                menuManager.updateItems(currentItems, index);
                log("Main: Элемент вставлен на позицию " + index + ", всего элементов: " + currentItems.length);
                
                // Отмечаем что контент готов и запускаем отложенное масштабирование
                markContentReady();
            }
        }

        /**
         * @brief Находит индекс элемента по текстовому лейблу
         * @param labelText Текст для поиска
         * @return Индекс найденного элемента или -1 если не найден
         */
        public function find(labelText:String):int {
            log("Main: Поиск элемента с лейблом '" + labelText + "'");
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return -1;
            }

            var currentItems:Array = getCurrentItems();
            
            for (var i:int = 0; i < currentItems.length; i++) {
                var item:Object = currentItems[i];
                if (item && item.labelText === labelText) {
                    log("Main: Элемент найден на позиции " + i);
                    return i;
                }
            }
            
            log("Main: Элемент не найден");
            return -1;
        }

        /**
         * @brief Очищает все элементы меню
         */
        public function clear():void {
            log("Main: Очистка меню");
            if (menuManager) {
                menuManager.updateItems([], 0);
            }
        }

        /**
         * @brief Полностью уничтожает меню и освобождает ресурсы
         */
        public function destroy():void {
            log("Main: Уничтожение меню");
            
            // Останавливаем отложенное масштабирование
            stopDelayedScaling();
            
            if (!menuManager) {
                log("Main: Меню уже уничтожено");
                return;
            }

            menuManager.hideMenu();
            clear();
            menuManager.destroy();
            menuManager = null;
            
            removeMenuFromScene();
            log("Main: Меню полностью уничтожено");
        }

        /**
         * @brief Удаляет компонент по названию
         * @param labelText Название компонента для удаления
         */
        public function deleteComponent(labelText:String):void {
            log("Main: Удаление компонента: '" + labelText + "'");
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }

            var currentItems:Array = menuManager.getCurrentItems();
            if (!currentItems || currentItems.length == 0) {
                log("Main: Нет элементов для удаления");
                return;
            }

            var filteredItems:Array = filterItemsByLabel(currentItems, labelText);
            var deletedCount:int = currentItems.length - filteredItems.length;

            if (deletedCount > 0) {
                menuManager.updateItems(filteredItems, 0);
                log("Main: Удалено " + deletedCount + " элемент(ов), осталось: " + filteredItems.length);
            } else {
                log("Main: Компонент '" + labelText + "' не найден");
            }
        }

        // ===== МЕТОДЫ МАСШТАБИРОВАНИЯ =====

        /**
         * @brief Принудительно обновляет масштабирование и позиционирование меню
         */
        public function updateMenuScale():void {
            log("Main: Обновление масштабирования");
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }
            
            // Обновляем внутреннее масштабирование компонентов через MenuManager
            // MenuScaler сам рассчитает правильные размеры и позицию
            menuManager.updateScale();
            
            log("Main: Масштабирование завершено через MenuManager");
        }

        /**
         * @brief Устанавливает отступы меню от краев экрана и обновляет размеры
         * @param right отступ справа в пикселях
         * @param top отступ сверху в пикселях  
         * @param bottom отступ снизу в пикселях
         */
        public function setMenuMargins(right:Number, top:Number, bottom:Number):void {
            log("Main: Установка отступов - право:" + right + ", верх:" + top + ", низ:" + bottom);
            
            if (!menuManager) {
                log("Main: ERROR - MenuManager не инициализирован");
                return;
            }
            
            // Передаем отступы в MenuManager, который передаст их в MenuScaler
            menuManager.setMenuMargins(right, top, bottom);
            
            log("Main: Отступы переданы в MenuManager");
        }

        /**
         * @brief Получает текущие отступы меню
         * @return Объект с отступами {right, top, bottom}
         */
        public function getMenuMargins():Object {
            return menuManager ? menuManager.getMenuMargins() : {right: 10, top: 10, bottom: 10};
        }

        /**
         * @brief Получает информацию о масштабировании
         * @return Объект с информацией о масштабе и разрешениях
         */
        public function getMenuScaleInfo():Object {
            if (!menuManager) return null;
            
            return {
                currentScale: menuManager.getCurrentScale(),
                baseResolution: menuManager.getBaseResolution(),
                currentResolution: {
                    width: stage.stageWidth,
                    height: stage.stageHeight
                }
            };
        }

        // ===== ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ =====

        /**
         * @brief Создает объект элемента меню из параметров
         * @param type Тип элемента
         * @param labelText Текст элемента
         * @param state Состояние элемента
         * @return Объект элемента или null если некорректный
         */
        private function createMenuItem(type:int, labelText:String, state:* = undefined):Object {
            switch (type) {
                case 0: // LABEL
                    return {type: 0, labelText: labelText};
                    
                case 1: // BUTTON
                    return {type: 1, labelText: labelText};
                    
                case 2: // CHECKBOX
                    var checked:Boolean = (state !== undefined) ? (state != 0) : false;
                    return {type: 2, labelText: labelText, checked: checked};
                    
                case 3: // SWITCHER
                    var selectedIndex:int = (state !== undefined) ? int(state) : 0;
                    return {type: 3, labelText: labelText, selectedIndex: selectedIndex};
                    
                default:
                    log("Main: Неизвестный тип элемента: " + type);
                    return null;
            }
        }

        /**
         * @brief Получает текущий массив элементов меню
         * @return Копия текущих элементов или пустой массив
         */
        private function getCurrentItems():Array {
            if (!menuManager) {
                return [];
            }
            
            // Получаем элементы из MenuManager
            var currentItems:Array = menuManager.getCurrentItems();
            return currentItems ? currentItems.slice() : []; // возвращаем копию
        }

        /**
         * @brief Убеждается что меню находится поверх всех элементов
         */
        private function ensureMenuOnTop():void {
            var scrollPane:* = this.getChildByName("scrollPane_mc");
            if (scrollPane && contains(scrollPane)) {
                setChildIndex(scrollPane, numChildren - 1);
            }
        }

        /**
         * @brief Удаляет контейнер меню со сцены
         */
        private function removeMenuFromScene():void {
            var scrollPane:* = this.getChildByName("scrollPane_mc");
            if (scrollPane && contains(scrollPane)) {
                removeChild(scrollPane);
                log("Main: scrollPane_mc удален со сцены");
            }
        }

        /**
         * @brief Преобразует массив элементов из C++ формата в AS3 формат
         * @param items Исходные элементы из C++
         * @return Преобразованные элементы для AS3
         */
        private function convertMenuItems(items:Array):Array {
            var convertedItems:Array = [];
            
            for (var i:int = 0; i < items.length; i++) {
                var item:Object = items[i];
                var convertedItem:Object = convertMenuArray(item);
                if (convertedItem) {
                    convertedItems.push(convertedItem);
                }
            }
            
            return convertedItems;
        }

        /**
         * @brief Фильтрует элементы меню, исключая элементы с указанным названием
         * @param items Исходные элементы
         * @param labelToRemove Название для удаления
         * @return Отфильтрованные элементы
         */
        private function filterItemsByLabel(items:Array, labelToRemove:String):Array {
            var filteredItems:Array = [];
            
            for (var i:int = 0; i < items.length; i++) {
                var item:Object = items[i];
                var itemLabel:String = item.labelText || "";
                
                if (itemLabel !== labelToRemove) {
                    filteredItems.push(item);
                }
            }
            
            return filteredItems;
        }

        /**
         * @brief Преобразует массив элемента [тип, лейбл, состояние?] в объект AS3
         * @param itemArray Массив в формате [тип, лейбл, состояние?]
         * @return Преобразованный объект для AS3 или null если некорректный
         */
        private function convertMenuArray(itemArray:*):Object {
            // Поддержка старого формата объектов (для обратной совместимости)
            if (itemArray && typeof(itemArray) === "object" && itemArray.hasOwnProperty("type")) {
                return convertMenuObject(itemArray);
            }
            
            // Новый формат - массив [тип, лейбл, состояние?]
            if (!itemArray || !(itemArray is Array) || itemArray.length < 2) {
                return null;
            }
            
            var type:int = int(itemArray[0]);
            var labelText:String = String(itemArray[1]);
            var state:* = (itemArray.length > 2) ? itemArray[2] : undefined;
            
            return createMenuItem(type, labelText, state);
        }

        /**
         * @brief Преобразует объект меню из старого C++ формата в AS3 формат (для обратной совместимости)
         * @param item Объект из C++
         * @return Преобразованный объект для AS3 или null если некорректный
         */
        private function convertMenuObject(item:Object):Object {
            if (!item || !item.hasOwnProperty("type") || !item.hasOwnProperty("labelText")) {
                return null;
            }
            
            var type:int = int(item.type);
            var labelText:String = String(item.labelText);
            var state:* = item.hasOwnProperty("state") ? item.state : undefined;
            
            return createMenuItem(type, labelText, state);
        }


        // ====== ИНТЕГРАЦИЯ С C++ КАК В NAFMenu ======
        
        /**
         * @brief Логирует сообщение через trace и дублирует в C++ через BGSCodeObj.Log
         * @param message Сообщение для логирования
         */
        private function log(message:String):void {
            trace(message);
            if (this.BGSCodeObj && typeof this.BGSCodeObj.Log === "function") {
                this.BGSCodeObj.Log(message);
            }
        }
        
        /**
         * @brief Вызывается C++ когда code object создан и готов к использованию
         * Аналог onCodeObjCreate() из NAFMenu
         */
        public function onCodeObjCreate():* {
            log("Main: onCodeObjCreate() вызван из C++");
            // Теперь BGSCodeObj содержит реальные C++ методы
            log("Main: BGSCodeObj готов к использованию");
            
            // Применяем тему Glass по умолчанию ПОСЛЕ инициализации C++
            if (menuManager) {
                log("Main: Применение темы Glass по умолчанию");
                
                // Цвета темы Glass - прозрачные синие тона
                menuManager.setScrollableMenuColorsRGBA(0x80404040, 0x60101010); // Серая граница и темный фон
                menuManager.setComponentColorsRGBA(0, 0x00000000, 0xFFE0E0E0, 0x40606060, 0x80808080); // Label - прозрачный фон, светло-серый текст
                menuManager.setComponentColorsRGBA(1, 0x804080A0, 0xFFFFFFFF, 0x906090C0, 0xA080A0D0); // Button - полупрозрачный синий
                menuManager.setComponentColorsRGBA(2, 0x603060A0, 0xFFE0E0E0, 0x804080B0, 0xA060A0E0); // Checkbox - синие тона
                menuManager.setComponentColorsRGBA(3, 0x70507090, 0xFFFFFFFF, 0x906080B0, 0xB080A0D0); // Switcher - сине-серые тона
                
                // Принудительно применяем цвета ScrollPane напрямую к ScrollableMenu
                if (menuManager.scrollableMenu) {
                    var scrollableMenuColors:Object = menuManager.getScrollableMenuColors();
                    if (scrollableMenuColors) {
                        menuManager.scrollableMenu.setScrollPaneColors(scrollableMenuColors.borderColor, scrollableMenuColors.backgroundColor);
                        log("Main: Цвета ScrollPane принудительно применены напрямую");
                    }
                }
                
                log("Main: Тема Glass применена");
            } else {
                log("Main: ERROR - MenuManager не инициализирован для применения темы Glass");
            }
        }
        
        /**
         * @brief Отправляет событие push (клик по кнопке) в C++ через ItemSelected
         * @param labelText Название нажатой кнопки
         */
        public function sendPushButtonCallback(labelText:String):void {
            log("Main: sendPushButtonCallback(" + labelText + ")");
            if (this.BGSCodeObj && typeof this.BGSCodeObj.ItemSelected === "function") {
                this.BGSCodeObj.ItemSelected(labelText);
                log("Main: BGSCodeObj.ItemSelected(" + labelText + ") вызван");
            } else {
                log("Main: BGSCodeObj.ItemSelected не доступен");
            }
        }

        /**
         * @brief Отправляет событие back (кнопка назад) в C++ через BackPressed
         * @param labelText Название кнопки
         */
        public function sendBackButtonCallback(labelText:String):void {
            log("Main: sendBackButtonCallback(" + labelText + ")");
            if (this.BGSCodeObj && typeof this.BGSCodeObj.BackPressed === "function") {
                this.BGSCodeObj.BackPressed();
                log("Main: BGSCodeObj.BackPressed() вызван");
            } else {
                log("Main: BGSCodeObj.BackPressed не доступен");
            }
        }

        /**
         * @brief Отправляет событие hover (наведение) в C++ через ItemHoverChanged
         * @param labelText Название кнопки
         */
        public function sendHoverButtonCallback(labelText:String):void {
            log("Main: sendHoverButtonCallback(" + labelText + ")");
            if (this.BGSCodeObj && typeof this.BGSCodeObj.ItemHoverChanged === "function") {
                this.BGSCodeObj.ItemHoverChanged(labelText);
                log("Main: BGSCodeObj.ItemHoverChanged(" + labelText + ") вызван");
            } else {
                log("Main: BGSCodeObj.ItemHoverChanged не доступен");
            }
        }

        /**
         * @brief Отправляет колбэк об изменении чекбокса в C++ через CheckboxChanged
         */
        public function sendChangeCheckboxCallback(labelText:String, checked:Boolean):void {
            log("Main: sendChangeCheckboxCallback(" + labelText + ", " + checked + ")");
            if (this.BGSCodeObj && typeof this.BGSCodeObj.CheckboxChanged === "function") {
                this.BGSCodeObj.CheckboxChanged(labelText, checked);
                log("Main: BGSCodeObj.CheckboxChanged(" + labelText + ", " + checked + ") вызван");
            } else {
                log("Main: BGSCodeObj.CheckboxChanged не доступен");
            }
        }

        /**
         * @brief Отправляет колбэк об изменении свитчера в C++ через SwitcherChanged
         */
        public function sendChangeSwitcherCallback(labelText:String, selectedIndex:int):void {
            log("Main: sendChangeSwitcherCallback(" + labelText + ", " + selectedIndex + ")");
            if (this.BGSCodeObj && typeof this.BGSCodeObj.SwitcherChanged === "function") {
                this.BGSCodeObj.SwitcherChanged(labelText, selectedIndex);
                log("Main: BGSCodeObj.SwitcherChanged(" + labelText + ", " + selectedIndex + ") вызван");
            } else {
                log("Main: BGSCodeObj.SwitcherChanged не доступен");
            }
        }

        // ===== СИСТЕМА ОТЛОЖЕННОГО МАСШТАБИРОВАНИЯ =====

        /**
         * @brief Настраивает систему отложенного масштабирования
         */
        private function setupDelayedScaling():void {
            log("Main: Настройка системы отложенного масштабирования");
            
            // Добавляем обработчик событий ENTER_FRAME для отслеживания готовности
            addEventListener(Event.ENTER_FRAME, onEnterFrame);
        }

        /**
         * @brief Обработчик событий ENTER_FRAME - проверяет готовность к масштабированию
         */
        private function onEnterFrame(event:Event):void {
            // Проверяем готовность stage и menuManager
            if (stage && menuManager && stage.stageWidth > 0 && stage.stageHeight > 0) {
                // Если контент готов, запускаем отложенное масштабирование
                if (isContentReady) {
                    scheduleContentReadyScale();
                }
            }
        }

        /**
         * @brief Запускает отложенное масштабирование при первом открытии меню
         */
        private function scheduleInitialScale():void {
            log("Main: Планирование начального масштабирования");
            
            // Отменяем предыдущий таймер если есть
            if (initialScaleTimer) {
                initialScaleTimer.stop();
                initialScaleTimer = null;
            }
            
            // Запускаем отложенное масштабирование через 50ms
            initialScaleTimer = new Timer(50, 1);
            initialScaleTimer.addEventListener(TimerEvent.TIMER, onInitialScaleTimer);
            initialScaleTimer.start();
        }

        /**
         * @brief Обработчик начального отложенного масштабирования
         */
        private function onInitialScaleTimer(event:TimerEvent):void {
            log("Main: Выполняется начальное отложенное масштабирование");
            
            // Принудительно обновляем масштаб через MenuManager
            if (menuManager) {
                menuManager.forceUpdateScale();
            }
            
            // Дополнительно обновляем через прямой вызов
            updateMenuScale();
        }

        /**
         * @brief Отмечает что контент готов и запускает отложенное масштабирование
         */
        private function markContentReady():void {
            if (!isContentReady) {
                log("Main: Контент помечен как готовый");
                isContentReady = true;
                
                // Немедленно запускаем масштабирование
                scheduleContentReadyScale();
            }
        }

        /**
         * @brief Запускает отложенное масштабирование после готовности контента
         */
        private function scheduleContentReadyScale():void {
            // Отменяем предыдущий таймер если есть
            if (contentReadyTimer) {
                contentReadyTimer.stop();
                contentReadyTimer = null;
            }
            
            // Запускаем отложенное масштабирование через 100ms
            contentReadyTimer = new Timer(100, 1);
            contentReadyTimer.addEventListener(TimerEvent.TIMER, onContentReadyScaleTimer);
            contentReadyTimer.start();
        }

        /**
         * @brief Обработчик отложенного масштабирования после готовности контента
         */
        private function onContentReadyScaleTimer(event:TimerEvent):void {
            log("Main: Выполняется отложенное масштабирование после готовности контента");
            
            // Принудительно обновляем масштаб через MenuManager
            if (menuManager) {
                menuManager.forceUpdateScale();
            }
            
            // Дополнительно обновляем через прямой вызов
            updateMenuScale();
        }

        /**
         * @brief Останавливает всю систему отложенного масштабирования
         */
        private function stopDelayedScaling():void {
            log("Main: Остановка отложенного масштабирования");
            
            // Останавливаем все таймеры
            if (initialScaleTimer) {
                initialScaleTimer.stop();
                initialScaleTimer.removeEventListener(TimerEvent.TIMER, onInitialScaleTimer);
                initialScaleTimer = null;
            }
            
            if (contentReadyTimer) {
                contentReadyTimer.stop();
                contentReadyTimer.removeEventListener(TimerEvent.TIMER, onContentReadyScaleTimer);
                contentReadyTimer = null;
            }
            
            // Убираем обработчик ENTER_FRAME
            removeEventListener(Event.ENTER_FRAME, onEnterFrame);
            
            // Сбрасываем состояние
            isContentReady = false;
            scaleRetryCount = 0;
            
            log("Main: Система отложенного масштабирования остановлена");
        }

        /**
         * @brief Устанавливает подсветку (жёлтую) для кнопки по индексу
         * @param index Индекс кнопки
         */
        public function setHoverHighlight(index:int):void {
            if (menuManager && menuManager.scrollableMenu) {
                menuManager.scrollableMenu.setHoverHighlight(index);
            }
        }

        /**
         * @brief Сбрасывает подсветку (жёлтую) для всех кнопок
         */
        public function clearHoverHighlight():void {
            if (menuManager && menuManager.scrollableMenu) {
                menuManager.scrollableMenu.clearAllHoverHighlights();
            }
        }
    }
}