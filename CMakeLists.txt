cmake_minimum_required(VERSION 3.31)

# Set project name and uppercase version
set(PROJECT_NAME "tasker")
set(PROJECT_NAME_UPPER "")

string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER) # For defines

set(root_dir "${CMAKE_CURRENT_SOURCE_DIR}")

project(${PROJECT_NAME} VERSION 0.1 LANGUAGES CXX)

# Build config
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Optimised" CACHE STRING "" FORCE)
set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_BUILD_PARALLEL_LEVEL 8)

# MSVC ONLY COMPILE OPTIONS
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_OPTIMISED "/O2 /Zi /DEBUG" CACHE STRING "Flags for Optimised configuration" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_OPTIMISED "/DEBUG /OPT:REF /OPT:ICF" CACHE STRING "Linker flags for Optimised configuration" FORCE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES Windows)
    add_definitions(-D${PROJECT_NAME_UPPER}_WINDOWS)
    set(platform "win")
elseif(CMAKE_SYSTEM_NAME MATCHES Linux)
    add_definitions(-D${PROJECT_NAME_UPPER}_LINUX)
    add_compile_options(-Wno-interference-size)
    set(platform "linux")
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
        src/extern
)

# CMake seems to complain on older versions if the variable is defined as an empty string,
# seeing the COMPARE below with only 4 arguments
SET(TSKR_BUILD_EXAMPLE "temp" CACHE STRING "example")
string(COMPARE GREATER ${TSKR_BUILD_EXAMPLE} "" example_defined)

# Build Examples
if(${TSKR_BUILD_EXAMPLES_ALL})
    # Library will propagate the CXX standart to the Examples projects as well
    target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

    file(GLOB EXAMPLE_SOURCES "examples/*.cpp")

    foreach(example ${EXAMPLE_SOURCES})
        get_filename_component(example_name ${example} NAME_WE)

        add_executable(${example_name} ${example})
        target_link_libraries(${example_name} PRIVATE ${PROJECT_NAME})
    endforeach()
elseif(example_defined)
    # Library will propagate the CXX standart to the Examples projects as well
    target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

    set(example_source "examples/${TSKR_BUILD_EXAMPLE}.cpp")

    message("Building example: ${example_source}")

    add_executable(${TSKR_BUILD_EXAMPLE} ${example_source})
    target_link_libraries(${TSKR_BUILD_EXAMPLE} PRIVATE ${PROJECT_NAME})
endif()
