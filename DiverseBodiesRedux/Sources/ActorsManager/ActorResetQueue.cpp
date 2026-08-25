#include "ActorResetQueue.h"
#include "LooksMenu/LooksMenuInterfaces.h"
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>

namespace logger = F4SE::log;

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

	// Добавляем F4SE-таск только если его ещё нет
	if (m_tasked.find(id) == m_tasked.end()) {
		m_tasked.insert(id);
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
		"body={} head={} detach={} headFix={}",
		formID,
		actor->GetDisplayFullName() ? actor->GetDisplayFullName() : "?",
		entry.needsBodyReset, entry.needsHeadReset,
		entry.needsDetach, entry.needsHeadFix);

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
// DoDetachOverlays — портирован из ResetBodyMorphs::Run
// ---------------------------------------------------------------------------

void ActorResetQueue::DoDetachOverlays(RE::Actor* actor)
{
	if (!actor) return;

	auto* bodyMorphInterface = LooksMenuInterfaces<BodyMorphInterface>::GetInterface();
	auto* overlayInterface   = LooksMenuInterfaces<OverlayInterface>::GetInterface();
	if (!bodyMorphInterface || !overlayInterface) return;

	const bool isPlayer = (actor == RE::PlayerCharacter::GetSingleton());
	RE::BipedAnim* equipData[2];
	equipData[0] = actor->GetBiped().get();
	equipData[1] = isPlayer ? RE::PlayerCharacter::GetSingleton()->GetBiped(true).get() : nullptr;

	for (uint32_t s = 0; s < (isPlayer ? 2u : 1u); ++s) {
		if (!equipData[s]) continue;
		for (uint32_t i = 0; i < 32; ++i) {
			RE::NiPointer<RE::NiAVObject> slotNode =
				equipData[s]->object[i].partClone ? equipData[s]->object[i].partClone : nullptr;
			if (!slotNode) continue;
			if (!IsNodeMorphable(slotNode.get())) continue;

			RE::NiPointer<RE::NiNode> parent(slotNode->parent);
			if (!parent) continue;

			RE::NiNode* rootNode = GetRootNode(actor, slotNode.get());
			if (!rootNode) continue;

			RE::NiNode* overlayRoot = overlayInterface->GetOverlayRoot(actor, rootNode);
			if (overlayRoot) {
				overlayInterface->DestroyOverlaySlot(actor, overlayRoot, i);
			}
		}
	}
}
