#pragma once

#include "utildefines.h"

namespace bas {

void *aligned_malloc(size_t size, size_t alignment);
void aligned_free(void *pointer);

void *aligned_malloc_fallback(size_t size, size_t alignment);
void aligned_free_fallback(void *pointer);

}  // namespace bas
