# DBMenu v2.0.1 - Адаптивная система меню для Fallout 4

Современная система меню для интеграции с Fallout 4 через Scaleform (Flash/ActionScript 3), разработанная для мода DirectApply.

## ✨ Ключевые особенности

- **Единственный ScrollPane** - элегантная прокручиваемая панель без лишних элементов
- **30% ширины экрана** - компактное размещение справа с адаптивным позиционированием  
- **Централизованная обработка событий** - предотвращение дублирования событий
- **Тематизация и масштабирование** - полная настройка внешнего вида
- **Надежные polling колбэки** - механизм переменных _root обеспечивает доставку событий
- **Чистая документация** - стандартизированные комментарии без избыточных тегов

## 🔧 Поддерживаемые компоненты

- **Button** - интерактивные кнопки
- **Checkbox** - переключатели вкл/выкл  
- **Switcher** - выпадающие списки с навигацией
- **Label** - текстовые заголовки и разделители

## 🏗️ Архитектура

```
Main.as (C++ API и интеграция)
├── MenuManager.as (Центральное управление)
│   ├── ThemeManager.as (Управление темами)
│   ├── MenuScaler.as (Адаптивное масштабирование)
│   └── ScrollableMenu.as (Прокрутка и навигация)
│       └── components/
│           ├── ButtonComponent.as
│           ├── CheckboxComponent.as
│           ├── SwitcherComponent.as
│           └── LabelComponent.as
└── view/CustomEvent.as (Система событий)
```

## 📋 Требования

- **Fallout 4** с поддержкой Scaleform
- **F4SE** (Fallout 4 Script Extender)  
- **Adobe Flash Professional** CS6+ (для редактирования)

## 🚀 Быстрый старт

1. **Интегрируйте C++ файлы** в ваш проект F4SE плагина
2. **Скопируйте dbMenu.swf** в папку ресурсов интерфейса
3. **Используйте DirectApply.cpp** как пример интеграции
4. **Смотрите ExampleUsage.md** для подробных примеров
```

## 🔧 API для C++

### Основные методы управления

```cpp
// Показать меню
menuObj.Invoke("show", nullptr, 0);

// Скрыть меню
menuObj.Invoke("hide", nullptr, 0);

// Очистить содержимое
menuObj.Invoke("clear", nullptr, 0);

// Уничтожить меню
menuObj.Invoke("destroy", nullptr, 0);
```

### Добавление элементов

```cpp
// Добавить кнопку
RE::Scaleform::GFx::Value args[2] = {1, "Новая игра"};
menuObj.Invoke("push", args, 2);

// Добавить чекбокс
RE::Scaleform::GFx::Value args[3] = {2, "Включить звук", 1};
menuObj.Invoke("push", args, 3);

// Добавить свитчер
RE::Scaleform::GFx::Value args[3] = {3, "Низкое,Среднее,Высокое", 0};
menuObj.Invoke("push", args, 3);

// Добавить заголовок
RE::Scaleform::GFx::Value args[2] = {0, "=== НАСТРОЙКИ ==="};
menuObj.Invoke("push", args, 2);
```

### Массовое добавление

```cpp
// Создать массив элементов
RE::Scaleform::GFx::Value menuArray;
uiMovie->CreateArray(&menuArray);

// Добавить элементы в массив
RE::Scaleform::GFx::Value item1;
uiMovie->CreateArray(&item1);
item1.PushBack(1);              // BUTTON
item1.PushBack("Продолжить");   // label
menuArray.PushBack(item1);

RE::Scaleform::GFx::Value item2;
uiMovie->CreateArray(&item2);
item2.PushBack(2);              // CHECKBOX
item2.PushBack("Включить звук"); // label
item2.PushBack(1);              // state
menuArray.PushBack(item2);

// Добавить весь массив
menuObj.Invoke("pushArray", &menuArray, 1);
```

### Поиск и управление

```cpp
// Найти элемент по названию
RE::Scaleform::GFx::Value label("Настройки");
RE::Scaleform::GFx::Value result;
menuObj.Invoke("find", &result, &label, 1);
int index = result.GetInt(); // -1 если не найден

// Вставить элемент перед найденным
RE::Scaleform::GFx::Value insertArgs[3] = {index, 1, "Помощь"};
menuObj.Invoke("insert", insertArgs, 3);

// Удалить элемент по названию
RE::Scaleform::GFx::Value deleteLabel("Старая кнопка");
menuObj.Invoke("deleteComponent", &deleteLabel, 1);
```

## 📊 Типы компонентов

| Тип | Описание | Параметры |
|-----|----------|-----------|
| 0 | LABEL | `push(0, "текст")` |
| 1 | BUTTON | `push(1, "текст")` |
| 2 | CHECKBOX | `push(2, "текст", состояние)` |
| 3 | SWITCHER | `push(3, "опция1,опция2,опция3", индекс)` |

## 📡 Колбэки через Polling механизм

Система использует надежный polling механизм для передачи событий от Flash к C++. 
При каждом взаимодействии с компонентами, Flash устанавливает переменные на корневом уровне (_root), 
которые C++ проверяет на каждом обновлении (kUpdate сообщение).

### Обработчики событий в C++

```cpp
// Нажатие кнопки
void DirectApply::onPushButtonComponent(const char* labelText) {
    std::string label(labelText);
    logger::info("Кнопка нажата: {}", label);
    // Ваша логика обработки
}

// Кнопка "назад" (Escape)
void DirectApply::onBackButtonComponent(const char* labelText) {
    std::string label(labelText);
    logger::info("Кнопка назад: {}", label);
    // Закрыть меню или вернуться
}

// Наведение мыши
void DirectApply::onHoverButtonComponent(const char* labelText) {
    std::string label(labelText);
    logger::info("Наведение на: {}", label);
    // Показать подсказку или подсветку
}

// Изменение чекбокса
void DirectApply::onChangeCheckboxComponent(const char* labelText, bool checked) {
    std::string label(labelText);
    logger::info("Чекбокс {}: {}", label, checked ? "включен" : "выключен");
    // Обновить настройки
}

// Изменение свитчера
void DirectApply::onChangeSwitcherComponent(const char* labelText, int selectedIndex) {
    std::string label(labelText);
    logger::info("Свитчер {}: выбран индекс {}", label, selectedIndex);
    // Обновить значения
}
```

### Как работает polling

В `ProcessMessage` при `kUpdate`:

```cpp
case RE::UI_MESSAGE_TYPE::kUpdate:
{
    // Проверяем pending callbacks от Flash
    if (isInitialized && uiMovie && menuObj.IsObject()) {
        pollPendingCallbacks();
    }
    return RE::UI_MESSAGE_RESULTS::kPassOn;
}
```

Функция `pollPendingCallbacks()` автоматически:
1. Проверяет `menuObj.GetMember("pendingCallback", ...)` на наличие имени колбэка
2. Читает `menuObj.GetMember("callbackArgs", ...)` для получения аргументов  
3. Вызывает соответствующий обработчик (`onPushButtonComponent`, etc.)
4. Очищает переменные после обработки через `menuObj.SetMember(...)`

### Правильная реализация pollPendingCallbacks()

```cpp
void DirectApply::pollPendingCallbacks() {
    if (!uiMovie || !menuObj.IsObject()) {
        return;
    }

    // Проверяем наличие pending callback через menuObj (Main объект в AS3)
    RE::Scaleform::GFx::Value pendingCallback;
    if (!menuObj.GetMember("pendingCallback", &pendingCallback)) {
        return; // Нет pending callback
    }

    if (!pendingCallback.IsString()) {
        return; // pendingCallback не строка
    }

    std::string callbackName = pendingCallback.GetString();
    if (callbackName.empty()) {
        return; // Пустой callback
    }

    // Получаем аргументы callback
    RE::Scaleform::GFx::Value callbackArgs;
    if (!menuObj.GetMember("callbackArgs", &callbackArgs)) {
        logger::warn("DirectApply: pendingCallback найден, но callbackArgs отсутствуют");
        return;
    }

    if (!callbackArgs.IsArray()) {
        logger::warn("DirectApply: callbackArgs не является массивом");
        return;
    }

    logger::info("DirectApply: Обработка pending callback: {}", callbackName);

    // Обрабатываем callback в зависимости от типа
    if (callbackName == "pushButtonComponent") {
        if (callbackArgs.GetArraySize() >= 1) {
            RE::Scaleform::GFx::Value arg0;
            callbackArgs.GetElement(0, &arg0);
            if (arg0.IsString()) {
                onPushButtonComponent(arg0.GetString());
            }
        }
    } else if (callbackName == "changeCheckboxComponent") {
        if (callbackArgs.GetArraySize() >= 2) {
            RE::Scaleform::GFx::Value arg0, arg1;
            callbackArgs.GetElement(0, &arg0);
            callbackArgs.GetElement(1, &arg1);
            if (arg0.IsString() && arg1.IsBool()) {
                onChangeCheckboxComponent(arg0.GetString(), arg1.GetBool());
            }
        }
    } 
    // ... добавьте другие типы колбэков по аналогии
    else {
        logger::warn("DirectApply: Неизвестный callback: {}", callbackName);
    }

    // Очищаем pending callback после обработки
    RE::Scaleform::GFx::Value emptyValue;
    emptyValue.SetString("");
    menuObj.SetMember("pendingCallback", emptyValue);
    
    RE::Scaleform::GFx::Value emptyArray;
    uiMovie->CreateArray(&emptyArray);
    menuObj.SetMember("callbackArgs", emptyArray);
}
```

### Flash сторона

```actionscript
// В Main.as при отправке колбэка
private function sendExternalCallback(callbackName:String, args:Array):void {
    // Устанавливаем callback через публичные переменные Main объекта
    this.pendingCallback = callbackName;
    this.callbackArgs = args;
    trace("Main: Установлены this.pendingCallback = " + callbackName);
}

// Объявление переменных в классе Main
public class Main extends MovieClip {
    // Переменные для polling колбэков из C++
    public var pendingCallback:String = "";
    public var callbackArgs:Array = [];
    // ...остальные поля
}
```

## ⚙️ Настройка макета

```cpp
// Установить отступы (право, верх, низ)
RE::Scaleform::GFx::Value margins[3] = {20.0, 100.0, 100.0};
menuObj.Invoke("setMenuMargins", margins, 3);

// Обновить масштабирование
menuObj.Invoke("updateMenuScale", nullptr, 0);

// Получить информацию о масштабе
RE::Scaleform::GFx::Value scaleInfo;
menuObj.Invoke("getMenuScaleInfo", &scaleInfo, 0);
```

## 🎮 Управление

- **↑/↓** - навигация между элементами
- **←/→** - изменение значений (чекбоксы, свитчеры)
- **Enter** - активация кнопок
- **Escape** - кнопка "назад"

## � Интеграция с DirectApply

### Инициализация в C++

```cpp
// В конструкторе DirectApply
DirectApply::DirectApply() {
    // Загрузка SWF файла
    ScaleformManager->LoadMovieEx(*this, "Interface/dbMenu.swf", 
        "root.menu_mc", RE::Scaleform::GFx::Movie::ScaleModeType::kShowAll);
    
    // menuObj будет инициализирован в ProcessMessage при kShow
}

// В ProcessMessage
case RE::UI_MESSAGE_TYPE::kShow:
    if (!isInitialized) {
        // Инициализация menuObj через _root (Main класс в AS3)
        bool success = uiMovie->GetVariable(&menuObj, "_root");
        if (success && menuObj.IsObject()) {
            isInitialized = true;
            
            // Настройка макета
            RE::Scaleform::GFx::Value margins[3] = {20.0, 100.0, 100.0};
            menuObj.Invoke("setMenuMargins", margins, 3);
            menuObj.Invoke("updateMenuScale", nullptr, 0);
        }
    }
    break;

case RE::UI_MESSAGE_TYPE::kUpdate:
    // Проверяем pending callbacks от Flash каждый кадр
    if (isInitialized && uiMovie && menuObj.IsObject()) {
        pollPendingCallbacks();
    }
    break;
```

### Использование в коде

```cpp
void DirectApply::showActors() {
    clear();
    
    // Добавляем актеров по одному
    auto actors = getActorsInRange(RE::PlayerCharacter::GetSingleton(), 1000, 10);
    
    for (const auto& actor : actors) {
        std::string name = actor->GetDisplayFullName();
        if (name.empty()) name = "<Unnamed>";
        
        push(MENU_OBJECT::BUTTON, name);
    }
}
```

## 📁 Структура файлов

```
Interface/
└── dbMenu.swf          # Скомпилированный SWF файл

Source/
├── Main.as             # Главный класс и API
├── view/
│   ├── MenuManager.as  # Управление меню
│   ├── MenuScaler.as   # Масштабирование
│   ├── ScrollableMenu.as # Прокрутка
│   ├── CustomEvent.as  # События
│   └── components/
│       ├── ButtonComponent.as
│       ├── CheckboxComponent.as
│       ├── SwitcherComponent.as
│       └── LabelComponent.as
└── C++/
    ├── DirectApply.h   # Заголовочный файл
    └── DirectApply.cpp # Реализация меню
```

## 🛠️ Сборка

1. **Компиляция AS3**: Используйте Flash Professional или mxmlc
2. **Компиляция C++**: Интегрируйте с вашим проектом F4SE
3. **Размещение**: Поместите `dbMenu.swf` в папку `Interface/`

## 📝 Пример использования

```cpp
// Создание простого меню
void DirectApply::createMainMenu() {
    clear();
    
    // Заголовок
    push(MENU_OBJECT::LABEL, "=== ГЛАВНОЕ МЕНЮ ===");
    
    // Кнопки
    push(MENU_OBJECT::BUTTON, "Новая игра");
    push(MENU_OBJECT::BUTTON, "Загрузить");
    push(MENU_OBJECT::BUTTON, "Настройки");
    
    // Настройки
    push(MENU_OBJECT::LABEL, "--- Настройки ---");
    push(MENU_OBJECT::CHECKBOX, "Включить звук", soundEnabled ? 1 : 0);
    push(MENU_OBJECT::SWITCHER, "Низкое,Среднее,Высокое", graphicsQuality);
    
    // Кнопка выхода
    push(MENU_OBJECT::BUTTON, "Выход");
}

// Обработка событий
extern "C" void pushButtonComponent(const char* labelText) {
    std::string label(labelText);
    
    if (label == "Новая игра") {
        // Логика новой игры
    } else if (label == "Настройки") {
        // Открыть меню настроек
    } else if (label == "Выход") {
        // Закрыть меню
        DirectApply::getSingleton()->hide();
    }
}
```

## 🔍 Отладка

Все компоненты используют `trace()` для отладки:

```actionscript
// В Main.as
trace("Main: Добавление элемента типа " + type);

// В MenuManager.as  
trace("MenuManager: Обновление " + items.length + " элементов");

// В ScrollableMenu.as
trace("ScrollableMenu: Навигация на индекс " + activeIndex);
```

## ✅ Готовность к использованию

- ✅ **Совместимость с Fallout 4** - протестировано с F4SE
- ✅ **Адаптивная верстка** - работает на всех разрешениях
- ✅ **Современный API** - простая интеграция с C++
- ✅ **Polling колбэки** - надежная передача событий без ExternalInterface
- ✅ **Новый дизайн** - компактный ScrollPane справа
- ✅ **Полная документация** - примеры и объяснения
- ✅ **Отладка** - подробное логирование
- ✅ **Чистый код** - убраны тестовые методы и лишний код
- ✅ **Стандартизированная документация** - единый стиль комментариев v2.0.1

## 📁 Итоговая структура проекта

```
DBMenu/
├── README.md           # Полная документация
├── ExampleUsage.md     # Примеры использования
├── DirectApply.h       # Заголовочный файл C++
├── DirectApply.cpp     # Реализация C++
├── Main.as             # Главный класс AS3
├── view/               # Компоненты AS3
│   ├── MenuManager.as
│   ├── MenuScaler.as
│   ├── ScrollableMenu.as
│   ├── CustomEvent.as
│   └── components/
│       ├── ButtonComponent.as
│       ├── CheckboxComponent.as
│       ├── SwitcherComponent.as
│       └── LabelComponent.as
├── dbMenu.fla          # Исходный Flash файл
├── dbMenu.swf          # Скомпилированный SWF
└── build.bat           # Скрипт сборки
```

---

**🎯 Система полностью готова к использованию в модификациях Fallout 4.**

Особенности финальной версии:
- **Реализован polling механизм** - надежная передача колбэков через _root переменные
- **Убран ExternalInterface** - больше не зависит от availability
- **Убран весь тестовый код** из Main.as и DirectApply.cpp
- **Оптимизированы методы преобразования** данных
- **Упрощен заголовочный файл** DirectApply.h
- **Создана полная документация** с примерами polling интеграции
- **Оставлен только необходимый код** для работы меню

Система готова к интеграции в любые проекты и моды для Fallout 4.
   - Очистите поле "Class" или измените имя класса
   - Нажмите OK

### Рекомендуемая структура FLA

Для корректной работы создайте в FLA:
- **MovieClip** с именем `menu_mc` на главной сцене (опционально)
- Если `menu_mc` отсутствует, система создаст fallback автоматически

## 🏗️ Архитектура

```
Main.as (Точка входа)
├── MenuManager.as (Центральное управление)
│   ├── MenuScaler.as (Масштабирование)
│   └── ScrollableMenu.as (Элементы меню)
│       └── components/
│           ├── ButtonComponent.as
│           ├── CheckboxComponent.as
│           ├── SwitcherComponent.as
│           └── LabelComponent.as
└── CustomEvent.as (События)
```

### Основные классы

#### 🎯 Main.as
**Точка входа приложения** - управляет инициализацией, API для C++, обработкой колбэков.

#### 🎛️ MenuManager.as  
**Центральный менеджер** - координирует работу всех компонентов, управляет отображением.

#### 📏 MenuScaler.as
**Система масштабирования** - автоматическое позиционирование и масштабирование меню.

#### 📜 ScrollableMenu.as
**Прокручиваемое меню** - управляет элементами внутри ScrollPane, навигация с клавиатуры.

#### 🧩 Компоненты интерфейса
- **ButtonComponent** - интерактивные кнопки с визуальной обратной связью
- **CheckboxComponent** - переключатели с поддержкой состояний вкл/выкл  
- **SwitcherComponent** - выпадающие списки с навигацией стрелками
- **LabelComponent** - статические текстовые элементы для заголовков

*Все классы и компоненты полностью документированы с подробными @brief описаниями методов и параметров.*

## 🔧 API для C++

### Управление меню

```cpp
// Показать меню
movie->Invoke("show", nullptr, 0);

// Скрыть меню  
movie->Invoke("hide", nullptr, 0);

// Полностью уничтожить меню
movie->Invoke("destroy", nullptr, 0);
```

### Управление содержимым

```cpp
// Очистить все элементы
movie->Invoke("clear", nullptr, 0);

// Установить заголовок
RE::Scaleform::GFx::Value title("Главное меню");
movie->Invoke("setTitle", &title, 1);

// Удалить элемент по названию
RE::Scaleform::GFx::Value labelToDelete("Настройки");
movie->Invoke("deleteComponent", &labelToDelete, 1);

// Найти индекс элемента по лейблу
RE::Scaleform::GFx::Value labelToFind("Включить звук");
RE::Scaleform::GFx::Value result;
movie->Invoke("find", &result, &labelToFind, 1);
// result содержит индекс или -1 если не найден
```

### Добавление элементов

```cpp
// Добавить один элемент в конец - push(тип, лейбл, состояние?)

// Кнопка
RE::Scaleform::GFx::Value buttonArgs[2] = {1, "Новая игра"};
movie->Invoke("push", buttonArgs, 2);

// Чекбокс
RE::Scaleform::GFx::Value checkboxArgs[3] = {2, "Включить звук", 1};
movie->Invoke("push", checkboxArgs, 3);

// Свитчер
RE::Scaleform::GFx::Value switcherArgs[3] = {3, "Низкое,Среднее,Высокое,Ультра", 2};
movie->Invoke("push", switcherArgs, 3);

// Заголовок
RE::Scaleform::GFx::Value labelArgs[2] = {0, "=== НАСТРОЙКИ ==="};
movie->Invoke("push", labelArgs, 2);
```

### Массовое добавление элементов

```cpp
// Добавить массив элементов - pushArray([элемент1, элемент2, ...])
RE::Scaleform::GFx::Value menuArray;
movie->CreateArray(&menuArray);

// Элемент 1: [тип, лейбл]
RE::Scaleform::GFx::Value item1;
movie->CreateArray(&item1);
item1.PushBack(0);                      // LABEL
item1.PushBack("=== ГЛАВНОЕ МЕНЮ ==="); // лейбл
menuArray.PushBack(item1);

// Элемент 2: [тип, лейбл]
RE::Scaleform::GFx::Value item2;
movie->CreateArray(&item2);
item2.PushBack(1);              // BUTTON
item2.PushBack("Новая игра");   // лейбл
menuArray.PushBack(item2);

// Элемент 3: [тип, лейбл, состояние]
RE::Scaleform::GFx::Value item3;
movie->CreateArray(&item3);
item3.PushBack(2);               // CHECKBOX
item3.PushBack("Включить звук"); // лейбл
item3.PushBack(1);               // состояние
menuArray.PushBack(item3);

movie->Invoke("pushArray", &menuArray, 1);
```

### Вставка элементов

```cpp
// Вставить элемент перед индексом - insert(индекс, тип, лейбл, состояние?)

// Вставить кнопку "Назад" в начало (индекс 0)
RE::Scaleform::GFx::Value insertArgs[3] = {0, 1, "Назад"};
movie->Invoke("insert", insertArgs, 3);

// Вставить разделитель перед 3-м элементом
RE::Scaleform::GFx::Value separatorArgs[3] = {2, 0, "--- НАСТРОЙКИ ---"};
movie->Invoke("insert", separatorArgs, 3);
```

### Типы компонентов

| Тип | Константа | Описание | Параметр state |
|-----|-----------|----------|----------------|
| 0 | LABEL | Текстовый заголовок | не используется |
| 1 | BUTTON | Интерактивная кнопка | не используется |
| 2 | CHECKBOX | Переключатель | 0=выкл, 1=вкл |
| 3 | SWITCHER | Выпадающий список | индекс выбранного элемента |

### Методы API

| Метод | Параметры | Описание |
|-------|-----------|----------|
| `show()` | - | Показать меню |
| `hide()` | - | Скрыть меню |
| `destroy()` | - | Уничтожить меню |
| `clear()` | - | Очистить все элементы |
| `push(type, label, state?)` | тип, лейбл, состояние | Добавить элемент в конец |
| `pushArray(items)` | массив элементов | Добавить массив элементов |
| `insert(index, type, label, state?)` | индекс, тип, лейбл, состояние | Вставить элемент |
| `find(label)` | лейбл | Найти индекс элемента |
| `deleteComponent(label)` | лейбл | Удалить элемент |
| `setTitle(title)` | заголовок | Установить заголовок |

## 📡 Колбэки в C++

Система автоматически вызывает функции в C++ при взаимодействии пользователя:

```cpp
// Нажатие кнопки
extern "C" void pushButtonComponent(const char* labelText);

// Кнопка "назад" (Escape)
extern "C" void backButtonComponent(const char* labelText);

// Наведение на кнопку
extern "C" void hoverButtonComponent(const char* labelText);

// Изменение чекбокса
extern "C" void changeCheckboxComponent(const char* labelText, bool checked);

// Изменение свитчера  
extern "C" void changeSwitcherComponent(const char* labelText, int selectedIndex);
```

## ⚙️ Настройка масштабирования

```cpp
// Установить отступы (право, верх, низ)
RE::Scaleform::GFx::Value args[3] = {50.0, 20.0, 20.0};
movie->Invoke("setMenuMargins", args, 3);

// Принудительное обновление масштаба
movie->Invoke("updateMenuScale", nullptr, 0);

// Получить информацию о масштабе
RE::Scaleform::GFx::Value result;
movie->Invoke("getMenuScaleInfo", &result, 0);
```

## 🎮 Управление с клавиатуры

- **↑/↓** - навигация между элементами
- **←/→** - изменение значений (чекбоксы, свитчеры)
- **Enter** - активация кнопок
- **Escape** - кнопка "назад"

## 🔄 Жизненный цикл

1. **Инициализация** - поиск menu_mc или создание fallback
2. **Настройка** - инициализация компонентов и масштабирования  
3. **Показ** - отображение меню и установка элементов
4. **Взаимодействие** - обработка пользовательского ввода
5. **Обновление** - изменение содержимого по необходимости
6. **Уничтожение** - корректная очистка ресурсов

## 🐛 Отладка

Система предоставляет подробное логирование через `trace()`:

- **Main:** общая инициализация и API
- **MenuManager:** управление компонентами  
- **MenuScaler:** расчеты масштабирования
- **ScrollableMenu:** навигация и элементы

## 📝 Примеры использования

### Простое меню
```cpp
// Показать меню с одной кнопкой
movie->Invoke("show", nullptr, 0);

// Добавить кнопку
RE::Scaleform::GFx::Value buttonArgs[2] = {1, "Продолжить"};
movie->Invoke("push", buttonArgs, 2);
```

### Меню настроек
```cpp
// Показать меню
movie->Invoke("show", nullptr, 0);

// Добавить заголовок
RE::Scaleform::GFx::Value headerArgs[2] = {0, "=== НАСТРОЙКИ ==="};
movie->Invoke("push", headerArgs, 2);

// Добавить чекбокс звука
RE::Scaleform::GFx::Value soundArgs[3] = {2, "Включить звук", settings.soundEnabled ? 1 : 0};
movie->Invoke("push", soundArgs, 3);

// Добавить свитчер качества
RE::Scaleform::GFx::Value qualityArgs[3] = {3, "Низкое,Среднее,Высокое,Ультра", settings.graphicsQuality};
movie->Invoke("push", qualityArgs, 3);

// Добавить кнопку "Назад" в конец
RE::Scaleform::GFx::Value backArgs[2] = {1, "Назад"};
movie->Invoke("push", backArgs, 2);

movie->Invoke("setTitle", "Настройки");
```

### Динамическое создание меню
```cpp
// Создать базовое меню массивом
RE::Scaleform::GFx::Value baseMenu;
movie->CreateArray(&baseMenu);

// Заголовок
RE::Scaleform::GFx::Value header;
movie->CreateArray(&header);
header.PushBack(0);
header.PushBack("Главное меню");
baseMenu.PushBack(header);

// Кнопки
RE::Scaleform::GFx::Value newGame;
movie->CreateArray(&newGame);
newGame.PushBack(1);
newGame.PushBack("Новая игра");
baseMenu.PushBack(newGame);

RE::Scaleform::GFx::Value loadGame;
movie->CreateArray(&loadGame);
loadGame.PushBack(1);
loadGame.PushBack("Загрузить");
baseMenu.PushBack(loadGame);

// Добавить все сразу
movie->Invoke("clear", nullptr, 0);
movie->Invoke("pushArray", &baseMenu, 1);

// Найти кнопку "Загрузить" и вставить "Настройки" перед ней
RE::Scaleform::GFx::Value findResult;
RE::Scaleform::GFx::Value findLabel("Загрузить");
movie->Invoke("find", &findResult, &findLabel, 1);

if (findResult.GetInt() >= 0) {
    RE::Scaleform::GFx::Value settingsArgs[3] = {findResult.GetInt(), 1, "Настройки"};
    movie->Invoke("insert", settingsArgs, 3);
}
```

## 📄 Лицензия

MIT License - свободное использование в коммерческих и некоммерческих проектах.

## 🤝 Поддержка

Система полностью документирована и готова к использованию. Все компоненты имеют fallback функциональность и подробное логирование для упрощения отладки.

## ✅ Состояние проекта

### Завершенные задачи
- ✅ **Полная структуризация и оптимизация кода**
- ✅ **Удаление дублирующих и устаревших методов**  
- ✅ **Современный API для интеграции с C++**
- ✅ **Подробная документация всех классов и методов**
- ✅ **Стандартизированные комментарии без избыточных тегов @brief**
- ✅ **Универсальная система колбэков**
- ✅ **Fallback функциональность при отсутствии элементов FLA**
- ✅ **Подробное логирование и отладочная информация**
- ✅ **Чистая архитектура с разделением ответственности**
- ✅ **Полная документация API и примеры использования**
- ✅ **Новый упрощенный API с методами push, pushArray, insert, find**
- ✅ **Исправлены все ошибки компиляции и синтаксические проблемы**
- ✅ **Добавлена инструкция по настройке FLA файла**
- ✅ **Очистка документации v2.0.1 - единые стандарты комментариев**

### Архитектурные улучшения
- **Main.as**: единая точка входа с современным упрощенным API
- **MenuManager**: централизованное управление компонентами
- **MenuScaler**: математическая модель позиционирования  
- **ScrollableMenu**: умная навигация с клавиатуры
- **Components**: полностью документированные UI элементы с чистыми комментариями
- **CustomEvent**: типизированная система событий

### Новый API (v2.0)
- `push(type, label, state?)` - добавить один элемент в конец
- `pushArray(items)` - добавить массив элементов
- `insert(index, type, label, state?)` - вставить элемент перед индексом
- `find(label)` - найти индекс элемента по лейблу
- Упрощенный формат элементов: `[тип, лейбл, состояние?]`
- Полная обратная совместимость со старым форматом

### Обновления v2.0.1
- **Стандартизированная документация** - убраны все теги `@brief`
- **Единообразные комментарии** - унифицированный стиль описаний
- **Структурированный код** - красивые разделители секций
- **Профессиональный вид** - чистые и читаемые комментарии

Система готова к продуктивному использованию в любых проектах Flash/AS3 с интеграцией C++.
