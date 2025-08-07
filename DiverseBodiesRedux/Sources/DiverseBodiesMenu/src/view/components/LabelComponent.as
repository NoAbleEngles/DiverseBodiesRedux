package view.components {
    import flash.display.MovieClip;
    import flash.text.TextField;
    import flash.text.TextFormat;
    import flash.events.MouseEvent;
    import flash.events.Event;
    import view.ScrollableMenu;

    /**
     * @brief Статический текстовый компонент для отображения заголовков и информации
     * 
     * Предоставляет простой текстовый элемент с минимальной интерактивностью для навигации.
     * Используется для заголовков разделов, информационных сообщений и разделителей в меню.
     * Поддерживает обработку правого клика для события "назад" в меню с только лейблами.
     */
    public class LabelComponent extends MovieClip {
        private var label:TextField;
        private var _labelText:String;
        
        private var bgWidth:Number = 500;
        private var bgHeight:Number = 45;

        /**
         * @brief Логирует сообщение
         * @param message Сообщение для логирования
         */
        private function log(message:String):void {
            trace(message);
        }

        /**
         * @brief Создает новый текстовый компонент с указанным текстом
         * @param labelText Текст для отображения
         */
        public function LabelComponent(labelText:String = "Label") {
            this._labelText = labelText;
            init();
        }

        private function init():void {
            createBackground();
            createLabel();
            addEventListeners();
        }

        private function createBackground():void {
            // Прозрачный фон для лейблов
            graphics.beginFill(0x000000, 0);
            graphics.drawRect(0, 0, bgWidth, bgHeight);
            graphics.endFill();
        }

        private function createLabel():void {
            label = new TextField();
            label.defaultTextFormat = new TextFormat("Arial", 16, 0xCCCCCC, false, false, false, null, null, "left");
            label.text = _labelText;
            label.x = 10;
            label.y = 10;
            label.width = bgWidth - 20;
            label.height = bgHeight - 20;
            label.selectable = false;
            label.mouseEnabled = false;
            addChild(label);
        }

        /**
         * @brief Добавляет обработчики событий мыши для базовой интерактивности
         */
        private function addEventListeners():void {
            // LabelComponent остается неинтерактивным - не добавляем обработчики
        }

        // Геттеры и сеттеры
        /**
         * @brief Возвращает текущий отображаемый текст
         * @return Строка с текстом компонента
         */
        public function get labelText():String {
            return _labelText;
        }

        /**
         * @brief Устанавливает новый текст для отображения
         * @param value Новый текст компонента
         */
        public function set labelText(value:String):void {
            _labelText = value;
            if (label) {
                label.text = value;
            }
        }

        // ===== МЕТОДЫ УПРАВЛЕНИЯ ЦВЕТАМИ =====

        /**
         * @brief Устанавливает цвета компонента с поддержкой альфа-канала
         * @param normalColorRGBA Обычный цвет фона (ARGB hex) - для лейблов обычно прозрачный
         * @param textColorRGBA Цвет текста (ARGB hex)
         * @param hoverColorRGBA Цвет при наведении (ARGB hex) - для лейблов обычно не используется
         * @param selectedColorRGBA Цвет выделения (ARGB hex) - для лейблов обычно не используется
         */
        public function setColorsRGBA(normalColorRGBA:uint, textColorRGBA:uint, 
                                     hoverColorRGBA:uint, selectedColorRGBA:uint):void {
            // Сохраняем только используемые цвета для лейблов
            normalColor = normalColorRGBA & 0x00FFFFFF; // Убираем альфа для RGB
            textColor = textColorRGBA & 0x00FFFFFF;
            // hoverColor и selectedColor не используются в LabelComponent
            
            // Применяем цвета
            updateBackgroundColor();
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

        // Свойства для хранения цветов
        private var normalColor:uint = 0x000000; // Прозрачный для лейблов
        private var textColor:uint = 0xFFFFFF;

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
         * @brief Обновляет цвет фона (для лейблов обычно прозрачный)
         */
        private function updateBackgroundColor():void {
            graphics.clear();
            // Для лейблов фон обычно прозрачный
            var alpha:Number = (normalColor == 0x000000) ? 0 : 0.3; // Если цвет не черный, делаем слегка видимым
            graphics.beginFill(normalColor, alpha);
            graphics.drawRect(0, 0, bgWidth, bgHeight);
            graphics.endFill();
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
                label.width = bgWidth - 20;
                label.height = bgHeight - 10;
            }
            
            // Применяем текущее состояние
            updateVisualState();
        }

        /**
         * @brief Обновляет визуальное состояние компонента (для лейблов просто обновляет фон)
         */
        private function updateVisualState():void {
            updateBackgroundColor();
		}
    }
}