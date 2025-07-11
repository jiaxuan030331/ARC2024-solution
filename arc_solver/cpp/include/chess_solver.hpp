#pragma once

#include <vector>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;

class ChessSolverCpp {
public:
    ChessSolverCpp();
    
    // Main interface functions matching Python ChessSolver
    bool can_solve(const std::vector<py::array_t<int>>& train_inputs, 
                   const std::vector<py::array_t<int>>& train_outputs);
    
    std::vector<py::array_t<int>> solve(
        const std::vector<py::array_t<int>>& train_inputs,
        const std::vector<py::array_t<int>>& train_outputs,
        const std::vector<py::array_t<int>>& test_inputs
    );

private:
    // Core chess pattern detection functions
    bool has_chess_pattern(const py::array_t<int>& matrix);
    bool has_antichess_pattern(const py::array_t<int>& matrix);
    
    // Grid structure detection functions
    bool check_grid_structure(const std::vector<py::array_t<int>>& matrices);
    bool check_chess_patterns(const std::vector<py::array_t<int>>& matrices, 
                              bool check_input = false, bool check_output = true);
    
    // Color arrangement and pattern prediction
    std::optional<std::vector<int>> find_optimal_colors(const py::array_t<int>& matrix);
    std::vector<py::array_t<int>> predict_chess_patterns(const py::array_t<int>& input_matrix);
    
    // Grid filtering and utility functions
    py::array_t<int> apply_grid_filter(const py::array_t<int>& matrix);
    std::tuple<int, std::vector<int>, std::vector<int>> detect_grid(const py::array_t<int>& matrix);
    int get_mode_color(const py::array_t<int>& matrix);
    
    // Helper functions for chess pattern analysis
    std::vector<int> get_unique_colors(const py::array_t<int>& matrix);
    std::unordered_set<int> get_pattern_indices(const py::array_t<int>& matrix, 
                                                int color, int num_colors, 
                                                bool is_antichess = false);
    
    // Matrix manipulation utilities
    py::array_t<int> create_matrix_copy(const py::array_t<int>& source);
    py::array_t<int> create_chess_pattern(const py::array_t<int>& template_matrix, 
                                          const std::vector<int>& colors, 
                                          int offset = 0);
}; 