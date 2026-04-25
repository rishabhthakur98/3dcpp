// This file exists solely to compile the Vulkan Memory Allocator implementation.
// We disable the VMA unused variable warnings to keep the compiler output clean.

#define VMA_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

// We include the VMA header from our vendor folder
#include <vk_mem_alloc.h>

#pragma GCC diagnostic pop