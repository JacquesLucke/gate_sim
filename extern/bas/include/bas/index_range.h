#pragma once

#include <iostream>

#include "utildefines.h"

namespace bas {

class IndexRange {
  private:
    size_t m_start = 0;
    size_t m_size = 0;

  public:
    IndexRange() = default;

    explicit IndexRange(size_t size) : m_start(0), m_size(size)
    {
    }

    IndexRange(size_t start, size_t size) : m_start(start), m_size(size)
    {
    }

    class Iterator {
      private:
        size_t m_current;

      public:
        Iterator(size_t current) : m_current(current)
        {
        }

        Iterator &operator++()
        {
            m_current++;
            return *this;
        }

        bool operator!=(const Iterator &iterator) const
        {
            return m_current != iterator.m_current;
        }

        size_t operator*() const
        {
            return m_current;
        }
    };

    Iterator begin() const
    {
        return Iterator(m_start);
    }

    Iterator end() const
    {
        return Iterator(m_start + m_size);
    }

    /**
     * Access an element in the range.
     */
    size_t operator[](size_t index) const
    {
        assert(index < m_size);
        return m_start + index;
    }

    /**
     * Two ranges compare equal when they contain the same numbers.
     */
    friend bool operator==(IndexRange a, IndexRange b)
    {
        return (a.m_size == b.m_size) &&
               (a.m_start == b.m_start || a.m_size == 0);
    }

    /**
     * Get the amount of numbers in the range.
     */
    size_t size() const
    {
        return m_size;
    }

    /**
     * Create a new range starting at the end of the current one.
     */
    IndexRange after(size_t n) const
    {
        return IndexRange(m_start + m_size, n);
    }

    /**
     * Create a new range that ends at the start of the current one.
     */
    IndexRange before(size_t n) const
    {
        return IndexRange(m_start - n, n);
    }

    /**
     * Get the first element in the range.
     * Asserts when the range is empty.
     */
    size_t first() const
    {
        assert(this->size() > 0);
        return m_start;
    }

    /**
     * Get the last element in the range.
     * Asserts when the range is empty.
     */
    size_t last() const
    {
        assert(this->size() > 0);
        return m_start + m_size - 1;
    }

    /**
     * Get the element one after the end. The returned value is undefined when
     * the range is empty.
     */
    size_t one_after_last() const
    {
        return m_start + m_size;
    }

    /**
     * Get the first element in the range. The returned value is undefined when
     * the range is empty.
     */
    size_t start() const
    {
        return m_start;
    }

    /**
     * Returns true when the range contains a certain number, otherwise false.
     */
    bool contains(size_t value) const
    {
        return value >= m_start && value < m_start + m_size;
    }

    IndexRange slice(size_t start, size_t size) const
    {
        size_t new_start = m_start + start;
        assert(new_start + size <= m_start + m_size || size == 0);
        return IndexRange(new_start, size);
    }

    IndexRange slice(IndexRange range) const
    {
        return this->slice(range.start(), range.size());
    }

    friend std::ostream &operator<<(std::ostream &stream, IndexRange range)
    {
        stream << "[" << range.start() << ", " << range.one_after_last()
               << ")";
        return stream;
    }
};

}  // namespace bas
