#include <cstdlib>

#include "bas/aligned_allocation.h"

namespace bas {

struct MemHeader {
    int offset;
};

void *aligned_malloc_fallback(size_t size, size_t alignment)
{
    size_t malloc_size = size + sizeof(MemHeader) + alignment - 1;
    uintptr_t real_ptr = ptr_to_int(malloc(malloc_size));
    if (real_ptr == 0) {
        return nullptr;
    }

    uintptr_t aligned_ptr = (real_ptr + sizeof(MemHeader) + alignment - 1) &
                            ~(alignment - 1);
    MemHeader *head = int_to_ptr<MemHeader>(aligned_ptr - sizeof(MemHeader));
    head->offset = (int)(aligned_ptr - real_ptr);

    return int_to_ptr<void>(aligned_ptr);
}

void aligned_free_fallback(void *pointer)
{
    uintptr_t aligned_ptr = ptr_to_int(pointer);
    MemHeader *head = int_to_ptr<MemHeader>(aligned_ptr - sizeof(MemHeader));
    uintptr_t real_ptr = aligned_ptr - head->offset;
    free(int_to_ptr<void>(real_ptr));
}

#if defined(_WIN32)

#    include <crtdbg.h>

static void *aligned_malloc_internal(size_t size, size_t alignment)
{
    return _aligned_malloc_dbg(size, alignment, __FILE__, __LINE__);
}

static void aligned_free_internal(void *pointer)
{
    _aligned_free_dbg(pointer);
}

#elif defined(__APPLE__) || defined(__linux__)

static void *aligned_malloc_internal(size_t size, size_t alignment)
{
    void *pointer;
    if (posix_memalign(&pointer, alignment, size)) {
        return nullptr;
    }
    return pointer;
}

static void aligned_free_internal(void *pointer)
{
    free(pointer);
}

#else

static void *aligned_malloc_internal(size_t size, size_t alignment)
{
    return aligned_malloc_fallback(size, alignment);
}

static void aligned_free_internal(void *pointer)
{
    return aligned_free_fallback(pointer);
}

#endif

void *aligned_malloc(size_t size, size_t alignment)
{
    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }

    return aligned_malloc_internal(size, alignment);
}

void aligned_free(void *pointer)
{
    aligned_free_internal(pointer);
}

}  // namespace bas
