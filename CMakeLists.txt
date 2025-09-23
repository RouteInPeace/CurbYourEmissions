cmake_minimum_required(VERSION 3.10)

project(cye VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
option(ENABLE_SANITIZERS "Enable runtime sanitizers (ASan, UBSan, etc.)" OFF)

if(ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(SANITIZER_FLAGS 
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all
    )

    add_compile_options("$<$<CONFIG:Debug>:${SANITIZER_FLAGS}>")
    add_link_options("$<$<CONFIG:Debug>:${SANITIZER_FLAGS}>")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer")


add_subdirectory("src")
add_subdirectory("lib/cye")
add_subdirectory("lib/serial")
add_subdirectory("lib/meta")
add_subdirectory("extern/rapidjson")
add_subdirectory("test")
add_subdirectory("benchmark")

message("Hello")

if(BUILD_PYTHON_BINDINGS)
    message("Building python")
    add_custom_target(copy_python_module ALL
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:cye_module>
            "${CMAKE_SOURCE_DIR}/python/cye_module$<TARGET_FILE_SUFFIX:cye_module>"
        DEPENDS cye_module
        COMMENT "Copying Python module to python/ directory"
    )
endif()