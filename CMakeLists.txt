cmake_minimum_required(VERSION 3.10)

project(Regex VERSION 1.0)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(Regex PUBLIC
    "$(PROJECT_BINARY_DIR)"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)