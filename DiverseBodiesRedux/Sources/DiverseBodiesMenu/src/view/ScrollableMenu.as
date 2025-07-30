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
        // Статическая ссылка на Main для колбэков
        public static var mainInstance:Object = null;
        
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
         * @brief Логирует сообщение через trace и дублирует в C++ через Main
         * @param message Сообщение для логирования
         */
        private function log(message:String):void {
            trace(message);
            if (mainInstance && mainInstance.BGSCodeObj && 
                typeof mainInstance.BGSCodeObj.Log === "function") {
                mainInstance.BGSCodeObj.Log(message);
            }
        }

        public function ScrollableMenu(scrollPaneComponent:ScrollPane) {
            this.scrollPane = scrollPaneComponent;
            log("ScrollableMenu: Инициализация с компонентом " + scrollPane.name);
            
            // Получаем ссылки на синглтоны менеджеров
            themeManager = ThemeManager.getInstance();
            menuScaler = MenuScaler.getInstance();
            
            // Подписываемся на события изменений
            themeManager.addEventListener(ThemeManager.THEME_CHANGED, onThemeChanged);
            themeManager.addEventListener(ThemeManager.COMPONENT_COLORS_CHANGED, onComponentColorsChanged);
            themeManager.addEventListener(ThemeManager.SCROLLABLE_MENU_COLORS_CHANGED, onScrollableMenuColorsChanged);
            
            menuScaler.addEventListener(MenuScaler.SCALE_CHANGED, onScaleChanged);
            
            log("ScrollableMenu: Подписка на события ThemeManager и MenuScaler выполнена");

            initializeScrollPane();
            setupKeyboardHandling();
        }

        /**
         * Принудительная инициализация компонентов меню (вызывается из C++)
         * Обеспечивает правильную инициализацию ScrollPane и его иерархии
         */
        public function initializeComponents():void {
            log("ScrollableMenu: initializeComponents() вызван");
            
            try {
                // Проверяем состояние основных компонентов
                log("ScrollableMenu: scrollPane=" + (scrollPane ? "да" : "нет") + 
                    ", contentContainer=" + (contentContainer ? "да" : "нет"));
                
                if (!scrollPane) {
                    log("ScrollableMenu: ОШИБКА - scrollPane не инициализирован");
                    return;
                }
                
                if (!contentContainer) {
                    log("ScrollableMenu: ОШИБКА - contentContainer не создан");
                    return;
                }
                
                // АГРЕССИВНАЯ ПЕРЕЗАГРУЗКА ScrollPane source
                try {
                    log("ScrollableMenu: Принудительная перезагрузка ScrollPane source");

                    // Шаг 1: Очищаем source и шаг 2: переустанавливаем его
                    scrollPane.source = null;
                    scrollPane.invalidate();
                    scrollPane.validateNow();
                    scrollPane.source = contentContainer;
                    log("ScrollableMenu: contentContainer переустановлен как source");

                    // Множественные принудительные обновления (Шаг 3 & 4)
                    scrollPane.invalidate();
                    scrollPane.validateNow();
                    scrollPane.drawNow();
                    scrollPane.update();
                    log("ScrollableMenu: Агрессивная перезагрузка завершена");
                } catch (reloadError:Error) {
                    log("ScrollableMenu: Ошибка при агрессивной перезагрузке: " + reloadError.message);
                }
                // Проверяем доступность stage
                if (scrollPane.stage) {
                    log("ScrollableMenu: scrollPane на сцене, принудительно обновляем");
                    
                    // Дополнительная принудительная синхронизация
                    if (scrollPane.stage.frameRate > 0) {
                        // Ждем один кадр и повторно обновляем
                        var updateTimer:Timer = new Timer(1000 / scrollPane.stage.frameRate, 1);
                        updateTimer.addEventListener(TimerEvent.TIMER, function(e:TimerEvent):void {
                            updateTimer.removeEventListener(TimerEvent.TIMER, arguments.callee);
                            
                            // Повторная агрессивная перезагрузка через кадр
                            scrollPane.source = null;
                            scrollPane.invalidate();
                            scrollPane.validateNow();
                            scrollPane.source = contentContainer;
                            scrollPane.invalidate();
                            scrollPane.validateNow();
                            
                            log("ScrollableMenu: Отложенное обновление ScrollPane завершено");
                        });
                        updateTimer.start();
                    }
                } else {
                    log("ScrollableMenu: scrollPane еще не на сцене, ожидаем добавления");
                    
                    // Ожидаем добавления на сцену
                    scrollPane.addEventListener(Event.ADDED_TO_STAGE, function(e:Event):void {
                        scrollPane.removeEventListener(Event.ADDED_TO_STAGE, arguments.callee);
                        log("ScrollableMenu: scrollPane добавлен на сцену в initializeComponents");
                        
                        // Повторная инициализация после добавления на сцену
                        scrollPane.source = null;
                        scrollPane.invalidate();
                        scrollPane.validateNow();
                        scrollPane.source = contentContainer;
                        scrollPane.invalidate();
                        scrollPane.validateNow();
                        scrollPane.drawNow();
                        scrollPane.update();
                        
                        log("ScrollableMenu: Отложенная инициализация ScrollPane завершена");
                    });
                }
                
                // Диагностика состояния после инициализации
                //logComponentState();
                
            } catch (error:Error) {
                log("ScrollableMenu: ОШИБКА в initializeComponents: " + error.message);
                log("ScrollableMenu: Stack trace: " + error.getStackTrace());
            }
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
            
            // Настраиваем ScrollPane согласно требованиям
            // Размеры будут установлены в Main.as, здесь только политики прокрутки
            scrollPane.horizontalScrollPolicy = "off"; // Отключаем полосу прокрутки по умолчанию
            scrollPane.verticalScrollPolicy = "off"; // Отключаем полосу прокрутки по умолчанию
            
            // Очищаем встроенные стили ScrollPane чтобы избежать лишних цветов
            clearScrollPaneStyles();
            
            // КРИТИЧЕСКИ ВАЖНО: правильная настройка ScrollPane для Flash CS
            try {
                // Сначала очищаем любой существующий source
                scrollPane.source = null;
                scrollPane.invalidate();
                scrollPane.validateNow();
                
                // Устанавливаем контейнер как source для ScrollPane
                scrollPane.source = contentContainer;

                // Получаем цвета из ThemeManager
                var colors:Object = themeManager.getScrollableMenuColors();
                setScrollPaneColors(colors.borderColor, colors.backgroundColor);
                
                // Принудительная перерисовка ScrollPane
                scrollPane.invalidate();
                scrollPane.drawNow();
                scrollPane.validateNow();
                scrollPane.update();
                
                log("ScrollableMenu: ScrollPane принудительно сконфигурирован с source=" + contentContainer.name);
                
                // Проверяем и исправляем установку source
                if (scrollPane.source !== contentContainer) {
                    // Пытаемся установить source повторно
                    scrollPane.source = contentContainer;
                    scrollPane.validateNow();
                    log("ScrollableMenu: scrollPane.source переназначен на contentContainer повторно");
                }
  
            } catch (setupError:Error) {
                log("ScrollableMenu: Ошибка при настройке ScrollPane: " + setupError.message);
            }
            
            // Цвета будут применены через ThemeManager из MenuManager
            
            log("ScrollableMenu: fl.containers.ScrollPane настроен с source=" + contentContainer.name);
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
                    
                    log("ScrollableMenu: Стили очищены через setStyle (безопасная очистка)");
                }
                
                log("ScrollableMenu: Пропуск удаления встроенных элементов оформления для предотвращения ошибок индексов");
                
                // НЕ очищаем graphics - это может нарушить структуру ScrollPane
                log("ScrollableMenu: Встроенные стили ScrollPane осторожно очищены");
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
                scrollPane.stage.focus = scrollPane.stage; // Устанавливаем фокус
                setupMouseScrolling();
                log("ScrollableMenu: Обработка клавиатуры и мыши настроена");
            } else {
                scrollPane.addEventListener(Event.ADDED_TO_STAGE, function(e:Event):void {
                    scrollPane.stage.addEventListener(KeyboardEvent.KEY_DOWN, onKeyDown);
                    scrollPane.stage.focus = scrollPane.stage;
                    setupMouseScrolling();
                    log("ScrollableMenu: Обработка клавиатуры и мыши настроена после добавления на Stage");
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
                
                log("ScrollableMenu: Прокрутка колесом мыши настроена (множественные слушатели)");
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
            
            log("ScrollableMenu: Прокрутка мышью - delta: " + event.delta + 
                ", было: " + currentScrollY.toFixed(1) + 
                ", стало: " + newScrollY.toFixed(1) + 
                ", макс: " + maxScroll.toFixed(1));
        }

        /**
         * Устанавливает новый список элементов меню
         */
        public function setItems(items:Array, activeIndex:int = 0):void {
            log("ScrollableMenu: Установка " + items.length + " элементов");
            
            this.menuItems = items;
            clearVisualElements();
            
            // Восстанавливаем связь contentContainer с ScrollPane перед созданием элементов
            ensureContentContainerInScrollPane();
            
            createVisualElements();
            updateContentSize();
            
            // Сохраняем текущий активный индекс или устанавливаем на первый интерактивный элемент
            var targetActiveIndex:int = activeIndex; // Сохраняем существующий activeIndex
            var firstInteractiveIndex:int = -1;
            
            // Находим первый интерактивный элемент
            for (var i:int = 0; i < menuItems.length; i++) {
                if (menuItems[i].type !== 0) { // Не LabelComponent
                    firstInteractiveIndex = i;
                    break;
                }
            }
            
            // Проверяем, что сохраненный activeIndex валиден
            var isActiveIndexValid:Boolean = false;
            if (targetActiveIndex >= 0 && targetActiveIndex < menuItems.length) {
                if (menuItems[targetActiveIndex].type !== 0) { // Не LabelComponent
                    isActiveIndexValid = true;
                }
            }
            
            // Устанавливаем финальный activeIndex
            if (isActiveIndexValid) {
                setActiveIndex(targetActiveIndex);
                log("ScrollableMenu: Сохранен существующий активный индекс: " + targetActiveIndex);
            } else if (firstInteractiveIndex >= 0) {
                setActiveIndex(firstInteractiveIndex);
                log("ScrollableMenu: Активный индекс установлен на первый интерактивный элемент: " + firstInteractiveIndex);
            } else {
                // Если нет интерактивных элементов, сбрасываем activeIndex
                setActiveIndex(-1);
                log("ScrollableMenu: Нет интерактивных элементов, активный индекс сброшен");
            }
            
            // Обеспечиваем правильную связь ScrollPane с контейнером
            if (scrollPane.source !== contentContainer) {
                try {
                    scrollPane.source = contentContainer;
                    scrollPane.invalidate();
                    scrollPane.update();
                } catch (fixError:Error) {
                    log("ScrollableMenu: ОШИБКА при установке source: " + fixError.message);
                }
            }
            
            log("ScrollableMenu: Элементы установлены успешно");
        }

        /**
         * Очищает все визуальные элементы
         */
        private function clearVisualElements():void {
            // Удаляем все элементы из массива
            for each (var element:* in visualElements) {
                if (element && element.parent) {
                    element.parent.removeChild(element);
                }
            }
            visualElements = [];
            
            // Очищаем контейнер
            while (contentContainer.numChildren > 0) {
                contentContainer.removeChildAt(0);
            }
            
            log("ScrollableMenu: Визуальные элементы очищены");
        }

        /**
         * Создает визуальные элементы по данным
         */
        private function createVisualElements():void {
            var scaledVerticalMargin:Number = menuScaler.getScaledVerticalMargin();
            var scaledItemHeight:Number = menuScaler.getScaledItemHeight();
            var scaledItemSpacing:Number = menuScaler.getScaledItemSpacing();
            
            var yPos:Number = scaledVerticalMargin; // Отступ сверху
            
            for (var i:int = 0; i < menuItems.length; i++) {
                var itemData:Object = menuItems[i];
                var element:* = createElementByType(itemData, i);
                
                if (element) {
                    element.x = 5; // 5px отступ слева
                    element.y = yPos;
                    
                    // Устанавливаем ширину элемента: ширина ScrollPane минус 5px слева и справа
                    var availableWidth:Number = scrollPane.width - 10; // -5px слева и -5px справа
                    if (element.hasOwnProperty('width')) {
                        var oldWidth:Number = element.width;
                        element.width = availableWidth;
                        log("ScrollableMenu: Установка ширины элемента - scrollPane.width: " + scrollPane.width + 
                            ", availableWidth: " + availableWidth + 
                            ", старая ширина: " + oldWidth + 
                            ", новая ширина: " + element.width);
                    }
                    
                    contentContainer.addChild(element);
                    
                    // Принудительно делаем элемент видимым
                    element.visible = true;
                    element.alpha = 1.0;
                    
                    // Обеспечиваем, что элемент находится на верхнем слое
                    contentContainer.setChildIndex(element, contentContainer.numChildren - 1);
                    
                    // Принудительно применяем цвета к новому элементу
                    applyThemeColorsToElement(element, itemData.type);
                    
                    visualElements.push(element);
                    
                    // Добавляем диагностику реальных размеров после добавления в контейнер
                    var bounds:Rectangle = element.getBounds(contentContainer);
                    log("ScrollableMenu: Элемент добавлен в контейнер - реальные границы: " + 
                        "x=" + bounds.x + ", y=" + bounds.y + ", width=" + bounds.width + ", height=" + bounds.height);
                    
                    yPos += scaledItemHeight + scaledItemSpacing;
                    
                    log("ScrollableMenu: Создан элемент " + i + " типа " + itemData.type + " (" + itemData.labelText + ")");
                    log("  - Позиция: (" + element.x + ", " + element.y + "), ширина: " + (element.hasOwnProperty('width') ? element.width : 'не задана'));
                }
            }
            
            // Устанавливаем размер контейнера с учетом нижнего отступа
            updateContentSize();
            
            // Принудительно обновляем ScrollPane
            try {
                scrollPane.invalidate();
                scrollPane.drawNow();
                scrollPane.validateNow();
                scrollPane.update();
            } catch (error:Error) {
                log("WARNING: Ошибка при обновлении ScrollPane: " + error.message);
            }
            
            // Проверяем видимость элементов
            for (var checkIndex:int = 0; checkIndex < visualElements.length; checkIndex++) {
                var checkElement:* = visualElements[checkIndex];
                if (checkElement) {
                    log("  - Элемент " + checkIndex + ": visible=" + checkElement.visible + 
                        ", alpha=" + checkElement.alpha + 
                        ", parent=" + (checkElement.parent ? "да" : "нет") +
                        ", на сцене=" + (checkElement.stage ? "да" : "нет"));
                }
            }
            
            var scaledHorizontalMargin:Number = menuScaler.getScaledHorizontalMargin();
            var scaledContainerWidth:Number = menuScaler.getScaledContainerWidth();
            var containerHeight:Number = yPos + scaledVerticalMargin; // Добавляем отступ снизу
            
            log("ScrollableMenu: Создано " + visualElements.length + " визуальных элементов");
            log("ScrollableMenu: Размер контейнера: " + (scrollPane.width - 10) + "x" + containerHeight);
            log("ScrollableMenu: Компоненты выровнены с отступами 5px слева и справа");
            
            // Применяем цвета темы ко всем созданным элементам
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
                    log("ScrollableMenu: Цвета ScrollPane '" + scrollPane.name + "' применены в applyThemeColorsToAllElements");
                }
                
                // ЗАТЕМ применяем цвета к элементам меню
                var appliedCount:int = 0;
                for (var i:int = 0; i < visualElements.length; i++) {
                    var element:MovieClip = visualElements[i] as MovieClip;
                    var itemData:Object = menuItems[i];
                    if (element && itemData) {
                        applyThemeColorsToElement(element, itemData.type);
                        appliedCount++;
                    }
                }
                log("ScrollableMenu: Цвета темы применены к ScrollPane '" + (scrollPane ? scrollPane.name : "null") + "' + " + 
                    appliedCount + " из " + visualElements.length + " элементов");
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
                        element.addEventListener(ButtonComponent.EVENT_HOVER, createHoverHandler(index, data));
                        // Добавляем обработчик клика для установки активного индекса
                        element.addEventListener(MouseEvent.CLICK, createMouseClickHandler(index, data));
                        break;
                        
                    case 2: // Checkbox
                        element = new CheckboxComponent(text, data.checked || false);
                        element.addEventListener(CheckboxComponent.EVENT_CHANGE, createElementHandler(index, data));
                        // Добавляем обработчик клика для установки активного индекса
                        element.addEventListener(MouseEvent.CLICK, createMouseClickHandler(index, data));
                        break;
                        
                    case 3: // Switcher
                        var options:Array = text.split(",");
                        // Очищаем пробелы в опциях
                        for (var j:int = 0; j < options.length; j++) {
                            options[j] = String(options[j]).replace(/^\s+|\s+$/g, "");
                        }
                        element = new SwitcherComponent(options, data.selectedIndex || 0);
                        element.addEventListener(SwitcherComponent.EVENT_CHANGE, createSwitcherHandler(index, data));
                        element.addEventListener(SwitcherComponent.EVENT_BACK, createBackHandler(index, data));
                        element.addEventListener(SwitcherComponent.EVENT_PUSH, createElementHandler(index, data));
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
                
                log("ScrollableMenu: Кастомная подложка создана и размещена под ScrollPane");
                log("  - Позиция: (" + backgroundLayer.x + ", " + backgroundLayer.y + ")");
                log("  - Размер: " + backgroundLayer.width + "x" + backgroundLayer.height);
                log("  - Фон RGB: 0x" + backgroundRGB.toString(16).toUpperCase() + ", Alpha: " + backgroundAlpha.toFixed(3));
                log("  - Граница RGB: 0x" + borderRGB.toString(16).toUpperCase() + ", Alpha: " + borderAlpha.toFixed(3));
            }
        }

        /**
         * @brief Применяет цвета к ScrollPane компоненту через кастомную подложку
         * @param borderColor Цвет границы (ARGB)  
         * @param backgroundColor Цвет фона (ARGB)
         */
        public function setScrollPaneColors(borderColor:uint, backgroundColor:uint):void {
            log("ScrollableMenu: setScrollPaneColors вызван для '" + (scrollPane ? scrollPane.name : "null") + 
                "' с цветами - граница: 0x" + borderColor.toString(16).toUpperCase() + 
                ", фон: 0x" + backgroundColor.toString(16).toUpperCase());
            
            if (!scrollPane) {
                log("ScrollableMenu: ERROR - ScrollPane не инициализирован");
                return;
            }
            
            try {
                // Создаём кастомную подложку вместо попыток изменить ScrollPane напрямую
                createBackgroundLayer(borderColor, backgroundColor);
                
                log("ScrollableMenu: Кастомная подложка для '" + scrollPane.name + "' создана успешно");
                
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
         * @brief Отправляет ItemSelected в C++
         */
        private function sendPushCallback(labelText:String):void {
            log("ScrollableMenu: sendPushCallback(" + labelText + ")");
            if (mainInstance && mainInstance.sendPushButtonCallback) {
                mainInstance.sendPushButtonCallback(labelText);
            } else {
                log("ScrollableMenu: sendPushButtonCallback недоступен");
            }
        }
        
        /**
         * @brief Отправляет BackPressed в C++
         */
        private function sendBackCallback():void {
            log("ScrollableMenu: sendBackCallback()");
            if (mainInstance && mainInstance.sendBackButtonCallback) {
                mainInstance.sendBackButtonCallback("");
            } else if (mainInstance && mainInstance.BGSCodeObj && 
                      typeof mainInstance.BGSCodeObj.BackPressed === "function") {
                mainInstance.BGSCodeObj.BackPressed();
            } else {
                log("ScrollableMenu: BackPressed недоступен");
            }
        }
        
        /**
         * @brief Отправляет ItemHoverChanged в C++
         */
        private function sendHoverCallback(labelText:String):void {
            log("ScrollableMenu: sendHoverCallback(" + labelText + ")");
            if (mainInstance && mainInstance.sendHoverButtonCallback) {
                mainInstance.sendHoverButtonCallback(labelText);
            } else {
                log("ScrollableMenu: sendHoverButtonCallback недоступен");
            }
        }
        
        /**
         * @brief Отправляет CheckboxChanged в C++
         */
        private function sendCheckboxCallback(labelText:String, checked:Boolean):void {
            log("ScrollableMenu: sendCheckboxCallback(" + labelText + ", " + checked + ")");
            if (mainInstance && mainInstance.sendChangeCheckboxCallback) {
                mainInstance.sendChangeCheckboxCallback(labelText, checked);
            } else {
                log("ScrollableMenu: sendChangeCheckboxCallback недоступен");
            }
        }
        
        /**
         * @brief Отправляет SwitcherChanged в C++
         */
        private function sendSwitcherCallback(labelText:String, selectedIndex:int):void {
            log("ScrollableMenu: sendSwitcherCallback(" + labelText + ", " + selectedIndex + ")");
            if (mainInstance && mainInstance.sendChangeSwitcherCallback) {
                mainInstance.sendChangeSwitcherCallback(labelText, selectedIndex);
            } else {
                log("ScrollableMenu: sendChangeSwitcherCallback недоступен");
            }
        }

        /**
         * Обработчик нажатия клавиш
         */
        private function onKeyDown(event:KeyboardEvent):void {
            // Обрабатываем ESC и TAB независимо от наличия элементов в меню
            if (event.keyCode === 27 || event.keyCode === 9) { // ESC или TAB
                log("ScrollableMenu: " + (event.keyCode === 27 ? "ESC" : "TAB") + " нажат - отправляем BackPressed");
                sendBackCallback();
                dispatchEvent(new CustomEvent("menuBack", {}));
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
            
            log("ScrollableMenu: onKeyDown - keyCode=" + event.keyCode + ", activeIndex=" + _activeIndex + 
                ", currentItem.type=" + (currentItem ? currentItem.type : "нет"));
            
            switch (event.keyCode) {
                case 27: // ESC - уже обработан выше
                case 9: // TAB - уже обработан выше
                    // Эти события уже обработаны в начале функции
                    break;
                    
                case 13: // Enter - отправляет ItemSelected только для ButtonComponent
                    if (currentItem && currentItem.type === 1) { // ButtonComponent
                        log("ScrollableMenu: Enter нажат на ButtonComponent - отправляем ItemSelected");
                        sendPushCallback(currentItem.labelText);
                    } else {
                        log("ScrollableMenu: Enter нажат НЕ на ButtonComponent (тип " + (currentItem ? currentItem.type : "нет") + ") - игнорируем");
                    }
                    break;
                    
                case 32: // Space - CheckboxChanged только для CheckboxComponent
                    if (currentItem && currentItem.type === 2 && currentElement) { // CheckboxComponent
                        log("ScrollableMenu: Space нажат на CheckboxComponent");
                        currentElement.selected = !currentElement.selected
                        sendCheckboxCallback(currentItem.labelText, currentElement.selected);
                    } else {
                        log("ScrollableMenu: Space нажат НЕ на CheckboxComponent - игнорируем");
                    }
                    break;
                    
                case 38: // Up arrow - навигация вверх
                    if (interactiveIndices.length === 0) {
                        log("ScrollableMenu: Нет интерактивных элементов для навигации вверх");
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
                                log("ScrollableMenu: Навигация вверх на ButtonComponent - отправляем ItemHoverChanged");
                                sendHoverCallback(upItem.labelText);
                            }
                            log("ScrollableMenu: Активный элемент изменен стрелкой вверх на индекс " + upCandidate);
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
                                    log("ScrollableMenu: Fallback вниз на ButtonComponent - отправляем ItemHoverChanged");
                                    sendHoverCallback(fallbackItem.labelText);
                                }
                                log("ScrollableMenu: Fallback вниз на индекс " + fallbackCandidate);
                                break;
                            }
                            fallbackDownIndex++;
                        }
                    }
                    break;

                case 40: // Down arrow - навигация вниз
                    if (interactiveIndices.length === 0) {
                        log("ScrollableMenu: Нет интерактивных элементов для навигации вниз");
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
                                log("ScrollableMenu: Навигация вниз на ButtonComponent - отправляем ItemHoverChanged");
                                sendHoverCallback(downItem.labelText);
                            }
                            log("ScrollableMenu: Активный элемент изменен стрелкой вниз на индекс " + downCandidate);
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
                                    log("ScrollableMenu: Fallback вверх на ButtonComponent - отправляем ItemHoverChanged");
                                    sendHoverCallback(fallbackItem2.labelText);
                                }
                                log("ScrollableMenu: Fallback вверх на индекс " + fallbackCandidate2);
                                break;
                            }
                            fallbackUpIndex--;
                        }
                    }
                    break;
                    
                case 39: // Right arrow
                    if (currentItem && currentElement) {
                        if (currentItem.type === 1) { // ButtonComponent - то же что Enter
                            log("ScrollableMenu: Стрелка вправо на ButtonComponent - отправляем ItemSelected");
                            sendPushCallback(currentItem.labelText);
                        } else if (currentItem.type === 3) { // SwitcherComponent - делегируем обработку компоненту
                            log("ScrollableMenu: Стрелка вправо на SwitcherComponent - вызываем switchRight()");
                            if (currentElement.hasOwnProperty("switchRight")) {
                                currentElement.switchRight(); // Компонент сам отправит нужные события
                            }
                        } else if (currentItem.type === 2) { // CheckboxComponent - устанавливает если не установлен
                            log("ScrollableMenu: Стрелка вправо на CheckboxComponent");
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
                            log("ScrollableMenu: Стрелка влево на ButtonComponent - отправляем BackPressed");
                            sendBackCallback();
                            dispatchEvent(new CustomEvent("menuBack", {}));
                        } else if (currentItem.type === 3) { // SwitcherComponent - делегируем обработку компоненту
                            log("ScrollableMenu: Стрелка влево на SwitcherComponent - вызываем switchLeft()");
                            if (currentElement.hasOwnProperty("switchLeft")) {
                                currentElement.switchLeft(); // Компонент сам отправит нужные события
                            }
                        } else if (currentItem.type === 2) { // CheckboxComponent - снимает если установлен
                            log("ScrollableMenu: Стрелка влево на CheckboxComponent");
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
                log("ScrollableMenu: Событие элемента " + index + " типа " + data.type);
                
                // Отправляем колбэк в C++
                log("ScrollableMenu: mainInstance=" + (mainInstance ? "есть" : "нет"));
                if (mainInstance) {
                    log("ScrollableMenu: mainInstance.BGSCodeObj=" + (mainInstance.BGSCodeObj ? "есть" : "нет"));
                    if (mainInstance.BGSCodeObj) {
                        log("ScrollableMenu: typeof ItemSelected=" + typeof mainInstance.BGSCodeObj.ItemSelected);
                    }
                }
                
                if (mainInstance && mainInstance.BGSCodeObj && 
                    typeof mainInstance.BGSCodeObj.ItemSelected === "function") {
                    log("ScrollableMenu: Вызываем BGSCodeObj.ItemSelected(" + data.labelText + ")");
                    try {
                        mainInstance.BGSCodeObj.ItemSelected(data.labelText);
                        log("ScrollableMenu: BGSCodeObj.ItemSelected вызван успешно");
                    } catch (callError:Error) {
                        log("ScrollableMenu: ОШИБКА вызова BGSCodeObj.ItemSelected: " + callError.message);
                    }
                } else {
                    log("ScrollableMenu: BGSCodeObj.ItemSelected недоступен для вызова");
                }
                
                // Диспетчируем событие
                dispatchEvent(new CustomEvent("itemSelected", {index: index, data: data}));
            };
        }

        /**
         * Создает обработчик hover событий
         */
        private function createHoverHandler(index:int, data:Object):Function {
            return function(event:Event):void {
                log("ScrollableMenu: Hover элемента " + index + " типа " + data.type);
                
                // Отправляем ItemHoverChanged только для ButtonComponent при клике мышью
                if (data.type === 1) { // ButtonComponent
                    log("ScrollableMenu: ButtonComponent hover - отправляем ItemHoverChanged");
                    sendHoverCallback(data.labelText);
                } else {
                    log("ScrollableMenu: НЕ ButtonComponent (тип " + data.type + ") - ItemHoverChanged не отправляем");
                }
            };
        }

        /**
         * Создает обработчик кликов мыши для установки активного индекса
         */
        private function createMouseClickHandler(index:int, data:Object):Function {
            return function(event:MouseEvent):void {
                log("ScrollableMenu: Клик мыши на элементе " + index + " типа " + data.type);

                // Устанавливаем активный индекс при клике на интерактивный элемент
                if (data.type !== 0) { // Не LabelComponent
                    setActiveIndex(index);
                    log("ScrollableMenu: Активный индекс установлен в " + index + " по клику мыши");

                    // Отправляем ItemHoverChanged для ButtonComponent при клике мыши
                    if (data.type === 1) { // ButtonComponent
                        log("ScrollableMenu: ButtonComponent клик - отправляем ItemHoverChanged");
                        sendHoverCallback(data.labelText);
                    }

                    if (data.type === 2) { // CheckboxComponent
                        var element:* = visualElements[index];
                        if (element && element.hasOwnProperty("selected")) {
                            sendCheckboxCallback(data.labelText, element.selected);
                        }
                    }
                    // SwitcherComponent events are handled by createSwitcherHandler, не дублируем здесь
                }
            };
        }

        /**
         * Создает обработчик back событий от компонентов
         */
        private function createBackHandler(index:int, data:Object):Function {
            return function(event:Event):void {
                log("ScrollableMenu: Back событие от элемента " + index + " типа " + data.type);
                
                // Отправляем back в C++
                sendBackCallback();
                
                // Диспетчируем событие menuBack для MenuManager
                dispatchEvent(new CustomEvent("menuBack", {}));
            };
        }

        /**
         * Создает обработчик событий CHANGE для SwitcherComponent
         */
        private function createSwitcherHandler(index:int, data:Object):Function {
            return function(event:Event):void {
                log("ScrollableMenu: Switcher CHANGE событие от элемента " + index);
                
                // Получаем текущий индекс из компонента
                var element:* = visualElements[index];
                if (element && element.hasOwnProperty("index")) {
                    var currentIndex:int = element.index;
                    log("ScrollableMenu: Switcher изменен на индекс " + currentIndex);
                    
                    // Отправляем в C++
                    sendSwitcherCallback(data.labelText, currentIndex);
                } else {
                    log("ScrollableMenu: ОШИБКА - не удалось получить индекс из SwitcherComponent");
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
            
            log("ScrollableMenu: Размер контейнера обновлен: " + scaledContainerWidth + "x" + totalHeight);
        }

        /**
         * Устанавливает активный индекс
         */
        public function setActiveIndex(newIndex:int):void {
            if (newIndex < 0 || newIndex >= visualElements.length) return;
            
            // Убираем выделение с предыдущего элемента
            if (_activeIndex >= 0 && _activeIndex < visualElements.length) {
                var prevElement:* = visualElements[_activeIndex];
                if (prevElement && prevElement.hasOwnProperty("active")) {
                    prevElement.active = false;
                    log("ScrollableMenu: Сброшено active для элемента " + _activeIndex);
                }
            }
            
            _activeIndex = newIndex;
            
            // Устанавливаем выделение на новый элемент
            if (_activeIndex >= 0 && _activeIndex < visualElements.length) {
                var newElement:* = visualElements[_activeIndex];
                if (newElement && newElement.hasOwnProperty("active")) {
                    newElement.active = true;
                    log("ScrollableMenu: Установлено active для элемента " + _activeIndex);
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
                    log("ScrollableMenu: Цвета темы применены к элементу типа " + componentType);
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
            log("ScrollableMenu: onThemeChanged - применяем цвета темы ко всем элементам");
            applyThemeColorsToAllElements();
        }

        /**
         * @brief Обработчик изменения цветов компонентов
         */
        private function onComponentColorsChanged(event:*):void {
            if (event && event.hasOwnProperty("componentType")) {
                var componentType:int = event.componentType;
                log("ScrollableMenu: onComponentColorsChanged для типа " + componentType);
                
                // Применяем цвета только к элементам указанного типа
                for (var i:int = 0; i < visualElements.length; i++) {
                    if (menuItems[i] && menuItems[i].type === componentType) {
                        applyThemeColorsToElement(visualElements[i], componentType);
                    }
                }
            } else {
                log("ScrollableMenu: onComponentColorsChanged для всех компонентов");
                applyThemeColorsToAllElements();
            }
        }

        /**
         * @brief Обработчик изменения цветов ScrollableMenu
         */
        private function onScrollableMenuColorsChanged(event:Event):void {
            log("ScrollableMenu: onScrollableMenuColorsChanged - обновляем цвета ScrollPane");
            
            var colors:Object = themeManager.getScrollableMenuColors();
            if (colors && scrollPane) {
                setScrollPaneColors(colors.borderColor, colors.backgroundColor);
            }
        }

        /**
         * @brief Обработчик изменения масштаба
         */
        private function onScaleChanged(event:Event):void {
            log("ScrollableMenu: onScaleChanged - обновляем размеры и позиции элементов");
            
            // Пересоздаем все элементы с новыми размерами
            if (menuItems && menuItems.length > 0) {
                var currentItems:Array = menuItems.slice(); // Копия
                var currentActiveIndex:int = activeIndex;
                
                clearVisualElements();
                createVisualElements();
                updateContentSize();
                setActiveIndex(currentActiveIndex);
                
                log("ScrollableMenu: Элементы пересозданы с новым масштабом");
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
    }
}
