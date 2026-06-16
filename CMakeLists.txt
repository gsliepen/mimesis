cmake_minimum_required(VERSION 3.10.0)
project(libmimesis VERSION 0.1.0 LANGUAGES C CXX)

file(GLOB mimesis_src
"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(libmimesis ${mimesis_src})
add_executable(parse_test  ${mimesis_src} ${CMAKE_CURRENT_SOURCE_DIR}/test/load-parse-test.cpp)
target_include_directories(parse_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/")
