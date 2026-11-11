#include "ClothWorldFix.h"
#include "Patches.h"
#include "PatchesMemUtils.h"
#include <F4SE/API.h>

namespace patches
{
    void ClothDetachQueue::ScheduleDrain()
    {
        // ВАЖНО: AddTask вызывается ПОСЛЕ освобождения _mtx — тот же класс AB-BA
        // дедлока, что нашли и исправили в ActorResetQueue::Schedule (2026-10-23):
        // F4SE::TaskInterface::AddTask берёт свою внутреннюю критическую секцию,
        // которую F4SE же держит, когда синхронно вызывает тело уже поставленной
        // задачи — если держать здесь _mtx во время AddTask, а другой поток в этот
        // момент ждёт _mtx изнутри такой задачи, оба зависнут навечно.
        {
            std::scoped_lock lk(ClothDetachQueue::_mtx);
            if (ClothDetachQueue::_drainScheduled) {
                return;
            }
            ClothDetachQueue::_drainScheduled = true;
        }

        auto ti = F4SE::GetTaskInterface();
        if (!ti) {
            std::scoped_lock lk(ClothDetachQueue::_mtx);
            ClothDetachQueue::_drainScheduled = false;
            return;
        }

        ti->AddTask([]() {
            // Должны выполняться на главном потоке; TLS проверим на всякий
            if (!HasGameTLSGuardSlot()) {
                std::scoped_lock lk2(ClothDetachQueue::_mtx);
                ClothDetachQueue::_drainScheduled = false;
                return;
            }

            ClothDetachQueue::DrainOnce();

            std::scoped_lock lk2(ClothDetachQueue::_mtx);
            ClothDetachQueue::_drainScheduled = false;
            });
    }

    void ClothDetachQueue::DrainOnce()
    {
        std::vector<void*> local;
        {
            std::scoped_lock lk(ClothDetachQueue::_mtx);
            if (ClothDetachQueue::_queue.empty()) {
                return;
            }
            local.swap(ClothDetachQueue::_queue);
        }

        for (void* p : local) {
            auto* cloth = static_cast<RE::BSClothExtraData*>(p);
            if (!cloth) {
                continue;
            }
            // Единожды мягко отсоединяем world через безопасную нативную реализацию
            Hooked_BSClothExtraData_SetWorld(cloth, nullptr);
            ClothGuard::MarkDetached(p);
        }
    }

    void ClothDetachQueue::Enqueue(void* key)
    {
        if (!ClothGuard::MarkPendingDetach(key)) {
            return; // уже в очереди или детачен
        }
        {
            std::scoped_lock lk(ClothDetachQueue::_mtx);
            ClothDetachQueue::_queue.emplace_back(key);
        }
        ClothDetachQueue::ScheduleDrain();
    }

    bool ClothGuard::ShouldWarn(void* key, std::chrono::milliseconds period)
    {
        std::scoped_lock lk(ClothGuard::_mtx);
        auto& e = ClothGuard::_map[key];
        auto now = std::chrono::steady_clock::now();
        if (now - e.lastWarn >= period) {
            e.lastWarn = now;
            return true;
        }
        return false;
    }

    void ClothGuard::Quarantine(void* key)
    {
    }

    bool ClothGuard::IsQuarantined(void* key)
    {
        return false;
    }

    bool ClothGuard::MarkPendingDetach(void* key)
    {
        return true;
    }

    void ClothGuard::MarkDetached(void* key)
    {
    }
}