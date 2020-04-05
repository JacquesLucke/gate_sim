#pragma once

#include "allocator.h"
#include "array_ref.h"
#include "index_range.h"
#include "memory_utils.h"

#if defined(_MSC_VER)
#    pragma warning(disable : 4324)
#endif

namespace bas {

template<typename T, size_t N = 4, typename Allocator = RawAllocator>
class Array {
  private:
    T *m_data;
    size_t m_size;
    Allocator m_allocator;
    AlignedBuffer<sizeof(T) * N, alignof(T)> m_inline_storage;

  public:
    Array()
    {
        m_data = this->inline_storage();
        m_size = 0;
    }

    Array(ArrayRef<T> values)
    {
        m_size = values.size();
        m_data = this->get_buffer_for_size(values.size());
        uninitialized_copy_n(values.begin(), m_size, m_data);
    }

    Array(const std::initializer_list<T> &values) : Array(ArrayRef<T>(values))
    {
    }

    explicit Array(size_t size)
    {
        m_size = size;
        m_data = this->get_buffer_for_size(size);

        for (size_t i = 0; i < m_size; i++) {
            new (m_data + i) T();
        }
    }

    Array(size_t size, const T &value)
    {
        m_size = size;
        m_data = this->get_buffer_for_size(size);
        uninitialized_fill_n(m_data, m_size, value);
    }

    Array(const Array &other)
    {
        m_size = other.size();
        m_allocator = other.m_allocator;

        m_data = this->get_buffer_for_size(other.size());
        copy_n(other.begin(), m_size, m_data);
    }

    Array(Array &&other) noexcept
    {
        m_size = other.m_size;
        m_allocator = other.m_allocator;

        if (!other.uses_inline_storage()) {
            m_data = other.m_data;
        }
        else {
            m_data = this->get_buffer_for_size(m_size);
            uninitialized_relocate_n(other.m_data, m_size, m_data);
        }

        other.m_data = other.inline_storage();
        other.m_size = 0;
    }

    ~Array()
    {
        destruct_n(m_data, m_size);
        if (!this->uses_inline_storage()) {
            m_allocator.free((void *)m_data);
        }
    }

    Array &operator=(const Array &other)
    {
        if (this == &other) {
            return *this;
        }

        this->~Array();
        new (this) Array(other);
        return *this;
    }

    Array &operator=(Array &&other)
    {
        if (this == &other) {
            return *this;
        }

        this->~Array();
        new (this) Array(std::move(other));
        return *this;
    }

    operator ArrayRef<T>() const
    {
        return ArrayRef<T>(m_data, m_size);
    }

    operator MutableArrayRef<T>()
    {
        return MutableArrayRef<T>(m_data, m_size);
    }

    ArrayRef<T> as_ref() const
    {
        return *this;
    }

    MutableArrayRef<T> as_mutable_ref()
    {
        return *this;
    }

    T &operator[](size_t index)
    {
        assert(index < m_size);
        return m_data[index];
    }

    const T &operator[](size_t index) const
    {
        assert(index < m_size);
        return m_data[index];
    }

    size_t size() const
    {
        return m_size;
    }

    void fill(const T &value)
    {
        MutableArrayRef<T>(*this).fill(value);
    }

    void fill_indices(ArrayRef<size_t> indices, const T &value)
    {
        MutableArrayRef<T>(*this).fill_indices(indices, value);
    }

    const T *begin() const
    {
        return m_data;
    }

    const T *end() const
    {
        return m_data + m_size;
    }

    T *begin()
    {
        return m_data;
    }

    T *end()
    {
        return m_data + m_size;
    }

    IndexRange index_range() const
    {
        return IndexRange(m_size);
    }

  private:
    T *get_buffer_for_size(size_t size)
    {
        if (size <= N) {
            return this->inline_storage();
        }
        else {
            return this->allocate(size);
        }
    }

    T *inline_storage() const
    {
        return (T *)m_inline_storage.ptr();
    }

    T *allocate(size_t size)
    {
        return (T *)m_allocator.allocate(size * sizeof(T), alignof(T));
    }

    bool uses_inline_storage() const
    {
        return m_data == this->inline_storage();
    }
};

}  // namespace bas
