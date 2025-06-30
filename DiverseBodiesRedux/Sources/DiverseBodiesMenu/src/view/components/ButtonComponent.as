package view.components {

    import flash.display.Sprite;
    import flash.text.TextField;
    import flash.text.TextFormat;
    import flash.events.MouseEvent;
    import flash.events.Event;

    /**
     * ButtonComponent — визуальный компонент кнопки для меню.
     * 
     * Этот класс реализует простую кнопку с текстовой меткой, поддерживает смену внешнего вида при наведении мыши,
     * а также обработку кликов. Используется для отображения элементов меню.
     * 
     * Основные свойства:
     * - label:TextField — текстовая метка кнопки.
     * - isActive:Boolean — флаг активности кнопки (можно ли взаимодействовать).
     * 
     * Основные методы:
     * - createButton — создание и стилизация текстовой метки.
     * - drawButton — отрисовка кнопки в обычном состоянии.
     * - drawButtonHover — отрисовка кнопки при наведении мыши.
     * - addEventListeners — добавление обработчиков событий мыши.
     * - onClick, onMouseOver, onMouseOut — обработчики событий мыши.
     * 
     * Использование:
     * var btn:ButtonComponent = new ButtonComponent("Текст кнопки");
     * btn.active = true; // сделать кнопку активной
     */
    public class ButtonComponent extends Sprite {
        public static const EVENT_PUSH:String = "eventButtonComponentPush";
        // Текстовая метка кнопки
        private var label:TextField;
        // Флаг активности кнопки (можно ли кликать)
        private var isActive:Boolean;

        /**
         * Конструктор компонента кнопки.
         * @param labelText — текст, который будет отображаться на кнопке.
         */
        public function ButtonComponent(labelText:String) {
            this.isActive = true;
            createButton(labelText);
            addEventListeners();
        }

        /**
         * Сеттер для свойства активности кнопки.
         * При изменении активности меняется внешний вид кнопки.
         */
        public function set active(value:Boolean):void {
            isActive = value;
            drawButton();
        }

        /**
         * Геттер для свойства активности кнопки.
         * @return true, если кнопка активна.
         */
        public function get active():Boolean {
            return isActive;
        }

        /**
         * Геттер для текста кнопки.
         * @return Строка, отображаемая на кнопке.
         */
        public function get labelText():String {
            return label.text;
        }

        /**
         * Создаёт текстовую метку и настраивает её внешний вид.
         * @param labelText — текст для отображения на кнопке.
         */
        private function createButton(labelText:String):void {
            label = new TextField();
            label.text = labelText;
            // Используем системный шрифт Impact, размер 24, белый цвет
            label.defaultTextFormat = new TextFormat("Impact", 24, 0xFFFFFF);
            label.width = 200;
            label.height = 50;
            label.selectable = false; // Запретить выделение текста
            label.mouseEnabled = false; // Запретить взаимодействие мышью с текстом
            addChild(label);
            drawButton();
        }

        /**
         * Отрисовывает кнопку в обычном состоянии.
         * Цвет зависит от активности кнопки.
         */
        private function drawButton():void {
            graphics.clear();
            graphics.beginFill(isActive ? 0x007ACC : 0xCCCCCC); // Синий или серый
            graphics.drawRect(0, 0, label.width, label.height);
            graphics.endFill();
        }

        /**
         * Добавляет обработчики событий мыши для кнопки.
         * Обрабатываются клик, наведение и уход мыши.
         */
        private function addEventListeners():void {
            this.addEventListener(MouseEvent.CLICK, onClick);
            this.addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
            this.addEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
        }

        /**
         * Обработчик клика по кнопке.
         * Здесь можно реализовать логику действия при нажатии.
         * @param event — объект события мыши.
         */
        private function onClick(event:MouseEvent):void {
            if (isActive) {
                dispatchEvent(new Event(EVENT_PUSH));
            }
        }

        /**
         * Обработчик наведения мыши на кнопку.
         * Меняет внешний вид кнопки при наведении.
         * @param event — объект события мыши.
         */
        private function onMouseOver(event:MouseEvent):void {
            if (isActive) {
                drawButtonHover();
            }
        }

        /**
         * Обработчик ухода мыши с кнопки.
         * Возвращает внешний вид кнопки к обычному состоянию.
         * @param event — объект события мыши.
         */
        private function onMouseOut(event:MouseEvent):void {
            drawButton();
        }

        /**
         * Отрисовывает кнопку в состоянии наведения мыши.
         * Используется другой цвет заливки.
         */
        private function drawButtonHover():void {
            graphics.clear();
            graphics.beginFill(0x005FA3); // Светло-жёлтый или другой цвет для выделения
            graphics.drawRect(0, 0, label.width, label.height);
            graphics.endFill();
        }
    }
}