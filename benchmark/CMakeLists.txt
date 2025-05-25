include(FetchContent)

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.4
)

set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_MakeAvailable(benchmark)

add_executable(${PROJECT_NAME}_bench
  patchable_vector_bench.cpp
  alns_bench.cpp
  ga_bench.cpp
)

target_compile_options(${PROJECT_NAME}_bench PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(${PROJECT_NAME}_bench PRIVATE ${PROJECT_NAME}_lib benchmark::benchmark serial_lib meta_lib)