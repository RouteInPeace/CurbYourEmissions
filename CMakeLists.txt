cmake_minimum_required(VERSION 3.10)
project(cye VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DENABLE_DEBUG_MACRO")

find_package(Eigen3 REQUIRED NO_MODULE)

add_subdirectory("src")
add_subdirectory("lib/cye")
add_subdirectory("extern/rapidjson")
add_subdirectory("test")