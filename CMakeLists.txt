cmake_minimum_required(VERSION 3.10)

# project name 
project(OdeExamples)


# enable c++ 17 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# Find Boost 
find_package(Boost REQUIRED)



# --- Target 1: Lorenz Attractor ---
# executable target 
add_executable(LorenzDemo main.cpp)

# Add path to locally-cloned Eigen
target_include_directories(LorenzDemo PRIVATE ../eigen)
# add path to the Boost headers (modern way to add Boost)
# target_include_directories(LorenzDemo PRIVATE Boost::boost)
target_link_libraries(LorenzDemo PRIVATE Boost::boost)




# --- Target 2: Simple ODE ---
# Creates an executable named "SimpleODEDemo" from "simple_ode.cpp"
add_executable(SimpleODEDemo simple_ode.cpp)

# Link SimpleODEDemo to Boost (it doesn't need Eigen)
target_link_libraries(SimpleODEDemo PRIVATE Boost::boost) # <-- Fixed: Use target_link_libraries for Boost