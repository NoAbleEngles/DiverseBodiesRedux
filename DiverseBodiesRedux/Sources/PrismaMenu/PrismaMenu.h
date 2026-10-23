#pragma once

#include "PrismaMenu/PrismaViewHost.h"
#include "Preset/Details/Conditions.h"
#include "Preset/Details/PresetEnums.h"

#include <chrono>
#include <map>
#include <memory>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/json.hpp>

/**
 * @brief Меню выбора пресетов на PrismaUI + React (ui/). Заменяет старое меню на AS 3.0
 * (DirectApply, dbMenu.swf), если установлена PrismaUI_F4.dll нужной минимальной версии
 * (интерфейс V10, PrismaUI 2.0+), включено [MENU] bUsePrismaUI и на месте файлы вью.
 * Иначе main.cpp открывает старое меню.
 *
 * Один экран: слева-сверху список персонажей рядом (наведение подсвечивает персонажа
 * шейдером в мире, после выбора подсветка снимается), ниже вкладки типов пресетов, фильтры и список подходящих пресетов
 * (наведение показывает пресет, клик применяет и сохраняет). Всё в игровом потоке.
 *
 * Вторая страница ("Сохранённые пресеты"): персонажи, для которых DBR хранит пресеты, и
 * предпросмотр точной копии выбранного (PreviewCopy) по кнопке или двойному клику.
 */
class Preset;

class PrismaMenu : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	static PrismaMenu& get();

	// Можно ли открывать это меню вместо AS3. Проверяется один раз, результат пишется в лог.
	static bool IsAvailable();

	void Toggle();
	bool IsOpen() const noexcept { return m_open; }

	// Загрузка сохранения / новая игра: меню закрывается, подсветка снимается.
	void OnGameLoaded();

	RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& a_event,
		RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source) override;

private:
	PrismaMenu() = default;

	struct ActorInfo
	{
		std::uint32_t formID{ 0 };
		std::string   name;
		float         distance{ 0.0f };
		bool          female{ false };
	};

	void EnsureInit();
	void Open();
	void Close();
	void Show();

	void RefreshActors();
	void PushState();
	void PushPreview();
	void PushCurrent();
	void PushToast(bool a_ok, const std::string& a_key, const std::string& a_name = {}, std::size_t a_count = 0);
	boost::json::object PreviewJson() const;
	boost::json::object HotkeyJson() const;

	boost::json::array BuildPresets(RE::Actor* a_actor, bool& a_excluded) const;
	boost::json::array  BuildSaved() const;
	boost::json::object BuildOverrides() const;
	void                StopPreview();
	void                ApplyToCopy(const std::string& a_id);
	void                Transfer();
	void                OnPrune();
	void               ApplyPreset(const std::string& a_id, bool a_persist);

	void OnSelect(const boost::json::object& a_args);
	void OnSetType(const boost::json::object& a_args);
	void OnSetFilter(const boost::json::object& a_args);
	void OnHover(const boost::json::object& a_args);
	void OnPage(const boost::json::object& a_args);
	void OnSetHotkey(const boost::json::object& a_args);
	void OnResetHotkey();
	void OnSetLanguage(const boost::json::object& a_args);
	void OnPreviewActor(const boost::json::object& a_args);
	void OnForget(const boost::json::object& a_args);
	void OnOvToggleType(const boost::json::object& a_args);
	void OnOvAdd(const boost::json::object& a_args);

	PrismaViewHost m_view{ "DiverseBodiesRedux/index.html" };

	bool m_inited{ false };
	bool m_open{ false };
	bool m_showPending{ false };
	bool m_reopenAfterPause{ false };
	std::chrono::steady_clock::time_point m_showRequested{};

	enum class Page { Main, Saved, Overrides, Settings };
	Page          m_page{ Page::Main };
	std::uint32_t m_previewRef{ 0 };          // персонаж, чья копия показана / готовится
	std::string   m_previewStatus{ "none" };  // none / loading / ready / failed
	std::map<PresetType, std::shared_ptr<Preset>> m_pending;  // пресеты, наложенные на копию, но не перенесённые на оригинал

	std::vector<ActorInfo> m_actors;
	std::uint32_t          m_actor{ 0 };  // выбранный
	PresetType             m_type{ PresetType::BODYMORPHS };
	Filter                 m_filter{ AllFilters };

	// последний показанный/применённый пресет: повторный показ того же пропускается
	std::uint32_t m_lastShownActor{ 0 };
	std::string   m_lastShownPreset;

	static inline bool s_failed{ false };  // вью не ожил: дальше работает старое меню
};
