/**
 * A linear allocator is the simplest form of an allocator. It never reuses any
 * memory, and therefore does not need a deallocation method. It simply hands
 * out consecutive buffers of memory. When the current buffer is full, it
 * reallocates a new larger buffer and continues.
 */

#pragma once

#include "string_ref.h"
#include "utildefines.h"
#include "vector.h"

namespace bas {

template<typename Allocator = RawAllocator>
class LinearAllocator : NonCopyable, NonMovable {
  private:
    Allocator m_allocator;
    Vector<void *> m_owned_buffers;
    Vector<ArrayRef<char>> m_unused_borrowed_buffers;

    uintptr_t m_current_begin;
    uintptr_t m_current_end;
    size_t m_next_min_alloc_size;

#ifdef DEBUG
    size_t m_debug_allocated_amount = 0;
#endif

  public:
    LinearAllocator()
    {
        m_current_begin = 0;
        m_current_end = 0;
        m_next_min_alloc_size = 64;
    }

    ~LinearAllocator()
    {
        for (void *ptr : m_owned_buffers) {
            m_allocator.free(ptr);
        }
    }

    void provide_buffer(void *buffer, size_t size)
    {
        m_unused_borrowed_buffers.append(ArrayRef<char>((char *)buffer, size));
    }

    template<size_t Size, size_t Alignment>
    void provide_buffer(AlignedBuffer<Size, Alignment> &aligned_buffer)
    {
        this->provide_buffer(aligned_buffer.ptr(), Size);
    }

    template<typename T> T *allocate()
    {
        return (T *)this->allocate(sizeof(T), alignof(T));
    }

    template<typename T> MutableArrayRef<T> allocate_array(size_t length)
    {
        return MutableArrayRef<T>((T *)this->allocate(sizeof(T) * length),
                                  length);
    }

    void *allocate(size_t size, size_t alignment = 4)
    {
        assert(alignment >= 1);
        assert(is_power_of_2(alignment));

#ifdef DEBUG
        m_debug_allocated_amount += size;
#endif

        uintptr_t alignment_mask = alignment - 1;
        uintptr_t potential_allocation_begin = (m_current_begin +
                                                alignment_mask) &
                                               ~alignment_mask;
        uintptr_t potential_allocation_end = potential_allocation_begin + size;

        if (potential_allocation_end <= m_current_end) {
            m_current_begin = potential_allocation_end;
            return (void *)potential_allocation_begin;
        }
        else {
            this->allocate_new_buffer(size + alignment);
            return this->allocate(size, alignment);
        }
    };

    StringRefNull copy_string(StringRef str)
    {
        size_t alloc_size = str.size() + 1;
        char *buffer = (char *)this->allocate(alloc_size, 1);
        str.copy(buffer, alloc_size);
        return StringRefNull((const char *)buffer);
    }

    template<typename T, typename... Args> T *construct(Args &&... args)
    {
        void *buffer = this->allocate(sizeof(T), alignof(T));
        T *value = new (buffer) T(std::forward<Args>(args)...);
        return value;
    }

    template<typename T, typename... Args>
    ArrayRef<T *> construct_elements_and_pointer_array(size_t n,
                                                       Args &&... args)
    {
        void *pointer_buffer = this->allocate(n * sizeof(T *), alignof(T *));
        void *element_buffer = this->allocate(n * sizeof(T), alignof(T));

        MutableArrayRef<T *> pointers((T **)pointer_buffer, n);
        T *elements = (T *)element_buffer;

        for (size_t i : IndexRange(n)) {
            pointers[i] = elements + i;
        }
        for (size_t i : IndexRange(n)) {
            new (elements + i) T(std::forward<Args>(args)...);
        }

        return pointers;
    }

    template<typename T>
    MutableArrayRef<T> construct_array_copy(ArrayRef<T> source)
    {
        T *buffer = (T *)this->allocate(source.byte_size(), alignof(T));
        source.copy_to(buffer);
        return MutableArrayRef<T>(buffer, source.size());
    }

  private:
    void allocate_new_buffer(size_t min_allocation_size)
    {
        for (size_t i : m_unused_borrowed_buffers.index_range()) {
            ArrayRef<char> buffer = m_unused_borrowed_buffers[i];
            if (buffer.size() >= min_allocation_size) {
                m_unused_borrowed_buffers.remove_and_reorder(i);
                m_current_begin = (uintptr_t)buffer.begin();
                m_current_end = (uintptr_t)buffer.end();
                return;
            }
        }

        size_t size_in_bytes = ceil_power_of_2(
            std::max(min_allocation_size, m_next_min_alloc_size));
        m_next_min_alloc_size = size_in_bytes * 2;

        void *buffer = m_allocator.allocate(size_in_bytes, 8);
        m_owned_buffers.append(buffer);
        m_current_begin = (uintptr_t)buffer;
        m_current_end = m_current_begin + size_in_bytes;
    }
};

}  // namespace bas
