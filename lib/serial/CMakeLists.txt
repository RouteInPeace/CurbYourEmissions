set(PROJECT_HEADERS 
    include/json_archive.hpp
    include/utils.hpp
    include/exceptions.hpp
    include/archive.hpp
)

set(PROJECT_SOURCES 
    src/json_archive.cpp
    src/utils.cpp
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DNDEBUG")

add_library(serial_lib STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})
target_compile_options(serial_lib PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(serial_lib PUBLIC rapidjson)
target_include_directories(serial_lib 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> 
)