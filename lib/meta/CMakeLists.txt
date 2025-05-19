set(PROJECT_HEADERS 
    include/meta/crossover.hpp
    include/meta/ga.hpp
    include/meta/common.hpp
    include/meta/mutation.hpp
    include/meta/selection.hpp
    include/meta/simulated_annealing.hpp
)

add_library(meta_lib INTERFACE)

target_compile_features(meta_lib INTERFACE cxx_std_23)

target_compile_options(meta_lib INTERFACE -Wall -Wextra -Wpedantic)

target_include_directories(meta_lib 
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
