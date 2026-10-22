#pragma once
#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <functional>

/**
 * @brief Единая очередь сброса 3D для актёров.
 *
 * Решает проблему визуального артефакта "голова без тела":
 * разные пресеты (HEAD, BODYMORPHS, OVERLAY) вызывали Reset3D асинхронно
 * в разных очередях, что давало промежуточные состояния видимые игроку.
 *
 * Принцип работы:
 * - Каждый вызов Schedule() накапливает (мержит) флаги для актёра.
 * - Один F4SE-таск на актёра — добавляется при первом Schedule(), дальше только мерж.
 * - При выполнении таска: DetachOverlays (если нужно) → Reset3D с накопленными флагами.
 * - priority=true (DirectApply): актёр перемещается в начало deque без создания нового таска.
 * - Clear() — при загрузке игры, чтобы не выполнять устаревшие сбросы.
 */
class ActorResetQueue
{
public:
	/**
	 * @brief Флаги описывающие что нужно сделать при сбросе.
	 * Накапливаются через OR при нескольких вызовах Schedule() для одного актёра.
	 */
	struct ResetEntry {
		bool needsBodyReset = false; // Reset3D для тела (excludeFlags=kHead|kFace)
		bool needsHeadReset = false; // Reset3D для головы (flags=kHead|kFace, excludeFlags=kNone)
		bool needsDetach    = false; // DetachOverlays перед body Reset3D
		bool needsHeadFix   = false; // AddChange(0x800) для faceNPC перед head Reset3D
		bool needsOverlayUpdate = false; // OverlayInterface::UpdateOverlays(actor) после body Reset3D (применить таблицу оверлеев к текущему 3D)
		bool needsSkinTintRefresh = false; // пересчитать TESNPC::bodyTintColor из tintingData и перекрасить кожу текущего 3D (тело/ладони)

		ResetEntry& merge(const ResetEntry& other) noexcept {
			needsBodyReset = needsBodyReset || other.needsBodyReset;
			needsHeadReset = needsHeadReset || other.needsHeadReset;
			needsDetach    = needsDetach    || other.needsDetach;
			needsHeadFix   = needsHeadFix   || other.needsHeadFix;
			needsOverlayUpdate = needsOverlayUpdate || other.needsOverlayUpdate;
			needsSkinTintRefresh = needsSkinTintRefresh || other.needsSkinTintRefresh;
			return *this;
		}
	};

	ActorResetQueue() = default;
	ActorResetQueue(const ActorResetQueue&) = delete;
	ActorResetQueue& operator=(const ActorResetQueue&) = delete;

	/** Глобальный экземпляр (живёт в ActorResetQueue.cpp). */
	static ActorResetQueue& GetSingleton() noexcept;

	/**
	 * @brief Запланировать сброс 3D для актёра.
	 *
	 * Если для этого актёра уже есть pending-запись — мержит флаги.
	 * Если нет — добавляет F4SE-таск.
	 * @param actor    Актёр. Должен быть валиден на момент вызова.
	 * @param entry    Флаги и параметры сброса.
	 * @param priority Если true — перемещает актёра в начало очереди (для DirectApply).
	 */
	void Schedule(RE::Actor* actor, ResetEntry entry, bool priority = false);

	/**
	 * @brief Очистить очередь. Вызывается при notifyGameStartLoading().
	 * Все pending-таски будут отменены при выполнении (запись не найдена).
	 */
	void Clear() noexcept;

private:
	// F4SE-таск который выполняет фактический сброс
	struct RunTask : F4SE::ITaskDelegate {
		uint32_t formID;
		explicit RunTask(uint32_t id) : formID(id) {}
		void Run() override;
	};

	// Выполнить сброс для актёра (вызывается из RunTask::Run на игровом потоке)
	void ExecuteReset(uint32_t formID);

	// Вспомогательный: DetachOverlays для актёра
	static void DoDetachOverlays(RE::Actor* actor);

	// Вспомогательный: синхронизация цвета кожи тела/ладоней с тинтами лица (см. .cpp)
	static void DoRefreshSkinTint(RE::Actor* actor);

	std::mutex m_mutex;

	// Порядок выполнения (formID). Позволяет priority-bump через move_to_front.
	std::deque<uint32_t> m_order;

	// Накопленные флаги. Запись удаляется при выполнении таска.
	std::unordered_map<uint32_t, ResetEntry> m_pending;

	// Для кого уже добавлен F4SE-таск (но он ещё не выполнился).
	// Нужно чтобы не добавлять дубли.
	std::unordered_set<uint32_t> m_tasked;
};
