set(PROJECT_HEADERS 
    include/ga/crossover.hpp
    include/ga/ga.hpp
    include/ga/common.hpp
    include/ga/mutation.hpp
)

set(PROJECT_SOURCES 
    src/ga.cpp
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DNDEBUG")

add_library(ga_lib STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})
target_compile_options(ga_lib PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
# target_link_libraries(ga_lib PUBLIC rapidjson)
target_include_directories(ga_lib 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> 
)