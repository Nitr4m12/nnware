#pragma once

#include <cstddef>
#include <cstdint>

namespace nn::atk::detail::fnd {

template <typename T>
inline T Clamp(T x, T low, T high) {
    if (x > high)
        return high;

    if (x < low)
        return low;

    return x;
}

inline float FloatClamp(float value, float min, float max) {
    if (value >= min)
        return value < max ? value : max;

    return min;
}

template <typename ValueT>
inline ValueT RoundUp(ValueT x, int base) {
    int mask{base - 1};
    return (x + mask) & ~mask;
}

template <typename ValueT = void>
inline ValueT* RoundUp(ValueT* x, int base) {
    uintptr_t value{RoundUp<uintptr_t>(reinterpret_cast<uintptr_t>(x), base)};
    return reinterpret_cast<ValueT*>(value);
}

template <typename ValueT>
inline ValueT RoundDown(ValueT x, int base) {
    return x & ~(base - 1);
}

template <typename ValueT = void>
inline ValueT* RoundDown(ValueT* x, int base) {
    uintptr_t value{RoundDown<uintptr_t>(reinterpret_cast<uintptr_t>(x), base)};
    return reinterpret_cast<ValueT*>(value);
}

template <typename TDiff>
inline TDiff GetOffsetFromPtr(const void* start, const void* end) {
    return reinterpret_cast<TDiff>(end) - reinterpret_cast<TDiff>(start);
}

inline ptrdiff_t GetOffsetFromPtr(const void* start, const void* end) {
    return reinterpret_cast<ptrdiff_t>(end) - reinterpret_cast<ptrdiff_t>(start);
}

template <typename TOffset>
inline void* AddOffsetToPtr(void* ptr, TOffset offset) {
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + offset);
}

}  // namespace nn::atk::detail::fnd
