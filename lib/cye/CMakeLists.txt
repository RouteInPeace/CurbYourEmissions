set(PROJECT_HEADERS 
    include/cye/instance.hpp
    include/cye/node.hpp
    include/cye/solution.hpp
    include/cye/init_heuristics.hpp
    include/cye/repair.hpp
    include/cye/destroy.hpp
    include/cye/patchable_vector.hpp
    include/cye/individual.hpp
    include/cye/operators.hpp
    include/cye/stall_handler.hpp
)

set(PROJECT_SOURCES 
    src/instance.cpp
    src/solution.cpp
    src/init_heuristics.cpp
    src/operators.cpp
    src/repair.cpp
    src/destroy.cpp
    src/individual.cpp
)


add_library(${PROJECT_NAME}_lib STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})
target_compile_options(${PROJECT_NAME}_lib PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(${PROJECT_NAME}_lib PRIVATE serial_lib meta_lib)
target_include_directories(${PROJECT_NAME}_lib 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/lib/serial/include 
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> 
)