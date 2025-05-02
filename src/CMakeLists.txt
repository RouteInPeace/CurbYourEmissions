add_executable(${PROJECT_NAME}_exec main.cpp)
target_compile_options(${PROJECT_NAME}_exec PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(${PROJECT_NAME}_exec  PRIVATE ${PROJECT_NAME}_lib)
target_include_directories(${PROJECT_NAME}_exec PRIVATE ../lib/${PROJECT_NAME}/include)

add_executable(${PROJECT_NAME}_learn learn.cpp)
target_compile_options(${PROJECT_NAME}_learn PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(${PROJECT_NAME}_learn  PRIVATE ${PROJECT_NAME}_lib Eigen3::Eigen)
target_include_directories(${PROJECT_NAME}_learn PRIVATE ${CMAKE_SOURCE_DIR}/lib/${PROJECT_NAME}/include)