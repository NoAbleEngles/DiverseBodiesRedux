package view.components {

    import flash.display.Sprite;
    import flash.text.TextField;
    import flash.text.TextFormat;

    /**
     * LabelComponent — визуальный компонент для отображения статического текста в меню.
     *
     * @brief
     * - Используется для вывода заголовков, описаний и других неинтерактивных элементов меню.
     * - Не выделяется при наведении мыши.
     * - Не реагирует на клики и не отправляет никаких событий.
     * - Не участвует в навигации по элементам меню.
     */
    public class LabelComponent extends Sprite {
        private var label:TextField;

        /**
         * @brief Конструктор компонента LabelComponent.
         * @param labelText Текст, который будет отображаться.
         *
         * Создаёт текстовую метку, настраивает её внешний вид и размещает на сцене.
         */
        public function LabelComponent(labelText:String) {            label = new TextField();
            label.text = labelText;
            label.defaultTextFormat = new TextFormat("Impact", 24, 0xFFFFFF);
            label.width = 200;
            label.height = 50;
            label.selectable = false;
            label.mouseEnabled = false;
            addChild(label);
        }

        /**
         * @brief Геттер для текста метки.
         * @return Строка, отображаемая на метке.
         */
        public function get labelText():String {
            return label.text;
        }
    }
}