#include "stdafx.h"



#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(condition) BOF_ASSERT(condition, "stb assert") 
#include "stb_image.h"


#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>