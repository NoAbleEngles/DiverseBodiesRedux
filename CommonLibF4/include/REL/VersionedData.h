#pragma once

/**
 * @file VersionedData.h
 * @brief Provides compile-time selection between OG and NG game data
 * 
 * This header provides utilities for working with data that differs between
 * Original Game (OG) and Next-Gen (NG) versions of Fallout 4.
 */

namespace Versioned
{
    /**
     * @brief Select between OG and NG values at compile time
     * @tparam T Type of the value
     * @param a_og Value for Original Game (1.10.163 and below)
     * @param a_ng Value for Next-Gen (1.11.191+)
     * @return The appropriate value based on F4_SUPPORT_NG macro
     * 
     * Example usage:
     * @code
     * constexpr auto value = Versioned::Select(100, 200);
     * // Returns 100 for OG, 200 for NG
     * @endcode
     */
    template <class T>
    [[nodiscard]] constexpr T Select(T a_og, T a_ng) noexcept
    {
#ifdef F4_SUPPORT_NG
        return a_ng;
#else
        return a_og;
#endif
    }

    /**
     * @brief Check if building for Next-Gen version
     * @return true if F4_SUPPORT_NG is defined, false otherwise
     */
    [[nodiscard]] constexpr bool IsNG() noexcept
    {
#ifdef F4_SUPPORT_NG
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Check if building for Original Game version
     * @return true if F4_SUPPORT_NG is not defined, false otherwise
     */
    [[nodiscard]] constexpr bool IsOG() noexcept
    {
        return !IsNG();
    }

    /**
     * @brief Get the version string
     * @return "NG" for Next-Gen, "OG" for Original Game
     */
    [[nodiscard]] constexpr const char* GetVersionString() noexcept
    {
#ifdef F4_SUPPORT_NG
        return "NG";
#else
        return "OG";
#endif
    }
}

/**
 * @brief Macro to select between OG and NG values
 * 
 * This is useful when you need to select between different types or expressions,
 * not just values.
 * 
 * Example usage:
 * @code
 * auto value = F4_SELECT_VERSION(ogExpression, ngExpression);
 * @endcode
 */
#ifdef F4_SUPPORT_NG
    #define F4_SELECT_VERSION(og, ng) ng
#else
    #define F4_SELECT_VERSION(og, ng) og
#endif

/**
 * @brief Macro for code that should only compile for OG version
 * 
 * Example usage:
 * @code
 * F4_OG_ONLY({
 *     // Code here only compiles for OG
 *     DoSomethingOGSpecific();
 * })
 * @endcode
 */
#ifdef F4_SUPPORT_NG
    #define F4_OG_ONLY(code) 
#else
    #define F4_OG_ONLY(code) code
#endif

/**
 * @brief Macro for code that should only compile for NG version
 * 
 * Example usage:
 * @code
 * F4_NG_ONLY({
 *     // Code here only compiles for NG
 *     DoSomethingNGSpecific();
 * })
 * @endcode
 */
#ifdef F4_SUPPORT_NG
    #define F4_NG_ONLY(code) code
#else
    #define F4_NG_ONLY(code)
#endif
