# Find Python first
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# Try to find pybind11, if not found, fetch it
find_package(pybind11 QUIET)

if(NOT pybind11_FOUND)
    message(STATUS "pybind11 not found, fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.11.1
    )
    FetchContent_MakeAvailable(pybind11)
endif()

pybind11_add_module(cye_module main.cpp)
target_compile_options(cye_module PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(cye_module PRIVATE ${PROJECT_NAME}_lib serial_lib meta_lib)
target_include_directories(cye_module
  PRIVATE 
    ${CMAKE_SOURCE_DIR}/extern 
    ${CMAKE_SOURCE_DIR}/lib/${PROJECT_NAME}/include 
    ${CMAKE_SOURCE_DIR}/lib/serial/include
)

