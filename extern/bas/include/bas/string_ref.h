#pragma once

#include <cstring>
#include <sstream>
#include <string>

#include "array_ref.h"

namespace bas {

class StringRef;

class StringRefBase {
  protected:
    const char *m_data;
    size_t m_size;

    StringRefBase(const char *data, size_t size) : m_data(data), m_size(size)
    {
    }

  public:
    /**
     * Return the (byte-)length of the referenced string, without any
     * null-terminator.
     */
    size_t size() const
    {
        return m_size;
    }

    /**
     * Return a pointer to the start of the string.
     */
    const char *data() const
    {
        return m_data;
    }

    char operator[](size_t index) const
    {
        assert(index <= m_size);
        return m_data[index];
    }

    operator ArrayRef<char>() const
    {
        return ArrayRef<char>(m_data, m_size);
    }

    operator std::string() const
    {
        return std::string(m_data, m_size);
    }

    const char *begin() const
    {
        return m_data;
    }

    const char *end() const
    {
        return m_data + m_size;
    }

    void unsafe_copy(char *dst) const
    {
        memcpy(dst, m_data, m_size);
        dst[m_size] = '\0';
    }

    void copy(char *dst, size_t dst_size) const
    {
        if (m_size < dst_size) {
            this->unsafe_copy(dst);
        }
        else {
            assert(false);
            dst[0] = '\0';
        }
    }

    template<size_t N> void copy(char (&dst)[N])
    {
        this->copy(dst, N);
    }

    /**
     * Returns true when the string begins with the given prefix. Otherwise
     * false.
     */
    bool startswith(StringRef prefix) const;
    bool startswith(char c) const;
    bool startswith_lower_ascii(StringRef prefix) const;

    /**
     * Returns true when the string ends with the given suffix. Otherwise
     * false.
     */
    bool endswith(StringRef suffix) const;
    bool endswith(char c) const;

    StringRef substr(size_t start, size_t size) const;

    StringRef lstrip(ArrayRef<char> chars = {' ', '\t', '\n', '\r'}) const;
    StringRef rstrip(ArrayRef<char> chars = {' ', '\t', '\n', '\r'}) const;
    StringRef strip(ArrayRef<char> chars = {' ', '\t', '\n', '\r'}) const;

    float to_float(bool *r_success = nullptr) const;
    int to_int(bool *r_success = nullptr) const;

    bool contains(char c) const;

    size_t first_index_of(char c, size_t start = 0) const;
    ssize_t try_first_index_of(char c, size_t start = 0) const;
};

/**
 * References a null-terminated char array.
 */
class StringRefNull : public StringRefBase {

  public:
    StringRefNull() : StringRefBase("", 0)
    {
    }

    StringRefNull(const char *str) : StringRefBase(str, strlen(str))
    {
        assert(str != NULL);
        assert(m_data[m_size] == '\0');
    }

    StringRefNull(const char *str, size_t size) : StringRefBase(str, size)
    {
        assert(str[size] == '\0');
    }

    StringRefNull(const std::string &str) : StringRefNull(str.data())
    {
    }

    StringRefNull lstrip(ArrayRef<char> chars = {' ', '\t', '\r', '\n'}) const
    {
        for (size_t i = 0; i < m_size; i++) {
            char c = m_data[i];
            if (!chars.contains(c)) {
                return StringRefNull(m_data + i, m_size - i);
            }
        }
        return "";
    }
};

/**
 * References a char array. It might not be null terminated.
 */
class StringRef : public StringRefBase {
  public:
    StringRef() : StringRefBase(nullptr, 0)
    {
    }

    StringRef(StringRefNull other) : StringRefBase(other.data(), other.size())
    {
    }

    StringRef(const char *str) : StringRefBase(str, str ? strlen(str) : 0)
    {
    }

    StringRef(const char *str, size_t length) : StringRefBase(str, length)
    {
    }

    StringRef(const std::string &str) : StringRefBase(str.data(), str.size())
    {
    }

    /**
     * Return a new StringRef that does not contain the first n chars.
     */
    StringRef drop_prefix(size_t n) const
    {
        assert(n <= m_size);
        return StringRef(m_data + n, m_size - n);
    }

    /**
     * Return a new StringRef that with the given prefix being skipped.
     * Asserts that the string begins with the given prefix.
     */
    StringRef drop_prefix(StringRef prefix) const
    {
        assert(this->startswith(prefix));
        return this->drop_prefix(prefix.size());
    }

    StringRef drop_suffix(size_t n) const
    {
        assert(n <= m_size);
        return StringRef(m_data, m_size - n);
    }

    StringRef drop_suffix(StringRef suffix) const
    {
        assert(this->endswith(suffix));
        return this->drop_suffix(suffix.size());
    }
};

/* More inline functions
 ***************************************/

inline std::ostream &operator<<(std::ostream &stream, StringRef ref)
{
    stream << std::string(ref);
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, StringRefNull ref)
{
    stream << std::string(ref.data(), ref.size());
    return stream;
}

inline std::string operator+(StringRef a, StringRef b)
{
    return std::string(a) + std::string(b);
}

inline bool operator==(StringRef a, StringRef b)
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

inline bool operator!=(StringRef a, StringRef b)
{
    return !(a == b);
}

inline bool StringRefBase::startswith(StringRef prefix) const
{
    if (m_size < prefix.m_size) {
        return false;
    }
    for (size_t i = 0; i < prefix.m_size; i++) {
        if (m_data[i] != prefix.m_data[i]) {
            return false;
        }
    }
    return true;
}

inline char tolower_ascii(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

inline bool StringRefBase::startswith_lower_ascii(StringRef prefix) const
{
#ifdef DEBUG
    for (char c : prefix) {
        char lower_c = tolower_ascii(c);
        assert(c == lower_c);
    }
#endif
    if (m_size < prefix.m_size) {
        return false;
    }
    for (size_t i = 0; i < prefix.m_size; i++) {
        char c = tolower_ascii(m_data[i]);
        if (c != prefix.m_data[i]) {
            return false;
        }
    }
    return true;
}

inline bool StringRefBase::startswith(char c) const
{
    if (m_size == 0) {
        return false;
    }
    return m_data[0] == c;
}

inline bool StringRefBase::endswith(StringRef suffix) const
{
    if (m_size < suffix.m_size) {
        return false;
    }
    size_t offset = m_size - suffix.m_size;
    for (size_t i = 0; i < suffix.m_size; i++) {
        if (m_data[offset + i] != suffix.m_data[i]) {
            return false;
        }
    }
    return true;
}

inline bool StringRefBase::endswith(char c) const
{
    if (m_size == 0) {
        return false;
    }
    return m_data[m_size - 1] == c;
}

inline StringRef StringRefBase::substr(size_t start, size_t size) const
{
    assert(start + size <= m_size);
    return StringRef(m_data + start, size);
}

inline StringRef StringRefBase::lstrip(ArrayRef<char> chars) const
{
    for (size_t i = 0; i < m_size; i++) {
        char c = m_data[i];
        if (!chars.contains(c)) {
            return StringRef(m_data + i, m_size - i);
        }
    }
    return "";
}

inline StringRef StringRefBase::rstrip(ArrayRef<char> chars) const
{
    for (ssize_t i = m_size - 1; i >= 0; i--) {
        char c = m_data[i];
        if (!chars.contains(c)) {
            return StringRef(m_data, i + 1);
        }
    }
    return "";
}

inline StringRef StringRefBase::strip(ArrayRef<char> chars) const
{
    StringRef lstripped = this->lstrip(chars);
    StringRef stripped = lstripped.rstrip(chars);
    return stripped;
}

inline float StringRefBase::to_float(bool *r_success) const
{
    char *str_with_null = (char *)alloca(m_size + 1);
    this->unsafe_copy(str_with_null);
    char *end;
    float value = std::strtof(str_with_null, &end);
    if (r_success) {
        *r_success = str_with_null != end;
    }
    return value;
}

inline int StringRefBase::to_int(bool *r_success) const
{
    char *str_with_null = (char *)alloca(m_size + 1);
    this->unsafe_copy(str_with_null);
    char *end;
    int value = std::strtol(str_with_null, &end, 10);
    if (r_success) {
        *r_success = str_with_null != end;
    }
    return value;
}

inline bool StringRefBase::contains(char c) const
{
    return this->try_first_index_of(c) >= 0;
}

inline size_t StringRefBase::first_index_of(char c, size_t start) const
{
    ssize_t index = this->try_first_index_of(c, start);
    assert(index >= 0);
    return (size_t)index;
}

inline ssize_t StringRefBase::try_first_index_of(char c, size_t start) const
{
    for (size_t i = start; i < m_size; i++) {
        if (m_data[i] == c) {
            return i;
        }
    }
    return -1;
}

}  // namespace bas
