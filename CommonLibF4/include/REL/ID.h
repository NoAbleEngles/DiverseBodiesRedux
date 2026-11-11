#pragma once

namespace REL
{
    /**
     * @brief Represents a versioned game identifier that can differ between OG and NG versions
     * 
     * This class allows you to define different IDs for Original Game (OG) and Next-Gen (NG) versions.
     * The appropriate ID is selected at compile-time based on the F4_SUPPORT_NG macro.
     * 
     * Example usage:
     * @code
     * // For a function that has different addresses in OG vs NG
     * inline constexpr VersionedID SomeFunction(123456, 789012);
     * 
     * // Use it like a regular ID
     * REL::Relocation<func_t> func{ SomeFunction };
     * @endcode
     */
    class VersionedID
    {
    public:
        constexpr VersionedID() noexcept = default;

        /**
         * @brief Construct a versioned ID with different values for OG and NG
         * @param a_og ID for Original Game (1.10.163 and below)
         * @param a_ng ID for Next-Gen (1.11.191+)
         */
        constexpr VersionedID(std::uint64_t a_og, std::uint64_t a_ng) noexcept :
#ifdef F4_SUPPORT_NG
            _id(a_ng)
#else
            _id(a_og)
#endif
        {}

        /**
         * @brief Construct a versioned ID with the same value for both versions
         * @param a_id ID that is the same in both OG and NG
         */
        constexpr explicit VersionedID(std::uint64_t a_id) noexcept :
            _id(a_id)
        {}

        [[nodiscard]] constexpr std::uint64_t id() const noexcept { return _id; }
        [[nodiscard]] constexpr operator std::uint64_t() const noexcept { return _id; }

    private:
        std::uint64_t _id{ 0 };
    };

    /**
     * @brief Helper macro to define a VersionedID constant
     * 
     * Example usage:
     * @code
     * inline constexpr auto SomeFunction = RELID(123456, 789012);
     * @endcode
     */
    #define RELID(og, ng) ::REL::VersionedID(og, ng)

    /**
     * @brief Helper macro for IDs that are the same in both versions
     * 
     * Example usage:
     * @code
     * inline constexpr auto SomeFunction = RELID_SAME(123456);
     * @endcode
     */
    #define RELID_SAME(id) ::REL::VersionedID(id)
}
