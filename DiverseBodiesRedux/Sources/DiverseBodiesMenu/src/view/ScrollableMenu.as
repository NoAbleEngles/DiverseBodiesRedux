package view {
    import flash.display.MovieClip;
    import flash.display.Sprite;
    import flash.events.EventDispatcher;
    import flash.events.KeyboardEvent;
    import flash.events.Event;
    import flash.events.MouseEvent;
    import flash.utils.Timer;
    import flash.events.TimerEvent;
    import flash.geom.Rectangle;
    import fl.containers.ScrollPane;
    import flash.display.Graphics;
    import flash.display.DisplayObject;
    import view.components.*;
    import view.CustomEvent;
    import view.Logger;

    /**
     * @brief Прокручиваемое меню с навигацией и управлением компонентами
     * 
     * Управляет отображением и взаимодействием с элементами меню внутри ScrollPane:
     * - Создание и позиционирование различных типов компонентов (кнопки, чекбоксы, свитчеры, лейблы)
     * - Навигация с клавиатуры (стрелки вверх/вниз для перемещения, влево/вправо для изменения значений)
     * - Автоматическая прокрутка к активному элементу с плавной анимацией
     * - Обработка событий взаимодействия и отправка колбэков в C++
     * - Адаптивное масштабирование элементов с учетом размеров ScrollPane
     * - Поддержка отступов между элементами (3px по умолчанию)
     * 
     * Поддерживаемые типы компонентов:
     * - LABEL (0): текстовые лейблы и разделители
     * - BUTTON (1): интерактивные кнопки с hover эффектами  
     * - CHECKBOX (2): переключаемые чекбоксы
     * - SWITCHER (3): переключатели с множественным выбором
     * 
     * Навигация:
     * - ↑/↓: перемещение между интерактивными элементами
     * - ←/→: изменение значений чекбоксов и свитчеров
     * - Enter: активация кнопок
     * - Escape: выход (генерирует событие "назад")
     */
    public class ScrollableMenu extends EventDispatcher {
        // (Удалено mainInstance - больше не используется)
        
        private var scrollPane:ScrollPane;        // fl.containers.ScrollPane
        private var contentContainer:MovieClip;   // Контейнер для элементов меню
        
        private var menuItems:Array = [];         // Данные элементов
        private var visualElements:Array = [];    // Визуальные компоненты
        private var _activeIndex:int = -1;
        
        // Ссылки на менеджеры с паттерном Observer
        private var themeManager:ThemeManager;
        private var menuScaler:MenuScaler;
        private var customUpSkin:ScrollPane_upSkin;
        private var backgroundLayer:MovieClip; // Кастомная подложка под ScrollPane

        /**
         * @brief Логирует сообщение через централизованный Logger
         * @param message Сообщение для логирования
         */
        private function log(message:String):void {
            Logger.getInstance().debug(message, "ScrollableMenu");
        }

        public function ScrollableMenu(scrollPaneComponent:ScrollPane) {
            this.scrollPane = scrollPaneComponent;
            
            if (!scrollPane) {
                log("КРИТИЧЕСКАЯ ОШИБКА - scrollPaneComponent null!");
                return;
            }
            
            // ПРИНУДИТЕЛЬНО ДЕЛАЕМ SCROLLPANE ВИДИМЫМ
            scrollPane.visible = true;
            scrollPane.alpha = 1.0;
            
            // Получаем ссылки на синглтоны менеджеров
            themeManager = ThemeManager.getInstance();
            menuScaler = MenuScaler.getInstance();

            initializeScrollPane();
            setupKeyboardHandling();
        }

        /**
         * Инициализирует ScrollPane и создает контейнер для содержимого
         */
        private function initializeScrollPane():void {
            // Создаем контейнер для элементов меню
            contentContainer = new MovieClip();
            contentContainer.name = "menuContentContainer";
            
            // Принудительно делаем контейнер видимым
            contentContainer.visible = true;
            contentContainer.alpha = 1.0;
            contentContainer.mouseEnabled = true;
            contentContainer.mouseChildren = true;
            
            // Настраиваем ScrollPane
            scrollPane.horizontalScrollPolicy = "off";
            scrollPane.verticalScrollPolicy = "off";
            scrollPane.focusEnabled = true;
            scrollPane.tabEnabled = true;
            
            // Очищаем встроенные стили ScrollPane
            clearScrollPaneStyles();
            
            // Устанавливаем контейнер как source для ScrollPane
            try {
                scrollPane.source = contentContainer;
                scrollPane.invalidate();
                scrollPane.validateNow();
                
                // Получаем цвета из ThemeManager и применяем
                var colors:Object = themeManager.getScrollableMenuColors();
                setScrollPaneColors(colors.borderColor, colors.backgroundColor);
                
            } catch (setupError:Error) {
                log("ScrollableMenu: Ошибка при настройке ScrollPane: " + setupError.message);
            }
        }

        /**
         * Очищает встроенные стили ScrollPane для предотвращения лишних цветов
         */
        private function clearScrollPaneStyles():void {
            try {
                // Попытка 1: через setStyle (осторожная очистка без нарушения структуры)
                if (scrollPane.hasOwnProperty("setStyle")) {
                    // НЕ трогаем backgroundColor и borderColor - они будут установлены через ThemeManager
                    // НЕ трогаем skin элементы - они нужны для корректной работы ScrollPane
                    
                    // Очищаем только безопасные стили
                    scrollPane.setStyle("focusRectPadding", 0);
                    scrollPane.setStyle("contentPadding", 0);
                }
                
                // НЕ очищаем graphics - это может нарушить структуру ScrollPane
            } catch (error:Error) {
                log("ScrollableMenu: Ошибка при очистке стилей: " + error.message);
            } catch (e:Error) {
                log("ScrollableMenu: Ошибка при очистке стилей ScrollPane: " + e.message);
            }
        }

        /**
         * Настраивает обработку клавиатуры и мыши
         */
        private function setupKeyboardHandling():void {
            if (scrollPane.stage) {
                scrollPane.stage.addEventListener(KeyboardEvent.KEY_DOWN, onKeyDown);
                scrollPane.stage.focus = scrollPane; // Устанавливаем фокус на ScrollPane
                setupMouseScrolling();
            } else {
                scrollPane.addEventListener(Event.ADDED_TO_STAGE, function(e:Event):void {
                    // Убираем предыдущий обработчик, если он был добавлен
                    scrollPane.stage.removeEventListener(KeyboardEvent.KEY_DOWN, onKeyDown);
                    // Добавляем обработчик заново
                    scrollPane.stage.addEventListener(KeyboardEvent.KEY_DOWN, onKeyDown);
                    scrollPane.stage.focus = scrollPane;
                    setupMouseScrolling();
                });
            }
        }

        /**
         * Настраивает прокрутку колесом мыши
         */
        private function setupMouseScrolling():void {
            import flash.events.MouseEvent;
            
            if (scrollPane && scrollPane.stage) {
                // Добавляем слушатели на разные уровни для надежности
                scrollPane.addEventListener(MouseEvent.MOUSE_WHEEL, onMouseWheel, false, 0, true);
                scrollPane.stage.addEventListener(MouseEvent.MOUSE_WHEEL, onMouseWheel, false, 0, true);
                contentContainer.addEventListener(MouseEvent.MOUSE_WHEEL, onMouseWheel, false, 0, true);
                
                // Включаем возможность получения фокуса для прокрутки
                scrollPane.focusEnabled = true;
                scrollPane.tabEnabled = true;
                scrollPane.mouseEnabled = true;
                scrollPane.mouseChildren = true;
            }
        }

        /**
         * Обработчик прокрутки колесом мыши
         */
        private function onMouseWheel(event:MouseEvent):void {
            // Предотвращаем всплытие события
            event.stopPropagation();
            event.stopImmediatePropagation();
            
            var scrollAmount:Number = event.delta * 30; // Увеличиваем шаг прокрутки для лучшей отзывчивости
            var currentScrollY:Number = scrollPane.verticalScrollPosition;
            var newScrollY:Number = currentScrollY - scrollAmount;
            
            // Ограничиваем прокрутку границами содержимого
            var maxScroll:Number = Math.max(0, contentContainer.height - scrollPane.height);
            newScrollY = Math.max(0, Math.min(newScrollY, maxScroll));
            
            scrollPane.verticalScrollPosition = newScrollY;
            scrollPane.update(); // Принудительно обновляем отображение
            scrollPane.invalidate(); // Дополнительная перерисовка
        }



        /**
         * Очищает все визуальные элементы
         */
        private function clearVisualElements():void {
            // Сначала отключаем source ScrollPane чтобы избежать конфликтов
            if (scrollPane && scrollPane.source === contentContainer) {
                scrollPane.source = null;
                scrollPane.invalidate();
                scrollPane.validateNow();
            }
            
            // Удаляем все элементы из массива
            for each (var element:* in visualElements) {
                if (element && element.parent) {
                    element.parent.removeChild(element);
                }
            }
            visualElements = [];
            
            // Очищаем старый контейнер если он существует
            if (contentContainer) {
                // Принудительно очищаем контейнер
                while (contentContainer.numChildren > 0) {
                    contentContainer.removeChildAt(0);
                }
                contentContainer.graphics.clear();
            } else {
                // Создаём новый контейнер если его нет
                contentContainer = new MovieClip();
                contentContainer.name = "menuContentContainer";
                contentContainer.visible = true;
                contentContainer.alpha = 1.0;
                contentContainer.mouseEnabled = true;
                contentContainer.mouseChildren = true;
            }
            
            // Сбрасываем размеры контейнера
            if (contentContainer.graphics) {
                contentContainer.graphics.clear();
                contentContainer.graphics.beginFill(0x000000, 0);
                contentContainer.graphics.drawRect(0, 0, 1, 1);
                contentContainer.graphics.endFill();
            }
        }

        /**
         * Создает визуальные элементы по данным
         */
        private function createVisualElements():void {
            var scaledVerticalMargin:Number = menuScaler.getScaledVerticalMargin();
            var scaledItemHeight:Number = menuScaler.getScaledItemHeight();
            var scaledItemSpacing:Number = menuScaler.getScaledItemSpacing();
            
            var yPos:Number = scaledVerticalMargin;
            
            for (var i:int = 0; i < menuItems.length; i++) {
                var itemData:Object = menuItems[i];
                var element:* = createElementByType(itemData, i);
                
                if (element) {
                    element.x = 5;
                    element.y = yPos;
                    
                    // Устанавливаем ширину элемента
                    var availableWidth:Number = scrollPane.width - 10;
                    if (element.hasOwnProperty('width')) {
                        element.width = availableWidth;
                    }
                    
                    // Проверка размеров
                    if (element.width <= 0 || element.height <= 0) {
                        if (element.width <= 0) element.width = availableWidth;
                        if (element.height <= 0) element.height = 30;
                    }
                    
                    contentContainer.addChild(element);
                    
                    // Принудительно делаем элемент видимым
                    element.visible = true;
                    element.alpha = 1.0;
                    
                    // Применяем цвета темы
                    applyThemeColorsToElement(element, itemData.type);
                    
                    visualElements.push(element);
                    yPos += scaledItemHeight + scaledItemSpacing;
                }
            }
            
            // Обновляем размер контейнера
            updateContentSize();
            
            // Применяем цвета темы ко всем элементам
            applyThemeColorsToAllElements();
        }

        /**
         * @brief Применяет цвета темы ко всем элементам
         */
        private function applyThemeColorsToAllElements():void {
            if (!themeManager) {
                log("ScrollableMenu: ThemeManager недоступен, пропускаем применение цветов");
                return;
            }
            
            try {
                // СНАЧАЛА применяем цвета к самому ScrollPane (рамка и фон)
                var scrollableMenuColors:Object = themeManager.getScrollableMenuColors();
                if (scrollableMenuColors && scrollPane) {
                    setScrollPaneColors(scrollableMenuColors.borderColor, scrollableMenuColors.backgroundColor);
                }
                
                // ЗАТЕМ применяем цвета к элементам меню
                var appliedCount:int = 0;
                var minLength:int = Math.min(visualElements.length, menuItems.length);
                for (var i:int = 0; i < minLength; i++) {
                    var element:MovieClip = visualElements[i] as MovieClip;
                    var itemData:Object = menuItems[i];
                    if (element && itemData) {
                        applyThemeColorsToElement(element, itemData.type);
                        appliedCount++;
                    }
                }
            } catch (error:Error) {
                log("ScrollableMenu: ERROR при применении цветов ко всем элементам: " + error.message);
            }
        }

        /**
         * Создает элемент по типу
         */
        private function createElementByType(data:Object, index:int):* {
            var type:int = data.type || 0;
            var text:String = data.labelText || data.text || "";
            var element:*;
            
            try {
                switch (type) {
                    case 0: // Label
                        element = new LabelComponent(text);
                        break;
                        
                    case 1: // Button
                        element = new ButtonComponent(text);
                        element.addEventListener(ButtonComponent.EVENT_PUSH, createElementHandler(index, data));
                        element.addEventListener(ButtonComponent.EVENT_HOVER, createElementHandler(index, data));
                        // Добавляем обработчик клика для установки активного индекса
                        element.addEventListener(MouseEvent.CLICK, createMouseClickHandler(index, data));
                        break;
                        
                    case 2: // Checkbox
                        element = new CheckboxComponent(text, data.checked || false);
                        element.addEventListener("eventCheckboxComponentChange", createElementHandler(index, data));
                        // Добавляем обработчик клика для установки активного индекса
                        element.addEventListener(MouseEvent.CLICK, createMouseClickHandler(index, data));
                        break;
                        
                    case 3: // Switcher
                        var options:Array;
                        if (data.options && data.options.length > 0) {
                            // Используем готовый массив опций
                            options = data.options;
                        } else {
                            // Fallback: разбираем text как строку с разделителями
                            options = text.split(",");
                            // Очищаем пробелы в опциях
                            for (var j:int = 0; j < options.length; j++) {
                                options[j] = String(options[j]).replace(/^\s+|\s+$/g, "");
                            }
                        }
                        element = new SwitcherComponent(options, data.selectedIndex || 0);
                        element.addEventListener("eventSwitcherComponentChange", createSwitcherHandler(index, data));
                        element.addEventListener("eventSwitcherComponentPush", createElementHandler(index, data));
                        // Добавляем обработчик клика для установки активного индекса
                        element.addEventListener(MouseEvent.CLICK, createMouseClickHandler(index, data));
                        break;
                        
                    default:
                        log("WARNING: Неизвестный тип элемента: " + type);
                        element = new LabelComponent(text);
                        break;
                }
                
            } catch (error:Error) {
                log("ERROR: Ошибка создания элемента типа " + type + ": " + error.message);
                element = new LabelComponent("ERROR: " + text);
                // Цвета будут применены через ThemeManager
            }
            
            return element;
        }

        /**
         * @brief Создаёт кастомную подложку под ScrollPane с заданными цветами
         * @param borderColor Цвет границы (ARGB)
         * @param backgroundColor Цвет фона (ARGB)
         */
        private function createBackgroundLayer(borderColor:uint, backgroundColor:uint):void {
            // Извлекаем RGB компоненты и альфу из ARGB формата
            var borderRGB:uint = borderColor & 0xFFFFFF;
            var backgroundRGB:uint = backgroundColor & 0xFFFFFF;
            var borderAlpha:Number = ((borderColor >> 24) & 0xFF) / 255.0;
            var backgroundAlpha:Number = ((backgroundColor >> 24) & 0xFF) / 255.0;
            
            // Удаляем предыдущую подложку, если она есть
            if (backgroundLayer && backgroundLayer.parent) {
                backgroundLayer.parent.removeChild(backgroundLayer);
            }
            
            // Создаём новую подложку
            backgroundLayer = new MovieClip();
            backgroundLayer.name = "scrollPaneBackground";

            // Рисуем фон
            backgroundLayer.graphics.clear();
            if (backgroundAlpha > 0) {
                backgroundLayer.graphics.beginFill(backgroundRGB, backgroundAlpha);
                backgroundLayer.graphics.drawRect(0, 0, scrollPane.width, scrollPane.height);
                backgroundLayer.graphics.endFill();
            }
            
            // Рисуем границу
            if (borderAlpha > 0) {
                backgroundLayer.graphics.lineStyle(1, borderRGB, borderAlpha);
                backgroundLayer.graphics.drawRect(0, 0, scrollPane.width, scrollPane.height);
            }
            
            // Позиционируем подложку точно под ScrollPane
            backgroundLayer.x = scrollPane.x;
            backgroundLayer.y = scrollPane.y;
            
            // Добавляем подложку в тот же контейнер, что и scrollPane, но под него
            if (scrollPane.parent) {
                var scrollPaneIndex:int = scrollPane.parent.getChildIndex(scrollPane);
                scrollPane.parent.addChildAt(backgroundLayer, scrollPaneIndex);
            }
        }

        /**
         * @brief Применяет цвета к ScrollPane компоненту через кастомную подложку
         * @param borderColor Цвет границы (ARGB)  
         * @param backgroundColor Цвет фона (ARGB)
         */
        public function setScrollPaneColors(borderColor:uint, backgroundColor:uint):void {
            if (!scrollPane) {
                log("ScrollableMenu: ERROR - ScrollPane не инициализирован");
                return;
            }
            
            try {
                // Создаём кастомную подложку вместо попыток изменить ScrollPane напрямую
                createBackgroundLayer(borderColor, backgroundColor);
                
            } catch (error:Error) {
                log("ScrollableMenu: ERROR - Ошибка создания подложки для ScrollPane '" + scrollPane.name + "': " + error.message);
            }
        }       
        
        /**
         * @brief Уничтожает ScrollableMenu и очищает все ресурсы
         */
        public function destroy():void {
            log("ScrollableMenu: Начало уничтожения...");
            
            // Отписываемся от событий менеджеров
            if (themeManager) {
                themeManager.removeEventListener(ThemeManager.THEME_CHANGED, onThemeChanged);
                themeManager.removeEventListener(ThemeManager.COMPONENT_COLORS_CHANGED, onComponentColorsChanged);
                themeManager.removeEventListener(ThemeManager.SCROLLABLE_MENU_COLORS_CHANGED, onScrollableMenuColorsChanged);
            }
            
            if (menuScaler) {
                menuScaler.removeEventListener(MenuScaler.SCALE_CHANGED, onScaleChanged);
            }
            
            // Очищаем все элементы
            clearAllItems();
            
            // Убираем обработчики событий
            if (scrollPane && scrollPane.stage) {
                scrollPane.stage.removeEventListener(KeyboardEvent.KEY_DOWN, onKeyDown);
            }
            
            // Очищаем массивы
            menuItems = [];
            visualElements = [];
            
            // Очищаем ссылки
            scrollPane = null;
            contentContainer = null;
            themeManager = null;
            menuScaler = null;
            
            _activeIndex = -1;
            
            log("ScrollableMenu: Уничтожение завершено");
        }

        // ===== ФУНКЦИИ ОТПРАВКИ КОЛБЭКОВ В C++ =====
        
        /**
         * @brief Отправляет ItemSelected через MenuManager
         */
        /**
         * @brief Отправляет событие активации элемента в MenuManager
         */
        private function sendPushCallback(labelText:String):void {
            // Находим индекс и данные элемента по labelText
            for (var i:int = 0; i < menuItems.length; i++) {
                if (menuItems[i] && menuItems[i].labelText === labelText) {
                    log("ScrollableMenu: sendPushCallback(" + labelText + ") - найден элемент с индексом " + i);
                    // Отправляем событие в MenuManager с правильной структурой данных
                    dispatchEvent(new CustomEvent("menuItemSelected", {index: i, item: menuItems[i]}));
                    return;
                }
            }
            log("ScrollableMenu: ERROR - sendPushCallback(" + labelText + ") - элемент не найден");
        }
        
        /**
         * @brief Отправляет событие возврата в MenuManager
         */
        private function sendBackCallback():void {
            // Отправляем событие в MenuManager
            dispatchEvent(new CustomEvent("menuBack", {}));
        }
        
        /**
         * @brief Отправляет событие выбора элемента в MenuManager
         */
        private function sendHoverCallback(labelText:String):void {
            // Отправляем событие в MenuManager
            dispatchEvent(new CustomEvent("menuItemHover", {labelText: labelText}));
        }
        
        /**
         * @brief Отправляет событие изменения checkbox в MenuManager
         */
        private function sendCheckboxCallback(labelText:String, checked:Boolean):void {
            log("ScrollableMenu: sendCheckboxCallback(" + labelText + ", " + checked + ")");
            // Отправляем событие в MenuManager
            dispatchEvent(new CustomEvent("menuCheckboxChanged", {labelText: labelText, checked: checked}));
        }
        
        /**
         * @brief Отправляет событие изменения switcher в MenuManager
         */
        private function sendSwitcherCallback(labelText:String, selectedIndex:int):void {
            log("ScrollableMenu: sendSwitcherCallback(" + labelText + ", " + selectedIndex + ")");
            // Отправляем событие в MenuManager
            dispatchEvent(new CustomEvent("menuSwitcherChanged", {labelText: labelText, selectedIndex: selectedIndex}));
        }

        /**
         * Обработчик нажатия клавиш
         */
        private function onKeyDown(event:KeyboardEvent):void {
            // Обрабатываем только ESC и TAB независимо от наличия элементов в меню
            if (event.keyCode === 27 || event.keyCode === 9) { // ESC
                sendBackCallback();
                return;
            }
            
            if (!menuItems || menuItems.length === 0) return;
            
            var interactiveIndices:Array = [];
            for (var i:int = 0; i < menuItems.length; i++) {
                if (menuItems[i].type !== 0) { // Не лейбл
                    interactiveIndices.push(i);
                }
            }
            
            if (interactiveIndices.length === 0) return;
            
            var currentInteractiveIndex:int = interactiveIndices.indexOf(_activeIndex);
            var currentItem:Object = (_activeIndex >= 0 && _activeIndex < menuItems.length) ? menuItems[_activeIndex] : null;
            var currentElement:* = (_activeIndex >= 0 && _activeIndex < visualElements.length) ? visualElements[_activeIndex] : null;
            
            switch (event.keyCode) {
                case 13: // Enter - отправляет ItemSelected только для ButtonComponent
                    if (currentItem && currentItem.type === 1) { // ButtonComponent
                        sendPushCallback(currentItem.labelText);
                    }
                    break;
                    
                case 32: // Space - CheckboxChanged только для CheckboxComponent
                    if (currentItem && currentItem.type === 2 && currentElement) { // CheckboxComponent
                        currentElement.selected = !currentElement.selected
                        sendCheckboxCallback(currentItem.labelText, currentElement.selected);
                    }
                    break;
                    
                case 38: // Up arrow - навигация вверх
                    if (interactiveIndices.length === 0) {
                        break;
                    }
                    var upIndex:int = currentInteractiveIndex;
                    var foundUp:Boolean = false;
                    while (--upIndex >= 0) {
                        var upCandidate:int = interactiveIndices[upIndex];
                        if (menuItems[upCandidate].type !== 0) {
                            setActiveIndex(upCandidate);
                            var upItem:Object = menuItems[upCandidate];
                            if (upItem && upItem.type === 1) {
                                sendHoverCallback(upItem.labelText);
                            }
                            foundUp = true;
                            break;
                        }
                    }
                    // Fallback только если крайний элемент сверху — type === 0
                    if (!foundUp && currentInteractiveIndex === 0 && menuItems[interactiveIndices[0]].type === 0 && interactiveIndices.length > 1) {
                        var fallbackDownIndex:int = currentInteractiveIndex + 1;
                        while (fallbackDownIndex < interactiveIndices.length) {
                            var fallbackCandidate:int = interactiveIndices[fallbackDownIndex];
                            if (menuItems[fallbackCandidate].type !== 0) {
                                setActiveIndex(fallbackCandidate);
                                var fallbackItem:Object = menuItems[fallbackCandidate];
                                if (fallbackItem && fallbackItem.type === 1) {
                                    sendHoverCallback(fallbackItem.labelText);
                                }
                                break;
                            }
                            fallbackDownIndex++;
                        }
                    }
                    break;

                case 40: // Down arrow - навигация вниз
                    if (interactiveIndices.length === 0) {
                        break;
                    }
                    var downIndex:int = currentInteractiveIndex;
                    var foundDown:Boolean = false;
                    while (++downIndex < interactiveIndices.length) {
                        var downCandidate:int = interactiveIndices[downIndex];
                        if (menuItems[downCandidate].type !== 0) {
                            setActiveIndex(downCandidate);
                            var downItem:Object = menuItems[downCandidate];
                            if (downItem && downItem.type === 1) {
                                sendHoverCallback(downItem.labelText);
                            }
                            foundDown = true;
                            break;
                        }
                    }
                    // Fallback только если крайний элемент снизу — type === 0
                    if (!foundDown && currentInteractiveIndex === interactiveIndices.length - 1 && menuItems[interactiveIndices[interactiveIndices.length - 1]].type === 0 && interactiveIndices.length > 1) {
                        var fallbackUpIndex:int = currentInteractiveIndex - 1;
                        while (fallbackUpIndex >= 0) {
                            var fallbackCandidate2:int = interactiveIndices[fallbackUpIndex];
                            if (menuItems[fallbackCandidate2].type !== 0) {
                                setActiveIndex(fallbackCandidate2);
                                var fallbackItem2:Object = menuItems[fallbackCandidate2];
                                if (fallbackItem2 && fallbackItem2.type === 1) {
                                    sendHoverCallback(fallbackItem2.labelText);
                                }
                                break;
                            }
                            fallbackUpIndex--;
                        }
                    }
                    break;
                    
                case 39: // Right arrow
                    if (currentItem && currentElement) {
                        if (currentItem.type === 1) { // ButtonComponent - то же что Enter
                            sendPushCallback(currentItem.labelText);
                        } else if (currentItem.type === 3) { // SwitcherComponent - делегируем обработку компоненту
                            if (currentElement.hasOwnProperty("switchRight")) {
                                currentElement.switchRight(); // Компонент сам отправит нужные события
                            }
                        } else if (currentItem.type === 2) { // CheckboxComponent - устанавливает если не установлен
                            if (!currentElement.selected) {
                                currentElement.selected = true;
                                sendCheckboxCallback(currentItem.labelText, true);
                            }
                        }
                    }
                    break;
                    
                case 37: // Left arrow
                    if (currentItem && currentElement) {
                        if (currentItem.type === 1) { // ButtonComponent - то же что ESC/TAB
                            sendBackCallback();
                        } else if (currentItem.type === 3) { // SwitcherComponent - делегируем обработку компоненту
                            if (currentElement.hasOwnProperty("switchLeft")) {
                                currentElement.switchLeft(); // Компонент сам отправит нужные события
                            }
                        } else if (currentItem.type === 2) { // CheckboxComponent - снимает если установлен
                            if (currentElement.selected) {
                                currentElement.selected = false;
                                sendCheckboxCallback(currentItem.labelText, false);
                            }
                        }
                    }
                    break;
            }
        }

        /**
         * Создает обработчик событий для элемента
         */
        private function createElementHandler(index:int, data:Object):Function {
            return function(event:Event):void {
                // Обработка событий ButtonComponent
                if (data.type === 1) { // ButtonComponent
                    if (event.type === ButtonComponent.EVENT_PUSH) {
                        // Двойной клик или Enter - активация
                        sendPushCallback(data.labelText);
                        return;
                    } else if (event.type === ButtonComponent.EVENT_HOVER) {
                        // Одиночный клик - выбор
                        sendHoverCallback(data.labelText);
                        return;
                    }
                }
                
                // Для CheckboxComponent CHANGE события отправляем CheckboxChanged
                if (data.type === 2 && event.type === "eventCheckboxComponentChange") { // CheckboxComponent
                    var checkboxElement:* = visualElements[index];
                    if (checkboxElement && checkboxElement.hasOwnProperty("selected")) {
                        sendCheckboxCallback(data.labelText, checkboxElement.selected);
                    }
                    return; // Не отправляем ItemSelected для checkbox change событий
                }
                
                // Для остальных событий диспетчируем событие в MenuManager
                dispatchEvent(new CustomEvent("menuItemSelected", {index: index, item: data}));
            };
        }

        /**
         * Создает обработчик кликов мыши для установки активного индекса
         */
        private function createMouseClickHandler(index:int, data:Object):Function {
            return function(event:MouseEvent):void {
                // Устанавливаем активный индекс при клике на интерактивный элемент
                if (data.type !== 0) { // Не LabelComponent
                    setActiveIndex(index);
                }
            };
        }

        /**
         * Создает обработчик back событий от компонентов
         */
        private function createBackHandler(index:int, data:Object):Function {
            return function(event:Event):void {
                // Отправляем событие в MenuManager
                sendBackCallback();
            };
        }

        /**
         * Создает обработчик событий CHANGE для SwitcherComponent
         */
        private function createSwitcherHandler(index:int, data:Object):Function {
            return function(event:Event):void {
                // Получаем текущий индекс из компонента
                var element:* = visualElements[index];
                if (element && element.hasOwnProperty("index")) {
                    var currentIndex:int = element.index;
                    // Отправляем в C++
                    sendSwitcherCallback(data.labelText, currentIndex);
                }
            };
        }

        /**
         * Получает текущий активный индекс элемента
         */
        public function get activeIndex():int {
            return this._activeIndex;
        }

        /**
         * Обновляет размер контейнера содержимого
         */
        private function updateContentSize():void {
            if (!contentContainer || !menuScaler) return;
            
            var scaledVerticalMargin:Number = menuScaler.getScaledVerticalMargin();
            var scaledItemHeight:Number = menuScaler.getScaledItemHeight();
            var scaledItemSpacing:Number = menuScaler.getScaledItemSpacing();
            var scaledContainerWidth:Number = menuScaler.getScaledContainerWidth();
            
            var totalHeight:Number = scaledVerticalMargin * 2; // Верхний и нижний отступы
            if (visualElements.length > 0) {
                totalHeight += (visualElements.length * scaledItemHeight) + 
                              ((visualElements.length - 1) * scaledItemSpacing);
            }
            
            // Устанавливаем размер контейнера
            var graphics:* = contentContainer.graphics;
            if (graphics) {
                graphics.clear();
                graphics.beginFill(0x000000, 0); // Прозрачная заливка для определения размера
                graphics.drawRect(0, 0, scaledContainerWidth, totalHeight);
                graphics.endFill();
            }
            
            // Принудительно обновляем ScrollPane после изменения размеров
            if (scrollPane) {
                scrollPane.invalidate();
                scrollPane.validateNow();
                scrollPane.update();
            }
        }

        /**
         * Устанавливает активный индекс
         */
        public function setActiveIndex(newIndex:int):void {
            if (newIndex < 0 || newIndex >= visualElements.length) {
                return;
            }
            
            // Убираем выделение с предыдущего элемента
            if (_activeIndex >= 0 && _activeIndex < visualElements.length) {
                var prevElement:* = visualElements[_activeIndex];
                if (prevElement && prevElement.hasOwnProperty("active")) {
                    prevElement.active = false;
                }
            }
            
            _activeIndex = newIndex;
            
            // Устанавливаем выделение на новый элемент
            if (_activeIndex >= 0 && _activeIndex < visualElements.length) {
                var newElement:* = visualElements[_activeIndex];
                if (newElement && newElement.hasOwnProperty("active")) {
                    newElement.active = true;
                }
                scrollToElement(_activeIndex);
            }
        }

        /**
         * Прокручивает к элементу
         */
        private function scrollToElement(index:int):void {
            if (!scrollPane || index < 0 || index >= visualElements.length) return;
            
            var element:* = visualElements[index];
            if (!element) return;
            
            var elementY:Number = element.y;
            var elementHeight:Number = element.height;
            var scrollY:Number = scrollPane.verticalScrollPosition;
            var viewHeight:Number = scrollPane.height;
            
            // Проверяем, нужна ли прокрутка
            if (elementY < scrollY) {
                // Элемент выше видимой области
                scrollPane.verticalScrollPosition = elementY;
            } else if (elementY + elementHeight > scrollY + viewHeight) {
                // Элемент ниже видимой области
                scrollPane.verticalScrollPosition = elementY + elementHeight - viewHeight;
            }
        }

        /**
         * Обеспечивает, что contentContainer находится в ScrollPane
         */
        private function ensureContentContainerInScrollPane():void {
            if (!contentContainer || !scrollPane) return;
            
            // Проверяем, установлен ли contentContainer как source для ScrollPane
            if (scrollPane.source !== contentContainer) {
                try {
                    scrollPane.source = contentContainer;
                    scrollPane.invalidate();
                    scrollPane.validateNow();
                    log("ScrollableMenu: contentContainer переустановлен как source для ScrollPane");
                } catch (error:Error) {
                    log("ScrollableMenu: Ошибка при установке contentContainer как source: " + error.message);
                }
            }
        }

        /**
         * @brief Применяет цвета темы к конкретному элементу
         * @param element Визуальный элемент
         * @param componentType Тип компонента (0=Label, 1=Button, 2=Checkbox, 3=Switcher)
         */
        private function applyThemeColorsToElement(element:*, componentType:int):void {
            if (!element || !themeManager) return;
            
            try {
                var colors:Object = themeManager.getComponentColors(componentType);
                if (colors && element.hasOwnProperty("setColorsRGBA")) {
                    element.setColorsRGBA(colors.normal, colors.text, colors.hover, colors.selected);
                }
            } catch (error:Error) {
                log("ScrollableMenu: Ошибка применения цветов к элементу типа " + componentType + ": " + error.message);
            }
        }

        // ===== ОБРАБОТЧИКИ СОБЫТИЙ МЕНЕДЖЕРОВ =====

        /**
         * @brief Обработчик изменения темы
         */
        private function onThemeChanged(event:Event):void {
            applyThemeColorsToAllElements();
        }

        /**
         * @brief Обработчик изменения цветов компонентов
         */
        private function onComponentColorsChanged(event:*):void {
            if (event && event.hasOwnProperty("componentType")) {
                var componentType:int = event.componentType;
                
                // Применяем цвета только к элементам указанного типа
                var minLength:int = Math.min(visualElements.length, menuItems.length);
                for (var i:int = 0; i < minLength; i++) {
                    if (menuItems[i] && menuItems[i].type === componentType && visualElements[i]) {
                        applyThemeColorsToElement(visualElements[i], componentType);
                    }
                }
            } else {
                applyThemeColorsToAllElements();
            }
        }

        /**
         * @brief Обработчик изменения цветов ScrollableMenu
         */
        private function onScrollableMenuColorsChanged(event:Event):void {
            var colors:Object = themeManager.getScrollableMenuColors();
            if (colors && scrollPane) {
                setScrollPaneColors(colors.borderColor, colors.backgroundColor);
            }
        }

        /**
         * @brief Обработчик изменения масштаба
         */
        private function onScaleChanged(event:Event):void {
            // Пересоздаем все элементы с новыми размерами
            if (menuItems && menuItems.length > 0) {
                var currentItems:Array = menuItems.slice(); // Копия
                var currentActiveIndex:int = activeIndex;
                
                clearVisualElements();
                createVisualElements();
                updateContentSize();
                setActiveIndex(currentActiveIndex);
            }
        }

        /**
         * Очищает все элементы
         */
        private function clearAllItems():void {
            clearVisualElements();
            menuItems = [];
            _activeIndex = -1;
            log("ScrollableMenu: Все элементы очищены");
        }

        /**
         * Обновляет ScrollPane
         */
        private function refreshScrollPane():void {
            if (!scrollPane) return;
            
            try {
                scrollPane.invalidate();
                scrollPane.validateNow();
                scrollPane.drawNow();
                scrollPane.update();
                log("ScrollableMenu: ScrollPane обновлен");
            } catch (error:Error) {
                log("ScrollableMenu: Ошибка обновления ScrollPane: " + error.message);
            }
        }

        /**
         * Получает текущие элементы меню
         */
        public function getCurrentItems():Array {
            return menuItems ? menuItems.slice() : [];
        }

        // ===== ПУБЛИЧНЫЕ МЕТОДЫ ДЛЯ MENUMANAGER =====

        /**
         * @brief Обновляет масштаб (вызывается из MenuManager)
         */
        public function updateScale():void {
            onScaleChanged(null);
        }

        /**
         * @brief Обновляет тему (вызывается из MenuManager)
         */
        public function updateTheme():void {
            onThemeChanged(null);
        }

        /**
         * @brief Обновляет цвета компонентов (вызывается из MenuManager)
         */
        public function updateComponentColors():void {
            onComponentColorsChanged(null);
        }

        /**
         * @brief Обновляет цвета ScrollPane (вызывается из MenuManager)
         */
        public function updateScrollPaneColors():void {
            onScrollableMenuColorsChanged(null);
        }

        /**
         * @brief Устанавливает элементы меню и обновляет отображение
         * @param items Массив элементов меню
         * @param activeIndex Индекс активного элемента (по умолчанию 0)
         */
        public function setItems(items:Array, activeIndex:int = 0):void {
            if (!items) {
                items = [];
            }
            
            // Сохраняем новые элементы
            menuItems = items.slice();
            
            // Очищаем старые элементы
            clearVisualElements();
            
            // Создаем новые элементы
            if (menuItems.length > 0) {
                createVisualElements();
                updateContentSize();
                
                // Устанавливаем активный индекс
                if (activeIndex >= 0 && activeIndex < menuItems.length) {
                    setActiveIndex(activeIndex);
                } else if (menuItems.length > 0) {
                    var firstInteractiveIndex:int = findFirstInteractiveElement();
                    if (firstInteractiveIndex >= 0) {
                        setActiveIndex(firstInteractiveIndex);
                    }
                }
            } else {
                _activeIndex = -1;
            }
            
            // Принудительное обновление ScrollPane
            if (scrollPane) {
                scrollPane.visible = true;
                scrollPane.alpha = 1.0;
                
                // Устанавливаем source
                scrollPane.source = contentContainer;
                
                scrollPane.invalidate();
                scrollPane.validateNow();
                scrollPane.update();
                
                // Дополнительная принудительная перерисовка
                if (scrollPane.hasOwnProperty("drawNow")) {
                    scrollPane.drawNow();
                }
            }
        }

        /**
         * @brief Добавляет один элемент в меню без пересоздания всех остальных
         * @param item Объект элемента для добавления
         */
        public function addSingleItem(item:Object):void {
            if (!item) {
                log("ScrollableMenu: ERROR - item равен null");
                return;
            }
            
            // Добавляем в массив данных
            if (!menuItems) {
                menuItems = [];
            }
            
            var newIndex:int = menuItems.length;
            menuItems.push(item);
            
            try {
                // Создаем визуальный элемент для нового item
                var element:* = createElementByType(item, newIndex);
                
                if (element) {
                    // Позиционируем элемент
                    var scaledVerticalMargin:Number = menuScaler.getScaledVerticalMargin();
                    var scaledItemHeight:Number = menuScaler.getScaledItemHeight();
                    var scaledItemSpacing:Number = menuScaler.getScaledItemSpacing();
                    
                    // Вычисляем Y позицию для нового элемента
                    var yPos:Number = scaledVerticalMargin;
                    if (visualElements.length > 0) {
                        // Позиционируем после последнего элемента
                        yPos = visualElements[visualElements.length - 1].y + scaledItemHeight + scaledItemSpacing;
                    }
                    
                    element.x = 5;
                    element.y = yPos;
                    
                    // Устанавливаем ширину элемента
                    var availableWidth:Number = scrollPane.width - 10;
                    if (element.hasOwnProperty('width')) {
                        element.width = availableWidth;
                    }
                    
                    // Проверка размеров
                    if (element.width <= 0 || element.height <= 0) {
                        if (element.width <= 0) element.width = availableWidth;
                        if (element.height <= 0) element.height = 30;
                    }
                    
                    // Добавляем в контейнер
                    contentContainer.addChild(element);
                    
                    // Принудительно делаем элемент видимым
                    element.visible = true;
                    element.alpha = 1.0;
                    
                    // Применяем цвета темы
                    applyThemeColorsToElement(element, item.type);
                    
                    // Добавляем в массив визуальных элементов
                    visualElements.push(element);
                    
                    // Обновляем размер контейнера
                    updateContentSize();
                    
                    // Обновляем ScrollPane
                    if (scrollPane) {
                        scrollPane.invalidate();
                        scrollPane.validateNow();
                        scrollPane.update();
                        
                        if (scrollPane.hasOwnProperty("drawNow")) {
                            scrollPane.drawNow();
                        }
                    }
                    
                    // Если это первый интерактивный элемент и нет активного индекса, делаем его активным
                    if (_activeIndex < 0 && item.type !== 0) {
                        setActiveIndex(newIndex);
                    }
                    
                } else {
                    log("ScrollableMenu: ERROR - Не удалось создать визуальный элемент");
                }
                
            } catch (error:Error) {
                log("ScrollableMenu: ERROR в addSingleItem(): " + error.message);
            }
        }

        /**
         * @brief Ищет первый интерактивный элемент (не Label)
         * @return Индекс первого интерактивного элемента или -1 если не найден
         */
        private function findFirstInteractiveElement():int {
            for (var i:int = 0; i < menuItems.length; i++) {
                var item:Object = menuItems[i];
                if (item && item.type !== 0) { // 0 = LabelComponent (не интерактивный)
                    return i;
                }
            }
            return -1;
        }
    }
}
