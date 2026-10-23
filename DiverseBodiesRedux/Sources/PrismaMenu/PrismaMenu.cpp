#include "PCH.h"
#include "PrismaMenu/PrismaMenu.h"

#include "ActorsManager/ActorsManager.h"
#include "DirectApply/DirectApply.h"
#include "PresetsManager/PresetsManager.h"
#include "PrismaMenu/ActorHighlight.h"
#include "PrismaMenu/MenuLocales.h"
#include "Overrides/SessionOverrides.h"
#include "PrismaMenu/PreviewCopy.h"
#include "globals.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <set>

namespace json = boost::json;

namespace
{
	constexpr auto kViewIndex = "Data/PrismaUI_F4/views/DiverseBodiesRedux/index.html";
	constexpr std::uint32_t kMaxActors = 128;

	// Порядок типов и фильтров во вкладках UI.
	constexpr PresetType kTypes[] = {
		PresetType::BODYMORPHS, PresetType::BODYHAIRS, PresetType::HEAD, PresetType::BODYTATTOOS, PresetType::NAILS
	};
	constexpr Filter kFilters[] = {
		Filter::HasKeyword, Filter::HasNotKeyword, Filter::InFaction, Filter::NotInFaction, Filter::FormIDs, Filter::NotFormIDs
	};

	std::string HexID(std::uint32_t a_id)
	{
		return fmt::format("0x{:08X}", a_id);
	}

	std::uint32_t ParseID(const std::string& a_text)
	{
		if (a_text.empty()) {
			return 0;
		}
		try {
			return static_cast<std::uint32_t>(std::stoul(a_text, nullptr, 16));
		} catch (...) {
			return 0;
		}
	}

	std::string GetString(const json::object& a_object, std::string_view a_key)
	{
		const auto* value = a_object.if_contains(a_key);
		return value && value->is_string() ? std::string(value->as_string()) : std::string{};
	}

	bool GetBool(const json::object& a_object, std::string_view a_key, bool a_default = false)
	{
		const auto* value = a_object.if_contains(a_key);
		return value && value->is_bool() ? value->as_bool() : a_default;
	}

	// Аргументы JS приходят строкой JSON; нечитаемое - пустой объект.
	json::object ParseArgs(const std::string& a_text)
	{
		try {
			const auto value = json::parse(a_text);
			if (value.is_object()) {
				return value.as_object();
			}
		} catch (const std::exception& e) {
			logger::warn("PrismaMenu: unreadable event payload: {}", e.what());
		}
		return {};
	}

	RE::Actor* ActorByID(std::uint32_t a_id)
	{
		return a_id ? functions::getFormByFormID<RE::Actor>(a_id) : nullptr;
	}

	RE::TESNPC* BaseByID(std::uint32_t a_id)
	{
		return a_id ? functions::getFormByFormID<RE::TESNPC>(a_id) : nullptr;
	}

	std::string NpcName(RE::TESNPC* a_npc)
	{
		const char* name = a_npc ? a_npc->GetFullName() : nullptr;
		return name && *name ? name : "";
	}

	// getExcludedTypes: nullopt - не исключён, пустой/нулевой набор - исключён целиком, иначе набор типов.
	bool TypeExcluded(RE::Actor* a_actor, PresetType a_type)
	{
		const auto excluded = ActorsManager::get().getExcludedTypes(a_actor);
		if (!excluded) {
			return false;
		}
		if (!*excluded || (*excluded)->empty()) {
			return true;
		}
		return (*excluded)->contains(a_type);
	}

	using ExcludedOverride = SessionOverrides::ExclusionOverride;

	// Читаемое имя кнопки: "Numpad_Divide" -> "Numpad Divide"; неизвестная - её код.
	std::string ButtonName(int a_code)
	{
		std::string name(RE::ToString(static_cast<RE::BS_BUTTON_CODE>(a_code)));
		if (name.empty()) {
			return fmt::format("#{}", a_code);
		}
		std::ranges::replace(name, '_', ' ');
		return name;
	}
}

PrismaMenu& PrismaMenu::get()
{
	static PrismaMenu instance;
	return instance;
}

bool PrismaMenu::IsAvailable()
{
	static const bool available = [] {
		if (!globals::g_ini || !globals::g_ini->at("MENU/bUsePrismaUI", true)) {
			logger::info("PrismaMenu: disabled by [MENU] bUsePrismaUI -- classic menu");
			return false;
		}
		if (!GetModuleHandleW(L"PrismaUI_F4.dll")) {
			logger::info("PrismaMenu: PrismaUI_F4.dll is not loaded -- classic menu");
			return false;
		}
		if (!PrismaViewHost::IsSupported()) {
			logger::warn("PrismaMenu: PrismaUI_F4.dll is too old (the menu needs PrismaUI 2.0 or newer) -- classic menu");
			return false;
		}
		if (!std::filesystem::exists(kViewIndex)) {
			logger::warn("PrismaMenu: {} is missing -- classic menu", kViewIndex);
			return false;
		}
		logger::info("PrismaMenu: PrismaUI menu enabled");
		return true;
	}();
	return available && !s_failed;
}

void PrismaMenu::EnsureInit()
{
	if (m_inited) {
		return;
	}
	m_inited = true;

	MenuLocales::get().reload();

	if (auto* ui = RE::UI::GetSingleton()) {
		ui->RegisterSink<RE::MenuOpenCloseEvent>(this);
	}

	m_view.Bind("dbrClose", [this](const std::string&) { Close(); });
	m_view.Bind("dbrRefresh", [this](const std::string&) {
		RefreshActors();
		PushState();
	});
	m_view.Bind("dbrSelect", [this](const std::string& a) { OnSelect(ParseArgs(a)); });
	m_view.Bind("dbrSetType", [this](const std::string& a) { OnSetType(ParseArgs(a)); });
	m_view.Bind("dbrSetFilter", [this](const std::string& a) { OnSetFilter(ParseArgs(a)); });
	m_view.Bind("dbrHover", [this](const std::string& a) { OnHover(ParseArgs(a)); });
	m_view.Bind("dbrPreview", [this](const std::string& a) { ApplyPreset(GetString(ParseArgs(a), "id"), false); });
	m_view.Bind("dbrApply", [this](const std::string& a) { ApplyPreset(GetString(ParseArgs(a), "id"), true); });
	m_view.Bind("dbrPage", [this](const std::string& a) { OnPage(ParseArgs(a)); });
	m_view.Bind("dbrPreviewActor", [this](const std::string& a) { OnPreviewActor(ParseArgs(a)); });
	m_view.Bind("dbrStopPreview", [this](const std::string&) { StopPreview(); });
	m_view.Bind("dbrApplyToCopy", [this](const std::string& a) { ApplyToCopy(GetString(ParseArgs(a), "id")); });
	m_view.Bind("dbrTransfer", [this](const std::string&) { Transfer(); });
	m_view.Bind("dbrPrune", [this](const std::string&) { OnPrune(); });
	m_view.Bind("dbrForget", [this](const std::string& a) { OnForget(ParseArgs(a)); });
	m_view.Bind("dbrLift", [](const std::string& a) {
		if (PreviewCopy::get().IsReady()) {
			const auto  args = ParseArgs(a);
			const auto* value = args.if_contains("value");
			if (value && value->is_number()) PreviewCopy::get().Lift(static_cast<float>(value->to_number<double>()));
		}
	});
	m_view.Bind("dbrSetHotkey", [this](const std::string& a) { OnSetHotkey(ParseArgs(a)); });
	m_view.Bind("dbrSetLanguage", [this](const std::string& a) { OnSetLanguage(ParseArgs(a)); });
	m_view.Bind("dbrResetHotkey", [this](const std::string&) { OnResetHotkey(); });
	m_view.Bind("dbrOvToggleType", [this](const std::string& a) { OnOvToggleType(ParseArgs(a)); });
	m_view.Bind("dbrOvAdd", [this](const std::string& a) { OnOvAdd(ParseArgs(a)); });
	m_view.Bind("dbrOvReset", [this](const std::string& a) {
		SessionOverrides::get().resetExclusion(ParseID(GetString(ParseArgs(a), "key")));
		PushState();
	});
	m_view.Bind("dbrOvPreset", [this](const std::string& a) {
		const auto args = ParseArgs(a);
		SessionOverrides::get().setPresetDisabled(GetString(args, "id"), GetBool(args, "disabled"));
		PushState();
	});
	m_view.Bind("dbrOvResetAll", [this](const std::string&) {
		SessionOverrides::get().resetAll();
		PushState();
	});
	m_view.Bind("dbrRotate", [](const std::string& a) {
		if (PreviewCopy::get().IsReady()) {
			const auto  args = ParseArgs(a);
			const auto* value = args.if_contains("value");
			if (value && value->is_number()) PreviewCopy::get().Rotate(static_cast<float>(value->to_number<double>()));
		}
	});
	m_view.Bind("dbrZoom", [](const std::string& a) {
		if (PreviewCopy::get().IsReady()) {
			const auto  args = ParseArgs(a);
			const auto* value = args.if_contains("value");
			if (value && value->is_number()) PreviewCopy::get().Zoom(static_cast<float>(value->to_number<double>()));
		}
	});
	m_view.SetOnReady([this] {
		if (m_showPending) {
			m_showPending = false;
			Show();
		}
	});
}

void PrismaMenu::Toggle()
{
	EnsureInit();
	if (m_open) {
		Close();
		return;
	}
	if (m_showPending) {
		// Второе нажатие, а вью так и не ожил: сдаёмся, дальше открывается старое меню.
		const auto waited = std::chrono::steady_clock::now() - m_showRequested;
		if (waited > std::chrono::seconds(4)) {
			logger::error("PrismaMenu: the view never reported ready -- falling back to the classic menu");
			m_showPending = false;
			s_failed = true;
		}
		return;
	}
	Open();
}

void PrismaMenu::Open()
{
	if (!m_view.Create(PRISMA_UI_API::ViewRole::kPanel)) {
		s_failed = true;
		return;
	}
	if (!m_view.IsReady()) {
		m_showPending = true;
		m_showRequested = std::chrono::steady_clock::now();
		return;
	}
	Show();
}

void PrismaMenu::Show()
{
	RefreshActors();
	PushState();
	m_view.SetOwnsEscape(true);
	m_view.Show();
	// Мир не ставим на паузу: предпросмотр пресетов и подсветка NPC работают в живой сцене.
	m_view.Focus(false);
	m_open = true;
	m_view.Call("onShown", "{}");
	logger::info("PrismaMenu: opened");
}

void PrismaMenu::Close()
{
	m_showPending = false;
	ActorHighlight::Set(0);
	StopPreview();
	m_page = Page::Main;
	if (!m_open) {
		return;
	}
	m_view.SetOwnsEscape(false);
	m_view.Unfocus();
	m_view.Hide();
	m_open = false;
	logger::info("PrismaMenu: closed");
}

void PrismaMenu::OnGameLoaded()
{
	if (!m_inited) {
		return;
	}
	Close();
	PreviewCopy::get().End();
	m_actor = 0;
	m_lastShownActor = 0;
	m_lastShownPreset.clear();
}

RE::BSEventNotifyControl PrismaMenu::ProcessEvent(const RE::MenuOpenCloseEvent& a_event,
	RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
{
	const std::string_view name = a_event.menuName.c_str();
	if (name.starts_with("PrismaUI_")) {
		return RE::BSEventNotifyControl::kContinue;  // служебные меню самого PrismaUI (фокус/пауза)
	}

	if (name == "LoadingMenu" && a_event.opening && m_open) {
		F4SE::GetTaskInterface()->AddTask([this] { Close(); });
		return RE::BSEventNotifyControl::kContinue;
	}

	// Alt-Tab открывает PauseMenu и отнимает фокус у PrismaUI: отходим в сторону и возвращаемся.
	if (name == "PauseMenu") {
		if (a_event.opening && m_open) {
			m_reopenAfterPause = true;
			F4SE::GetTaskInterface()->AddTask([this] { Close(); });
		} else if (!a_event.opening && m_reopenAfterPause) {
			m_reopenAfterPause = false;
			F4SE::GetTaskInterface()->AddTask([this] { Open(); });
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	if (!a_event.opening && m_open) {
		F4SE::GetTaskInterface()->AddTask([this] {
			if (m_open && !m_view.HasFocus()) {
				m_view.Focus(false);
			}
		});
	}
	return RE::BSEventNotifyControl::kContinue;
}

void PrismaMenu::RefreshActors()
{
	m_actors.clear();
	auto* player = RE::PlayerCharacter::GetSingleton();
	if (!player) {
		return;
	}

	auto found = functions::getActorsInRange(player, 10000, 1024, false, [](const RE::Actor* a_actor) -> bool {
		return a_actor && a_actor->GetFullyLoaded3D() && !ActorsManager::isPreviewCopy(a_actor->formID) && ActorsManager::isQualifiedRace(a_actor) && !ActorsManager::isExcluded(a_actor);
	});

	const auto origin = player->GetPosition();
	for (auto* actor : found) {
		if (!actor) {
			continue;
		}
		ActorInfo info;
		info.formID = actor->formID;
		const char* name = actor->GetDisplayFullName();
		info.name = name && *name ? name : "";
		info.distance = origin.GetDistance(actor->GetPosition());
		info.female = actor->GetSex() == RE::Actor::Sex::Female;
		m_actors.push_back(std::move(info));
	}
	std::ranges::sort(m_actors, [](const ActorInfo& a, const ActorInfo& b) { return a.distance < b.distance; });
	if (m_actors.size() > kMaxActors) {
		m_actors.resize(kMaxActors);
	}
	logger::info("PrismaMenu: {} actor(s) in range", m_actors.size());
}

// Тот же отбор, что в старом меню: тип пресета + совпадение хотя бы по одному фильтру, из них
// остаются только пресеты с максимальным уровнем совпадения.
json::array PrismaMenu::BuildPresets(RE::Actor* a_actor, bool& a_excluded) const
{
	json::array result;
	a_excluded = false;
	if (!a_actor) {
		return result;
	}

	auto& actorsManager = ActorsManager::get();
	if (TypeExcluded(a_actor, m_type)) {
		a_excluded = true;
		return result;
	}

	const Filter filter = m_filter | Filter::Gender;  // пол проверяется всегда
	auto presets = PresetsManager::get().getPresets(a_actor, [&](const RE::Actor* a_checked, const std::shared_ptr<Preset> a_preset) -> bool {
		return a_preset->type() == m_type && a_preset->check(a_checked, filter) != CoincidenceLevel::NONE;
	});

	std::vector<int> levels;
	levels.reserve(presets.size());
	int maxLevel = 0;
	for (const auto& preset : presets) {
		const int level = preset ? static_cast<int>(preset->check(a_actor, filter)) : 0;
		levels.push_back(level);
		maxLevel = std::max(maxLevel, level);
	}

	const auto current = actorsManager.getActorPreset(a_actor->formID, m_type);
	for (std::size_t i = 0; i < presets.size(); ++i) {
		if (!presets[i] || levels[i] != maxLevel) {
			continue;
		}
		json::object item;
		item["id"] = presets[i]->id();
		item["current"] = current && current->id() == presets[i]->id();
		result.push_back(std::move(item));
	}
	return result;
}

json::object PrismaMenu::PreviewJson() const
{
	json::object preview;
	preview["ref"] = m_previewRef ? HexID(m_previewRef) : std::string{};
	preview["status"] = m_previewStatus;
	// Править копию и переносить на оригинал можно, только пока оригинал есть в памяти: условия
	// пресетов проверяются по нему.
	preview["editable"] = m_previewRef != 0 && ActorByID(m_previewRef) != nullptr;
	json::array pending;
	for (const auto& [type, preset] : m_pending) {
		pending.push_back(json::object{ { "type", GetPresetTypeString(type) }, { "id", preset->id() } });
	}
	preview["pending"] = std::move(pending);
	return preview;
}

void PrismaMenu::PushState()
{
	auto& locales = MenuLocales::get();

	json::object state;
	state["language"] = locales.activeId();
	json::object strings;
	for (const auto& [key, text] : locales.activeStrings()) {
		strings[key] = text;
	}
	state["strings"] = std::move(strings);

	json::array actors;
	for (const auto& info : m_actors) {
		json::object item;
		item["ref"] = HexID(info.formID);
		item["name"] = info.name;
		item["distance"] = std::round(info.distance / 70.0f * 10.0f) / 10.0f;  // 70 игровых единиц = 1 м
		item["female"] = info.female;
		actors.push_back(std::move(item));
	}
	state["actors"] = std::move(actors);

	auto* actor = ActorByID(m_actor);
	if (!actor) {
		m_actor = 0;
	}
	state["selected"] = m_actor ? HexID(m_actor) : std::string{};

	json::array types;
	for (const auto type : kTypes) {
		types.push_back(json::string(GetPresetTypeString(type)));
	}
	state["types"] = std::move(types);
	state["type"] = GetPresetTypeString(m_type);

	json::array filters;
	for (const auto filter : kFilters) {
		json::object item;
		item["id"] = std::string(getFilterString(filter));
		item["on"] = (m_filter & filter) != Filter::None;
		filters.push_back(std::move(item));
	}
	state["filters"] = std::move(filters);

	// Список пресетов считается для того, чьи пресеты правят: выбранного персонажа на главной
	// странице, оригинала показанной копии на странице сохранённых.
	RE::Actor* subject = nullptr;
	if (m_page == Page::Main) {
		subject = actor;
	} else if (m_page == Page::Saved && m_previewStatus == "ready") {
		subject = ActorByID(m_previewRef);
	}
	bool excluded = false;
	state["page"] = m_page == Page::Saved ? "saved" : m_page == Page::Overrides ? "overrides" : m_page == Page::Settings ? "settings" : "main";
	state["settings"] = HotkeyJson();
	state["presets"] = BuildPresets(subject, excluded);
	state["excluded"] = excluded;
	state["saved"] = m_page == Page::Saved ? BuildSaved() : json::array{};
	state["overrides"] = m_page == Page::Overrides ? BuildOverrides() : json::object{};
	state["preview"] = PreviewJson();

	m_view.Call("onState", json::serialize(state));
}

void PrismaMenu::PushPreview()
{
	m_view.Call("onPreview", json::serialize(PreviewJson()));
}

// Персонажи, для которых в хранилище есть пресеты. Загруженные - первыми по расстоянию, потом
// существующие, но не загруженные, потом те, кого игра сейчас не знает (временные 0xFF...... из
// других ячеек, давно пропавшие и т.п.). Копию предпросмотра не показываем.
json::array PrismaMenu::BuildSaved() const
{
	struct Row
	{
		std::uint32_t                                   id{ 0 };
		std::string                                     name;
		float                                           distance{ -1.0f };
		int                                             rank{ 2 };  // 0 загружен, 1 не загружен, 2 нет в памяти
		bool                                            previewable{ false };
		std::vector<std::pair<PresetType, std::string>> presets;
	};

	auto*            player = RE::PlayerCharacter::GetSingleton();
	std::vector<Row> rows;
	for (auto& stored : ActorsManager::get().getStoredPresets()) {
		if (ActorsManager::isPreviewCopy(stored.formID)) {
			continue;
		}
		Row row;
		row.id = stored.formID;
		row.presets = std::move(stored.presets);
		if (auto* actor = ActorByID(stored.formID)) {
			const char* name = actor->GetDisplayFullName();
			row.name = name && *name ? name : "";
			row.rank = actor->GetFullyLoaded3D() ? 0 : 1;
			if (row.rank == 0 && player) {
				row.distance = player->GetPosition().GetDistance(actor->GetPosition());
			}
			row.previewable = actor->GetNPC() != nullptr;
		} else if (stored.baseId) {
			// Актёра в памяти нет, но известно, чей он: базу можно показать
			if (auto* base = BaseByID(stored.baseId)) {
				row.name = NpcName(base);
				row.previewable = true;
			}
		}
		rows.push_back(std::move(row));
	}
	std::ranges::sort(rows, [](const Row& a, const Row& b) {
		if (a.rank != b.rank) return a.rank < b.rank;
		if (a.rank == 0) return a.distance < b.distance;
		return a.name < b.name;
	});
	constexpr std::size_t kMaxSaved = 400;
	if (rows.size() > kMaxSaved) {
		rows.resize(kMaxSaved);
	}

	json::array result;
	for (const auto& row : rows) {
		json::object item;
		item["ref"] = HexID(row.id);
		item["name"] = row.name;
		item["status"] = row.rank == 0 ? "loaded" : row.rank == 1 ? "unloaded" : "missing";
		item["previewable"] = row.previewable;
		item["distance"] = row.rank == 0 ? std::round(row.distance / 70.0f * 10.0f) / 10.0f : -1.0f;
		json::array presets;
		for (const auto type : kTypes) {  // в порядке вкладок
			for (const auto& [presetType, id] : row.presets) {
				if (presetType == type) {
					presets.push_back(json::object{ { "type", GetPresetTypeString(type) }, { "id", id } });
				}
			}
		}
		item["presets"] = std::move(presets);
		result.push_back(std::move(item));
	}
	return result;
}

// Вкладка оверрайдов: правила исключения из JSON и то, что игрок поменял в этой сессии; пресеты
// выбранного типа с отметкой "отключён".
json::object PrismaMenu::BuildOverrides() const
{
	auto&       overrides = SessionOverrides::get();
	const auto  rules = ActorsManager::get().getExclusionRules();
	const auto  changes = overrides.exclusions();

	struct Row
	{
		std::uint32_t        key{ 0 };
		std::string          name;
		std::string          kind;
		bool                 fromJson{ false };
		std::set<PresetType> base;
	};
	std::map<std::uint32_t, Row> rows;
	for (const auto& rule : rules) {
		auto& row = rows[rule.key];
		row.key = rule.key;
		row.fromJson = true;
		row.base = rule.types;
	}
	for (const auto& [key, entry] : changes) {
		rows[key].key = key;
	}

	std::vector<Row> ordered;
	for (auto& [key, row] : rows) {
		auto* form = RE::TESForm::GetFormByID(key);
		if (auto* actor = form ? RE::fallout_cast<RE::Actor*>(form) : nullptr) {
			const char* name = actor->GetDisplayFullName();
			row.name = name && *name ? name : "";
			row.kind = "actor";
		} else if (auto* npc = form ? RE::fallout_cast<RE::TESNPC*>(form) : nullptr) {
			row.name = NpcName(npc);
			row.kind = "npc";
		} else {
			row.kind = "unknown";
		}
		ordered.push_back(row);
	}
	std::ranges::sort(ordered, [](const Row& a, const Row& b) {
		if (a.fromJson != b.fromJson) return a.fromJson;
		if (a.name.empty() != b.name.empty()) return !a.name.empty();
		return a.name < b.name;
	});
	constexpr std::size_t kMaxRules = 500;
	if (ordered.size() > kMaxRules) {
		ordered.resize(kMaxRules);
	}

	const auto typesJson = [](const std::set<PresetType>& a_types) {
		json::array array;
		for (const auto type : kTypes) {
			if (a_types.contains(type)) {
				array.push_back(json::string(GetPresetTypeString(type)));
			}
		}
		return array;
	};

	json::array exclusions;
	for (const auto& row : ordered) {
		ExcludedOverride entry;
		if (const auto it = changes.find(row.key); it != changes.end()) {
			entry = it->second;
		}
		std::set<PresetType> effective = row.base;
		for (const auto type : entry.allow) effective.erase(type);
		for (const auto type : entry.deny) effective.insert(type);

		json::object item;
		item["key"] = HexID(row.key);
		item["name"] = row.name;
		item["kind"] = row.kind;
		item["source"] = row.fromJson ? "json" : "session";
		item["base"] = typesJson(row.base);
		item["allow"] = typesJson(entry.allow);
		item["deny"] = typesJson(entry.deny);
		item["effective"] = typesJson(effective);
		exclusions.push_back(std::move(item));
	}

	const auto  disabled = overrides.disabledPresets();
	json::array presets;
	for (const auto& preset : PresetsManager::get().getAllPresets()) {
		if (preset && preset->type() == m_type) {
			presets.push_back(json::object{ { "id", preset->id() }, { "disabled", disabled.contains(preset->id()) } });
		}
	}

	json::object result;
	result["exclusions"] = std::move(exclusions);
	result["presets"] = std::move(presets);
	result["disabledTotal"] = disabled.size();
	result["anyOverride"] = overrides.any();
	return result;
}

void PrismaMenu::StopPreview()
{
	const bool was = m_previewRef != 0 || PreviewCopy::get().IsActive();
	PreviewCopy::get().End();
	m_previewRef = 0;
	m_previewStatus = "none";
	m_pending.clear();
	if (was && m_open) {
		PushPreview();
	}
}

void PrismaMenu::OnPage(const json::object& a_args)
{
	const auto name = GetString(a_args, "page");
	const auto page = name == "saved" ? Page::Saved : name == "overrides" ? Page::Overrides : name == "settings" ? Page::Settings : Page::Main;
	ActorHighlight::Set(0);
	if (m_page == Page::Saved && page != Page::Saved) {
		StopPreview();
	}
	m_page = page;
	if (page == Page::Main || page == Page::Overrides) {
		RefreshActors();  // главная страница и выбор персонажа для исключения
	}
	PushState();
}

// Копия строится только по явной команде (кнопка или двойной клик), не по наведению.
void PrismaMenu::OnPreviewActor(const json::object& a_args)
{
	const auto id = ParseID(GetString(a_args, "ref"));

	PreviewCopy::Source source;
	source.actor = ActorByID(id);
	// Что для него сохранено (и что накладывается на копию, если оригинал не в сцене)
	auto& manager = ActorsManager::get();
	for (const auto& stored : manager.getStoredPresets()) {
		if (stored.formID != id) {
			continue;
		}
		if (!source.actor && stored.baseId) {
			source.npc = BaseByID(stored.baseId);
		}
		for (const auto type : kTypes) {
			if (auto preset = manager.getActorPreset(id, type)) {
				source.presets.push_back(std::move(preset));
			}
		}
		break;
	}

	PreviewCopy::get().End();
	m_pending.clear();
	m_previewRef = id;
	if (!source.actor && !source.npc) {
		logger::warn("PrismaMenu: {:08X} can't be previewed: the game doesn't know the actor or its base", id);
		m_previewStatus = "failed";
		PushPreview();
		return;
	}
	m_previewStatus = "loading";
	PushPreview();
	PreviewCopy::get().Begin(source, [this, id](bool a_ok) {
		if (m_previewRef != id) {
			return;  // пока готовилась, выбрали другого или закрыли
		}
		m_previewStatus = a_ok ? "ready" : "failed";
		if (!a_ok) {
			logger::warn("PrismaMenu: preview of {:08X} failed", id);
		}
		PushState();  // готовая копия показывает редактор пресетов
	});
}

// Пресет на копию: проверить, как он выглядит, прежде чем отдавать оригиналу.
void PrismaMenu::ApplyToCopy(const std::string& a_id)
{
	auto* copy = PreviewCopy::get().CopyActor();
	if (!PreviewCopy::get().IsReady() || !copy || a_id.empty()) {
		return;
	}
	auto preset = PresetsManager::get().getPreset(a_id);
	if (!preset) {
		PushToast(false, "toast.error", a_id);
		return;
	}
	if (auto* original = ActorByID(m_previewRef); original && TypeExcluded(original, preset->type())) {
		PushToast(false, "toast.error", a_id);
		return;
	}
	ActorsManager::get().applyActorPreset(copy, preset, true);
	m_pending[preset->type()] = preset;
	PushPreview();
}

// Всё, что накладывали на копию, - оригиналу: применить и сохранить в хранилище (как кнопка
// применения на главной странице).
void PrismaMenu::Transfer()
{
	auto* original = ActorByID(m_previewRef);
	if (!original || m_pending.empty()) {
		return;
	}
	auto&       manager = ActorsManager::get();
	std::size_t done = 0;
	for (const auto& [type, preset] : m_pending) {
		if (TypeExcluded(original, type)) {
			continue;
		}
		manager.applyActorPreset(original, preset, true);
		if (manager.updateActorPresetInStorage(original, preset)) {
			++done;
		}
	}
	m_pending.clear();
	PushToast(done > 0, "toast.transferred", {}, done);
	PushState();  // чипы сохранённых пресетов и метки "текущий"
}

void PrismaMenu::OnPrune()
{
	const auto result = ActorsManager::get().pruneStalePresets(false);
	PushToast(true, "toast.pruned", {}, result.removed());
	PushState();
}

void PrismaMenu::OnForget(const json::object& a_args)
{
	const auto id = ParseID(GetString(a_args, "ref"));
	if (id && ActorsManager::get().forgetActor(id)) {
		if (m_previewRef == id) {
			StopPreview();
		}
		PushToast(true, "toast.forgotten", GetString(a_args, "name").empty() ? HexID(id) : GetString(a_args, "name"));
	}
	PushState();
}

void PrismaMenu::OnOvToggleType(const json::object& a_args)
{
	const auto key = ParseID(GetString(a_args, "key"));
	const auto type = GetPresetTypeFromString(GetString(a_args, "type"));
	if (!key || type == PresetType::NONE || type >= PresetType::END) {
		return;
	}
	bool baseExcluded = false;
	for (const auto& rule : ActorsManager::get().getExclusionRules()) {
		if (rule.key == key) {
			baseExcluded = rule.types.contains(type);
			break;
		}
	}
	SessionOverrides::get().toggleExclusion(key, type, baseExcluded);
	PushState();
}

void PrismaMenu::OnOvAdd(const json::object& a_args)
{
	const auto id = ParseID(GetString(a_args, "ref"));
	if (ActorByID(id)) {
		SessionOverrides::get().addExclusion(id);
	}
	RefreshActors();  // исключённый больше не попадает в список персонажей
	PushState();
}

void PrismaMenu::PushCurrent()
{
	json::object current;
	if (auto* actor = ActorByID(m_actor)) {
		const auto preset = ActorsManager::get().getActorPreset(actor->formID, m_type);
		current["id"] = preset ? preset->id() : std::string{};
	}
	m_view.Call("onCurrent", json::serialize(current));
}

// key - ключ строки перевода (toast.*), name и n - её подстановки {name} и {n}.
void PrismaMenu::PushToast(bool a_ok, const std::string& a_key, const std::string& a_name, std::size_t a_count)
{
	json::object toast;
	toast["ok"] = a_ok;
	toast["key"] = a_key;
	toast["name"] = a_name;
	toast["n"] = a_count;
	m_view.Call("onToast", json::serialize(toast));
}

void PrismaMenu::OnSelect(const json::object& a_args)
{
	const auto id = ParseID(GetString(a_args, "ref"));
	if (!ActorByID(id)) {
		logger::warn("PrismaMenu: selected actor {:08X} is gone", id);
		RefreshActors();
		PushState();
		return;
	}
	m_actor = id;
	m_lastShownActor = 0;
	m_lastShownPreset.clear();
	ActorHighlight::Set(0);  // выбран - подсветка больше не нужна
	PushState();
}

void PrismaMenu::OnSetType(const json::object& a_args)
{
	const auto type = GetPresetTypeFromString(GetString(a_args, "type"));
	if (type == PresetType::NONE || type >= PresetType::END) {
		return;
	}
	m_type = type;
	m_lastShownPreset.clear();
	PushState();
}

void PrismaMenu::OnSetFilter(const json::object& a_args)
{
	const auto filter = getFilterFromString(GetString(a_args, "id"));
	if (filter == Filter::None || filter == Filter::Gender) {
		return;
	}
	m_filter = GetBool(a_args, "on") ? (m_filter | filter) : (m_filter & ~filter);
	m_filter |= Filter::Gender;
	PushState();
}

void PrismaMenu::OnHover(const json::object& a_args)
{
	// Подсветка только пока персонажа выбирают: наведён в списке - светится, курсор ушёл - нет.
	const auto hovered = ParseID(GetString(a_args, "ref"));
	ActorHighlight::Set(hovered && ActorByID(hovered) ? hovered : 0);
}

void PrismaMenu::ApplyPreset(const std::string& a_id, bool a_persist)
{
	auto* actor = ActorByID(m_actor);
	if (!actor || a_id.empty()) {
		return;
	}

	auto preset = PresetsManager::get().getPreset(a_id);
	if (!preset) {
		logger::error("PrismaMenu: preset '{}' not found", a_id);
		PushToast(false, "toast.error", a_id);
		return;
	}

	auto& manager = ActorsManager::get();
	if (TypeExcluded(actor, preset->type())) {
		logger::warn("PrismaMenu: preset type '{}' is excluded for [0x{:x}], not applied", GetPresetTypeString(preset->type()), actor->GetFormID());
		PushToast(false, "toast.error", a_id);
		return;
	}

	// Предпросмотр одного и того же пресета подряд не повторяем (каждый - сброс 3D актёра).
	if (!a_persist && m_lastShownActor == m_actor && m_lastShownPreset == a_id) {
		return;
	}

	manager.applyActorPreset(actor, preset, true);
	m_lastShownActor = m_actor;
	m_lastShownPreset = a_id;

	if (a_persist) {
		const bool stored = manager.updateActorPresetInStorage(actor, preset);
		PushToast(stored, stored ? "toast.applied" : "toast.error", a_id);
		PushCurrent();
	}
}

// Кнопка открытия меню и время удержания (общие для меню Prisma UI и классического).
json::object PrismaMenu::HotkeyJson() const
{
	auto&     handler = MenuButtonHandler::get();
	const int key = static_cast<int>(handler.button());
	json::object settings;
	settings["key"] = key;
	settings["keyName"] = ButtonName(key);
	settings["holdSeconds"] = handler.holdSeconds();
	settings["defaultKey"] = MenuButtonHandler::kDefaultButton;
	settings["defaultKeyName"] = ButtonName(MenuButtonHandler::kDefaultButton);
	settings["defaultHoldSeconds"] = MenuButtonHandler::kDefaultHoldSeconds;
	settings["maxHoldSeconds"] = MenuButtonHandler::kMaxHoldSeconds;

	// Язык интерфейса: "auto" - язык игры, остальные принудительно.
	const auto& locales = MenuLocales::get();
	settings["language"] = locales.chosen();
	settings["gameLanguage"] = locales.gameId();
	json::array languages;
	for (const auto& entry : locales.list()) {
		languages.push_back(json::object{ { "id", entry.id }, { "name", entry.displayName } });
	}
	settings["languages"] = std::move(languages);
	return settings;
}

void PrismaMenu::OnSetHotkey(const json::object& a_args)
{
	auto& handler = MenuButtonHandler::get();
	if (const auto* key = a_args.if_contains("key"); key && key->is_number()) {
		const int code = static_cast<int>(key->to_number<std::int64_t>());
		if (!handler.setButton(static_cast<RE::BS_BUTTON_CODE>(code))) {
			logger::warn("PrismaMenu: button {} can't open the menu", code);
			PushToast(false, "toast.keyInvalid");
			PushState();  // вернуть в UI прежнее значение
			return;
		}
		PushToast(true, "toast.key", ButtonName(code));
	}
	if (const auto* hold = a_args.if_contains("holdSeconds"); hold && hold->is_number()) {
		handler.setHoldSeconds(static_cast<float>(hold->to_number<double>()));
	}
	if (!handler.saveToIni()) {
		logger::warn("PrismaMenu: couldn't write the menu key settings to the ini; they apply until the game is closed");
	}
	PushState();
}

void PrismaMenu::OnResetHotkey()
{
	auto& handler = MenuButtonHandler::get();
	handler.setButton(static_cast<RE::BS_BUTTON_CODE>(MenuButtonHandler::kDefaultButton));
	handler.setHoldSeconds(MenuButtonHandler::kDefaultHoldSeconds);
	handler.saveToIni();
	PushToast(true, "toast.key", ButtonName(MenuButtonHandler::kDefaultButton));
	PushState();
}

void PrismaMenu::OnSetLanguage(const json::object& a_args)
{
	if (!MenuLocales::get().setLanguage(GetString(a_args, "language"))) {
		logger::warn("PrismaMenu: unknown interface language '{}'", GetString(a_args, "language"));
	}
	PushState();  // новые строки и язык приходят вместе с состоянием
}
