#pragma once

#include <cstdlib>

#include "aligned_allocation.h"

namespace bas {

class RawAllocator {
  public:
    void *allocate(size_t size, size_t alignment) const
    {
        return aligned_malloc(size, alignment);
    }

    void free(void *pointer) const
    {
        aligned_free(pointer);
    }
};

}  // namespace bas
