# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(IndirectAccessAnalyser VERSION 1.0)

# 指定 C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 指定二进制文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# 添加可执行文件
add_executable(IndirectAccessAnalyser IndirectAccessAnalyser.cpp)

find_package(Clang REQUIRED)
include_directories(${CLANG_INCLUDE_DIRS})
target_link_libraries(
  IndirectAccessAnalyser 
  ${CLANG_LIBRARIES}   
  clangAST
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
)