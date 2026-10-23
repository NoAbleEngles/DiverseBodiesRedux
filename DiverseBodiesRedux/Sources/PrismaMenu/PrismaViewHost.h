#pragma once

#include "PrismaMenu/PrismaUI_Compat.h"
#include "PrismaMenu/PrismaUI_F4_API.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

// Один PrismaUI-вью (меню DBR).
//
// Готовность: DOM-ready у PrismaUI приходит раньше, чем React успевает повесить window.*-приёмники,
// поэтому всё, что C++ отправил до этого, теряется. UI сам вызывает window.onUiReady, когда
// слушает; SetOnReady срабатывает на это, а InteropCall'ы, сделанные до него, ставятся в очередь.
//
// Все колбэки JS -> C++ переносятся в игровой поток (F4SE task): PrismaUI зовёт их из своего потока.
class PrismaViewHost
{
public:
	using EventCallback = std::function<void(const std::string& a_argument)>;
	using ReadyCallback = std::function<void()>;

	explicit PrismaViewHost(std::string a_htmlPath) :
		_htmlPath(std::move(a_htmlPath)) {}

	// Есть ли PrismaUI_F4.dll нужной минимальной версии: интерфейс V4 (вью, BindUIEvent,
	// консоль) и V10 (SetViewOwnsEscape, роли вью). V10 появился в PrismaUI 2.0, в 1.8 его нет.
	// Номер версии DLL для этого не годится (в метаданных он не стабилен), проверяется сам API.
	static bool IsSupported();

	// Создаёт вью (скрытым). Повторные вызовы ничего не делают.
	bool Create(PRISMA_UI_API::ViewRole a_role);
	bool IsCreated() const { return _view != 0; }
	bool IsReady() const { return _uiReady; }

	// Вызывается (в игровом потоке) каждый раз, когда UI сообщает onUiReady.
	void SetOnReady(ReadyCallback a_callback) { _onReady = std::move(a_callback); }

	// JS -> C++. Регистрировать до Create().
	void Bind(std::string a_function, EventCallback a_callback);

	// C++ -> JS. До готовности UI копится в очереди (остаётся последний вызов на функцию).
	void Call(const char* a_function, const std::string& a_argument);

	void Show();
	void Hide();
	bool IsHidden() const;
	bool HasFocus() const;
	void Focus(bool a_pauseGame);
	void Unfocus();
	void SetRole(PRISMA_UI_API::ViewRole a_role);
	void SetOwnsEscape(bool a_owns);

private:
	struct Binding
	{
		std::string   name;
		EventCallback callback;
		int           slot{ -1 };
	};

	static void OnDomReadyThunk(PrismaView a_view);
	void        OnDomReady();
	void        OnUiReady();

	std::string          _htmlPath;
	PrismaView           _view{ 0 };
	bool                 _uiReady{ false };
	ReadyCallback        _onReady;
	std::vector<Binding> _bindings;
	std::vector<std::pair<std::string, std::string>> _pending;
};
