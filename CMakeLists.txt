cmake_minimum_required(VERSION 3.31)

# Set project name and uppercase version
set(TSKR_PROJECT_NAME "tasker")
set(TSKR_PROJECT_NAME_UPPER "")

include(CMake/funcs.cmake)

option(TSKR_STANDALONE "Build the project by itself" ON)
option(TSKR_BUILD_EXAMPLES_ALL "Build all examples" OFF)
set(TSKR_BUILD_EXAMPLE "" CACHE STRING "example name")

string(TOUPPER ${TSKR_PROJECT_NAME} TSKR_PROJECT_NAME_UPPER) # For defines

project(${TSKR_PROJECT_NAME} VERSION 0.1 LANGUAGES CXX)

if(CMAKE_SYSTEM_NAME MATCHES Windows)
    add_definitions(-D${TSKR_PROJECT_NAME_UPPER}_WINDOWS)
    set(platform "win")
elseif(CMAKE_SYSTEM_NAME MATCHES Linux)
    add_definitions(-D${TSKR_PROJECT_NAME_UPPER}_LINUX)
    add_compile_options(-Wno-interference-size)
    set(platform "linux")
elseif(CMAKE_SYSTEM_NAME MATCHES Android)
    add_definitions(-D${TSKR_PROJECT_NAME_UPPER}_ANDROID)
    set(platform "android")
elseif(CMAKE_SYSTEM_NAME MATCHES Emscripten)
    add_definitions(-D${TSKR_PROJECT_NAME_UPPER}_WASM)
    set(platform "wasm")
else()
    add_definitions(-D${TSKR_PROJECT_NAME_UPPER}_UNKNOWN)
    set(platform "unknown")
endif()

# File GLOBs
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "src/public/*.hpp")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/private/*.cpp")

# Target and properties
add_library(${TSKR_PROJECT_NAME} STATIC
    ${headers}
)

set_target_properties(${TSKR_PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    LINKER_LANGUAGE CXX
    OUTPUT_NAME ${TSKR_PROJECT_NAME}
)

target_sources(${TSKR_PROJECT_NAME}
    PRIVATE
        ${sources}
)

target_include_directories(${TSKR_PROJECT_NAME}
    PUBLIC
        src/public
        src/extern
)


if(${TSKR_STANDALONE})
    set(tskr_root_dir "${CMAKE_CURRENT_SOURCE_DIR}")

    # Build config
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Optimised" CACHE STRING "" FORCE)
    set(CMAKE_SUPPRESS_REGENERATION true)
    set(CMAKE_BUILD_PARALLEL_LEVEL 8)

    set(tskr_build_dir "${tskr_root_dir}/build/${platform}/${TSKR_PROJECT_NAME}")
    set(CMAKE_BUILD_PATH ${tskr_build_dir})

    set(tskr_out_dir "${tskr_root_dir}/bin/${platform}/")

    # IDE config
    set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

    # MSVC ONLY COMPILE OPTIONS
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS_OPTIMISED "/O2 /Zi /DEBUG" CACHE STRING "Flags for Optimised configuration" FORCE)
        set(CMAKE_EXE_LINKER_FLAGS_OPTIMISED "/DEBUG /OPT:REF /OPT:ICF" CACHE STRING "Linker flags for Optimised configuration" FORCE)
    endif()

    set_target_properties(${TSKR_PROJECT_NAME} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${tskr_out_dir}/debug"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${tskr_out_dir}/release"
        ARCHIVE_OUTPUT_DIRECTORY_OPTIMISED "${tskr_out_dir}/optimised"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${tskr_out_dir}/debug"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${tskr_out_dir}/release"
        LIBRARY_OUTPUT_DIRECTORY_OPTIMISED "${tskr_out_dir}/optimised"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${tskr_out_dir}/debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${tskr_out_dir}/release"
        RUNTIME_OUTPUT_DIRECTORY_OPTIMISED "${tskr_out_dir}/optimised"
    )

    set(build_dx3d FALSE)

    # Build Examples
    if(${TSKR_BUILD_EXAMPLES_ALL})
        # Library will propagate the CXX standart to the Examples projects as well
        target_compile_features(${TSKR_PROJECT_NAME} PUBLIC cxx_std_20)

        file(GLOB EXAMPLE_SOURCES "examples/*.cpp")

        # All examples
        foreach(example ${EXAMPLE_SOURCES})
            tskr_create_example(SRC_FILE "${example}")
        endforeach()
    elseif(TSKR_BUILD_EXAMPLE)
        #Single example
        set(example_source "examples/${TSKR_BUILD_EXAMPLE}.cpp")
        tskr_create_example(SRC_FILE "${example_source}")
    endif()
endif()
