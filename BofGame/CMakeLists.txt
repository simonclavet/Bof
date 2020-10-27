
cmake_minimum_required(VERSION 3.18)

project(BofGame VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)



set(_src_root_path "${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB_RECURSE _source_list
    LIST_DIRECTORIES false
    "${_src_root_path}/*.cpp"
    "${_src_root_path}/*.h"
    "${_src_root_path}/*.hpp"
    )

# vs filters
foreach(_source IN ITEMS ${_source_list})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${_src_root_path}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()



add_executable(BofGame ${_source_list})

target_precompile_headers(BofGame PRIVATE stdafx.h)

target_link_libraries(BofGame LINK_PUBLIC BofEngine)


set_target_properties(BofGame PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
