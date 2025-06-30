package view.components {
    import flash.display.Sprite;
    import flash.text.TextField;
    import flash.text.TextFormat;
    import flash.events.MouseEvent;
    import flash.events.Event;

    /**
     * SwitcherComponent — компонент-переключатель для меню.
     * Состоит из трёх областей: "<" [значение] ">".
     * При наведении подсвечивается вся область, а "<" и ">" скрываются на крайних позициях.
     * При изменении положения диспатчит событие "change" с новым индексом.
     */
    public class SwitcherComponent extends Sprite {
        public static const EVENT_CHANGE:String = "eventSwitcherComponentChange"; // Событие изменения значения   
        private var options:Array;
        private var currentIndex:int;
        private var label:TextField;
        private var leftArrow:TextField;
        private var rightArrow:TextField;
        private var isActive:Boolean = true;

        /**
         * @brief Конструктор компонента SwitcherComponent.
         * @param options Массив значений для переключения.
         * @param startIndex Начальный индекс выбранного значения.
         * 
         * Создаёт визуальные элементы переключателя, подписывает обработчики событий и отрисовывает компонент.
         */
        public function SwitcherComponent(options:Array, startIndex:int = 0) {
            this.options = options;
            this.currentIndex = startIndex;
            createSwitcher();
            addEventListeners();
            drawSwitcher();
        }

        /**
         * @brief Сеттер для свойства активности компонента.
         * При изменении активности меняет внешний вид компонента.
         * @param value true — компонент активен, false — неактивен.
         */
        public function set active(value:Boolean):void {
            isActive = value;
            drawSwitcher();
        }

        /**
         * @brief Геттер для свойства активности компонента.
         * @return true, если компонент активен.
         */
        public function get active():Boolean {
            return isActive;
        }

        /**
         * @brief Геттер для текущего значения опции.
         * @return Текущее выбранное значение из массива options.
         */
        public function get value():* {
            return options[currentIndex];
        }

        /**
         * @brief Геттер для текущего индекса выбранного значения.
         * @return Индекс текущего значения.
         */
        public function get index():int {
            return currentIndex;
        }

        /**
         * @brief Геттер для текста текущего значения.
         * @return Строка, отображаемая по центру между стрелками.
         */
        public function get labelText():String {
            return label.text;
        }

        // ----- Приватные методы -----

        /**
         * @brief Создаёт визуальные элементы переключателя: левую стрелку, текст значения и правую стрелку.
         * Настраивает форматирование текста, размеры и размещение всех частей компонента.
         */
        private function createSwitcher():void {
            // Левая стрелка
            leftArrow = new TextField();
            leftArrow.defaultTextFormat = new TextFormat("Impact", 24, 0xFFFFFF);
            leftArrow.text = "<";
            leftArrow.width = 30;
            leftArrow.height = 50;
            leftArrow.selectable = false;
            leftArrow.mouseEnabled = false;
            addChild(leftArrow);

            // Значение опции
            label = new TextField();
            label.defaultTextFormat = new TextFormat("Impact", 24, 0xFFFFFF);
            label.text = options[currentIndex];
            label.width = 140;
            label.height = 50;
            label.x = 30;
            label.selectable = false;
            label.mouseEnabled = false;
            addChild(label);

            // Правая стрелка
            rightArrow = new TextField();
            rightArrow.defaultTextFormat = new TextFormat("Impact", 24, 0xFFFFFF);
            rightArrow.text = ">";
            rightArrow.width = 30;
            rightArrow.height = 50;
            rightArrow.x = 170;
            rightArrow.selectable = false;
            rightArrow.mouseEnabled = false;
            addChild(rightArrow);
        }

        /**
         * @brief Добавляет обработчики событий мыши для компонента.
         * Обрабатывает наведение, уход мыши и клики по компоненту.
         */
        private function addEventListeners():void {
            this.addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
            this.addEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
            this.addEventListener(MouseEvent.CLICK, onClick);
        }

        /**
         * @brief Обработчик события наведения мыши на компонент.
         * При наведении подсвечивает весь компонент, если он активен.
         * @param e Событие мыши.
         */
        private function onMouseOver(e:MouseEvent):void {
            if (isActive) drawSwitcherHover();
        }

        /**
         * @brief Обработчик события ухода мыши с компонента.
         * Возвращает компонент к обычному виду.
         * @param e Событие мыши.
         */
        private function onMouseOut(e:MouseEvent):void {
            drawSwitcher();
        }

        /**
         * @brief Переключает значение влево (на предыдущий элемент).
         * Если возможно, уменьшает индекс, обновляет отображение и диспатчит событие "change".
         */
        public function switchLeft():void {
            if (!isActive) return;
            if (currentIndex > 0) {
                currentIndex--;
                updateLabel();
                drawSwitcherHover();
                dispatchEvent(new Event(EVENT_CHANGE));
            }
        }

        /**
         * @brief Переключает значение вправо (на следующий элемент).
         * Если возможно, увеличивает индекс, обновляет отображение и диспатчит событие "change".
         */
        public function switchRight():void {
            if (!isActive) return;
            if (currentIndex < options.length - 1) {
                currentIndex++;
                updateLabel();
                drawSwitcherHover();
                dispatchEvent(new Event(EVENT_CHANGE));
            }
        }

        /**
         * @brief Обработчик клика по компоненту.
         * Если клик был по левой или правой области — вызывает соответствующий метод переключения.
         * @param e Событие мыши.
         */
        private function onClick(e:MouseEvent):void {
            if (!isActive) return;
            var localX:Number = mouseX;
            if (localX < 30) {
                switchLeft();
            } else if (localX > 170) {
                switchRight();
            }
        }

        /**
         * @brief Обновляет текст значения и видимость стрелок.
         * Скрывает левую стрелку на крайнем левом положении и правую — на крайнем правом.
         */
        private function updateLabel():void {
            label.text = options[currentIndex];
            leftArrow.visible = currentIndex > 0;
            rightArrow.visible = currentIndex < options.length - 1;
        }

        /**
         * @brief Отрисовывает компонент в обычном состоянии.
         * Использует основной цвет заливки и обновляет текст/стрелки.
         */
        private function drawSwitcher():void {
            graphics.clear();
            graphics.beginFill(isActive ? 0x007ACC : 0xCCCCCC, 1);
            graphics.drawRect(0, 0, 200, 50);
            graphics.endFill();
            updateLabel();
        }

        /**
         * @brief Отрисовывает компонент в состоянии наведения мыши.
         * Использует цвет подсветки и обновляет текст/стрелки.
         */
        private function drawSwitcherHover():void {
            graphics.clear();
            graphics.beginFill(0xFFD700, 1); // Светло-жёлтый
            graphics.drawRect(0, 0, 200, 50);
            graphics.endFill();
            updateLabel();
        }
    }
}