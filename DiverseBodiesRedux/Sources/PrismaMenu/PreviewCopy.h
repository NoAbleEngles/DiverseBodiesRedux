#pragma once

#include "PrismaMenu/Ticker.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class Preset;

/**
 * @brief Копия персонажа для предпросмотра сохранённых пресетов. Настоящий актёр (модель
 * PrismaUI не покажет лицо, морфы и оверлеи): создаётся из дубликата базового NPC персонажа
 * (TESNPC::Copy), одевается в то, что надето на оригинале, получает его оверлеи, морфы и скин из
 * LooksMenu (то есть все пресеты DBR, уже применённые к оригиналу), встаёт перед камерой слева
 * (справа панель меню).
 *
 * Копия безопасна для окружения: AI не работает (restrained, атаки и магия отключены, не умирает),
 * фракция - CaptiveFaction (Fallout4.esm 0x03E0C8: во всех её отношениях с фракциями рейдеров,
 * ганнеров, супермутантов, синтов, игрока и т.д. стоит "friend", в обе стороны), агрессия 0, так
 * что враждебного оригинала не бойся и на копию никто не нападёт. DBR копии пресеты не раздаёт
 * (ActorsManager::setPreviewCopy). Только игровой поток.
 */
class PreviewCopy
{
public:
	using ReadyCallback = std::function<void(bool a_ok)>;

	// С чего делается копия. actor - оригинал, если игра о нём знает (может быть и не загружен в
	// сцену); npc - базовая форма, если актёра нет в памяти совсем. presets - сохранённые пресеты:
	// когда оригинал не загружен, его внешний вид живёт только в них, и они накладываются на копию.
	struct Source
	{
		RE::Actor*                           actor{ nullptr };
		RE::TESNPC*                          npc{ nullptr };
		std::vector<std::shared_ptr<Preset>> presets;
	};

	static PreviewCopy& get();

	// Запоминает режим камеры, уходит в вид от первого лица, создаёт копию. a_ready вызывается
	// (игровой поток), когда 3D копии загружен, она одета и "осела", либо с false.
	void Begin(const Source& a_source, ReadyCallback a_ready);
	// Удаляет копию, возвращает камеру.
	void End();

	bool          IsActive() const { return m_copy != 0; }
	bool          IsReady() const { return m_ready; }
	std::uint32_t CopyID() const { return m_copy; }
	std::uint32_t TargetID() const { return m_target; }

	RE::Actor* CopyActor() const { return Copy(); }

	void Rotate(float a_degrees);
	void Zoom(float a_steps);  // зубцы колеса, + = ближе
	void Lift(float a_units);  // + = выше (игровые единицы)

private:
	PreviewCopy() = default;

	void Poll();
	void Dress();
	void Finish();
	void Place();
	void FollowCamera();
	void AttachIfDetached();
	void DeleteRef();

	RE::Actor* Copy() const;
	RE::Actor* Target() const;

	RE::TESNPC*   m_npc{ nullptr };  // дубликат базы: одна форма на сессию, перед каждым показом Copy(цель)
	std::uint32_t m_copy{ 0 };
	std::uint32_t m_target{ 0 };
	bool          m_ready{ false };
	bool          m_dressed{ false };
	bool          m_attachedByUs{ false };
	bool          m_cameraChanged{ false };  // камеру трогали: End() её вернёт (даже если копия не появилась)
	bool          m_wasThirdPerson{ false };
	float         m_savedPitch{ 0.0f };
	float         m_yaw{ 0.0f };         // направление камеры при постановке
	RE::NiPoint3  m_eye{};               // положение камеры тогда
	float         m_turn{ 0.0f };        // поворот пользователем, радианы
	float         m_distance{ 200.0f };  // игровых единиц от камеры
	float         m_lift{ 0.0f };        // смещение копии вверх/вниз
	bool          m_targetLoaded{ false };
	void*         m_lastRoot{ nullptr };  // 3D копии при последней постановке: сменился - коллизии и узел заново
	std::vector<std::shared_ptr<Preset>> m_stored;
	ReadyCallback m_onReady;
	std::chrono::steady_clock::time_point m_spawnedAt{};
	std::chrono::steady_clock::time_point m_dressedAt{};
	PollTicker    m_poller;
};
