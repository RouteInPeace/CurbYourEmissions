add_executable(${PROJECT_NAME}_exec main.cpp)
target_compile_options(${PROJECT_NAME}_exec PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(${PROJECT_NAME}_exec  PRIVATE ${PROJECT_NAME}_lib)
target_include_directories(${PROJECT_NAME}_exec PRIVATE ../lib/${PROJECT_NAME}/include)

add_executable(${PROJECT_NAME}_learn_exec learn.cpp)
target_compile_options(${PROJECT_NAME}_learn_exec PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(${PROJECT_NAME}_learn_exec  PRIVATE ${PROJECT_NAME}_lib)
target_include_directories(${PROJECT_NAME}_learn_exec PRIVATE ${CMAKE_SOURCE_DIR}/lib/${PROJECT_NAME}/include)