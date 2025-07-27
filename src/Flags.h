#pragma once

namespace trost {

#include <type_traits>

template<typename Enum>
struct EnableBitMaskOperators {
    static constexpr bool enable = false;
};

// Overload bitwise operators only if EnableBitMaskOperators<Enum>::enable is true
template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator|(Enum lhs, Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(
        static_cast<underlying>(lhs) | static_cast<underlying>(rhs)
        );
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator&(Enum lhs, Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(
        static_cast<underlying>(lhs) & static_cast<underlying>(rhs)
        );
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator^(Enum lhs, Enum rhs) {
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(
        static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs)
        );
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator~(Enum e) {
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(~static_cast<underlying>(e));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum&>::type
operator|=(Enum& lhs, Enum rhs) {
    lhs = lhs | rhs;
    return lhs;
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum&>::type
operator&=(Enum& lhs, Enum rhs) {
    lhs = lhs & rhs;
    return lhs;
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum&>::type
operator^=(Enum& lhs, Enum rhs) {
    lhs = lhs ^ rhs;
    return lhs;
}

} // namespace trost
