# Changelog - DBMenu

## Version 2.0.0 (2025-07-30) - Release

### 🎯 Основные улучшения
- **Централизованная обработка событий** - предотвращение дублирования EVENT_BACK событий
- **Архитектурная оптимизация** - делегирование событий от компонентов к ScrollableMenu
- **Специализированные обработчики** - улучшенная обработка SwitcherComponent событий
- **Очистка кода** - удаление ~200+ строк неиспользуемого кода

### ✅ Исправленные проблемы
- **Двойные события EVENT_BACK** - теперь события обрабатываются только в ScrollableMenu.onKeyDown()
- **Дублирующиеся методы** - удалены избыточные методы addItem(), updateItems() из ScrollableMenu
- **Неиспользуемые поля** - очищены currentHoverIndex, pushTimer, неактивные цвета в LabelComponent
- **Избыточное логирование** - упрощена диагностика, сохранена только необходимая информация

### 🔧 Архитектурные изменения
- **Поток событий**: Компонент → ScrollableMenu → C++ (без дублирования)
- **Hover management**: Централизован через setActiveIndex() в ScrollableMenu
- **Event delegation**: SwitcherComponent использует специализированный createSwitcherHandler()
- **Code organization**: Удалены все временные диагностические методы

### 🧹 Очистка кода
#### Удалено из ScrollableMenu.as:
- `logComponentState()` метод (71 строка)
- Дублирующий `addItem()` метод
- `updateItems()` обертка
- `currentHoverIndex` поле и связанные методы
- Избыточное диагностическое логирование

#### Удалено из Main.as:
- `setHoverHighlight()` и `clearHoverHighlight()` методы
- Устаревшие ExternalInterface callbacks

#### Удалено из компонентов:
- **LabelComponent**: неиспользуемые `hoverColor`, `selectedColor`
- **ButtonComponent**: система `pushTimer` и связанные методы

### 📊 Статистика
- **Удалено**: ~200+ строк неиспользуемого кода
- **Ошибки компиляции**: 0 (все исправлены)
- **Архитектурная целостность**: ✅ Улучшена
- **Производительность**: ✅ Оптимизирована

### 💡 Улучшения для разработчиков
- Чистая кодовая база без дублирования
- Понятная архитектура событий
- Упрощенная система hover/active состояний
- Надежная обработка компонентов

### ✅ Исправлено в C++ (DirectApply.cpp)
1. **Удалены дубликаты функций pollPendingCallbacks** - оставлена только одна правильная реализация
2. **Исправлен polling механизм** в единственной оставшейся функции:
```cpp
// БЫЛО (устаревшее с _root):
if (!uiMovie->GetVariable(&pendingCallback, "_root.pendingCallback")) {
    return;
}
uiMovie->SetVariable("_root.pendingCallback", emptyValue);

// СТАЛО (правильное с menuObj):
if (!menuObj.GetMember("pendingCallback", &pendingCallback)) {
    return;
}
menuObj.SetMember("pendingCallback", emptyValue);
```

3. **Оптимизация файла** - размер уменьшен с 1933 до 1451 строки за счет удаления дубликатов
4. **Единый подход** - весь polling теперь использует только `menuObj.GetMember/SetMember`

## Архитектура polling механизма

### AS3 сторона (Flash):
1. При событии компонента вызывается `sendExternalCallback()`
2. Устанавливаются `this.pendingCallback` и `this.callbackArgs`
3. Переменные доступны C++ через `menuObj.GetMember()`

### C++ сторона:
1. В `ProcessMessage(kUpdate)` вызывается `pollPendingCallbacks()`
2. Читаются переменные через `menuObj.GetMember()`
3. Вызывается соответствующий обработчик
4. Переменные очищаются через `menuObj.SetMember()`

## Преимущества нового подхода

1. ✅ **Совместимость с AS3** - используются корректные конструкции
2. ✅ **Простота** - прямой доступ к переменным объекта
3. ✅ **Надежность** - нет зависимости от ExternalInterface
4. ✅ **Производительность** - быстрый доступ через GetMember/SetMember

## Тестирование

После исправления:
- ✅ AS3 код компилируется без ошибок
- ✅ C++ код корректно читает переменные из menuObj
- ✅ Polling работает независимо от ExternalInterface
- ✅ Документация обновлена с правильными примерами

## Статус
🟢 **ИСПРАВЛЕНО** - Система polling колбэков полностью рабочая и документированная

## Статус C++ исправлений
✅ **ЗАВЕРШЕНО** - Полная интеграция AS3 ↔ C++ через polling работает правильно:
- AS3 устанавливает `this.pendingCallback` и `this.callbackArgs`
- C++ читает их через `menuObj.GetMember()` в `pollPendingCallbacks()`
- C++ очищает переменные через `menuObj.SetMember()` после обработки
- Все дубликаты кода удалены, архитектура clean и maintainable
