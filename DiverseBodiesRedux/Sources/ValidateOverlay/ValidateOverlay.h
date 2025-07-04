#pragma once
#include <set>
#include <string>
#include <mutex>
#include <future>
#include <atomic>
#include <chrono>

class ValidateOverlay
{
public:
    /**
     * @brief Синглтон. Проверяет, является ли указанный идентификатор оверлея действительным оверлеем.
     *
     * Если с момента последней проверки прошло более 15 секунд, инициирует асинхронное обновление списка действительных оверлеев.
     * Если обновление уже выполняется, ожидает его завершения в отдельном потоке (не блокируя основной вызывающий поток).
     * После завершения обновления возвращает актуальное состояние overlayID.
     *
     * @param overlayID Идентификатор оверлея для проверки.
     * @return std::future<bool> Будущее, которое возвращает true, если overlayID найден в текущем списке допустимых оверлеев, иначе false.
     *
     * @note Метод является потокобезопасным. Чтобы получить результат, необходимо дождаться завершения работы std::future<bool>.
     * @note Не блокирует главный поток, если вызывается из него.
     */

    std::future<bool> operator()(const std::string& overlayID);

    static ValidateOverlay& validateOverlay();

private:
    ValidateOverlay() = default;
    ValidateOverlay(const ValidateOverlay&) = delete;
    ValidateOverlay& operator=(const ValidateOverlay&) = delete;
    ValidateOverlay(ValidateOverlay&&) = delete;
    ValidateOverlay& operator=(ValidateOverlay&&) = delete;

    /**
     * @brief Обновляет список действительных оверлеев асинхронно.
     *
     * Если обновление уже выполняется, возвращается существующий future.
     * В противном случае запускается новое асинхронное обновление и возвращается его future.
     *
     * @return std::shared_ptr<std::future<void>> Общий указатель на future, представляющий операцию обновления.
     */

    std::set<std::string> m_validOverlays;
    int64_t m_lastCheckTime = 0;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_updateInProgress{ false };
    std::shared_ptr<std::future<void>> m_updateFuture;

    void update();
    std::shared_ptr<std::future<void>> update_async();
};