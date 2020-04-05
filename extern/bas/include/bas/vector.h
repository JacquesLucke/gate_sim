#pragma once

#include "allocator.h"
#include "array_ref.h"
#include "memory_utils.h"
#include "utildefines.h"

#if defined(_MSC_VER)
#    pragma warning(disable : 4324)
#endif

namespace bas {

template<typename T, size_t N = 4, typename Allocator = RawAllocator>
class Vector {
  private:
    T *m_begin;
    T *m_end;
    T *m_capacity_end;
    Allocator m_allocator;
    AlignedBuffer<sizeof(T) * N, alignof(T)> m_small_buffer;

#ifndef NDEBUG
    /* Storing size in debug builds, because it makes debugging much easier
     * sometimes. */
    size_t m_debug_size;
#    define UPDATE_VECTOR_SIZE(ptr) \
        (ptr)->m_debug_size = (size_t)((ptr)->m_end - (ptr)->m_begin)
#else
#    define UPDATE_VECTOR_SIZE(ptr) ((void)0)
#endif

    template<typename OtherT, size_t OtherN, typename OtherAllocator>
    friend class Vector;

  public:
    /**
     * Create an empty vector.
     * This does not do any memory allocation.
     */
    Vector()
    {
        m_begin = this->small_buffer();
        m_end = m_begin;
        m_capacity_end = m_begin + N;
        UPDATE_VECTOR_SIZE(this);
    }

    /**
     * Create a vector with a specific size.
     * The elements will be default initialized.
     */
    explicit Vector(size_t size) : Vector()
    {
        this->reserve(size);
        this->increase_size_unchecked(size);
        for (T *current = m_begin; current != m_end; current++) {
            new (current) T();
        }
    }

    /**
     * Create a vector filled with a specific value.
     */
    Vector(size_t size, const T &value) : Vector()
    {
        this->reserve(size);
        this->increase_size_unchecked(size);
        uninitialized_fill_n(m_begin, size, value);
    }

    /**
     * Create a vector from an initializer list.
     */
    Vector(std::initializer_list<T> values) : Vector(ArrayRef<T>(values))
    {
    }

    /**
     * Create a vector from an array ref.
     */
    Vector(ArrayRef<T> values) : Vector()
    {
        this->reserve(values.size());
        this->increase_size_unchecked(values.size());
        uninitialized_copy_n(values.begin(), values.size(), this->begin());
    }

    /**
     * Create a vector from any sequence. It must be possible to use the
     * sequence in a range-for loop.
     */
    template<typename Sequence>
    static Vector FromSequence(const Sequence &sequence)
    {
        Vector vector;
        for (const auto &value : sequence) {
            vector.append(value);
        }
        return vector;
    }

    /**
     * Create a copy of another vector.
     * The other vector will not be changed.
     * If the other vector has less than N elements, no allocation will be
     * made.
     */
    Vector(const Vector &other) : m_allocator(other.m_allocator)
    {
        this->init_copy_from_other_vector(other);
    }

    template<size_t OtherN>
    Vector(const Vector<T, OtherN, Allocator> &other)
        : m_allocator(other.m_allocator)
    {
        this->init_copy_from_other_vector(other);
    }

    /**
     * Steal the elements from another vector.
     * This does not do an allocation.
     * The other vector will have zero elements afterwards.
     */
    template<size_t OtherN>
    Vector(Vector<T, OtherN, Allocator> &&other) noexcept
        : m_allocator(other.m_allocator)
    {
        size_t size = other.size();

        if (other.is_small()) {
            if (size <= N) {
                /* Copy between inline buffers. */
                m_begin = this->small_buffer();
                m_end = m_begin + size;
                m_capacity_end = m_begin + N;
                uninitialized_relocate_n(other.m_begin, size, m_begin);
            }
            else {
                /* Copy from inline buffer to newly allocated buffer. */
                size_t capacity = size;
                m_begin = (T *)m_allocator.allocate(
                    sizeof(T) * capacity, std::alignment_of<T>::value);
                m_end = m_begin + size;
                m_capacity_end = m_begin + capacity;
                uninitialized_relocate_n(other.m_begin, size, m_begin);
            }
        }
        else {
            /* Steal the pointer. */
            m_begin = other.m_begin;
            m_end = other.m_end;
            m_capacity_end = other.m_capacity_end;
        }

        other.m_begin = other.small_buffer();
        other.m_end = other.m_begin;
        other.m_capacity_end = other.m_begin + OtherN;
        UPDATE_VECTOR_SIZE(this);
        UPDATE_VECTOR_SIZE(&other);
    }

    ~Vector()
    {
        destruct_n(m_begin, this->size());
        if (!this->is_small()) {
            m_allocator.free(m_begin);
        }
    }

    operator ArrayRef<T>() const
    {
        return ArrayRef<T>(m_begin, this->size());
    }

    operator MutableArrayRef<T>()
    {
        return MutableArrayRef<T>(m_begin, this->size());
    }

    ArrayRef<T> as_ref() const
    {
        return *this;
    }

    MutableArrayRef<T> as_mutable_ref()
    {
        return *this;
    }

    Vector &operator=(const Vector &other)
    {
        if (this == &other) {
            return *this;
        }

        this->~Vector();
        new (this) Vector(other);

        return *this;
    }

    Vector &operator=(Vector &&other)
    {
        if (this == &other) {
            return *this;
        }

        /* This can fail, when the vector is used to build a recursive data
           structure. See https://youtu.be/7Qgd9B1KuMQ?t=840. */
        this->~Vector();
        new (this) Vector(std::move(other));

        return *this;
    }

    /**
     * Make sure that enough memory is allocated to hold size elements.
     * This won't necessarily make an allocation when size is small.
     * The actual size of the vector does not change.
     */
    void reserve(size_t size)
    {
        this->grow(size);
    }

    /**
     * Afterwards the vector has 0 elements, but will still have
     * memory to be refilled again.
     */
    void clear()
    {
        destruct_n(m_begin, this->size());
        m_end = m_begin;
        UPDATE_VECTOR_SIZE(this);
    }

    /**
     * Afterwards the vector has 0 elements and any allocated memory
     * will be freed.
     */
    void clear_and_make_small()
    {
        destruct_n(m_begin, this->size());
        if (!this->is_small()) {
            m_allocator.free(m_begin);
        }

        m_begin = this->small_buffer();
        m_end = m_begin;
        m_capacity_end = m_begin + N;
        UPDATE_VECTOR_SIZE(this);
    }

    /**
     * Insert a new element at the end of the vector.
     * This might cause a reallocation with the capacity is exceeded.
     */
    void append(const T &value)
    {
        this->ensure_space_for_one();
        this->append_unchecked(value);
    }

    void append(T &&value)
    {
        this->ensure_space_for_one();
        this->append_unchecked(std::move(value));
    }

    size_t append_and_get_index(const T &value)
    {
        size_t index = this->size();
        this->append(value);
        return index;
    }

    void append_non_duplicates(const T &value)
    {
        if (!this->contains(value)) {
            this->append(value);
        }
    }

    void append_unchecked(const T &value)
    {
        assert(m_end < m_capacity_end);
        new (m_end) T(value);
        m_end++;
        UPDATE_VECTOR_SIZE(this);
    }

    void append_unchecked(T &&value)
    {
        assert(m_end < m_capacity_end);
        new (m_end) T(std::move(value));
        m_end++;
        UPDATE_VECTOR_SIZE(this);
    }

    /**
     * Insert the same element n times at the end of the vector.
     * This might result in a reallocation internally.
     */
    void append_n_times(const T &value, size_t n)
    {
        this->reserve(this->size() + n);
        uninitialized_fill_n(m_end, n, value);
        this->increase_size_unchecked(n);
    }

    void increase_size_unchecked(size_t n)
    {
        assert(m_end + n <= m_capacity_end);
        m_end += n;
        UPDATE_VECTOR_SIZE(this);
    }

    /**
     * Copy the elements of another array to the end of this vector.
     */
    void extend(ArrayRef<T> array)
    {
        this->extend(array.begin(), array.size());
    }

    void extend(const T *start, size_t amount)
    {
        this->reserve(this->size() + amount);
        this->extend_unchecked(start, amount);
    }

    void extend_non_duplicates(ArrayRef<T> array)
    {
        for (const T &value : array) {
            this->append_non_duplicates(value);
        }
    }

    void extend_unchecked(ArrayRef<T> array)
    {
        this->extend_unchecked(array.begin(), array.size());
    }

    void extend_unchecked(const T *start, size_t amount)
    {
        assert(m_begin + amount <= m_capacity_end);
        uninitialized_copy_n(start, amount, m_end);
        m_end += amount;
        UPDATE_VECTOR_SIZE(this);
    }

    /**
     * Return a reference to the last element in the vector.
     * This will assert when the vector is empty.
     */
    const T &last() const
    {
        assert(this->size() > 0);
        return *(m_end - 1);
    }

    T &last()
    {
        assert(this->size() > 0);
        return *(m_end - 1);
    }

    /**
     * Replace every element with a new value.
     */
    void fill(const T &value)
    {
        std::fill(m_begin, m_end, value);
    }

    void fill_indices(ArrayRef<size_t> indices, const T &value)
    {
        MutableArrayRef<T>(*this).fill_indices(indices, value);
    }

    /**
     * Return how many values are currently stored in the vector.
     */
    size_t size() const
    {
        assert(m_debug_size == (size_t)(m_end - m_begin));
        return (size_t)(m_end - m_begin);
    }

    /**
     * Returns true when the vector contains no elements, otherwise false.
     */
    bool is_empty() const
    {
        return m_begin == m_end;
    }

    /**
     * Deconstructs the last element and decreases the size by one.
     * This will assert when the vector is empty.
     */
    void remove_last()
    {
        assert(!this->is_empty());
        m_end--;
        destruct(m_end);
        UPDATE_VECTOR_SIZE(this);
    }

    /**
     * Remove the last element from the vector and return it.
     */
    T pop_last()
    {
        assert(!this->is_empty());
        m_end--;
        T value = std::move(*m_end);
        destruct(m_end);
        UPDATE_VECTOR_SIZE(this);
        return value;
    }

    /**
     * Delete any element in the vector.
     * The empty space will be filled by the previously last element.
     */
    void remove_and_reorder(size_t index)
    {
        assert(index < this->size());
        T *element_to_remove = m_begin + index;
        m_end--;
        if (element_to_remove < m_end) {
            *element_to_remove = std::move(*m_end);
        }
        destruct(m_end);
        UPDATE_VECTOR_SIZE(this);
    }

    void remove_first_occurrence_and_reorder(const T &value)
    {
        size_t index = this->index(value);
        this->remove_and_reorder((size_t)index);
    }

    /**
     * Do a linear search to find the value in the vector.
     * When found, return the first index, otherwise return -1.
     */
    ssize_t index_try(const T &value) const
    {
        for (T *current = m_begin; current != m_end; current++) {
            if (*current == value) {
                return current - m_begin;
            }
        }
        return -1;
    }

    /**
     * Do a linear search to find the value in the vector.
     * When found, return the first index, otherwise fail.
     */
    size_t index(const T &value) const
    {
        ssize_t index = this->index_try(value);
        assert(index >= 0);
        return (size_t)index;
    }

    /**
     * Do a linear search to see of the value is in the vector.
     * Return true when it exists, otherwise false.
     */
    bool contains(const T &value) const
    {
        return this->index_try(value) != -1;
    }

    /**
     * Compare vectors element-wise.
     * Return true when they have the same length and all elements
     * compare equal, otherwise false.
     */
    static bool all_equal(const Vector &a, const Vector &b)
    {
        if (a.size() != b.size()) {
            return false;
        }
        for (size_t i = 0; i < a.size(); i++) {
            if (a[i] != b[i]) {
                return false;
            }
        }
        return true;
    }

    const T &operator[](size_t index) const
    {
        assert(index < this->size());
        return m_begin[index];
    }

    T &operator[](size_t index)
    {
        assert(index < this->size());
        return m_begin[index];
    }

    T *begin()
    {
        return m_begin;
    }
    T *end()
    {
        return m_end;
    }

    const T *begin() const
    {
        return m_begin;
    }
    const T *end() const
    {
        return m_end;
    }

    /**
     * Get the current capacity of the vector.
     */
    size_t capacity() const
    {
        return (size_t)(m_capacity_end - m_begin);
    }

    IndexRange index_range() const
    {
        return IndexRange(this->size());
    }

    void print_stats() const
    {
        std::cout << "Small Vector at " << (void *)this << ":" << std::endl;
        std::cout << "  Elements: " << this->size() << std::endl;
        std::cout << "  Capacity: " << (m_capacity_end - m_begin) << std::endl;
        std::cout << "  Small Elements: " << N
                  << "  Size on Stack: " << sizeof(*this) << std::endl;
    }

  private:
    T *small_buffer() const
    {
        return (T *)m_small_buffer.ptr();
    }

    bool is_small() const
    {
        return m_begin == this->small_buffer();
    }

    void ensure_space_for_one()
    {
        if (BAS_UNLIKELY(m_end >= m_capacity_end)) {
            this->grow(std::max(this->size() * 2, (size_t)1));
        }
    }

    BAS_NOINLINE void grow(size_t min_capacity)
    {
        if (this->capacity() >= min_capacity) {
            return;
        }

        /* Round up to the next power of two. Otherwise consecutive calls to
         * grow can cause a reallocation every time even though the
         * min_capacity only increments. */
        min_capacity = ceil_power_of_2<size_t>(min_capacity);

        size_t size = this->size();

        T *new_array = (T *)m_allocator.allocate(
            min_capacity * (size_t)sizeof(T), std::alignment_of<T>::value);
        uninitialized_relocate_n(m_begin, size, new_array);

        if (!this->is_small()) {
            m_allocator.free(m_begin);
        }

        m_begin = new_array;
        m_end = m_begin + size;
        m_capacity_end = m_begin + min_capacity;
    }

    /**
     * Initialize all properties, except for m_allocator, which has to be
     * initialized beforehand.
     */
    template<size_t OtherN>
    void init_copy_from_other_vector(const Vector<T, OtherN, Allocator> &other)
    {
        m_allocator = other.m_allocator;

        size_t size = other.size();
        size_t capacity = size;

        if (size <= N) {
            m_begin = this->small_buffer();
            capacity = N;
        }
        else {
            m_begin = (T *)m_allocator.allocate(sizeof(T) * size,
                                                std::alignment_of<T>::value);
            capacity = size;
        }

        m_end = m_begin + size;
        m_capacity_end = m_begin + capacity;

        uninitialized_copy(other.begin(), other.end(), m_begin);
        UPDATE_VECTOR_SIZE(this);
    }
};

#undef UPDATE_VECTOR_SIZE

}  // namespace bas
