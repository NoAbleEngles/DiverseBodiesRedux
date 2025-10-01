#pragma once
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <type_traits>

template <typename Container>
class ProcessingGuard {
public:
    using Key = typename Container::key_type;

    ProcessingGuard(const Key& key, Container& container, std::shared_mutex& mutex)
        : container_(container), mutex_(mutex), key_(key) {
    }

    ProcessingGuard(const ProcessingGuard&) = delete;
    ProcessingGuard& operator=(const ProcessingGuard&) = delete;

    ~ProcessingGuard() {
        if (acquired_.load(std::memory_order_relaxed)) {
            release();
        }
    }

    // Быстрая неблокирующая попытка
    bool try_acquire() {
        if (acquired_.load(std::memory_order_relaxed)) return true;

        std::unique_lock lock(mutex_);
        auto [it, inserted] = container_.insert(key_);
        if (inserted) {
            acquired_.store(true, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    // Блокирующее ожидание с таймаутом
    template <class Rep, class Period>
    bool acquire_sync_for(std::chrono::duration<Rep, Period> timeout) {
        if (try_acquire()) return true;

        auto deadline = std::chrono::steady_clock::now() + timeout;
        std::unique_lock lock(mutex_);

        while (!container_.insert(key_).second) {
            if (cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
                // Последняя попытка перед таймаутом
                return container_.insert(key_).second;
            }
        }

        acquired_.store(true, std::memory_order_relaxed);
        return true;
    }

    bool acquire_sync() {
        return acquire_sync_for(std::chrono::hours(24 * 365));
    }

    bool is_acquired() const noexcept {
        return acquired_.load(std::memory_order_relaxed);
    }

    void release() {
        if (!acquired_.exchange(false, std::memory_order_relaxed)) return;

        {
            std::unique_lock lock(mutex_);
            container_.erase(key_);
        }
        cv_.notify_one(); // Будим только одного ожидающего
    }

private:
    Container& container_;
    std::shared_mutex& mutex_;
    Key key_;
    std::condition_variable_any cv_; // Локальная CV - проще и быстрее
    std::atomic<bool> acquired_{ false };
};