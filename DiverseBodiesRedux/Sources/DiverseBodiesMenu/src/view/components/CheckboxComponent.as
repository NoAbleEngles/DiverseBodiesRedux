package view.components {

    import flash.display.Sprite;
    import flash.text.TextField;
    import flash.text.TextFormat;
    import flash.events.MouseEvent;
    import flash.events.Event;

    /**
     * CheckboxComponent — визуальный компонент чекбокса для меню.
     * 
     * Компонент состоит из прямоугольного чекбокса и текстового описания справа.
     * Подсвечивается вся строка при наведении мыши.
     * При клике по компоненту меняет состояние (отмечен/не отмечен) и диспатчит событие "change".
     * 
     * Основные свойства:
     * - label:TextField — текстовое описание чекбокса.
     * - checked:Boolean — текущее состояние чекбокса.
     * - isActive:Boolean — флаг активности (можно ли взаимодействовать).
     */
    public class CheckboxComponent extends Sprite {
        public static const EVENT_CHANGE:String = "eventCheckboxComponentChange"; // Событие изменения значения 
        private var label:TextField;
        private var checked:Boolean;
        private var isActive:Boolean;
        private var boxSize:int = 24;
        private var labelOffset:int = 36;

        /**
         * @brief Конструктор компонента CheckboxComponent.
         * @param labelText Текст, который будет отображаться справа от чекбокса.
         * @param initialChecked Начальное состояние чекбокса (отмечен/не отмечен).
         * 
         * Создаёт чекбокс и текстовую метку, подписывает обработчики событий.
         */
        public function CheckboxComponent(labelText:String, initialChecked:Boolean = false) {
            this.checked = initialChecked;
            this.isActive = true;
            createCheckbox(labelText);
            addEventListeners();
            drawCheckbox();
        }

        /**
         * @brief Сеттер для свойства активности компонента.
         * 
         * При изменении активности меняет внешний вид компонента.
         * @param value true — компонент активен, false — неактивен.
         */
        public function set active(value:Boolean):void {
            isActive = value;
            drawCheckbox();
        }

        /**
         * @brief Геттер для свойства активности компонента.
         * @return true, если компонент активен.
         */
        public function get active():Boolean {
            return isActive;
        }

        /**
         * @brief Геттер для состояния чекбокса.
         * @return true, если чекбокс отмечен.
         */
        public function get selected():Boolean {
            return checked;
        }

        /**
         * @brief Сеттер для состояния чекбокса.
         * @param value true — отметить чекбокс, false — снять отметку.
         */
        public function set selected(value:Boolean):void {
            checked = value;
            drawCheckbox();
        }

        /**
         * Геттер для текста кнопки.
         * @return Строка, отображаемая на кнопке.
         */
        public function get labelText():String {
            return label.text;
        }

        /**
         * @brief Создаёт визуальные элементы чекбокса и текстовую метку.
         * @param labelText Текст для отображения справа от чекбокса.
         * 
         * Настраивает форматирование текста, размеры и размещение элементов.
         */
        private function createCheckbox(labelText:String):void {
            // Текстовая метка
            label = new TextField();
            label.defaultTextFormat = new TextFormat("Impact", 24, 0xFFFFFF);
            label.text = labelText;
            label.x = labelOffset;
            label.y = 0;
            label.width = 200 - labelOffset;
            label.height = 50;
            label.selectable = false;
            label.mouseEnabled = false;
            addChild(label);
        }

        /**
         * @brief Добавляет обработчики событий мыши для компонента.
         * 
         * Обрабатывает наведение, уход мыши и клики по компоненту.
         */
        private function addEventListeners():void {
            this.addEventListener(MouseEvent.CLICK, onClick);
            this.addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
            this.addEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
        }

        /**
         * @brief Обработчик клика по компоненту.
         * 
         * Меняет состояние чекбокса (отмечен/не отмечен), обновляет отображение.
         * @param event Событие мыши.
         */
        private function onClick(event:MouseEvent):void {
            if (isActive) {
                setChecked(!checked)
            }
        }

        private function setChecked(bool:Boolean):void {
            checked = bool;
            drawCheckbox();
            dispatchEvent(new Event(EVENT_CHANGE));
        }

        /**
         * @brief Обработчик наведения мыши на компонент.
         * 
         * Подсвечивает всю строку (чекбокс и текст) при наведении, если компонент активен.
         * @param event Событие мыши.
         */
        private function onMouseOver(event:MouseEvent):void {
            if (isActive) {
                drawCheckboxHover();
            }
        }

        /**
         * @brief Обработчик ухода мыши с компонента.
         * 
         * Возвращает внешний вид компонента к обычному состоянию.
         * @param event Событие мыши.
         */
        private function onMouseOut(event:MouseEvent):void {
            drawCheckbox();
        }

        /**
         * @brief Отрисовывает компонент в обычном состоянии.
         * 
         * Рисует прямоугольник чекбокса, галочку (если отмечен) и текстовую метку.
         */
        private function drawCheckbox():void {
            graphics.clear();
            // Фон всей строки
            graphics.beginFill(isActive ? 0x007ACC : 0xCCCCCC);
            graphics.drawRect(0, 0, 200, 50);
            graphics.endFill();

            // Чекбокс
            graphics.lineStyle(2, 0xFFFFFF);
            graphics.beginFill(0x000000, 0.7);
            graphics.drawRect(6, 13, boxSize, boxSize);
            graphics.endFill();

            // Галочка, если отмечен
            if (checked) {
                graphics.lineStyle(4, 0xFFFFFF);
                graphics.moveTo(12, 25);
                graphics.lineTo(18, 35);
                graphics.lineTo(28, 17);
            }
        }

        /**
         * @brief Отрисовывает компонент в состоянии наведения мыши.
         * 
         * Использует цвет подсветки для всей строки, чекбокс и галочка рисуются так же.
         */
        private function drawCheckboxHover():void {
            graphics.clear();
            // Фон всей строки подсвечен
            graphics.beginFill(0xFFD700, 1); // Светло-жёлтый
            graphics.drawRect(0, 0, 200, 50);
            graphics.endFill();

            // Чекбокс
            graphics.lineStyle(2, 0xFFFFFF);
            graphics.beginFill(0x000000, 0.7);
            graphics.drawRect(6, 13, boxSize, boxSize);
            graphics.endFill();

            // Галочка, если отмечен
            if (checked) {
                graphics.lineStyle(4, 0xFFFFFF);
                graphics.moveTo(12, 25);
                graphics.lineTo(18, 35);
                graphics.lineTo(28, 17);
            }
        }
    }
}