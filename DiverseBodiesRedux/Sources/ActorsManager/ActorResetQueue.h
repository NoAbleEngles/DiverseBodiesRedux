#pragma once
#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>

/**
 * @brief Единая очередь сброса 3D для актёров.
 *
 * Решает проблему визуального артефакта "голова без тела" и зависания
 * из-за массового Reset3D при загрузке ячейки:
 * фоновый поток ActorResetQueue пошагово отдаёт по одному актёру на сброс
 * в F4SE с паузой в 200 мс между ними.
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
		bool needsOverlayUpdate = false; // OverlayInterface::UpdateOverlays(actor) после body Reset3D
		bool needsSkinTintRefresh = false; // пересчитать TESNPC::bodyTintColor из tintingData

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

	ActorResetQueue();
	~ActorResetQueue();
	ActorResetQueue(const ActorResetQueue&) = delete;
	ActorResetQueue& operator=(const ActorResetQueue&) = delete;

	/** Глобальный экземпляр (живёт в ActorResetQueue.cpp). */
	static ActorResetQueue& GetSingleton() noexcept;

	/**
	 * @brief Запланировать сброс 3D для актёра.
	 *
	 * Если для этого актёра уже есть pending-запись — мержит флаги.
	 * @param actor    Актёр. Должен быть валиден на момент вызова.
	 * @param entry    Флаги и параметры сброса.
	 * @param priority Если true — перемещает актёра в начало очереди (для DirectApply/Player).
	 */
	void Schedule(RE::Actor* actor, ResetEntry entry, bool priority = false);

	/**
	 * @brief Очистить очередь. Вызывается при notifyGameStartLoading().
	 */
	void Clear() noexcept;

	// Вспомогательный: синхронизация кэша TESNPC::bodyTintColor (можно звать сразу после applyTints)
	static bool SyncNPCSkinTintCache(RE::Actor* actor, RE::NiColorA* outColor = nullptr);

	// Вспомогательный: синхронизация цвета кожи тела/ладоней с тинтами лица + покраска шейдеров 3D (см. .cpp)
	static void DoRefreshSkinTint(RE::Actor* actor);

private:
	void StartWorker();
	void WorkerLoop();

	// Выполнить сброс для актёра (вызывается на игровом потоке через F4SE AddTask).
	// Возвращает true, если реально был вызван Reset3D (требует 200мс паузы).
	bool ExecuteReset(uint32_t formID);

	// Вспомогательный: DetachOverlays для актёра
	static void DoDetachOverlays(RE::Actor* actor);

	std::mutex m_mutex;
	std::condition_variable m_cv;

	// Порядок выполнения (formID). Позволяет priority-bump через move_to_front.
	std::deque<uint32_t> m_order;

	// Накопленные флаги. Запись удаляется при выполнении таска.
	std::unordered_map<uint32_t, ResetEntry> m_pending;

	std::atomic<bool> m_stop{ false };
	std::thread m_workerThread;
};
