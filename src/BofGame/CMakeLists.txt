
cmake_minimum_required(VERSION 3.18)

project(BofGame VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_executable(BofGame 
    BofGame.cpp)

target_link_libraries(BofGame LINK_PUBLIC BofEngine)


set_target_properties(BofGame PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})