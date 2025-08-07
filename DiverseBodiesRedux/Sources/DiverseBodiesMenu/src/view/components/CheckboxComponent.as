package view.components {
    import flash.display.MovieClip;
    import flash.text.TextField;
    import flash.text.TextFormat;
    import flash.events.MouseEvent;
    import flash.events.Event;
    import view.ScrollableMenu;
    /**
     * @brief Интерактивный чекбокс с текстовой меткой
     * 
     * Предоставляет элемент управления типа checkbox с текстовой меткой,
     * визуальной индикацией состояния (отмечен/не отмечен, активен/неактивен)
     * и событиями изменения состояния.
     * 
     * ПРИМЕЧАНИЕ: Если есть конфликт с символами библиотеки, удалите символы
     * ButtonComponent и CheckboxComponent из FLA библиотеки или отвяжите их от классов.
     */
    public class CheckboxComponent extends MovieClip {
        public static const EVENT_CHANGE:String = "eventCheckboxComponentChange";
        public static const EVENT_HOVER:String = "eventCheckboxComponentHover";
        
        private var label:TextField;
        private var checkbox:MovieClip;
        private var _selected:Boolean = false;
        private var _active:Boolean = false;
        private var _labelText:String;
        
        private var checkboxSize:Number = 20;
        private var bgWidth:Number = 500;
        private var bgHeight:Number = 45;

        /**
         * @brief Логирует сообщение через trace и дублирует в C++ через Main
         * @param message Сообщение для логирования
         */
        private function log(message:String):void {
            trace(message);
        }

        /**
         * @brief Создает новый чекбокс с указанным текстом и состоянием
         * @param labelText Текст для отображения рядом с чекбоксом
         * @param initialSelected Начальное состояние (отмечен/не отмечен)
         */
        public function CheckboxComponent(labelText:String = "Checkbox", initialSelected:Boolean = false) {
            this._labelText = labelText;
            this._selected = initialSelected;
            init();
        }

        private function init():void {
            createBackground();
            createCheckbox();
            createLabel();
            addEventListeners();
            updateVisualState();
        }

        private function createBackground():void {
            graphics.beginFill(0x333333, 1);
            graphics.lineStyle(1, 0x666666);
            graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 6, 6);
            graphics.endFill();
        }

        private function createCheckbox():void {
            checkbox = new MovieClip();
            checkbox.x = 10;
            checkbox.y = (bgHeight - checkboxSize) / 2;
            addChild(checkbox);
            drawCheckbox();
        }

        private function createLabel():void {
            label = new TextField();
            label.defaultTextFormat = new TextFormat("Arial", 16, 0xFFFFFF);
            label.text = _labelText;
            label.x = 40;
            label.y = 10;
            label.width = bgWidth - 50;
            label.height = bgHeight - 20;
            label.selectable = false;
            label.mouseEnabled = false;
            addChild(label);
        }

        private function drawCheckbox():void {
            checkbox.graphics.clear();
            
            // Фон чекбокса
            checkbox.graphics.beginFill(0x000000, 1);
            checkbox.graphics.lineStyle(2, 0x999999);
            checkbox.graphics.drawRoundRect(0, 0, checkboxSize, checkboxSize, 3, 3);
            checkbox.graphics.endFill();
            
            // Галочка если выбран
            if (_selected) {
                checkbox.graphics.lineStyle(3, 0x00FF00);
                checkbox.graphics.moveTo(4, 10);
                checkbox.graphics.lineTo(8, 14);
                checkbox.graphics.lineTo(16, 6);
            }
        }

        private function addEventListeners():void {
            this.buttonMode = true;
            this.addEventListener(MouseEvent.CLICK, onClick);
            this.addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
            this.addEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
        }

        // Геттеры и сеттеры
        /**
         * @brief Устанавливает состояние чекбокса (отмечен/не отмечен)
         * @param value true для отмеченного состояния, false для неотмеченного
         */
        public function set selected(value:Boolean):void {
            _selected = value;
            drawCheckbox();
        }

        /**
         * @brief Возвращает текущее состояние чекбокса
         * @return true если отмечен, false если не отмечен
         */
        public function get selected():Boolean {
            return _selected;
        }

        /**
         * @brief Устанавливает активное состояние чекбокса
         * @param value true для активного состояния, false для неактивного
         */
        public function set active(value:Boolean):void {
            _active = value;
            updateVisualState();
        }

        public function get active():Boolean {
            return _active;
        }

        public function get labelText():String {
            return _labelText;
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
            
            // Обновляем размеры label
            if (label) {
                label.width = bgWidth - 50;
                label.height = bgHeight - 10;
            }
            
            // Перерисовываем чекбокс в правильной позиции
            if (checkbox) {
                checkbox.x = bgWidth - 35;
                drawCheckbox();
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
            _selected = !_selected;
            drawCheckbox();
            log("CheckboxComponent: '" + _labelText + "' = " + _selected);
            
            // Отправляем change событие в ScrollableMenu
            log("CheckboxComponent: Dispatching EVENT_CHANGE = '" + EVENT_CHANGE + "'");
            dispatchEvent(new Event(EVENT_CHANGE));
            
            // Отправляем push при установке, back при снятии
            if (_selected) {
                log("CheckboxComponent: чекбокс установлен - отправляем change");
            } else {
                log("CheckboxComponent: чекбокс снят - отправляем change");
            }
            
            // Одиночный клик также отправляет hover
            dispatchEvent(new Event(EVENT_HOVER));
        }

        private function onMouseOver(e:MouseEvent):void {
            if (!_active) {
                graphics.clear();
                graphics.beginFill(hoverColor, 1);
                graphics.lineStyle(1, hoverColor);
                graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 6, 6);
                graphics.endFill();
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
            originalTextColor = textColorRGBA & 0x00FFFFFF; // Для текста берем только RGB
            hoverColor = hoverColorRGBA;
            selectedColor = selectedColorRGBA;
            
            // Применяем цвета
            updateVisualState();
            setTextColor(originalTextColor);
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

        // Свойства для хранения цветов
        private var normalColor:uint = 0x333333;
        private var hoverColor:uint = 0x444444;
        private var selectedColor:uint = 0x0066CC;
        private var originalTextColor:uint = 0xFFFFFF;

        /**
         * @brief Устанавливает цвет текста
         * @param color Цвет в формате 0xRRGGBB
         */
        private function setTextColor(color:uint):void {
            if (label) {
                var textFormat:TextFormat = label.getTextFormat();
                textFormat.color = color;
                label.setTextFormat(textFormat);
            }
        }
    }
}