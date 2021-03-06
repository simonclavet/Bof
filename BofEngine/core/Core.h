﻿#pragma once

#define __STDC_WANT_SECURE_LIB__ 1

#define _USE_MATH_DEFINES
#include <cmath>

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <set>
#include <unordered_map>

// In here everything writen by other people, except BofAsserts.h (because my assert is passed to some external stuff).
// This is expected to go in the precompiled header for the game executable (not the engine lib).
// Don't put things that could change often here.
#include "utils/BofAsserts.h"

#include "utils/Utils.h"

// don't care about warnings from those
#pragma warning(push, 0)

#define VULKAN_HPP_NO_EXCEPTIONS 
// defining my own assert for vk assert on bad result. Message and result is supposed to be there in the context.
#define VULKAN_HPP_ASSERT_ON_RESULT(condition) BOF_ASSERT_MSG(condition, "%s %s", message, vk::to_string(result).c_str()) 
#define VMA_ASSERT(condition) BOF_ASSERT(condition) 
#include <VulkanMemoryAllocator/vk_mem_alloc.hpp>
//#include <vulkan/vulkan.hpp>



#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils/AABB.hpp"


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define STBI_ASSERT(condition) BOF_ASSERT_MSG((condition), "stb assert") 
#include "stb_image/stb_image.h"

#include "tiny_obj_loader/tiny_obj_loader.h"

#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"


#include "ktx/ktx.h"
#include "ktx/ktxvulkan.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf/tiny_gltf.h"



#include <GLFW/glfw3.h>

#pragma warning(pop)
