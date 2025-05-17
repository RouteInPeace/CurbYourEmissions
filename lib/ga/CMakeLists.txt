set(PROJECT_HEADERS 
    include/ga/crossover.hpp
    include/ga/ga.hpp
    include/ga/common.hpp
    include/ga/mutation.hpp
)

add_library(ga_lib INTERFACE)

target_compile_features(ga_lib INTERFACE cxx_std_23)

target_compile_options(ga_lib INTERFACE -Wall -Wextra -Wpedantic)

target_include_directories(ga_lib 
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
