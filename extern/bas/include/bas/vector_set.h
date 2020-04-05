#pragma once
/**
 *
 * A VectorSet is a set built on top of a vector. The elements are stored in a
 * continuous array, but every element exists at most once. The insertion order
 * is maintained, as long as there are no deletes. The expected time to check
 * if a value is in the VectorSet is O(1).
 */

#include "hash.h"
#include "open_addressing.h"
#include "vector.h"

namespace bas {

// clang-format off

#define ITER_SLOTS_BEGIN(VALUE, ARRAY, OPTIONAL_CONST, R_SLOT) \
  uint32_t hash = DefaultHash<T>{}(VALUE); \
  uint32_t perturb = hash; \
  while (true) { \
    for (uint32_t i = 0; i < 4; i++) {\
      uint32_t slot_index = (hash + i) & ARRAY.slot_mask(); \
      OPTIONAL_CONST Slot &R_SLOT = ARRAY.item(slot_index);

#define ITER_SLOTS_END \
    } \
    perturb >>= 5; \
    hash = hash * 5 + 1 + perturb; \
  } ((void)0)

// clang-format on

template<typename T, typename Allocator = RawAllocator> class VectorSet {
  private:
    static constexpr int32_t IS_EMPTY = -1;
    static constexpr int32_t IS_DUMMY = -2;

    class Slot {
      private:
        int32_t m_value = IS_EMPTY;

      public:
        static constexpr uint32_t slots_per_item = 1;

        bool is_set() const
        {
            return m_value >= 0;
        }

        bool is_empty() const
        {
            return m_value == IS_EMPTY;
        }

        bool is_dummy() const
        {
            return m_value == IS_DUMMY;
        }

        bool has_value(const T &value, const Vector<T> &elements) const
        {
            return this->is_set() && elements[this->index()] == value;
        }

        bool has_index(uint32_t index) const
        {
            return m_value == (int32_t)index;
        }

        uint32_t index() const
        {
            assert(this->is_set());
            return (uint32_t)m_value;
        }

        int32_t &index_ref()
        {
            return m_value;
        }

        void set_index(uint32_t index)
        {
            assert(!this->is_set());
            m_value = (int32_t)index;
        }

        void set_dummy()
        {
            assert(this->is_set());
            m_value = IS_DUMMY;
        }
    };

    using ArrayType = OpenAddressingArray<Slot, 4, Allocator>;
    ArrayType m_array;
    Vector<T, 4, Allocator> m_elements;

  public:
    VectorSet()
    {
        assert(m_array.slots_usable() <= m_elements.capacity());
    }

    VectorSet(ArrayRef<T> values) : VectorSet()
    {
        this->add_multiple(values);
    }

    VectorSet(const std::initializer_list<T> &values) : VectorSet()
    {
        this->add_multiple(values);
    }

    VectorSet(const Vector<T> &values) : VectorSet()
    {
        this->add_multiple(values);
    }

    /**
     * Allocate memory such that at least min_usable_slots can be added without
     * having to grow again.
     */
    void reserve(uint32_t min_usable_slots)
    {
        if (m_array.slots_usable() < min_usable_slots) {
            this->grow(min_usable_slots);
        }
    }

    /**
     * Add a new element. The method assumes that the value did not exist
     * before.
     */
    void add_new(const T &value)
    {
        this->add_new__impl(value);
    }
    void add_new(T &&value)
    {
        this->add_new__impl(std::move(value));
    }

    /**
     * Add a new element if it does not exist yet. Does not add the value again
     * if it exists already.
     */
    bool add(const T &value)
    {
        return this->add__impl(value);
    }
    bool add(T &&value)
    {
        return this->add__impl(std::move(value));
    }

    /**
     * Add multiple values. Duplicates will not be inserted.
     */
    void add_multiple(ArrayRef<T> values)
    {
        for (const T &value : values) {
            this->add(value);
        }
    }

    /**
     * Returns true when the value is in the set-vector, otherwise false.
     */
    bool contains(const T &value) const
    {
        ITER_SLOTS_BEGIN(value, m_array, const, slot)
        {
            if (slot.is_empty()) {
                return false;
            }
            else if (slot.has_value(value, m_elements)) {
                return true;
            }
        }
        ITER_SLOTS_END;
    }

    /**
     * Remove a value from the set-vector. The method assumes that the value
     * exists.
     */
    void remove(const T &value)
    {
        assert(this->contains(value));
        ITER_SLOTS_BEGIN(value, m_array, , slot)
        {
            if (slot.has_value(value, m_elements)) {
                uint32_t old_index = (uint32_t)m_elements.size() - 1;
                uint32_t new_index = slot.index();

                m_elements.remove_and_reorder(new_index);
                slot.set_dummy();
                m_array.update__set_to_dummy();

                if (old_index != new_index) {
                    T &moved_value = m_elements[new_index];
                    this->update_slot_index(moved_value, old_index, new_index);
                }
                return;
            }
        }
        ITER_SLOTS_END;
    }

    /**
     * Get and remove the last element of the vector.
     */
    T pop()
    {
        assert(this->size() > 0);
        T value = m_elements.pop_last();
        uint32_t old_index = (uint32_t)m_elements.size();

        ITER_SLOTS_BEGIN(value, m_array, , slot)
        {
            if (slot.has_index(old_index)) {
                slot.set_dummy();
                m_array.update__set_to_dummy();
                return value;
            }
        }
        ITER_SLOTS_END;
    }

    /**
     * Get the index of the value in the vector. It is assumed that the value
     * is in the vector.
     */
    uint32_t index(const T &value) const
    {
        assert(this->contains(value));
        ITER_SLOTS_BEGIN(value, m_array, const, slot)
        {
            if (slot.has_value(value, m_elements)) {
                return slot.index();
            }
        }
        ITER_SLOTS_END;
    }

    /**
     * Get the index of the value in the vector. If it does not exist return
     * -1.
     */
    int index_try(const T &value) const
    {
        ITER_SLOTS_BEGIN(value, m_array, const, slot)
        {
            if (slot.has_value(value, m_elements)) {
                return slot.index();
            }
            else if (slot.is_empty()) {
                return -1;
            }
        }
        ITER_SLOTS_END;
    }

    /**
     * Get the number of elements in the set-vector.
     */
    uint32_t size() const
    {
        return m_array.slots_set();
    }

    const T *begin() const
    {
        return m_elements.begin();
    }

    const T *end() const
    {
        return m_elements.end();
    }

    const T &operator[](uint32_t index) const
    {
        return m_elements[index];
    }

    ArrayRef<T> as_ref() const
    {
        return *this;
    }

    operator ArrayRef<T>() const
    {
        return m_elements;
    }

    void print_stats() const
    {
        std::cout << "VectorSet at " << (void *)this << ":\n";
        std::cout << "  Size: " << this->size() << "\n";
        std::cout << "  Usable Slots: " << m_array.slots_usable() << "\n";
        std::cout << "  Total Slots: " << m_array.slots_total() << "\n";
        std::cout << "  Average Collisions: "
                  << this->compute_average_collisions() << "\n";
    }

  private:
    void update_slot_index(T &value, uint32_t old_index, uint32_t new_index)
    {
        ITER_SLOTS_BEGIN(value, m_array, , slot)
        {
            int32_t &stored_index = slot.index_ref();
            if (stored_index == (int32_t)old_index) {
                stored_index = new_index;
                return;
            }
        }
        ITER_SLOTS_END;
    }

    template<typename ForwardT>
    void add_new_in_slot(Slot &slot, ForwardT &&value)
    {
        uint32_t index = (uint32_t)m_elements.size();
        slot.set_index(index);
        m_elements.append_unchecked(std::forward<ForwardT>(value));
        m_array.update__empty_to_set();
    }

    void ensure_can_add()
    {
        if (BAS_UNLIKELY(m_array.should_grow())) {
            this->grow(this->size() + 1);
        }
    }

    BAS_NOINLINE void grow(uint32_t min_usable_slots)
    {
        ArrayType new_array = m_array.init_reserved(min_usable_slots);

        for (uint32_t i = 0; i < (uint32_t)m_elements.size(); i++) {
            this->add_after_grow(i, new_array);
        }

        m_array = std::move(new_array);
        m_elements.reserve(m_array.slots_usable());
    }

    void add_after_grow(uint32_t index, ArrayType &new_array)
    {
        const T &value = m_elements[index];
        ITER_SLOTS_BEGIN(value, new_array, , slot)
        {
            if (slot.is_empty()) {
                slot.set_index(index);
                return;
            }
        }
        ITER_SLOTS_END;
    }

    float compute_average_collisions() const
    {
        if (m_elements.size() == 0u) {
            return 0.0f;
        }

        uint32_t collisions_sum = 0;
        for (const T &value : m_elements) {
            collisions_sum += this->count_collisions(value);
        }
        return (float)collisions_sum / (float)m_elements.size();
    }

    uint32_t count_collisions(const T &value) const
    {
        uint32_t collisions = 0;
        ITER_SLOTS_BEGIN(value, m_array, const, slot)
        {
            if (slot.is_empty() || slot.has_value(value, m_elements)) {
                return collisions;
            }
            collisions++;
        }
        ITER_SLOTS_END;
    }

    template<typename ForwardT> void add_new__impl(ForwardT &&value)
    {
        assert(!this->contains(value));
        this->ensure_can_add();
        ITER_SLOTS_BEGIN(value, m_array, , slot)
        {
            if (slot.is_empty()) {
                this->add_new_in_slot(slot, std::forward<ForwardT>(value));
                return;
            }
        }
        ITER_SLOTS_END;
    }

    template<typename ForwardT> bool add__impl(ForwardT &&value)
    {
        this->ensure_can_add();
        ITER_SLOTS_BEGIN(value, m_array, , slot)
        {
            if (slot.is_empty()) {
                this->add_new_in_slot(slot, std::forward<ForwardT>(value));
                return true;
            }
            else if (slot.has_value(value, m_elements)) {
                return false;
            }
        }
        ITER_SLOTS_END;
    }
};

#undef ITER_SLOTS_BEGIN
#undef ITER_SLOTS_END

}  // namespace bas
