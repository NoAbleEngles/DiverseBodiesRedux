package view.components {
    import flash.display.MovieClip;
    import flash.text.TextField;
    import flash.text.TextFormat;
    import flash.events.MouseEvent;
    import flash.events.Event;
    import view.ScrollableMenu;

    /**
     * @brief Интерактивный переключатель с множественным выбором
     * 
     * Предоставляет элемент управления для выбора одного значения из списка опций.
     * Использует стрелки навигации влево/вправо для переключения между вариантами.
     * Поддерживает активное/неактивное состояние и события изменения выбора.
     */
    public class SwitcherComponent extends MovieClip {
        public static const EVENT_CHANGE:String = "eventSwitcherComponentChange";
        public static const EVENT_PUSH:String = "eventSwitcherComponentPush";
        public static const EVENT_BACK:String = "eventSwitcherComponentBack";
        public static const EVENT_HOVER:String = "eventSwitcherComponentHover";
        
        private var label:TextField;
        private var leftArrow:TextField;
        private var rightArrow:TextField;
        private var _options:Array;
        private var _selectedIndex:int = 0;
        private var _active:Boolean = false;
        
        private var bgWidth:Number = 500;
        private var bgHeight:Number = 45;
        
        // Свойства для хранения цветов
        private var normalColor:uint = 0x333333;
        private var textColor:uint = 0xFFFFFF;
        private var hoverColor:uint = 0x444444;
        private var selectedColor:uint = 0x0066CC;

        /**
         * @brief Логирует сообщение через trace и дублирует в C++ через Main
         * @param message Сообщение для логирования
         */
        private function log(message:String):void {
            trace(message);
        }

        /**
         * @brief Создает новый переключатель с указанными опциями
         * @param options Массив строк с доступными вариантами выбора
         * @param selectedIndex Индекс выбранного по умолчанию варианта
         */
        public function SwitcherComponent(options:Array = null, selectedIndex:int = 0) {
            this._options = options || ["Option 1", "Option 2"];
            this._selectedIndex = selectedIndex;
            init();
        }

        private function init():void {
            createBackground();
            createArrows();
            createLabel();
            addEventListeners();
            updateVisualState();
            updateContent();
        }

        private function createBackground():void {
            graphics.beginFill(0x333333, 1);
            graphics.lineStyle(1, 0x666666);
            graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 6, 6);
            graphics.endFill();
        }

        private function createArrows():void {
            // Левая стрелка
            leftArrow = new TextField();
            leftArrow.defaultTextFormat = new TextFormat("Arial", 20, 0xFFFFFF, true, false, false, null, null, "center");
            leftArrow.text = "<";
            leftArrow.x = 10;
            leftArrow.y = 8;
            leftArrow.width = 30;
            leftArrow.height = 30;
            leftArrow.selectable = false;
            leftArrow.mouseEnabled = false;
            addChild(leftArrow);

            // Правая стрелка
            rightArrow = new TextField();
            rightArrow.defaultTextFormat = new TextFormat("Arial", 20, 0xFFFFFF, true, false, false, null, null, "center");
            rightArrow.text = ">";
            rightArrow.x = bgWidth - 40;
            rightArrow.y = 8;
            rightArrow.width = 30;
            rightArrow.height = 30;
            rightArrow.selectable = false;
            rightArrow.mouseEnabled = false;
            addChild(rightArrow);
        }

        private function createLabel():void {
            label = new TextField();
            label.defaultTextFormat = new TextFormat("Arial", 16, 0xFFFFFF, false, false, false, null, null, "center");
            label.x = 50;
            label.y = 10;
            label.width = bgWidth - 100;
            label.height = bgHeight - 20;
            label.selectable = false;
            label.mouseEnabled = false;
            addChild(label);
        }

        private function addEventListeners():void {
            this.buttonMode = true;
            this.doubleClickEnabled = true;
            this.addEventListener(MouseEvent.CLICK, onClick);
            this.addEventListener(MouseEvent.DOUBLE_CLICK, onDoubleClick);
            this.addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
            this.addEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
        }

        private function updateContent():void {
            if (_options && _options.length > 0) {
                label.text = _options[_selectedIndex];
            }
            
            // Восстанавливаем цвет текста после изменения содержимого
            setTextColor(textColor);
            
            // Управление видимостью стрелок
            leftArrow.alpha = (_selectedIndex > 0) ? 1.0 : 0.3;
            rightArrow.alpha = (_selectedIndex < _options.length - 1) ? 1.0 : 0.3;
        }

        // Публичные методы
        /**
         * @brief Переключает на предыдущий вариант в списке
         */
        public function switchLeft():void {
            if (_selectedIndex > 0) {
                _selectedIndex--;
                updateContent();
                log("SwitcherComponent: переключено влево на '" + _options[_selectedIndex] + "' - отправляем CHANGE");
                log("SwitcherComponent: Dispatching EVENT_CHANGE = '" + EVENT_CHANGE + "'");
                dispatchEvent(new Event(EVENT_CHANGE));
                // НЕ отправляем EVENT_BACK - это только для выхода из меню
            }
        }

        /**
         * @brief Переключает на следующий вариант в списке
         */
        public function switchRight():void {
            if (_selectedIndex < _options.length - 1) {
                _selectedIndex++;
                updateContent();
                log("SwitcherComponent: переключено вправо на '" + _options[_selectedIndex] + "' - отправляем CHANGE");
                log("SwitcherComponent: Dispatching EVENT_CHANGE = '" + EVENT_CHANGE + "'");
                dispatchEvent(new Event(EVENT_CHANGE));
                // НЕ отправляем EVENT_PUSH - это только для активации элементов
            }
        }

        // Геттеры и сеттеры
        /**
         * @brief Возвращает индекс текущего выбранного элемента
         * @return Индекс выбранного элемента в массиве опций
         */
        public function get index():int {
            return _selectedIndex;
        }

        /**
         * @brief Возвращает значение текущего выбранного элемента
         * @return Строковое значение выбранного элемента
         */
        public function get value():* {
            return _options[_selectedIndex];
        }

        /**
         * @brief Устанавливает активное состояние переключателя
         * @param value true для активного состояния, false для неактивного
         */
        public function set active(value:Boolean):void {
            _active = value;
            updateVisualState();
        }

        public function get active():Boolean {
            return _active;
        }

        /**
         * @brief Возвращает массив доступных опций
         * @return Массив строк с вариантами выбора
         */
        public function get options():Array {
            return _options;
        }

        /**
         * @brief Устанавливает новый массив опций для выбора
         * @param value Новый массив строк с вариантами выбора
         */
        public function set options(value:Array):void {
            _options = value;
            if (_selectedIndex >= value.length) {
                _selectedIndex = 0;
            }
            updateContent();
        }

        /**
         * @brief Устанавливает ширину компонента
         * @param value Новая ширина
         */
        override public function set width(value:Number):void {
            if (value > 0 && value !== bgWidth) {
                bgWidth = value;
                redrawComponent();
            }
        }
        
        /**
         * @brief Возвращает текущую ширину компонента
         */
        override public function get width():Number {
            return bgWidth;
        }
        
        /**
         * @brief Устанавливает высоту компонента
         * @param value Новая высота
         */
        override public function set height(value:Number):void {
            if (value > 0 && value !== bgHeight) {
                bgHeight = value;
                redrawComponent();
            }
        }
        
        /**
         * @brief Возвращает текущую высоту компонента
         */
        override public function get height():Number {
            return bgHeight;
        }
        
        /**
         * @brief Перерисовывает компонент с новыми размерами
         */
        private function redrawComponent():void {
            // Очищаем графику
            graphics.clear();
            
            // Перерисовываем фон
            createBackground();
            
            // Обновляем размеры и позиции элементов
            if (leftArrow) {
                leftArrow.x = 10;
            }
            
            if (rightArrow) {
                rightArrow.x = bgWidth - 40;
            }
            
            if (label) {
                label.width = bgWidth - 100;
                label.height = bgHeight - 10;
                label.x = 50;
            }
            
            // Применяем текущее состояние
            updateVisualState();
        }

        private function updateVisualState():void {
            graphics.clear();
            
            var bgColorARGB:uint = _active ? selectedColor : normalColor;
            var borderColorARGB:uint = _active ? selectedColor : normalColor;
            
            // Извлекаем RGB и альфа из ARGB
            var bgColorRGB:uint = bgColorARGB & 0x00FFFFFF;
            var bgAlpha:Number = ((bgColorARGB >> 24) & 0xFF) / 255.0;
            var borderColorRGB:uint = borderColorARGB & 0x00FFFFFF;
            var borderAlpha:Number = ((borderColorARGB >> 24) & 0xFF) / 255.0;
            
            // Применяем фон с правильной альфой
            if (bgAlpha > 0) {
                graphics.beginFill(bgColorRGB, bgAlpha);
                graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 6, 6);
                graphics.endFill();
            }
            
            // Применяем границу с правильной альфой
            if (borderAlpha > 0) {
                graphics.lineStyle(1, borderColorRGB, borderAlpha);
                graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 6, 6);
            }
        }

        // Обработчики событий
        private function onClick(e:MouseEvent):void {
            var clickX:Number = mouseX;
            
            if (clickX < 50) {
                // Клик по левой стрелке
                switchLeft();
            } else if (clickX > bgWidth - 50) {
                // Клик по правой стрелке
                switchRight();
            }
            
            // Одиночный клик отправляет hover
            log("SwitcherComponent: одиночный клик - отправляем hover");
            dispatchEvent(new Event(EVENT_HOVER));
        }

        private function onDoubleClick(e:MouseEvent):void {
            // Двойной клик отправляет push
            log("SwitcherComponent: двойной клик по '" + _options[_selectedIndex] + "' - отправляем push");
            dispatchEvent(new Event(EVENT_PUSH));
        }

        private function onMouseOver(e:MouseEvent):void {
            // Только визуальное изменение, НЕ отправляем hover события
            if (!_active) {
                graphics.clear();
                
                // Извлекаем RGB и альфа из ARGB hover цвета
                var hoverColorRGB:uint = hoverColor & 0x00FFFFFF;
                var hoverAlpha:Number = ((hoverColor >> 24) & 0xFF) / 255.0;
                
                // Применяем hover цвет с правильной альфой
                if (hoverAlpha > 0) {
                    graphics.beginFill(hoverColorRGB, hoverAlpha);
                    graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 6, 6);
                    graphics.endFill();
                }
                
                // Применяем hover границу
                if (hoverAlpha > 0) {
                    graphics.lineStyle(1, hoverColorRGB, hoverAlpha);
                    graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 6, 6);
                }
            }
        }

        private function onMouseOut(e:MouseEvent):void {
            updateVisualState();
        }

        // ===== МЕТОДЫ УПРАВЛЕНИЯ ЦВЕТАМИ =====

        /**
         * @brief Устанавливает цвета компонента с поддержкой альфа-канала
         * @param normalColorRGBA Обычный цвет фона (ARGB hex)
         * @param textColorRGBA Цвет текста (ARGB hex)
         * @param hoverColorRGBA Цвет при наведении (ARGB hex)
         * @param selectedColorRGBA Цвет выделения (ARGB hex)
         */
        public function setColorsRGBA(normalColorRGBA:uint, textColorRGBA:uint, 
                                     hoverColorRGBA:uint, selectedColorRGBA:uint):void {
            // Сохраняем ПОЛНЫЕ ARGB цвета для правильного применения альфы
            normalColor = normalColorRGBA;
            textColor = textColorRGBA & 0x00FFFFFF;    // Для текста берем только RGB
            hoverColor = hoverColorRGBA;
            selectedColor = selectedColorRGBA;
            
            log("SwitcherComponent: Установка цветов - normalColorRGBA: 0x" + normalColorRGBA.toString(16).toUpperCase() + 
                ", textColorRGBA: 0x" + textColorRGBA.toString(16).toUpperCase() + 
                " -> textColor: 0x" + textColor.toString(16).toUpperCase());
            
            // Применяем цвета
            updateVisualState();
            setTextColor(textColor);
        }

        /**
         * @brief Устанавливает цвета компонента (совместимость, без альфа-канала)
         * @param normalColor Обычный цвет фона (RGB hex)
         * @param textColor Цвет текста (RGB hex)
         * @param hoverColor Цвет при наведении (RGB hex)
         * @param selectedColor Цвет выделения (RGB hex)
         */
        public function setColors(normalColor:uint, textColor:uint, 
                                 hoverColor:uint, selectedColor:uint):void {
            setColorsRGBA(normalColor, textColor, hoverColor, selectedColor);
        }

        /**
         * @brief Устанавливает цвет текста для всех текстовых элементов
         * @param color Цвет в формате 0xRRGGBB
         */
        private function setTextColor(color:uint):void {
            log("SwitcherComponent: setTextColor вызван с цветом 0x" + color.toString(16).toUpperCase());
            
            var textFormat:TextFormat = new TextFormat();
            textFormat.color = color;
            
            if (label) {
                var labelFormat:TextFormat = label.getTextFormat();
                labelFormat.color = color;
                label.setTextFormat(labelFormat);
                log("SwitcherComponent: Цвет текста label установлен в 0x" + color.toString(16).toUpperCase());
            }
            
            if (leftArrow) {
                var leftFormat:TextFormat = leftArrow.getTextFormat();
                leftFormat.color = color;
                leftArrow.setTextFormat(leftFormat);
            }
            
            if (rightArrow) {
                var rightFormat:TextFormat = rightArrow.getTextFormat();
                rightFormat.color = color;
                rightArrow.setTextFormat(rightFormat);
            }
        }
    }
}