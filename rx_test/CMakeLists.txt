cmake_minimum_required(VERSION 3.0...3.5)
project(rx_test)

find_package(GTest REQUIRED)

include_directories(Rx/v2/src)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::GTest GTest::Main)

enable_testing()
add_test(
  NAME ${PROJECT_NAME} 
  COMMAND ${PROJECT_NAME}
)
