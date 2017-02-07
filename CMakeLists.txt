cmake_minimum_required(VERSION 2.7)
project(nBody)

# Require MPI for this project:
find_package(MPI REQUIRED)

add_executable(nBody main.cpp)
include_directories(${MPI_INCLUDE_PATH})
target_link_libraries(nBody ${MPI_LIBRARIES})

set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})