#pragma once

// In here everything writen by other people, except BofAsserts.h (because my assert is passed to some external stuff).
// This is expected to go in the precompiled header for the game executable (not the engine lib).
// Don't put things that could change often here.
#include "utils/BofAsserts.h"


#define VULKAN_HPP_NO_EXCEPTIONS 
// defining my own assert for vk assert on bad result. Message and result is supposed to be there in the context.
#define VULKAN_HPP_ASSERT_ON_RESULT(condition) BOF_ASSERT(condition, "%s %s", message, vk::to_string(result).c_str()) 
#include <vulkan/vulkan.hpp>

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

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#pragma warning(push)
#pragma warning(disable: 4201)
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#pragma warning(pop)

#define STBI_ASSERT(condition) BOF_ASSERT(condition, "stb assert") 
#include "stb_image.h"

#include <tiny_obj_loader.h>



