cmake_minimum_required(VERSION 3.12)
project(arc_solver_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(pybind11 REQUIRED)

# Set optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g")
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/symmetry_solver.cpp
    src/chess_solver.cpp
    src/tiling_solver.cpp
    src/ml_solver.cpp
    src/dag_solver.cpp
)

# Create pybind11 module
pybind11_add_module(arc_solver_cpp
    ${SOURCES}
    bindings/bindings.cpp
)

# Set properties
target_compile_definitions(arc_solver_cpp PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})
target_compile_features(arc_solver_cpp PRIVATE cxx_std_17)

# Enable compiler warnings
if(MSVC)
    target_compile_options(arc_solver_cpp PRIVATE /W4)
else()
    target_compile_options(arc_solver_cpp PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Link libraries (if needed)
# target_link_libraries(arc_solver_cpp PRIVATE some_library) 