# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)


# Project name 
project(GeodesicVisualizer)





# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# executable target 
add_executable(${PROJECT_NAME} main.cpp)

# Add the path to the Eigen headers 
target_include_directories(${PROJECT_NAME} PRIVATE ../eigen)
