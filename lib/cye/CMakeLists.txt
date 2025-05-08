set(PROJECT_HEADERS 
    include/core/instance.hpp
    include/core/node.hpp
    include/core/utils.hpp
    include/core/solution.hpp

    include/heuristics/init_heuristics.hpp
    include/heuristics/destruction_nn.hpp
)

set(PROJECT_SOURCES 
    src/core/instance.cpp
    src/core/node.cpp
    src/core/utils.cpp
    src/core/solution.cpp

    src/heuristics/init_heuristics.cpp
    src/heuristics/destruction_nn.cpp
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DNDEBUG")

add_library(${PROJECT_NAME}_lib STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})
target_compile_options(${PROJECT_NAME}_lib PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(${PROJECT_NAME}_lib PRIVATE Eigen3::Eigen serial_lib)
target_include_directories(${PROJECT_NAME}_lib 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/lib/serial/include 
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> 
)