
cmake_minimum_required(VERSION 3.18)

project(BofEngine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


list(APPEND include_dirs
    "${CMAKE_CURRENT_SOURCE_DIR}/external/"
    )

include_directories(${include_dirs})



set(_src_root_path "${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB_RECURSE _source_list
    LIST_DIRECTORIES false
    "${_src_root_path}/*.cpp"
    "${_src_root_path}/*.h"
    "${_src_root_path}/*.hpp"
    )

foreach(_source IN ITEMS ${_source_list})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${_src_root_path}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

add_library(BofEngine
    ${_source_list}
    )


target_include_directories(BofEngine PUBLIC 
    "${project_dir}"
    "${include_dirs}"
    )

target_link_libraries(BofEngine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/external/vulkan/Lib/vulkan-1.lib"
    debug "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/lib/Debug/glfw3.lib"
    optimized "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/lib/RelWithDebInfo/glfw3.lib"
    )
