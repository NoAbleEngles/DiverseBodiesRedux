package view {
    import flash.events.EventDispatcher;
    import flash.events.Event;
    import flash.display.MovieClip;
    import fl.containers.ScrollPane;
    import flash.geom.Rectangle;
    import flash.display.Stage;
    
    /**
     * @brief Централизованный менеджер масштабирования с паттерном Observer
     * 
     * Управляет масштабом всех компонентов меню и уведомляет подписчиков об изменениях.
     * Инициализируется стандартным масштабом по умолчанию.
     * 
     * События:
     * - MenuScaler.SCALE_CHANGED: изменение масштаба
     */
    public class MenuScaler extends EventDispatcher {
        // События
        public static const SCALE_CHANGED:String = "scaleChanged";
        
        // Singleton instance
        private static var _instance:MenuScaler;
        
        // Параметры масштабирования
        private static const DEFAULTS:Object = {
            globalScale: 1.0,
            itemHeight: 50,
            itemSpacing: 4,
            containerWidth: 550,
            horizontalMargin: 5,
            verticalMargin: 10
        };
        private var globalScale:Number = DEFAULTS.globalScale;
        private var currentScale:Number = DEFAULTS.globalScale;
        private var itemHeight:Number = DEFAULTS.itemHeight;
        private var itemSpacing:Number = DEFAULTS.itemSpacing;
        private var containerWidth:Number = DEFAULTS.containerWidth;
        private var horizontalMargin:Number = DEFAULTS.horizontalMargin;
        private var verticalMargin:Number = DEFAULTS.verticalMargin;
        
        // Базовые разрешения для масштабирования
        private var baseWidth:Number = 1920;
        private var baseHeight:Number = 1080;

        // Контейнер для масштабирования
        private var menuContainer:ScrollPane;
        
        // Флаг инициализации
        private var isInitialized:Boolean = false;
        
        /**
         * @brief Получает singleton instance MenuScaler
         */
        public static function getInstance():MenuScaler {
            if (!_instance) {
                _instance = new MenuScaler();
                _instance.initializeDefaults();
            }
            return _instance;
        }
        
        public function MenuScaler() {
            super();
            if (_instance) {
                throw new Error("MenuScaler является синглтоном. Используйте getInstance()");
            }
        }
        
        /**
         * @brief Инициализирует параметры масштабирования по умолчанию
         */
        public function initializeDefaults():void {
            log("MenuScaler: Инициализация параметров по умолчанию");
            globalScale      = DEFAULTS.globalScale;
            itemHeight       = DEFAULTS.itemHeight;
            itemSpacing      = DEFAULTS.itemSpacing;
            containerWidth   = DEFAULTS.containerWidth;
            horizontalMargin = DEFAULTS.horizontalMargin;
            verticalMargin   = DEFAULTS.verticalMargin;
            isInitialized = true;
            dispatchEvent(new Event(SCALE_CHANGED));
            log("MenuScaler: Параметры по умолчанию инициализированы");
        }
        
        /**
         * @brief Устанавливает глобальный масштаб (вызывается из C++)
         */
        public function setGlobalScale(scale:Number):void {
            if (!isInitialized) {
                initializeDefaults();
            }
            
            if (scale <= 0) {
                log("MenuScaler: Неверный масштаб: " + scale + ", используется 1.0");
                scale = 1.0;
            }
            
            globalScale = scale;
            
            log("MenuScaler: Глобальный масштаб установлен: " + scale);
            
            // Уведомляем подписчиков об изменении масштаба
            dispatchEvent(new Event(SCALE_CHANGED));
        }
        
        /**
         * @brief Устанавливает размеры элементов (вызывается из C++)
         */
        public function setItemDimensions(height:Number, spacing:Number, width:Number):void {
            if (!isInitialized) {
                initializeDefaults();
            }
            
            if (height > 0) itemHeight = height;
            if (spacing >= 0) itemSpacing = spacing;
            if (width > 0) containerWidth = width;
            
            log("MenuScaler: Размеры элементов обновлены - высота: " + itemHeight + 
                ", отступ: " + itemSpacing + ", ширина: " + containerWidth);
            
            // Уведомляем подписчиков об изменении масштаба
            dispatchEvent(new Event(SCALE_CHANGED));
        }
        
        /**
         * @brief Устанавливает отступы (вызывается из C++)
         */
        public function setMargins(horizontal:Number, vertical:Number):void {
            if (!isInitialized) {
                initializeDefaults();
            }
            
            if (horizontal >= 0) horizontalMargin = horizontal;
            if (vertical >= 0) verticalMargin = vertical;
            
            log("MenuScaler: Отступы обновлены - горизонтальные: " + horizontalMargin + 
                ", вертикальные: " + verticalMargin);
            
            // Уведомляем подписчиков об изменении масштаба
            dispatchEvent(new Event(SCALE_CHANGED));
        }
        
        /**
         * @brief Сбрасывает все параметры к умолчаниям
         */
        public function resetToDefaults():void {
            log("MenuScaler: Сброс параметров к умолчаниям");
            initializeDefaults();
        }
        
        /**
         * @brief Получает масштабированную высоту элемента
         */
        public function getScaledItemHeight():Number {
            if (!isInitialized) {
                initializeDefaults();
            }
            return itemHeight * globalScale;
        }
        
        /**
         * @brief Получает масштабированный отступ между элементами
         */
        public function getScaledItemSpacing():Number {
            if (!isInitialized) {
                initializeDefaults();
            }
            return itemSpacing * globalScale;
        }
        
        /**
         * @brief Получает масштабированную ширину контейнера
         */
        public function getScaledContainerWidth():Number {
            if (!isInitialized) {
                initializeDefaults();
            }
            return containerWidth * globalScale;
        }
        
        /**
         * @brief Получает масштабированные горизонтальные отступы
         */
        public function getScaledHorizontalMargin():Number {
            if (!isInitialized) {
                initializeDefaults();
            }
            return horizontalMargin * globalScale;
        }
        
        /**
         * @brief Получает масштабированные вертикальные отступы
         */
        public function getScaledVerticalMargin():Number {
            if (!isInitialized) {
                initializeDefaults();
            }
            return verticalMargin * globalScale;
        }
        
        /**
         * @brief Получает доступную ширину для элементов (с учётом отступов)
         */
        public function getAvailableWidth(scrollPaneWidth:Number):Number {
            if (!isInitialized) {
                initializeDefaults();
            }
            
            var margin:Number = getScaledHorizontalMargin() * 2; // С обеих сторон
            var availableWidth:Number = (scrollPaneWidth - margin) * globalScale;
            
            return Math.max(availableWidth, 100); // Минимум 100px
        }
        
        /**
         * @brief Получает текущий глобальный масштаб
         */
        public function get scale():Number {
            if (!isInitialized) {
                initializeDefaults();
            }
            return globalScale;
        }
        
        /**
         * @brief Проверяет, инициализирован ли MenuScaler
         */
        public function get initialized():Boolean {
            return isInitialized;
        }
        
        /**
         * @brief Получает все параметры масштабирования
         */
        public function getScalingParams():Object {
            if (!isInitialized) {
                initializeDefaults();
            }
            
            return {
                globalScale: globalScale,
                itemHeight: getScaledItemHeight(),
                itemSpacing: getScaledItemSpacing(),
                containerWidth: getScaledContainerWidth(),
                horizontalMargin: getScaledHorizontalMargin(),
                verticalMargin: getScaledVerticalMargin()
            };
        }
        
        /**
         * @brief Логирует сообщение
         */
        private function log(message:String):void {
            trace(message);
            // Не используем mainInstance, чтобы избежать циклических зависимостей
        }
        
        /**
         * @brief Инициализирует MenuScaler с контейнером
         * @param container Контейнер для масштабирования
         */
        public function initialize(container:ScrollPane):void {
            log("MenuScaler: Инициализация с контейнером");
            this.menuContainer = container;
            
            // Инициализируем значения по умолчанию
            initializeDefaults();
            
            log("MenuScaler: Инициализация завершена");
        }
        
        /**
         * @brief Обновляет масштаб
         */
        public function updateScale():void {
            currentScale = globalScale;
            log("MenuScaler: Обновление масштаба - currentScale=" + currentScale);
            dispatchEvent(new Event(SCALE_CHANGED));
        }
        
        /**
         * @brief Принудительно обновляет масштаб
         */
        public function forceUpdateScale():void {
            currentScale = globalScale;
            log("MenuScaler: Принудительное обновление масштаба - currentScale=" + currentScale);
            dispatchEvent(new Event(SCALE_CHANGED));
        }
        
        /**
         * @brief Обновляет масштаб с растягиванием
         */
        public function updateScaleStretch():void {
            currentScale = globalScale;
            log("MenuScaler: Обновление масштаба с растягиванием - currentScale=" + currentScale);
            dispatchEvent(new Event(SCALE_CHANGED));
        }
        
        /**
         * @brief Получает текущий масштаб
         */
        public function getCurrentScale():Number {
            return currentScale;
        }
        
        /**
         * @brief Получает базовое разрешение
         */
        public function getBaseResolution():Object {
            return {
                width: baseWidth,
                height: baseHeight
            };
        }
        
        /**
         * @brief Получает отступы
         */
        public function getMargins():Object {
            return {
                horizontal: horizontalMargin,
                vertical: verticalMargin
            };
        }
        
        /**
         * @brief Уничтожает MenuScaler и очищает ресурсы
         */
        public function destroy():void {
            log("MenuScaler: Начало уничтожения...");
            
            // Очищаем ссылки
            menuContainer = null;
            
            log("MenuScaler: Уничтожение завершено");
        }
    }
}
