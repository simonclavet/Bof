
#include "utils/BofAsserts.h"


// don't care about warnings from those
#pragma warning(push, 0)

#define VMA_ASSERT(condition) BOF_ASSERT(condition) 

#define VMA_IMPLEMENTATION

#include "VulkanMemoryAllocator/vk_mem_alloc.hpp"
#pragma warning(pop)

