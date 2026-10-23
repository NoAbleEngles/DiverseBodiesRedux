#include "PCH.h"
#include "PrismaMenu/PrismaViewHost.h"

#include <algorithm>
#include <array>

namespace
{
	struct Api
	{
		PRISMA_UI_API::IVPrismaUI4*  v4{ nullptr };
		PRISMA_UI_API::IVPrismaUI10* v10{ nullptr };
		bool                         requested{ false };
	};

	Api& GetApi()
	{
		static Api api;
		if (!api.requested) {
			api.requested = true;
			api.v4 = PRISMA_UI_API::RequestPluginAPI<PRISMA_UI_API::IVPrismaUI4>();
			bool modern = false;
			api.v10 = PRISMA_UI_COMPAT::RequestV10(&modern);
			logger::info("PrismaViewHost: V4 {}, V10 {}", api.v4 ? "ok" : "MISSING",
				api.v10 ? (modern ? "ok (2.2+)" : "ok (legacy 2.0.x)") : "MISSING");
		}
		return api;
	}

	// Колбэки PrismaUI - голые указатели на функции без user data, поэтому каждая привязанная
	// JS-функция получает слот и свой шаблонный трамплин.
	constexpr std::size_t kMaxSlots = 32;
	std::array<PrismaViewHost::EventCallback, kMaxSlots> g_slots;
	std::size_t                                          g_nextSlot = 0;

	template <std::size_t N>
	void Trampoline(const char* a_argument)
	{
		if (!g_slots[N]) {
			return;
		}
		// Вызывается из потока PrismaUI: копия аргумента и выполнение в игровом потоке.
		std::string argument = a_argument ? a_argument : "";
		F4SE::GetTaskInterface()->AddTask([argument = std::move(argument)]() {
			if (g_slots[N]) {
				g_slots[N](argument);
			}
		});
	}

	template <std::size_t... I>
	constexpr auto MakeTrampolines(std::index_sequence<I...>)
	{
		return std::array<PRISMA_UI_API::JSListenerCallback, kMaxSlots>{ &Trampoline<I>... };
	}
	constexpr auto kTrampolines = MakeTrampolines(std::make_index_sequence<kMaxSlots>{});

	std::vector<PrismaViewHost*> g_hosts;  // handle вью -> хост для DOM-ready

	// Консоль JS -> лог плагина (ошибки UI).
	void OnConsoleMessage(PrismaView a_view, PRISMA_UI_API::ConsoleMessageLevel a_level, const char* a_message)
	{
		if (a_level == PRISMA_UI_API::ConsoleMessageLevel::Error || a_level == PRISMA_UI_API::ConsoleMessageLevel::Warning) {
			logger::warn("[JS view {}] {}", a_view, a_message ? a_message : "");
		} else {
			logger::info("[JS view {}] {}", a_view, a_message ? a_message : "");
		}
	}
}

bool PrismaViewHost::IsSupported()
{
	const auto& api = GetApi();
	return api.v4 && api.v10;
}

bool PrismaViewHost::Create(PRISMA_UI_API::ViewRole a_role)
{
	if (_view != 0) {
		return true;
	}
	auto& api = GetApi();
	if (!api.v4) {
		logger::critical("PrismaViewHost: PrismaUI_F4 not available -- can't create '{}'", _htmlPath);
		return false;
	}

	if (std::ranges::find(g_hosts, this) == g_hosts.end()) {
		g_hosts.push_back(this);
	}
	// onUiReady привязывает сам хост: так UI сообщает, что приёмники window.* уже стоят.
	if (std::ranges::none_of(_bindings, [](const Binding& a_binding) { return a_binding.name == "onUiReady"; })) {
		Bind("onUiReady", [this](const std::string&) { OnUiReady(); });
	}
	_view = api.v4->CreateView(_htmlPath.c_str(), &PrismaViewHost::OnDomReadyThunk);
	api.v4->RegisterConsoleCallback(_view, OnConsoleMessage);
	api.v4->Hide(_view);
	SetRole(a_role);
	logger::info("PrismaViewHost: created '{}' (view {})", _htmlPath, _view);
	return _view != 0;
}

void PrismaViewHost::OnDomReadyThunk(PrismaView a_view)
{
	for (auto* host : g_hosts) {
		if (host->_view == a_view) {
			host->OnDomReady();
		}
	}
}

void PrismaViewHost::OnDomReady()
{
	auto& api = GetApi();
	// Привязка только здесь: BindUIEvent до DOM-ready ненадёжен.
	for (auto& binding : _bindings) {
		if (binding.slot < 0) {
			if (g_nextSlot >= kMaxSlots) {
				logger::critical("PrismaViewHost: out of callback slots binding '{}'", binding.name);
				continue;
			}
			binding.slot = static_cast<int>(g_nextSlot++);
			g_slots[binding.slot] = binding.callback;
		}
		api.v4->BindUIEvent(_view, binding.name.c_str(), kTrampolines[binding.slot]);
	}
	logger::info("PrismaViewHost: DOM ready '{}'", _htmlPath);
}

void PrismaViewHost::OnUiReady()
{
	_uiReady = true;
	logger::info("PrismaViewHost: UI ready '{}' -- flushing {} queued call(s)", _htmlPath, _pending.size());
	auto& api = GetApi();
	for (auto& [function, argument] : _pending) {
		api.v4->InteropCall(_view, function.c_str(), argument.c_str());
	}
	_pending.clear();
	if (_onReady) {
		_onReady();
	}
}

void PrismaViewHost::Bind(std::string a_function, EventCallback a_callback)
{
	_bindings.push_back({ std::move(a_function), std::move(a_callback), -1 });
}

void PrismaViewHost::Call(const char* a_function, const std::string& a_argument)
{
	if (!_view) {
		return;
	}
	if (!_uiReady) {
		// Все пуши - полное состояние, актуален только последний на функцию.
		std::erase_if(_pending, [&](const auto& a_entry) { return a_entry.first == a_function; });
		_pending.emplace_back(a_function, a_argument);
		return;
	}
	GetApi().v4->InteropCall(_view, a_function, a_argument.c_str());
}

void PrismaViewHost::Show()
{
	if (_view) GetApi().v4->Show(_view);
}

void PrismaViewHost::Hide()
{
	if (_view) GetApi().v4->Hide(_view);
}

bool PrismaViewHost::IsHidden() const
{
	return !_view || GetApi().v4->IsHidden(_view);
}

bool PrismaViewHost::HasFocus() const
{
	return _view && GetApi().v4->HasFocus(_view);
}

void PrismaViewHost::Focus(bool a_pauseGame)
{
	if (_view) GetApi().v4->Focus(_view, a_pauseGame, false);
}

void PrismaViewHost::Unfocus()
{
	if (_view) GetApi().v4->Unfocus(_view);
}

void PrismaViewHost::SetRole(PRISMA_UI_API::ViewRole a_role)
{
	if (auto* v10 = GetApi().v10; v10 && _view) v10->SetViewRole(_view, a_role);
}

void PrismaViewHost::SetOwnsEscape(bool a_owns)
{
	if (auto* v10 = GetApi().v10; v10 && _view) v10->SetViewOwnsEscape(_view, a_owns);
}
