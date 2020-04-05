#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "utildefines.h"

namespace bas {

template<typename T> struct DefaultHash {
};

#define TRIVIAL_DEFAULT_INT_HASH(TYPE) \
    template<> struct DefaultHash<TYPE> { \
        uint32_t operator()(TYPE value) const \
        { \
            return (uint32_t)value; \
        } \
    }

/**
 * Cannot make any assumptions about the distribution of keys, so use a trivial
 * hash function by default. The hash table implementations are designed to
 * take all bits of the hash into account to avoid really bad behavior when the
 * lower bits are all zero. Special hash functions can be implemented when more
 * knowledge about a specific key distribution is available.
 */
TRIVIAL_DEFAULT_INT_HASH(int8_t);
TRIVIAL_DEFAULT_INT_HASH(uint8_t);
TRIVIAL_DEFAULT_INT_HASH(int16_t);
TRIVIAL_DEFAULT_INT_HASH(uint16_t);
TRIVIAL_DEFAULT_INT_HASH(int32_t);
TRIVIAL_DEFAULT_INT_HASH(uint32_t);
TRIVIAL_DEFAULT_INT_HASH(int64_t);
TRIVIAL_DEFAULT_INT_HASH(uint64_t);

template<> struct DefaultHash<float> {
    uint32_t operator()(float value) const
    {
        return *(uint32_t *)&value;
    }
};

template<> struct DefaultHash<std::string> {
    uint32_t operator()(const std::string &value) const
    {
        uint32_t hash = 5381;
        for (char c : value) {
            hash = hash * 33 + c;
        }
        return hash;
    }
};

/**
 * While we cannot guarantee that the lower 3 bits or a pointer are zero, it is
 * safe to assume this in the general case. MEM_malloc only returns 8 byte
 * aligned addresses on 64-bit systems.
 */
template<typename T> struct DefaultHash<T *> {
    uint32_t operator()(const T *value) const
    {
        uintptr_t ptr = ptr_to_int(value);
        uint32_t hash = (uint32_t)(ptr >> 3);
        return hash;
    }
};

template<typename T> struct DefaultHash<std::unique_ptr<T>> {
    uint32_t operator()(const std::unique_ptr<T> &value) const
    {
        return DefaultHash<T *>{}(value.get());
    }
};

template<typename T1, typename T2> struct DefaultHash<std::pair<T1, T2>> {
    uint32_t operator()(const std::pair<T1, T2> &value) const
    {
        uint32_t hash1 = DefaultHash<T1>{}(value.first);
        uint32_t hash2 = DefaultHash<T2>{}(value.second);
        return hash1 ^ (hash2 * 33);
    }
};

}  // namespace bas
