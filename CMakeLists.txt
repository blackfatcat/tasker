cmake_minimum_required(VERSION 4.2)

# Set project name and uppercase version
set(PROJECT_NAME "tasker")

set(root_dir "${CMAKE_CURRENT_SOURCE_DIR}")

project(${PROJECT_NAME} VERSION 0.1 LANGUAGES CXX)

# Build config
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Optimised" CACHE STRING "" FORCE)
set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_BUILD_PARALLEL_LEVEL 8)

if(CMAKE_SYSTEM_NAME MATCHES Windows)
    add_definitions(-D${PROJECT_NAME_UPPER}_WINDOWS)
    set(platform "win")
elseif(CMAKE_SYSTEM_NAME MATCHES Android)
    add_definitions(-D${PROJECT_NAME_UPPER}_ANDROID)
    set(platform "android")
elseif(CMAKE_SYSTEM_NAME MATCHES Emscripten)
    add_definitions(-D${PROJECT_NAME_UPPER}_WASM)
    set(platform "wasm")
else()
    add_definitions(-D${PROJECT_NAME_UPPER}_UNKNOWN)
    set(platform "unknown")
endif()

set(build_dir "${root_dir}/build/${platform}/${PROJECT_NAME}")
set(CMAKE_BUILD_PATH ${build_dir})

set(out_dir "${root_dir}/bin/${platform}/")

# IDE config
set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

# File GLOBs
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "src/public/*.hpp")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/private/*.cpp")

# Target and properties
add_library(${PROJECT_NAME} STATIC
    ${headers}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    LINKER_LANGUAGE CXX
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${out_dir}/debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${out_dir}/release"
    ARCHIVE_OUTPUT_DIRECTORY_OPTIMISED "${out_dir}/optimised"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${out_dir}/debug"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${out_dir}/release"
    LIBRARY_OUTPUT_DIRECTORY_OPTIMISED "${out_dir}/optimised"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${out_dir}/debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${out_dir}/release"
    RUNTIME_OUTPUT_DIRECTORY_OPTIMISED "${out_dir}/optimised"
)

target_sources(${PROJECT_NAME}
    PRIVATE
        ${sources}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        src/public
)