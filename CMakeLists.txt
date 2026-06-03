cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 23)
if (LOCAL)
  set(CMAKE_CXX_COMPILER clang++-20)
  add_compile_options(-stdlib=libc++)
  add_link_options(-stdlib=libc++)
endif()

project(shell-cpp)
file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)
add_executable(shell ${SOURCE_FILES})
