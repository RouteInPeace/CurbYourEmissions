add_executable(${PROJECT_NAME}_exec main.cpp)
target_compile_options(${PROJECT_NAME}_exec PRIVATE -Wall -Wextra -Wpedantic -std=c++20)
target_link_libraries(${PROJECT_NAME}_exec  PRIVATE ${PROJECT_NAME}_lib)
target_include_directories(${PROJECT_NAME}_exec PRIVATE ../lib/${PROJECT_NAME}/include)