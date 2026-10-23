#include "ActorResetQueue.h"
#include "LooksMenu/LooksMenuInterfaces.h"
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <chrono>
#include <thread>

namespace logger = F4SE::log;

namespace
{
	// TESNPC::GetBodyTintColor (Fallout4.exe 1.10.163, RVA 0x5B58E0).
	// Возвращает цвет кожи NPC в out. force=true — заново найти в tintingData запись слота
	// kSkinTone и пересчитать кэш TESNPC::bodyTintColor{R,G,B,A}; force=false — вернуть кэш.
	// Голова зовёт её с force=true (поэтому берёт свежие тинты), тело и ладони — с force=false.
	// 5-й аргумент (байт на стеке) во всех вызовах движка равен 0.
	using GetBodyTintColor_t = void* (*)(RE::TESNPC*, RE::NiColorA*, void*, bool, bool);

	// Fallout4.exe RVA 0x68C530: рекурсивно красит skin-tint шейдеры (тип 5) поддерева в цвет color.
	using ApplySkinTintToTree_t = void* (*)(RE::NiAVObject*, const RE::NiColorA*);

	inline bool getBodyTintColor(RE::TESNPC* npc, RE::NiColorA& out, bool force) {
		if (!npc) return false;
		REL::Relocation<GetBodyTintColor_t> func{ REL::Module::get().base() + 0x5B58E0 };
		out = RE::NiColorA{};
		func(npc, &out, nullptr, force, false);
		return true;
	}

	inline void applySkinTintToTree(RE::NiAVObject* root, const RE::NiColorA& color) {
		if (!root) return;
		REL::Relocation<ApplySkinTintToTree_t> func{ REL::Module::get().base() + 0x68C530 };
		func(root, &color);
	}

	inline uint32_t packBodyTint(const RE::TESNPC* npc) {
		return (static_cast<uint32_t>(static_cast<uint8_t>(npc->bodyTintColorA)) << 24) |
		       (static_cast<uint32_t>(static_cast<uint8_t>(npc->bodyTintColorR)) << 16) |
		       (static_cast<uint32_t>(static_cast<uint8_t>(npc->bodyTintColorG)) << 8) |
		        static_cast<uint32_t>(static_cast<uint8_t>(npc->bodyTintColorB));
	}
}

// ---------------------------------------------------------------------------
// RunTask: выполняется на игровом потоке (F4SE task queue)
// ---------------------------------------------------------------------------

void ActorResetQueue::RunTask::Run()
{
	// Получаем актёра по formID на момент выполнения — он может быть уже выгружен.
	RE::Actor* actor = nullptr;
	if (auto* form = RE::TESForm::GetFormByID(formID); form) {
		actor = RE::fallout_cast<RE::Actor*>(form);
	}

	// Независимо от того, нашли ли актёра — вызываем ExecuteReset,
	// который сам вычистит запись из очереди.
	ActorResetQueue::GetSingleton().ExecuteReset(formID);
	(void)actor; // actor проверяется внутри ExecuteReset
}

// ---------------------------------------------------------------------------
// Синглтон (нужен RunTask, который не может держать указатель на очередь)
// ---------------------------------------------------------------------------

// Объявлен в .h как static method — реализуем здесь.
// Добавляем static-метод в тело класса через отдельный .cpp.
// Чтобы не менять .h, используем глобальный экземпляр-синглтон,
// получаемый через ActorResetQueue::GetSingleton().

static ActorResetQueue s_instance;

ActorResetQueue& ActorResetQueue::GetSingleton() noexcept
{
	return s_instance;
}

// ---------------------------------------------------------------------------
// Schedule
// ---------------------------------------------------------------------------

void ActorResetQueue::Schedule(RE::Actor* actor, ResetEntry entry, bool priority)
{
	if (!actor) return;
	const uint32_t id = actor->GetFormID();

	// ВАЖНО: AddTask вызывается ПОСЛЕ освобождения m_mutex (см. ниже) — иначе AB-BA
	// дедлок с внутренним локом самого F4SE: F4SE::TaskInterface::AddTask под капотом
	// берёт свою критическую секцию, а её же держит F4SE, когда синхронно вызывает
	// RunTask::Run() -> ActorResetQueue::ExecuteReset(), которому в свою очередь нужен
	// этот самый m_mutex (в начале ExecuteReset). Если Schedule() вызывает AddTask,
	// ещё держа m_mutex, а в этот момент F4SE-поток уже внутри RunTask::Run ждёт
	// m_mutex — оба потока ждут друг друга навечно. Подтверждено снятыми стеками
	// (2026-10-23): один поток стоял в ExecuteReset на std::lock_guard(m_mutex),
	// другой — в этой функции на RtlEnterCriticalSection внутри AddTask.
	bool needsTask = false;
	{
		std::lock_guard lock(m_mutex);

		// Мержим флаги, если запись уже есть
		auto it = m_pending.find(id);
		if (it != m_pending.end()) {
			logger::info("[ActorResetQueue] Schedule MERGE: actor={:#x} priority={} "
				"before body={} head={} detach={} headFix={} "
				"+ new body={} head={} detach={} headFix={}",
				id, priority,
				it->second.needsBodyReset, it->second.needsHeadReset,
				it->second.needsDetach, it->second.needsHeadFix,
				entry.needsBodyReset, entry.needsHeadReset,
				entry.needsDetach, entry.needsHeadFix);
			it->second.merge(entry);
			logger::info("[ActorResetQueue] Schedule MERGE result: actor={:#x} "
				"body={} head={} detach={} headFix={}",
				id,
				it->second.needsBodyReset, it->second.needsHeadReset,
				it->second.needsDetach, it->second.needsHeadFix);
		} else {
			logger::info("[ActorResetQueue] Schedule NEW: actor={:#x} priority={} "
				"body={} head={} detach={} headFix={}",
				id, priority,
				entry.needsBodyReset, entry.needsHeadReset,
				entry.needsDetach, entry.needsHeadFix);
			m_pending[id] = entry;
		}

		if (priority) {
			// Поднимаем в начало deque (убираем старое вхождение, вставляем в front)
			auto pos = std::find(m_order.begin(), m_order.end(), id);
			if (pos != m_order.end()) {
				m_order.erase(pos);
			}
			m_order.push_front(id);
		} else {
			// Добавляем в конец только если ещё не запланирован
			if (m_tasked.find(id) == m_tasked.end()) {
				m_order.push_back(id);
			}
		}

		// Решаем, нужен ли новый F4SE-таск — САМ вызов AddTask ниже, вне лока.
		if (m_tasked.find(id) == m_tasked.end()) {
			m_tasked.insert(id);
			needsTask = true;
		}
	} // m_mutex освобождён здесь

	if (needsTask) {
		F4SE::GetTaskInterface()->AddTask(new RunTask(id));
		logger::info("[ActorResetQueue] AddTask for actor={:#x}", id);
	}
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

void ActorResetQueue::Clear() noexcept
{
	std::lock_guard lock(m_mutex);
	m_pending.clear();
	m_order.clear();
	m_tasked.clear();
	logger::info("[ActorResetQueue] Cleared (game loading started)");
}

// ---------------------------------------------------------------------------
// ExecuteReset — вызывается из RunTask::Run на игровом потоке
// ---------------------------------------------------------------------------

void ActorResetQueue::ExecuteReset(uint32_t formID)
{
	ResetEntry entry{};
	{
		std::lock_guard lock(m_mutex);
		auto it = m_pending.find(formID);
		if (it == m_pending.end()) {
			// Запись была очищена (Clear() во время загрузки) — ничего не делаем
			m_tasked.erase(formID);
			logger::info("[ActorResetQueue] ExecuteReset: actor={:#x} — no pending entry (cleared), skip", formID);
			return;
		}
		entry = it->second;
		m_pending.erase(it);
		m_tasked.erase(formID);
		auto pos = std::find(m_order.begin(), m_order.end(), formID);
		if (pos != m_order.end()) m_order.erase(pos);
	}

	// Получаем актёра. Если выгружен — пропускаем.
	RE::Actor* actor = nullptr;
	if (auto* form = RE::TESForm::GetFormByID(formID); form) {
		actor = RE::fallout_cast<RE::Actor*>(form);
	}

	if (!actor) {
		logger::warn("[ActorResetQueue] ExecuteReset: actor {:#x} not found (unloaded), skip", formID);
		return;
	}

	// --- Цвет кожи тела/ладоней: делаем до проверки 3D — кэш bodyTintColor нужен и при будущей сборке ---
	if (entry.needsSkinTintRefresh) {
		DoRefreshSkinTint(actor);
	}

	if (!actor->GetFullyLoaded3D()) {
		// Get3D=false: движок ещё строит геометрию для этого актёра.
		// Reset3D здесь не нужен — движок сам прочитает уже обновлённые данные TESNPC
		// и построит правильную геометрию. Это нормальный путь при загрузке ячейки.
		// (Если таск всё-таки попал сюда — значит Schedule был вызван без проверки
		// GetFullyLoaded3D в вызывающем коде. Это баг там, а не здесь.)
		logger::warn("[ActorResetQueue] ExecuteReset: actor={:#x} Get3D=false — skip Reset3D "
			"(engine builds geometry from TESNPC data directly)",
			formID);
		return;
	}

	logger::info("[ActorResetQueue] ExecuteReset START: actor={:#x} name='{}' "
		"body={} head={} detach={} headFix={} overlayUpdate={} flags3D=0x{:x}",
		formID,
		actor->GetDisplayFullName() ? actor->GetDisplayFullName() : "?",
		entry.needsBodyReset, entry.needsHeadReset,
		entry.needsDetach, entry.needsHeadFix, entry.needsOverlayUpdate,
		actor->currentProcess ? static_cast<uint32_t>(actor->currentProcess->GetAll3DUpdateFlags()) : 0u);

	using R3D = RE::RESET_3D_FLAGS;

	// --- DetachOverlays (только перед body reset) ---
	if (entry.needsDetach) {
		logger::info("[ActorResetQueue] DoDetachOverlays: actor={:#x}", formID);
		DoDetachOverlays(actor);
	}

	// --- Body Reset3D: excludeFlags=kHead|kFace — голова не затрагивается ---
	// Это безопасно: async IO для headpart geometry не запускается.
	if (entry.needsBodyReset) {
		if (auto* npc = actor->GetNPC(); npc) {
			npc->AddChange(0x4000); // CHANGE_ACTOR_FULL_LOD_SWAPPED
		}
		logger::info("[ActorResetQueue] Body Reset3D: actor={:#x}", formID);
		actor->Reset3D(false, R3D::kNone, true, R3D::kHead | R3D::kFace);
		logger::info("[ActorResetQueue] Body Reset3D called: actor={:#x} flags3D=0x{:x}",
			formID,
			actor->currentProcess ? static_cast<uint32_t>(actor->currentProcess->GetAll3DUpdateFlags()) : 0u);
	}

	// --- UpdateOverlays: применить таблицу оверлеев LooksMenu к текущему 3D ---
	// AddOverlay/RemoveOverlay правят только данные; геометрию строит UpdateOverlays
	// (аналог UpdateMorphs для морфов). Выполняется на игровом потоке.
	if (entry.needsOverlayUpdate) {
		if (auto* overlayInterface = LooksMenuInterfaces<OverlayInterface>::GetInterface(); overlayInterface) {
			const bool ok = overlayInterface->UpdateOverlays(actor);
			logger::info("[ActorResetQueue] UpdateOverlays: actor={:#x} result={}", formID, ok);
		}
	}

	// Диагностика: отложенный Reset3D должен быть обработан движком — смотрим, остались ли флаги.
	if (entry.needsBodyReset) {
		std::thread([formID]() {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			F4SE::GetTaskInterface()->AddTask([formID]() {
				auto* form = RE::TESForm::GetFormByID(formID);
				auto* a = form ? RE::fallout_cast<RE::Actor*>(form) : nullptr;
				if (!a) return;
				logger::info("[ActorResetQueue] +1s: actor={:#x} flags3D=0x{:x} fullyLoaded3D={}",
					formID,
					a->currentProcess ? static_cast<uint32_t>(a->currentProcess->GetAll3DUpdateFlags()) : 0u,
					a->GetFullyLoaded3D() != nullptr);
			});
		}).detach();
	}

	// --- Head Reset3D: flags=kHead|kFace, excludeFlags=kNone ---
	// Запускает async IO для headpart geometry — выполняется отдельно от body,
	// чтобы body reset не мог случайно включить голову через merge-логику.
	if (entry.needsHeadReset) {
		if (auto* npc = actor->GetNPC(); npc) {
			if (entry.needsHeadFix) {
				if (auto* face = npc->GetRootFaceNPC(); face) {
					face->AddChange(0x800); // CHANGE_ACTOR_FACE_MORPHS
					npc->AddChange(0x800);
				}
			}
		}
		logger::info("[ActorResetQueue] Head Reset3D: actor={:#x}", formID);
		actor->Reset3D(false, R3D::kHead | R3D::kFace, true, R3D::kNone);
	}

	logger::info("[ActorResetQueue] ExecuteReset DONE: actor={:#x}", formID);
}

// ---------------------------------------------------------------------------
// DoRefreshSkinTint
//
// HEAD-пресет перезаписывает tintingData у faceNPC (в т.ч. тинт kSkinTone), и голова,
// которую движок собирает с force=true, получает новый цвет кожи. Тело и ладони читают
// кэш TESNPC::bodyTintColor, который никто не обновлял, — отсюда разный цвет кожи.
// Пересчитываем цвет из тинтов faceNPC, переносим в кэш базового NPC актёра и красим
// skin-tint шейдеры уже загруженного 3D (если Reset3D позже пересоберёт тело, оно
// прочитает уже обновлённый кэш).
// ---------------------------------------------------------------------------

void ActorResetQueue::DoRefreshSkinTint(RE::Actor* actor)
{
	if (!actor) return;

	auto* npc = actor->GetNPC();
	if (!npc) return;
	auto* face = npc->GetRootFaceNPC();
	auto* source = face ? face : npc;

	const uint32_t npcBefore = packBodyTint(npc);
	const uint32_t faceBefore = packBodyTint(source);

	RE::NiColorA color{};
	if (!getBodyTintColor(source, color, true)) return;

	// Базовый NPC актёра (для NPC с шаблоном лица он отличается от faceNPC) должен выдавать
	// тот же цвет: переносим пересчитанный кэш.
	if (npc != source) {
		npc->bodyTintColorR = source->bodyTintColorR;
		npc->bodyTintColorG = source->bodyTintColorG;
		npc->bodyTintColorB = source->bodyTintColorB;
		npc->bodyTintColorA = source->bodyTintColorA;
	}

	logger::info("[SkinTint] actor={:#x} npc={:#x} face={:#x} cache npc 0x{:08x}->0x{:08x}, face 0x{:08x}->0x{:08x}, color=({:.3f},{:.3f},{:.3f},{:.3f})",
		actor->GetFormID(), npc->GetFormID(), source->GetFormID(),
		npcBefore, packBodyTint(npc), faceBefore, packBodyTint(source),
		color.r, color.g, color.b, color.a);

	if (actor->GetFullyLoaded3D()) {
		applySkinTintToTree(actor->Get3D(false), color);
		if (actor == RE::PlayerCharacter::GetSingleton()) {
			applySkinTintToTree(actor->Get3D(true), color);
		}
		logger::info("[SkinTint] actor={:#x} applied to loaded 3D", actor->GetFormID());
	}
}

// ---------------------------------------------------------------------------
// DoDetachOverlays — портирован из ResetBodyMorphs::Run
// ---------------------------------------------------------------------------

void ActorResetQueue::DoDetachOverlays(RE::Actor* actor)
{
	if (!actor) return;

	auto* bodyMorphInterface = LooksMenuInterfaces<BodyMorphInterface>::GetInterface();
	auto* overlayInterface   = LooksMenuInterfaces<OverlayInterface>::GetInterface();
	if (!bodyMorphInterface || !overlayInterface) return;

	const bool isPlayer = (actor == RE::PlayerCharacter::GetSingleton());
	uint32_t seenSlots = 0, morphableSlots = 0, destroyedSlots = 0;
	RE::BipedAnim* equipData[2];
	equipData[0] = actor->GetBiped().get();
	equipData[1] = isPlayer ? RE::PlayerCharacter::GetSingleton()->GetBiped(true).get() : nullptr;

	for (uint32_t s = 0; s < (isPlayer ? 2u : 1u); ++s) {
		if (!equipData[s]) continue;
		for (uint32_t i = 0; i < 32; ++i) {
			RE::NiPointer<RE::NiAVObject> slotNode =
				equipData[s]->object[i].partClone ? equipData[s]->object[i].partClone : nullptr;
			if (!slotNode) continue;
			++seenSlots;
			if (!IsNodeMorphable(slotNode.get())) continue;
			++morphableSlots;

			RE::NiPointer<RE::NiNode> parent(slotNode->parent);
			if (!parent) continue;

			RE::NiNode* rootNode = GetRootNode(actor, slotNode.get());
			if (!rootNode) continue;

			RE::NiNode* overlayRoot = overlayInterface->GetOverlayRoot(actor, rootNode);
			if (overlayRoot) {
				overlayInterface->DestroyOverlaySlot(actor, overlayRoot, i);
				++destroyedSlots;
			}
		}
	}

	logger::info("[ActorResetQueue] DetachOverlays: actor={:#x} slots(seen={} morphable={} destroyed={})",
		actor->GetFormID(), seenSlots, morphableSlots, destroyedSlots);
}
