#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

// Выполняет a_onMain в игровом потоке (F4SE task) каждые a_interval, пока не остановлен.
// Stop() не ждёт рабочий поток: join изнутри F4SE task - дедлок (поток может сидеть в AddTask на
// очереди, которую как раз разбирает главный поток). Поток отсоединён и просто видит сброшенный
// флаг; уже поставленные им задачи тоже его проверяют.
class PollTicker
{
public:
	PollTicker() = default;
	PollTicker(const PollTicker&) = delete;
	PollTicker& operator=(const PollTicker&) = delete;
	~PollTicker() { Stop(); }

	void Start(std::chrono::milliseconds a_interval, std::function<void()> a_onMain)
	{
		Stop();
		auto alive = std::make_shared<std::atomic_bool>(true);
		m_alive = alive;
		std::thread([alive, a_interval, fn = std::move(a_onMain)] {
			while (alive->load()) {
				std::this_thread::sleep_for(a_interval);
				if (!alive->load()) {
					break;
				}
				if (auto* tasks = F4SE::GetTaskInterface()) {
					tasks->AddTask([alive, fn] {
						if (alive->load()) {
							fn();
						}
					});
				}
			}
		}).detach();
	}

	void Stop()
	{
		if (m_alive) {
			m_alive->store(false);
			m_alive.reset();
		}
	}

	bool Running() const { return m_alive != nullptr; }

private:
	std::shared_ptr<std::atomic_bool> m_alive;
};
