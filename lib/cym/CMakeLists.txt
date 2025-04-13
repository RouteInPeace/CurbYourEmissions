set(PROJECT_HEADERS 
    src/lib.hpp
)

set(PROJECT_SOURCES 
    src/lib.cpp
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DNDEBUG")

add_library(${PROJECT_NAME}_lib STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})
target_compile_options(${PROJECT_NAME}_lib PRIVATE -Wall -Wextra -Wpedantic -std=c++20)
target_link_libraries(${PROJECT_NAME}_lib rapidjson)
target_include_directories(${PROJECT_NAME}_lib  PRIVATE ../../external)