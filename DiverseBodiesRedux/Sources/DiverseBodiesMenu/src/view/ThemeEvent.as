package view {
    import flash.events.Event;
    
    /**
     * @brief Событие изменения темы
     * 
     * Используется ThemeManager для уведомления компонентов об изменениях цветов.
     */
    public class ThemeEvent extends Event {
        public var componentType:int = -1;
        
        public function ThemeEvent(type:String, componentType:int = -1, bubbles:Boolean = false, cancelable:Boolean = false) {
            super(type, bubbles, cancelable);
            this.componentType = componentType;
        }
        
        public override function clone():Event {
            var event:ThemeEvent = new ThemeEvent(type, componentType, bubbles, cancelable);
            return event;
        }
    }
}
