set(PROJECT_HEADERS 
    include/alns.hpp
    include/operator_selection.hpp
    include/acceptance_criterion.hpp
)

set(PROJECT_SOURCES 
    src/acceptance_criterion.cpp
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DNDEBUG")

add_library(alns_lib STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})
target_compile_options(alns_lib PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(alns_lib PUBLIC rapidjson)
target_include_directories(alns_lib 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> 
)