package view {
    import flash.display.MovieClip;
    
    /**
     * @brief Централизованный логгер для всей системы меню
     * 
     * Обеспечивает единую точку логирования через BGSCodeObj в Main.as
     * Все компоненты системы должны использовать этот логгер вместо прямых вызовов trace()
     */
    public class Logger {
        // Уровни логирования
        public static const LEVEL_NONE:int = 0;     // Никакого логирования
        public static const LEVEL_ERROR:int = 1;    // Только ошибки
        public static const LEVEL_WARNING:int = 2;  // Ошибки + предупреждения
        public static const LEVEL_INFO:int = 3;     // Ошибки + предупреждения + информация
        public static const LEVEL_DEBUG:int = 4;    // Все сообщения включая отладку
        
        private static var _instance:Logger;
        private static var _mainInstance:MovieClip; // Ссылка на Main для доступа к BGSCodeObj
        private static var _logLevel:int = LEVEL_DEBUG; // По умолчанию полное логирование
        
        /**
         * @brief Получает экземпляр синглтона логгера
         */
        public static function getInstance():Logger {
            if (!_instance) {
                _instance = new Logger();
            }
            return _instance;
        }
        
        /**
         * @brief Устанавливает ссылку на Main для доступа к BGSCodeObj
         * @param mainInstance Экземпляр Main.as
         */
        public static function setMainInstance(mainInstance:MovieClip):void {
            _mainInstance = mainInstance;
        }
        
        /**
         * @brief Получает текущий уровень логирования
         */
        public static function get logLevel():int {
            return _logLevel;
        }
        
        /**
         * @brief Устанавливает уровень логирования
         */
        public static function set logLevel(level:int):void {
            _logLevel = level;
        }
        
        /**
         * @brief Приватный конструктор для синглтона
         */
        public function Logger() {
            if (_instance) {
                throw new Error("Logger является синглтоном. Используйте Logger.getInstance()");
            }
        }
        
        /**
         * @brief Логирует сообщение через BGSCodeObj в Main.as
         * @param message Сообщение для логирования
         * @param category Категория сообщения (опционально)
         * @param level Уровень важности сообщения
         */
        public function log(message:String, category:String = "", level:int = LEVEL_INFO):void {
            // Проверяем уровень логирования
            if (level > _logLevel) {
                return;
            }
            
            // Добавляем префикс уровня
            var levelPrefix:String = "";
            switch (level) {
                case LEVEL_ERROR:
                    levelPrefix = "[ERROR] ";
                    break;
                case LEVEL_WARNING:
                    levelPrefix = "[WARNING] ";
                    break;
                case LEVEL_INFO:
                    levelPrefix = "[INFO] ";
                    break;
                case LEVEL_DEBUG:
                    levelPrefix = "[DEBUG] ";
                    break;
                default:
                    levelPrefix = "[LOG] ";
                    break;
            }
            
            var fullMessage:String = levelPrefix + (category ? "[" + category + "] " + message : message);
            
            // Всегда выводим в trace для отладки
            trace(fullMessage);
            
            // Отправляем в C++ через BGSCodeObj если доступен
            if (_mainInstance && _mainInstance.hasOwnProperty("BGSCodeObj")) {
                var bgsCodeObj:Object = _mainInstance["BGSCodeObj"];
                if (bgsCodeObj && typeof bgsCodeObj.Log === "function") {
                    try {
                        bgsCodeObj.Log(fullMessage);
                    } catch (error:Error) {
                        trace("Logger: Ошибка отправки в BGSCodeObj: " + error.message);
                    }
                }
            }
        }
        
        /**
         * @brief Логирует ошибку (всегда выводится)
         * @param message Сообщение об ошибке
         * @param category Категория (опционально)
         */
        public function error(message:String, category:String = ""):void {
            log(message, category, LEVEL_ERROR);
        }
        
        /**
         * @brief Логирует предупреждение
         * @param message Сообщение-предупреждение
         * @param category Категория (опционально)
         */
        public function warning(message:String, category:String = ""):void {
            log(message, category, LEVEL_WARNING);
        }
        
        /**
         * @brief Логирует информационное сообщение
         * @param message Информационное сообщение
         * @param category Категория (опционально)
         */
        public function info(message:String, category:String = ""):void {
            log(message, category, LEVEL_INFO);
        }
        
        /**
         * @brief Логирует отладочную информацию
         * @param message Отладочное сообщение
         * @param category Категория (опционально)
         */
        public function debug(message:String, category:String = ""):void {
            log(message, category, LEVEL_DEBUG);
        }
        
    }
}
