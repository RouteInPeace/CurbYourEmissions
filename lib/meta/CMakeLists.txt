set(PROJECT_HEADERS 
    include/meta/common.hpp

    include/meta/ga/crossover.hpp
    include/meta/ga/ga.hpp
    include/meta/ga/common.hpp
    include/meta/ga/mutation.hpp
    include/meta/ga/selection.hpp

    include/meta/sa/simulated_annealing.hpp

    include/meta/alns/alns.hpp
    include/meta/alns/operator_selection.hpp
    include/meta/alns/acceptance_criterion.hpp
)

set(PROJECT_SOURCES 
    src/alns/acceptance_criterion.cpp
)

add_library(meta_lib STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS})
target_compile_options(meta_lib PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(meta_lib PUBLIC rapidjson)
target_include_directories(meta_lib 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> 
)