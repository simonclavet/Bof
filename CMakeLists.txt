



cmake_minimum_required(VERSION 3.18)


project(Bof VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo CACHE TYPE INTERNAL FORCE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# debug has no optims
# release has speed optims
# relWithDebugInfo has almost same speed as release, but has debug symbols
# relWithDebugInfo also has asserts, 
# we always use relWithDebugInfo except when debugging very difficult bugs. We ship RELEASE
# The flag NO_BOF_ASSERTS is only set on RELEASE to get the true max speed
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNO_BOF_ASSERTS")

add_subdirectory(BofEngine)
add_subdirectory(BofGame)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT BofGame)

