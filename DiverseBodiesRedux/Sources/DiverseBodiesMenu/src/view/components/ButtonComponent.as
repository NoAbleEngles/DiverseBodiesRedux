package view.components {
    import flash.display.MovieClip;
    import flash.text.TextField;
    import flash.text.TextFormat;
    import flash.events.MouseEvent;
    import flash.events.Event;
    import view.ScrollableMenu;
    import view.Logger;

    /**
     * Интерактивная кнопка с визуальной обратной связью
     * 
     * Предоставляет базовый интерактивный элемент кнопки с текстовой меткой,
     * визуальными состояниями (обычное, активное, hover) и событиями клика.
     * Поддерживает автоматическое изменение внешнего вида при взаимодействии
     * и применение цветовых тем через ThemeManager.
     * 
     * События:
     * - EVENT_HOVER: одиночный клик (выбор элемента)
     * - EVENT_PUSH: двойной клик (активация элемента)
     * 
     * Поддерживаемые взаимодействия:
     * - Клавиша Enter/Right Arrow: активация
     * - Клавиша Left Arrow/ESC: возврат назад
     * - Автоматическая прокрутка при навигации клавиатурой
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
         * Логирует сообщение через централизованный Logger
         * @param message Сообщение для логирования
         */
        private function log(message:String):void {
            Logger.getInstance().debug(message, "ButtonComponent");
        }

        /**
         * Создает новую кнопку с указанным текстом
         * @param labelText Текст для отображения на кнопке
         */
        public function ButtonComponent(labelText:String = "Button") {
            this._labelText = labelText;
            init();
        }

        /**
         * Инициализирует компонент
         */
        private function init():void {
            createBackground();
            createLabel();
            addEventListeners();
            updateVisualState();
            
            this.visible = true;
            this.alpha = 1.0;
            
            this.addEventListener(Event.ADDED_TO_STAGE, onAddedToStage);
        }
        
        /**
         * Обработчик добавления на сцену
         */
        private function onAddedToStage(e:Event):void {
            this.removeEventListener(Event.ADDED_TO_STAGE, onAddedToStage);
        }

        /**
         * Создает фон кнопки
         */
        private function createBackground():void {
            graphics.beginFill(0x333333, 1);
            graphics.lineStyle(2, 0x666666);
            graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 8, 8);
            graphics.endFill();
        }

        /**
         * Создает текстовое поле
         */

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

        /**
         * Добавляет обработчики событий мыши
         */
        private function addEventListeners():void {
            this.buttonMode = true;
            this.doubleClickEnabled = true;
            this.mouseEnabled = true;
            this.mouseChildren = false; // Предотвращаем конфликты с TextField
            this.addEventListener(MouseEvent.CLICK, onClick);
            this.addEventListener(MouseEvent.DOUBLE_CLICK, onDoubleClick);
            this.addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
            this.addEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
            this.addEventListener(MouseEvent.MOUSE_DOWN, onMouseDown);
            this.addEventListener(MouseEvent.MOUSE_UP, onMouseUp);
        }

        // ===== ГЕТТЕРЫ И СЕТТЕРЫ =====
        
        /**
         * Устанавливает активное состояние кнопки
         * @param value true для активного состояния, false для обычного
         */
        public function set active(value:Boolean):void {
            _active = value;
            updateVisualState();
        }

        /**
         * Возвращает текущее активное состояние кнопки
         * @return true если кнопка активна, false если обычная
         */
        public function get active():Boolean {
            return _active;
        }

        /**
         * Возвращает текущий текст кнопки
         * @return Строка с текстом кнопки
         */
        public function get labelText():String {
            return _labelText;
        }

        /**
         * Устанавливает новый текст кнопки
         * @param value Новый текст для отображения
         */
        public function set labelText(value:String):void {
            _labelText = value;
            if (label) {
                label.text = value;
            }
        }

        /**
         * Устанавливает ширину компонента
         * @param value Новая ширина
         */
        override public function set width(value:Number):void {
            if (value > 0 && value !== bgWidth) {
                bgWidth = value;
                redrawComponent();
            }
        }
        
        /**
         * Возвращает текущую ширину компонента
         */
        override public function get width():Number {
            return bgWidth;
        }
        
        /**
         * Устанавливает высоту компонента
         * @param value Новая высота
         */
        override public function set height(value:Number):void {
            if (value > 0 && value !== bgHeight) {
                bgHeight = value;
                redrawComponent();
            }
        }
        
        /**
         * Возвращает текущую высоту компонента
         */
        override public function get height():Number {
            return bgHeight;
        }
        
        /**
         * Перерисовывает компонент с новыми размерами
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

        /**
         * Обновляет визуальное состояние кнопки
         */
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

        // ===== ОБРАБОТЧИКИ СОБЫТИЙ =====
        
        /**
         * Обработчик одиночного клика по кнопке
         */
        private function onClick(e:MouseEvent):void {
            log("ButtonComponent: одиночный клик по '" + _labelText + "' - отправляем EVENT_HOVER (выбор)");
            dispatchEvent(new Event(EVENT_HOVER));
        }

        /**
         * Обработчик двойного клика по кнопке
         */
        private function onDoubleClick(e:MouseEvent):void {
            log("ButtonComponent: двойной клик по '" + _labelText + "' - отправляем EVENT_PUSH (активация)");
            dispatchEvent(new Event(EVENT_PUSH));
        }

        /**
         * Обработчик наведения мыши на кнопку
         */
        private function onMouseOver(e:MouseEvent):void {
            // Только визуальное изменение, НЕ отправляем hover события
            graphics.clear();
            graphics.beginFill(hoverColor, 1);
            graphics.lineStyle(2, hoverColor);
            graphics.drawRoundRect(0, 0, bgWidth, bgHeight, 8, 8);
            graphics.endFill();
        }

        /**
         * Обработчик ухода мыши с кнопки
         */
        private function onMouseOut(e:MouseEvent):void {
            updateVisualState();
        }

        /**
         * Обработчик нажатия мыши на кнопку
         */
        private function onMouseDown(e:MouseEvent):void {
            // Начало нажатия
        }

        /**
         * Обработчик отпускания мыши
         */
        private function onMouseUp(e:MouseEvent):void {
            // Завершение нажатия
        }

        /**
         * Подсвечивает текст жёлтым цветом для hover события
         */
        public function highlightHover():void {
            setTextColor(0xFFFF00); // Жёлтый цвет
        }

        /**
         * Убирает подсветку hover и возвращает оригинальный цвет
         */
        public function clearHover():void {
            setTextColor(originalTextColor);
        }

        // ===== МЕТОДЫ УПРАВЛЕНИЯ ЦВЕТАМИ =====

        /**
         * Устанавливает цвета компонента с поддержкой альфа-канала
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
         * Устанавливает цвета компонента для совместимости без альфа-канала
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
         * Устанавливает цвет текста
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
