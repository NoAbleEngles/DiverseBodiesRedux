#pragma once
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>

namespace patches
{
    struct ClothGuardEntry
    {
        std::uint32_t skipCount{ 0 };
        std::chrono::steady_clock::time_point lastWarn{};
        bool quarantined{ false };
        bool pendingDetach{ false };
        bool detached{ false };
    };

    class ClothGuard
    {
    public:
        static bool ShouldWarn(void* key, std::chrono::milliseconds period = std::chrono::milliseconds(3000));
        static void Quarantine(void* key);
        static bool IsQuarantined(void* key);

        // Ставит флаг «ожидает детач», true — если впервые
        static bool MarkPendingDetach(void* key);
        static void MarkDetached(void* key);

    private:
        static inline std::mutex _mtx;
        static inline std::unordered_map<void*, ClothGuardEntry> _map;
    };

    // Коалесцированная очередь на один SetWorld(nullptr) на объект
    class ClothDetachQueue
    {
    public:
        static void Enqueue(void* key);

    private:
        static void ScheduleDrain();
        static void DrainOnce();

        static inline std::mutex _mtx;
        static inline std::vector<void*> _queue;
        static inline bool _drainScheduled{ false };
    };
}