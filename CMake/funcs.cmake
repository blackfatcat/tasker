Function(tskr_create_example)
    Set(options "")
    Set(oneValueArgs SRC_FILE)
    Set(multiValueArgs "")
    
    cmake_parse_arguments(EXAMPLE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    get_filename_component(example_name ${EXAMPLE_SRC_FILE} NAME_WE)
        
    message("Building example: ${example_name}...")

    # Library will propagate the CXX standart to the Examples projects as well
    target_compile_features(${TSKR_PROJECT_NAME} PUBLIC cxx_std_20)

    string(COMPARE EQUAL ${example_name} "dx3d_cmds" is_dx3d)

    # Build d3d example only on Windows
    if(${is_dx3d} AND (CMAKE_SYSTEM_NAME MATCHES Windows))

        # Try link and include d3d libs
        string(COMPARE GREATER $ENV{WindowsSdkDir} " " win_sdk_dir_defined)
        string(COMPARE GREATER $ENV{WindowsSDKVersion} " " win_sdk_ver_defined)
        if(${win_sdk_dir_defined} AND ${win_sdk_ver_defined})
            set(ARCH "x64")
            link_directories($ENV{WindowsSdkDir}/Lib/$ENV{WindowsSDKVersion}/um/${ARCH})
        
            add_executable(${example_name} ${EXAMPLE_SRC_FILE})
            set_target_properties(${example_name} PROPERTIES
                VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/examples"
            )
            target_link_libraries(${example_name} PRIVATE d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib)
            target_link_libraries(${example_name} PRIVATE ${TSKR_PROJECT_NAME})
            target_include_directories(${example_name}
                PUBLIC
                $ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/um
            )
            target_compile_definitions(${example_name} PRIVATE
                UNICODE _UNICODE
                NOMINMAX
                WIN32_LEAN_AND_MEAN
            )
        else()
            message(WARNING "d3d example not built! WindowsSdkDir and WindowsSDKVersion environment variables not present! WindowsSdkDir should be the directory pointing to the (Program Files (x86)/Windows Kits/10) or similar folder and the WindowsSDKVersion should be the latest one within the Include folder.")
        endif()
    elseif(NOT ${is_dx3d})
        
        # Just link tasker
        add_executable(${example_name} ${EXAMPLE_SRC_FILE})
        target_link_libraries(${example_name} PRIVATE ${TSKR_PROJECT_NAME})
        target_include_directories(${example_name}
            PUBLIC
                examples/extern
        )
    endif()

EndFunction()