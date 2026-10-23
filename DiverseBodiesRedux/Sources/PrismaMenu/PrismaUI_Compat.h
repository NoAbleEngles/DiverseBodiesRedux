// PrismaMenu/PrismaUI_Compat.h
#pragma once

#include "PrismaMenu/PrismaUI_F4_API.h"

// Version-tolerant interface acquisition across PrismaUI_F4 builds.
//
// PrismaUI 2.2 renumbered the interface tokens from V9 onwards (V9 = 0x88, V10 = 0x89, ...)
// while 2.0.x hands out V9/V10 under their old sequential values (8/9). Each build rejects the
// other's value as "unsupported interface version", so a plugin built against either header
// alone silently gets nullptr on the other. V1-V8 kept their values and need none of this.
//
// The V9/V10 vtables are layout-compatible between the two for every method the 2.0.x header
// declared; 2.2 only appended FocusOverlay/SetInputRegions to the END of V10. So the pointer
// returned here is safe for everything up to IsAnyPanelVisible -- but never call the 2.2-only
// methods through it unless the a_modern out-param came back true.
namespace PRISMA_UI_COMPAT
{
    namespace detail
    {
        inline constexpr uint8_t kLegacyV9  = 8;
        inline constexpr uint8_t kLegacyV10 = 9;

        template <class T>
        [[nodiscard]] inline T* Request(uint8_t a_legacyToken, bool* a_modern) noexcept
        {
            if (auto* api = PRISMA_UI_API::RequestPluginAPI<T>()) {
                if (a_modern) *a_modern = true;
                return api;
            }
            if (a_modern) *a_modern = false;
            return static_cast<T*>(
                PRISMA_UI_API::RequestPluginAPI(static_cast<PRISMA_UI_API::InterfaceVersion>(a_legacyToken)));
        }
    }

    // a_modern (optional) is set to true when the 2.2+ token was accepted.
    [[nodiscard]] inline PRISMA_UI_API::IVPrismaUI9* RequestV9(bool* a_modern = nullptr) noexcept
    {
        return detail::Request<PRISMA_UI_API::IVPrismaUI9>(detail::kLegacyV9, a_modern);
    }

    [[nodiscard]] inline PRISMA_UI_API::IVPrismaUI10* RequestV10(bool* a_modern = nullptr) noexcept
    {
        return detail::Request<PRISMA_UI_API::IVPrismaUI10>(detail::kLegacyV10, a_modern);
    }
}
