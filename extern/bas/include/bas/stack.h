#pragma once

#include "vector.h"

namespace bas {

template<typename T, size_t N = 4, typename Allocator = RawAllocator>
class Stack {
  private:
    Vector<T, N, Allocator> m_elements;

  public:
    Stack() = default;

    /**
     * Construct a stack from an array ref. The elements will be pushed in the
     * same order they are in the array.
     */
    Stack(ArrayRef<T> values) : m_elements(values)
    {
    }

    operator ArrayRef<T>()
    {
        return m_elements;
    }

    /**
     * Return the number of elements in the stack.
     */
    size_t size() const
    {
        return m_elements.size();
    }

    /**
     * Return true when the stack is empty, otherwise false.
     */
    bool is_empty() const
    {
        return this->size() == 0;
    }

    /**
     * Add a new element to the top of the stack.
     */
    void push(const T &value)
    {
        m_elements.append(value);
    }

    void push(T &&value)
    {
        m_elements.append(std::move(value));
    }

    void push_multiple(ArrayRef<T> values)
    {
        m_elements.extend(values);
    }

    /**
     * Remove the element from the top of the stack and return it.
     * This will assert when the stack is empty.
     */
    T pop()
    {
        return m_elements.pop_last();
    }

    /**
     * Return a reference to the value a the top of the stack.
     * This will assert when the stack is empty.
     */
    T &peek()
    {
        assert(!this->is_empty());
        return m_elements[this->size() - 1];
    }

    T *begin()
    {
        return m_elements.begin();
    }

    T *end()
    {
        return m_elements.end();
    }

    const T *begin() const
    {
        return m_elements.begin();
    }

    const T *end() const
    {
        return m_elements.end();
    }

    /**
     * Remove all elements from the stack but keep the memory.
     */
    void clear()
    {
        m_elements.clear();
    }

    /**
     * Remove all elements and free any allocated memory.
     */
    void clear_and_make_small()
    {
        m_elements.clear_and_make_small();
    }

    /**
     * Does a linear search to check if the value is in the stack.
     */
    bool contains(const T &value)
    {
        return m_elements.contains(value);
    }
};

}  // namespace bas
