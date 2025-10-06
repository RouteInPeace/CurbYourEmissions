set(PROJECT_HEADERS 
    include/caliper/caliper.hpp
)

set(PROJECT_SOURCES 
    src/caliper.cpp
)


add_library(caliper_lib STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})
target_compile_options(caliper_lib PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(caliper_lib PRIVATE serial_lib)
target_include_directories(caliper_lib
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
            # ${CMAKE_SOURCE_DIR}/lib/serial/include 
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> 
)