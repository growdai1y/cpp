cmake_minimum_required(VERSION 3.0...3.5)
project(cpp_features)

find_package(GTest REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::GTest GTest::Main)

set(CMAKE_CTEST_ARGUMENTS "--verbose")
enable_testing()
add_test(
  NAME ${PROJECT_NAME} 
  COMMAND ${PROJECT_NAME}
)
