#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace bas {

#if defined(__GNUC__)
#    define BAS_NOINLINE __attribute__((noinline))
#else
#    define BAS_NOINLINE
#endif

#ifdef __GNUC__
#    define BAS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#    define BAS_UNLIKELY(x) (x)
#endif

#define BAS_UNUSED_VAR(x) ((void)x)

using std::size_t;
using ssize_t = std::make_signed_t<size_t>;

using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::int8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

/* True is returned for powers of two and zero. */
template<typename IntT> inline bool is_power_of_2(IntT x)
{
    return (x & (x - 1)) == 0;
}

template<typename IntT> inline IntT ceil_power_of_2(IntT x)
{
    x -= 1;
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);

    if constexpr (sizeof(IntT) >= 2) {
        x |= (x >> 8);
    }
    if constexpr (sizeof(IntT) >= 4) {
        x |= (x >> 16);
    }
    if constexpr (sizeof(IntT) >= 8) {
        x |= (x >> 32);
    }

    return x + 1;
}

template<typename IntT> inline IntT floor_power_of_2(IntT x)
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);

    if constexpr (sizeof(IntT) >= 2) {
        x |= (x >> 8);
    }
    if constexpr (sizeof(IntT) >= 4) {
        x |= (x >> 16);
    }
    if constexpr (sizeof(IntT) >= 8) {
        x |= (x >> 32);
    }

    return x - (x >> 1);
}

template<typename T> inline T log2_floor_u(T x)
{
    return x <= 1 ? 0 : 1 + log2_floor_u(x >> 1);
}

template<typename IntT> inline IntT log2_ceil_u(IntT x)
{
    if (is_power_of_2(x)) {
        return log2_floor_u(x);
    }
    else {
        return log2_floor_u(x) + 1;
    }
}

template<typename T> inline uintptr_t ptr_to_int(T *ptr)
{
    return (uintptr_t)ptr;
}

template<typename T> inline T *int_to_ptr(uintptr_t ptr)
{
    return (T *)ptr;
}

template<typename T> inline bool is_aligned(T *ptr, size_t alignment)
{
    assert(is_power_of_2(alignment));
    return (ptr_to_int(ptr) & (alignment - 1)) == 0;
}

class NonCopyable {
  public:
    /* Disable copy construction and assignment. */
    NonCopyable(const NonCopyable &other) = delete;
    NonCopyable &operator=(const NonCopyable &other) = delete;

    /* Explicitly enable default construction, move construction and move
     * assignment. */
    NonCopyable() = default;
    NonCopyable(NonCopyable &&other) = default;
    NonCopyable &operator=(NonCopyable &&other) = default;
};

class NonMovable {
  public:
    /* Disable move construction and assignment. */
    NonMovable(NonMovable &&other) = delete;
    NonMovable &operator=(NonMovable &&other) = delete;

    /* Explicitly enable default construction, copy construction and copy
     * assignment. */
    NonMovable() = default;
    NonMovable(const NonMovable &other) = default;
    NonMovable &operator=(const NonMovable &other) = default;
};

}  // namespace bas
