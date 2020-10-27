
cmake_minimum_required(VERSION 3.18)

project(BofEngine VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


list(APPEND include_dirs
    #"C:/VulkanSDK/1.2.148.1/Include"
    #"C:/glfw/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/"
    )
#message(allo)
#message(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${include_dirs})

#include_directories(
    #"external/VulkanSDK/1.2.148.1/Include"
    #"external/glm"
    #"external/glfw/include"
    #)

file(GLOB externalBase
    "external/*.cpp"
    "external/*.h")

source_group("external" FILES ${externalBase})  


file(GLOB_RECURSE pods
    "external/pods/**.cpp"
    "external/pods/**.h")

source_group("external/pods" FILES ${pods})  

file(GLOB_RECURSE utils
    "utils/*.cpp"
    "utils/*.h")
source_group("utils" FILES ${utils})  

file(GLOB_RECURSE core
    "core/*.cpp"
    "core/*.h")

source_group("core" FILES ${core})  



add_library(BofEngine
    ${core}
    ${utils}
    ${externalBase}
    ${pods}
    )




target_include_directories(BofEngine PUBLIC 
    "${project_dir}"
    "${include_dirs}"
    )

#target_link_libraries(BofEngine PUBLIC
    #"C:/VulkanSDK/1.2.148.1/Lib/vulkan-1.lib"
#    "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/lib/RelWithDebInfo/glfw3.lib"
  # )

target_link_libraries(BofEngine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/external/vulkan/Lib/vulkan-1.lib"
    debug "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/lib/Debug/glfw3.lib"
    optimized "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/lib/RelWithDebInfo/glfw3.lib"
   )

#target_link_libraries(BofEngine optimized
#    "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/lib/RelWithDebInfo/glfw3.lib"
#   )
