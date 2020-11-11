
#include "Core.h"

// don't care about warnings from those
#pragma warning(push, 0)


#define STB_IMAGE_IMPLEMENTATION
#ifndef STBI_ASSERT
#define STBI_ASSERT(condition) BOF_ASSERT(condition, "stb assert")
#endif
// this is included by tiny_gltf.h
//#include "stb_image/stb_image.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf/tiny_gltf.h"

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_obj_loader/tiny_obj_loader.h"

#define VMA_IMPLEMENTATION
#include "VulkanMemoryAllocator/vk_mem_alloc.hpp"


//#include "VulkanglTFModel.h"




#pragma warning(pop)
