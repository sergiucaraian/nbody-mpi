cmake_minimum_required(VERSION 3.6)
project(nBody)

# Project files
include_directories(common)
set(SOURCE_FILES main.cpp common/shader.cpp common/shader.hpp Particle.cpp Particle.h common.h common.cpp shaders/VertexShader.vs.glsl shaders/FragmentShader.fs.glsl Cell.cpp Cell.h SerializedCell.cpp SerializedCell.h)
add_executable(nBody ${SOURCE_FILES})

# Boost
find_package(Boost COMPONENTS program_options mpi serialization REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(nBody ${Boost_LIBRARIES})

# Require MPI for this project:
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
target_link_libraries(nBody ${MPI_LIBRARIES})

# OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
target_link_libraries(nBody ${OPENGL_LIBRARIES})

# GLEW
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
target_link_libraries(nBody ${GLEW_LIBRARIES})

# GLM
include_directories(${GLM_INCLUDE_DIRS})
target_link_libraries(nBody ${GLM_LIBRARIES})

# GLFW
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
include_directories(${GLFW_INCLUDE_DIRS})
target_link_libraries(nBody ${GLFW_LIBRARIES})

# Project config
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
