include(FetchContent)

# Fetch GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

add_executable(${PROJECT_NAME}_test
  solution_test.cpp
  heuristics_test.cpp
  serial_test.cpp
  ga_test.cpp
)

target_compile_options(${PROJECT_NAME}_test PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(${PROJECT_NAME}_test PRIVATE ${PROJECT_NAME}_lib GTest::gtest_main serial_lib ga_lib)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_test)