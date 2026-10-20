cmake_minimum_required(VERSION 3.21)
project(DragonSense VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CommonLibSSE-NG defaults its tests ON, which would drag in Catch2.
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/commonlibsse-ng/CMakeLists.txt")
    message(FATAL_ERROR "CommonLibSSE-NG checkout missing at lib/commonlibsse-ng")
endif()

add_subdirectory(lib/commonlibsse-ng commonlib)

# A plain source add_subdirectory does NOT define add_commonlibsse_plugin.
if(NOT COMMAND add_commonlibsse_plugin)
    include(${CMAKE_CURRENT_SOURCE_DIR}/lib/commonlibsse-ng/cmake/CommonLibSSE.cmake)
endif()

add_commonlibsse_plugin(DragonSense
    AUTHOR "steamuser"
    SOURCES
        src/main.cpp
        src/skyrimnet_bridge.cpp
        src/dragon_roar.cpp
        src/world_events.cpp
)

target_compile_features(DragonSense PRIVATE cxx_std_23)
target_compile_options(DragonSense PRIVATE /Zc:preprocessor /permissive- /EHsc /MP /W4)
target_compile_definitions(DragonSense PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_include_directories(DragonSense PRIVATE src)

# CommonLibSSE-NG headers are PCH-dependent; consumers force-include one.
target_precompile_headers(DragonSense PRIVATE src/pch.h)
