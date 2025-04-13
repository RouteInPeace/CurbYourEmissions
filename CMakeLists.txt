cmake_minimum_required(VERSION 3.10)
project(cym VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DENABLE_DEBUG_MACRO")

add_subdirectory("src")
add_subdirectory("lib/cym")
add_subdirectory("extern/rapidjson")