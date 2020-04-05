#pragma once

#include <algorithm>
#include <memory>

#include "utildefines.h"

namespace bas {

using std::copy;
using std::copy_n;
using std::uninitialized_copy;
using std::uninitialized_copy_n;
using std::uninitialized_fill;
using std::uninitialized_fill_n;

template<typename T> void construct_default(T *ptr)
{
    new (ptr) T();
}

template<typename T> void construct_default_n(T *ptr, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        new (ptr + i) T();
    }
}

template<typename T> void destruct(T *ptr)
{
    ptr->~T();
}

template<typename T> void destruct_n(T *ptr, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        ptr[i].~T();
    }
}

template<typename T> void uninitialized_move_n(T *src, size_t n, T *dst)
{
    std::uninitialized_copy_n(std::make_move_iterator(src), n, dst);
}

template<typename T> void move_n(T *src, size_t n, T *dst)
{
    std::copy_n(std::make_move_iterator(src), n, dst);
}

template<typename T> void uninitialized_relocate(T *src, T *dst)
{
    new (dst) T(std::move(*src));
    destruct(src);
}

template<typename T> void uninitialized_relocate_n(T *src, size_t n, T *dst)
{
    uninitialized_move_n(src, n, dst);
    destruct_n(src, n);
}

template<typename T> void relocate(T *src, T *dst)
{
    *dst = std::move(*src);
    destruct(src);
}

template<typename T> void relocate_n(T *src, size_t n, T *dst)
{
    move_n(src, n, dst);
    destruct_n(src, n);
}

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename T> struct DestructValueAtAddress {
    void operator()(T *ptr)
    {
        ptr->~T();
    }
};

template<typename T>
using destruct_ptr = std::unique_ptr<T, DestructValueAtAddress<T>>;

template<size_t Size, size_t Alignment>
class alignas(Alignment) AlignedBuffer {
  private:
    /* Don't create an empty array. This causes problems with some compilers.
     */
    static constexpr size_t ActualSize = (Size > 0) ? Size : 1;
    char m_buffer[ActualSize];

  public:
    void *ptr()
    {
        return (void *)m_buffer;
    }

    const void *ptr() const
    {
        return (const void *)m_buffer;
    }
};

}  // namespace bas
