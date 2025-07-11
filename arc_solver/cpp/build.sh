#!/bin/bash

# Build script for ARC Solver C++ module

set -e  # Exit on any error

echo "Building ARC Solver C++ module..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the module
echo "Building..."
make -j$(nproc)

echo "Build completed successfully!"

# Optional: Install the module
# echo "Installing..."
# make install

# Go back to original directory
cd ..

echo "To install the Python module, run:"
echo "  pip install -e ." 