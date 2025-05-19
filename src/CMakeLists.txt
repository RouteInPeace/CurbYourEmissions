add_executable(${PROJECT_NAME}_exec main.cpp)
target_compile_options(${PROJECT_NAME}_exec PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(${PROJECT_NAME}_exec  PRIVATE ${PROJECT_NAME}_lib serial_lib meta_lib alns_lib)
target_include_directories(${PROJECT_NAME}_exec 
  PRIVATE 
    ${CMAKE_SOURCE_DIR}/extern 
    ${CMAKE_SOURCE_DIR}/lib/${PROJECT_NAME}/include 
    ${CMAKE_SOURCE_DIR}/lib/serial/include
)

add_executable(${PROJECT_NAME}_learn learn.cpp)
target_compile_options(${PROJECT_NAME}_learn PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
target_link_libraries(${PROJECT_NAME}_learn  PRIVATE ${PROJECT_NAME}_lib Eigen3::Eigen serial_lib alns_lib)
target_include_directories(${PROJECT_NAME}_learn 
  PRIVATE 
    ${CMAKE_SOURCE_DIR}/extern
    ${CMAKE_SOURCE_DIR}/lib/${PROJECT_NAME}/include 
    ${CMAKE_SOURCE_DIR}/lib/serial/include
)