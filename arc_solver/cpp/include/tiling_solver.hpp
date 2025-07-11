#pragma once

#include <vector>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <functional>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;

class TilingSolverCpp {
public:
    TilingSolverCpp();
    
    // Main interface functions matching Python TilingSolver
    bool can_solve(const std::vector<py::array_t<int>>& train_inputs, 
                   const std::vector<py::array_t<int>>& train_outputs);
    
    std::vector<py::array_t<int>> solve(
        const std::vector<py::array_t<int>>& train_inputs,
        const std::vector<py::array_t<int>>& train_outputs,
        const std::vector<py::array_t<int>>& test_inputs
    );

private:
    // Core tiling pattern detection functions
    std::optional<py::array_t<int>> has_tiles(const py::array_t<int>& matrix, int ignore = 0);
    std::optional<py::array_t<int>> has_tiles_shape(const py::array_t<int>& matrix, 
                                                     const std::tuple<int, int>& shape, 
                                                     int ignore = 0);
    
    // Pattern prediction functions
    std::vector<py::array_t<int>> predict_tiles_shape(const std::vector<py::array_t<int>>& train_inputs,
                                                       const std::vector<py::array_t<int>>& train_outputs,
                                                       const py::array_t<int>& test_input);
    
    // Utility functions
    std::optional<std::tuple<int, int, int, int>> trim_matrix_box(const py::array_t<int>& matrix, 
                                                                   const std::vector<int>& mask);
    std::vector<int> get_unique_colors(const py::array_t<int>& matrix);
    
    // Matrix manipulation utilities
    py::array_t<int> create_matrix_copy(const py::array_t<int>& source);
    py::array_t<int> create_full_matrix(int rows, int cols, int fill_value);
    py::array_t<int> apply_tiling_pattern(const py::array_t<int>& pattern, 
                                          const py::array_t<int>& template_matrix,
                                          int offset_rows, int offset_cols);
    
    // Pattern matching and transformation
    bool patterns_match(const py::array_t<int>& pattern1, const py::array_t<int>& pattern2, 
                       const py::array_t<int>& mask);
    py::array_t<int> apply_transform(const py::array_t<int>& pattern, int transform_type);
    std::vector<std::function<py::array_t<int>(const py::array_t<int>&)>> get_all_transforms();
    
    // Helper functions for tiling detection
    bool check_tiling_validity(const py::array_t<int>& matrix, 
                               const py::array_t<int>& pattern,
                               int start_row, int start_col);
    std::vector<std::tuple<int, int>> find_tiling_positions(const py::array_t<int>& matrix,
                                                             const py::array_t<int>& pattern);
    
    // Color and pattern analysis
    std::vector<int> get_color_frequencies(const py::array_t<int>& matrix);
    bool is_valid_tiling_color(const py::array_t<int>& matrix, int color);
}; 