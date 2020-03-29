project(landau-poisson-solver)
cmake_minimum_required(VERSION 3.5)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-modules")

add_subdirectory(source)
add_subdirectory(tests)