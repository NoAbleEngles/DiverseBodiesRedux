#pragma once

#include <map>
#include <string>
#include <vector>

// Переводы интерфейса меню: Data/F4SE/Plugins/DiverseBodiesRedux/locales/<id>.json
// ({"displayName": "...", "strings": {"ключ": "текст"}}), по файлу на язык Fallout 4
// (en, ru, de, es, esmx, fr, it, ja, ko, pl, ptbr, zhhans, zhhant). Только текст интерфейса -
// имена персонажей и пресетов приходят из игры как есть. Ключи собираются из ui/src/i18n/en.ts
// скриптом tools/gen_locales.py; чего нет в файле языка, UI берёт из встроенного английского.
class MenuLocales
{
public:
	static MenuLocales& get();

	// Читает файлы с диска. Безопасно вызывать повторно.
	void reload();

	// Код языка, на котором должен запуститься UI: [MENU] sLanguage из ini, а при "auto" -
	// язык игры (sLanguage из Fallout4Custom.ini / Fallout4.ini), если для него есть файл, иначе "en".
	std::string activeId() const;

	// Язык игры (sLanguage из Fallout4Custom.ini / Fallout4.ini), приведённый к коду файла перевода;
	// "en", если перевода для него нет.
	std::string gameId() const;

	struct Entry
	{
		std::string id;
		std::string displayName;  // на самом языке: "Русский", "Deutsch"...
	};
	// Все найденные переводы (английский первым, дальше по коду).
	std::vector<Entry> list() const;

	// Принудительно выбранный язык: код перевода или "auto" (язык игры). Пишется в ini
	// ([MENU] sLanguage). false - такого перевода нет.
	bool setLanguage(const std::string& a_id);

	// Выбранное в ini значение как есть: "auto" или код.
	std::string chosen() const;

	// Строки активного языка (пусто, если файла нет: UI покажет встроенный английский).
	const std::map<std::string, std::string>& activeStrings() const;

private:
	MenuLocales() = default;

	std::string resolve(std::string a_wanted) const;

	std::map<std::string, std::map<std::string, std::string>> m_locales;  // по id
	std::map<std::string, std::string> m_names;                           // id -> displayName
	bool m_loaded{ false };
};
