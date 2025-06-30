package view {
    import flash.display.Sprite;
    import flash.events.KeyboardEvent;
    import flash.events.Event;
    import view.components.LabelComponent;
    import view.components.ButtonComponent;
    import view.components.CheckboxComponent;
    import view.components.SwitcherComponent;

    /**
     * MenuView — визуальное представление меню для взаимодействия с C++ бэк-эндом.
     * 
     * Класс принимает массив элементов меню, отображает их на экране, отслеживает взаимодействие пользователя
     * и отправляет кастомные сигналы для C++ (через GFxBridge или другой механизм).
     * 
     * Поддерживаемые типы элементов:
     * 0 — label (только текст, не выделяется, не реагирует на ввод)
     * 1 — button 
     * 2 — checkbox 
     * 3 — switcher 
     */
    public class MenuView extends Sprite {
        public static const EVENT_BACK:String = "eventBack";
        public static const EVENT_PUSH:String = "eventPush";
        
        // Массив для хранения всех визуальных элементов меню
        private var menuElements:Array = [];
        // Массив исходных данных (тип, текст)
        private var menuData:Array = [];
        private var activeEl:int = -1; // Индекс активного элемента

        /**
         * @brief Конструктор MenuView.
         * @param items Массив элементов меню вида [{type:uint, labelText:String}, ...]
         * 
         * Создаёт визуальные элементы меню по переданному массиву.
         */
        public function MenuView(items:Array = null) {
            initialize();
            if (items) {
                renderMenu(items);
            }
        }

        /**
         * @brief Отрисовывает меню по массиву элементов.
         * Очищает предыдущие элементы и создаёт новые.
         * @param items Массив объектов вида {type:uint, labelText:String}
         */
        public function renderMenu(items:Array, activeElement:int = 0):void {
            clearMenuElements();
            this.menuData = items;
            var yPos:Number = 0;
            for (var i:int = 0; i < items.length; i++) {
                var item:Object = items[i];
                var type:uint = item.type;
                var label:String = item.labelText;
                var element:Sprite;

                switch (type) {
                    case 0: // label
                        element = createLabel(label, yPos);
                        break;
                    case 1: // button
                        element = createButton(label, yPos, i);
                        break;
                    case 2: // checkbox
                        element = createCheckbox(label, yPos, i);
                        break;
                    case 3: // switcher
                        element = createSwitcher(label, yPos, i);
                        break;
                }
                if (element) {
                    if (i == activeElement && type != 0 && "active" in element) {
                        element["active"] = true;
                    } 
                    addChild(element);
                    menuElements.push(element);
                    yPos += 54; // Отступ между элементами
                }
            }
        }

        /**
         * @brief Очищает все визуальные элементы меню.
         * Удаляет все элементы из контейнера и очищает массив menuElements.
         */
        public function clearMenuElements():void {
            for each (var el:Sprite in menuElements) {
                if (el.parent) el.parent.removeChild(el);
            }
            menuElements = [];
        }

        // ----- Приватные методы -----

        /**
         * @brief Инициализация подписки на события клавиатуры и мыши.
         * Обрабатывает навигацию и ввод с клавиатуры, а также клики мыши.
         */
        private function initialize():void {
            stage.addEventListener(KeyboardEvent.KEY_DOWN, onKeyDown);
        }

        /**
         * @brief Создаёт визуальный элемент LabelComponent (только текст).
         * Не выделяется, не реагирует на ввод.
         * @param text Текст для отображения.
         * @param y Позиция по Y.
         * @return Экземпляр LabelComponent.
         */
        private function createLabel(text:String, y:Number):LabelComponent {
            var labelComp:LabelComponent = new LabelComponent(text);
            labelComp.x = 0;
            labelComp.y = y;
            return labelComp;
        }

        /**
         * @brief Создаёт кнопку ButtonComponent.
         * Подписывает на обработку клика и наведения.
         * @param label Текст кнопки.
         * @param y Позиция по Y.
         * @param idx Индекс элемента в массиве меню.
         * @return Экземпляр ButtonComponent.
         */
        private function createButton(label:String, y:Number, idx:int):ButtonComponent {
            var btn:ButtonComponent = new ButtonComponent(label);
            btn.x = 0;
            btn.y = y;
            btn.addEventListener(ButtonComponent.EVENT_PUSH, function(e:Event):void {
                sendButtonPushed(idx);
            });
            return btn;
        }

        /**
         * @brief Создаёт чекбокс CheckboxComponent.
         * Подписывает на обработку клика и наведения.
         * @param label Текст чекбокса.
         * @param y Позиция по Y.
         * @param idx Индекс элемента в массиве меню.
         * @return Экземпляр CheckboxComponent.
         */
        private function createCheckbox(label:String, y:Number, idx:int):CheckboxComponent {
            var cb:CheckboxComponent = new CheckboxComponent(label, false);
            cb.x = 0;
            cb.y = y;
            cb.addEventListener(CheckboxComponent.EVENT_CHANGE, function(e:Event):void {
                sendChekboxPushed(idx, cb.selected);
            });
            return cb;
        }

        /**
         * @brief Создаёт переключатель SwitcherComponent.
         * Разделяет строку labelText по запятой, создаёт массив значений.
         * Реагирует только на "<", ">", стрелки влево/вправо.
         * @param labelText Строка с вариантами, разделёнными запятой.
         * @param y Позиция по Y.
         * @param idx Индекс элемента в массиве меню.
         * @return Экземпляр SwitcherComponent.
         */
        private function createSwitcher(labelText:String, y:Number, idx:int):SwitcherComponent {
            var arraySwitcherLabels:Array = labelText.split(",");
            var sw:SwitcherComponent = new SwitcherComponent(arraySwitcherLabels, 0);
            sw.x = 0;
            sw.y = y;
            sw.addEventListener(SwitcherComponent.EVENT_CHANGE, function(e:Event):void {
                sendSwitcherPushed(idx, sw.index);
            });
            return sw;
        }

        /**
         * @brief Обработчик нажатий клавиш.
         * Реализует навигацию и ввод для элементов меню.
         * @param e Событие клавиатуры.
         */
        private function onKeyDown(e:KeyboardEvent):void {
            // Обработка клавиш для навигации и взаимодействия с меню
            switch (e.keyCode) {
                case 38: // Вверх
                    processKeyUp();
                    break;
                case 40: // Вниз
                    processKeyDown();
                    break;
                case 37: // Влево
                    processKeyLeft();
                    break;
                case 39: // Вправо
                    processKeyRight(); 
                    break;       
                case 13: // Enter
                    processKeyENTER();
                    break;
                case 27: // Escape
                    processKeyESC();
                    break;
            }
        }

        /**
         * @brief Отправляет сигнал о нажатии кнопки.
         * @param num Индекс кнопки в массиве меню.
         */
        private function sendButtonPushed(num:uint):void {
            // Здесь вызовите bridge или другой механизм для отправки события в C++
            trace("sendButtonPushed", num);
        }

        /**
         * @brief Отправляет сигнал о нажатии чекбокса.
         * @param num Индекс чекбокса в массиве меню.
         * @param checked Текущее состояние чекбокса.
         */
        private function sendChekboxPushed(num:uint, checked:Boolean):void {
            // Здесь вызовите bridge или другой механизм для отправки события в C++
            trace("sendChekboxPushed", num, checked);
        }

        /**
         * @brief Отправляет сигнал о переключении switcher.
         * @param num Индекс switcher в массиве меню.
         * @param currentArrayId Индекс выбранного значения в массиве switcher.
         */
        private function sendSwitcherPushed(num:uint, currentArrayId:uint):void {
            // Здесь вызовите bridge или другой механизм для отправки события в C++
            trace("sendSwitcherPushed", num, currentArrayId);
        }

        private function updateActiveElement(newActiveEl:int):void {
            var newElement:Sprite = menuElements[activeEl];
            if (!("active" in newElement) || !newElement["active"]) {
                // Если новый элемент не поддерживает активность, сбросить индекс
                activeEl = -1;
                return;
            }

            if (activeEl >= 0 && activeEl < menuElements.length) {
                // Сбросить активность предыдущего элемента
                var prevEl:Sprite = menuElements[activeEl];
                if ("active" in prevEl) {
                    prevEl["active"] = false;
                }
            }
            
            activeEl = newActiveEl;
            if (activeEl >= 0 && activeEl < menuElements.length) {
                // Установить активность нового элемента
                var newEl:Sprite = menuElements[activeEl];
                if ("active" in newEl) {
                    newEl["active"] = true;
                }
            }
        }

        private function processKeyUp():void {
            if (menuElements.length == 0) return;

            var startIdx:int = activeEl;
            var idx:int = activeEl;
            var found:Boolean = false;

            do {
                idx--;
                if (idx < 0) {
                    idx = menuElements.length - 1; // Циклический переход к последнему элементу
                }
                // Если не LabelComponent — нашли активируемый элемент
                if (!(menuElements[idx] is LabelComponent)) {
                    found = true;
                    break;
                }
            } while (idx != startIdx);

            if (found) {
                updateActiveElement(idx);
            } else {
                // Все элементы — LabelComponent, не меняем активный элемент
                trace("Нет доступных для выбора элементов");
            }
            trace("Key Up");
        }

        private function processKeyDown():void {
            if (menuElements.length == 0) return;

            var startIdx:int = activeEl;
            var idx:int = activeEl;
            var found:Boolean = false;

            do {
                idx++;
                if (idx >= menuElements.length) {
                    idx = 0; // Циклический переход к первому элементу
                }
                // Если не LabelComponent — нашли активируемый элемент
                if (!(menuElements[idx] is LabelComponent)) {
                    found = true;
                    break;
                }
            } while (idx != startIdx);

            if (found) {
                updateActiveElement(idx);
            } else {
                // Все элементы — LabelComponent, не меняем активный элемент
                trace("Нет доступных для выбора элементов");
            }
            trace("Key Down");
        }

        private function processKeyLeft():void {
            if (menuElements[activeEl] is SwitcherComponent) {
                var tmp_sc:SwitcherComponent = menuElements[activeEl] as SwitcherComponent;
                tmp_sc.switchRight();
                dispatchEvent(new Event(EVENT_PUSH, activeEl, tmp_sc["index"]));
            } else if (menuElements[activeEl] is CheckboxComponent) {
                var tmp_cb:CheckboxComponent = menuElements[activeEl] as CheckboxComponent;
                tmp_cb["checked"] = false; // Переключаем состояние чекбокса
                dispatchEvent(new Event(EVENT_PUSH, activeEl, tmp_cb["checked"]));
            } else {
                processKeyESC();
            }
        }

        private function processKeyRight():void {
            if (menuElements[activeEl] is SwitcherComponent) {
                var tmp_sc:SwitcherComponent = menuElements[activeEl] as SwitcherComponent;
                tmp_sc.switchRight();
                dispatchEvent(new Event(EVENT_PUSH, activeEl, tmp_sc["index"]));
            } else if (menuElements[activeEl] is CheckboxComponent) {
                var tmp_cb:CheckboxComponent = menuElements[activeEl] as CheckboxComponent;
                tmp_cb["checked"] = true; // Переключаем состояние чекбокса
                dispatchEvent(new Event(EVENT_PUSH, activeEl, tmp_cb["checked"]));
            } else {
                processKeyENTER();
            }
        }

        /**
         * @brief Обрабатывает нажатие клавиши Escape и приравненные к ней.
         * Отправляет кастомный сигнал о возврате/выходе из меню для C++ бэк-энда.
         */
        private function processKeyESC():void {
            dispatchEvent(new Event(EVENT_BACK));
            trace("processKeyESC");
        }

         /**
         * @brief Обрабатывает нажатие клавиши Enter и приравненные к ней.
         * Отправляет кастомный сигнал о выборе пункта меню для C++ бэк-энда.
         */
        private function processKeyENTER():void {
            dispatchEvent(new Event(EVENT_PUSH, activeEl));
            trace("processKeyENTER");
        } 
    }
}