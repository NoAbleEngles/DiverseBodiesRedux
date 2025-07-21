package view.components {
    import flash.display.MovieClip;
    import flash.text.TextField;
    import flash.text.TextFormat;
    import flash.events.MouseEvent;
    import flash.events.Event;
    import flash.utils.Timer;
    import flash.events.TimerEvent;
    import view.ScrollableMenu;

    /**
     * @brief Интерактивная кнопка с визуальной обратной связью
     * 
     * Предоставляет базовый интерактивный элемент кнопки с текстовой меткой,
     * визуальными состояниями (обычное, активное, hover) и событиями клика.
     * Поддерживает автоматическое изменение внешнего вида при взаимодействии.
     * 
     * ПРИМЕЧАНИЕ: Если есть конфликт с символами библиотеки, удалите символы
     * ButtonComponent и CheckboxComponent из FLA библиотеки или отвяжите их от классов.
     */
    public class ButtonComponent extends MovieClip {
        public static const EVENT_PUSH:String = "eventButtonComponentPush";
        public static const EVENT_HOVER:String = "eventButtonComponentHover";
        
        private var label:TextField;
        private var _active:Boolean = false;
        private var _labelText:String;
        private var bgWidth:Number = 500;
        private var bgHeight:Number = 45;
        private var originalTextColor:uint = 0xFFFFFF;
        
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
        private var pushTimer:Timer;

        /**
         * @brief Создает новую кнопку с указанным текстом
         * @param labelText Текст для отображения на кнопке
         */
        public function ButtonComponent(labelText:String = "Button") {
            this._labelText = labelText;
            init();
        }

        private function init():void {
            createBackground();
            createLabel();
            addEventListeners();
            updateVisualState();
        }

        private function createBackground():void {
            // Создаем фон кнопки
            graphics.beginFill(0x333333, 1);
            graphics.lineStyle(2, 0x666666);
            graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 8, 8);
            graphics.endFill();
        }

        private function createLabel():void {
            label = new TextField();
            label.defaultTextFormat = new TextFormat("Arial", 18, 0xFFFFFF, false, false, false, null, null, "center");
            label.text = _labelText;
            label.x = 5;
            label.y = 10;
            label.width = bgWidth - 10;
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

        // Геттеры и сеттеры
        /**
         * @brief Устанавливает активное состояние кнопки
         * @param value true для активного состояния, false для обычного
         */
        public function set active(value:Boolean):void {
            _active = value;
            updateVisualState();
        }

        /**
         * @brief Возвращает текущее активное состояние кнопки
         * @return true если кнопка активна, false если обычная
         */
        public function get active():Boolean {
            return _active;
        }

        /**
         * @brief Возвращает текущий текст кнопки
         * @return Строка с текстом кнопки
         */
        public function get labelText():String {
            return _labelText;
        }

        /**
         * @brief Устанавливает новый текст кнопки
         * @param value Новый текст для отображения
         */
        public function set labelText(value:String):void {
            _labelText = value;
            if (label) {
                label.text = value;
            }
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
                label.width = bgWidth - 10;
                label.height = bgHeight - 20;
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
                graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 8, 8);
                graphics.endFill();
            }
            
            // Применяем границу с правильной альфой
            if (borderAlpha > 0) {
                graphics.lineStyle(2, borderColorRGB, borderAlpha);
                graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 8, 8);
            }
        }

        // Обработчики событий
        private function onClick(e:MouseEvent):void {
            log("ButtonComponent: клик по '" + _labelText + "' - отправляем hover");
            dispatchEvent(new Event(EVENT_HOVER));
        }

        private function onDoubleClick(e:MouseEvent):void {
            log("ButtonComponent: двойной клик по '" + _labelText + "' - отправляем push");
            dispatchEvent(new Event(EVENT_PUSH));
        }

        private function onMouseOver(e:MouseEvent):void {
            // Только визуальное изменение, НЕ отправляем hover события
            graphics.clear();
            graphics.beginFill(hoverColor, 1);
            graphics.lineStyle(2, hoverColor);
            graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 8, 8);
            graphics.endFill();
        }

        private function onMouseOut(e:MouseEvent):void {
            updateVisualState();
        }

        // ===== МЕТОДЫ ПОДСВЕТКИ =====

        /**
         * @brief Подсвечивает текст зелёным на 0.5 секунды (для push события)
         */
        public function highlightPush():void {
            if (pushTimer) {
                pushTimer.stop();
                pushTimer = null;
            }
            
            setTextColor(0x00FF00); // Зелёный цвет
            
            pushTimer = new Timer(500, 1); // 0.5 секунды
            pushTimer.addEventListener(TimerEvent.TIMER_COMPLETE, onPushTimerComplete);
            pushTimer.start();
        }

        /**
         * @brief Подсвечивает текст жёлтым (для hover события)
         */
        public function highlightHover():void {
            setTextColor(0xFFFF00); // Жёлтый цвет
        }

        /**
         * @brief Убирает подсветку hover (возвращает оригинальный цвет)
         */
        public function clearHover():void {
            setTextColor(originalTextColor);
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
        private var normalColor:uint = 0x2233BB;
        private var hoverColor:uint = 0x0088FF;
        private var selectedColor:uint = 0x00CCFF;

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

        /**
         * @brief Обработчик завершения таймера push-анимации
         */
        private function onPushTimerComplete(e:TimerEvent):void {
            setTextColor(originalTextColor);
            if (pushTimer) {
                pushTimer.removeEventListener(TimerEvent.TIMER_COMPLETE, onPushTimerComplete);
                pushTimer = null;
            }
        }
    }
}
