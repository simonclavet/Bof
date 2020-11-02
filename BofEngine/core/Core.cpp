#include "Core.h"



#define STB_IMAGE_IMPLEMENTATION
#ifndef STBI_ASSERT
#define STBI_ASSERT(condition) BOF_ASSERT(condition, "stb assert")
#endif
#include "stb_image.h"


#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

//
//
//#define VMA_IMPLEMENTATION
//#include "VulkanMemoryAllocator/vk_mem_alloc.hpp"

//#define VMA_IMPLEMENTATION
//#include "VulkanMemoryAllocator/vk_mem_alloc.h"
