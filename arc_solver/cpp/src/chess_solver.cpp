#include "../include/chess_solver.hpp"
#include <algorithm>
#include <unordered_map>
#include <set>
#include <iostream>
#include <cstring>

ChessSolverCpp::ChessSolverCpp() {
    // Constructor
}

bool ChessSolverCpp::can_solve(const std::vector<py::array_t<int>>& train_inputs, 
                               const std::vector<py::array_t<int>>& train_outputs) {
    // Check if task involves chess patterns
    if (!check_grid_structure(train_inputs)) {
        return false;
    }
    
    return check_chess_patterns(train_outputs, false, true);
}

std::vector<py::array_t<int>> ChessSolverCpp::solve(
    const std::vector<py::array_t<int>>& train_inputs,
    const std::vector<py::array_t<int>>& train_outputs,
    const std::vector<py::array_t<int>>& test_inputs) {
    
    if (!can_solve(train_inputs, train_outputs)) {
        return {};
    }
    
    std::vector<py::array_t<int>> candidates;
    
    for (const auto& test_input : test_inputs) {
        // Apply grid filter and predict chess patterns
        auto filtered_input = apply_grid_filter(test_input);
        auto chess_candidates = predict_chess_patterns(filtered_input);
        candidates.insert(candidates.end(), chess_candidates.begin(), chess_candidates.end());
    }
    
    return candidates;
}

bool ChessSolverCpp::has_chess_pattern(const py::array_t<int>& matrix) {
    auto colors = get_unique_colors(matrix);
    int counts = colors.size();
    
    if (counts < 2) {
        return false;
    }
    
    std::vector<bool> indexes(counts, false);
    
    for (int c : colors) {
        auto pattern_indices = get_pattern_indices(matrix, c, counts, false);
        
        if (pattern_indices.size() > 1) {
            return false;
        }
        
        if (pattern_indices.empty()) {
            continue;
        }
        
        int index = *pattern_indices.begin();
        if (indexes[index]) {
            return false;
        }
        indexes[index] = true;
    }
    
    return true;
}

bool ChessSolverCpp::has_antichess_pattern(const py::array_t<int>& matrix) {
    auto colors = get_unique_colors(matrix);
    int counts = colors.size();
    
    if (counts < 2) {
        return false;
    }
    
    std::vector<bool> indexes(counts, false);
    
    for (int c : colors) {
        auto pattern_indices = get_pattern_indices(matrix, c, counts, true);
        
        if (pattern_indices.size() > 1) {
            return false;
        }
        
        if (pattern_indices.empty()) {
            continue;
        }
        
        int index = *pattern_indices.begin();
        if (indexes[index]) {
            return false;
        }
        indexes[index] = true;
    }
    
    return true;
}

bool ChessSolverCpp::check_grid_structure(const std::vector<py::array_t<int>>& matrices) {
    for (const auto& matrix : matrices) {
        auto [grid_color, cols, rows] = detect_grid(matrix);
        if (grid_color != -1) {
            return true;
        }
    }
    return false;
}

bool ChessSolverCpp::check_chess_patterns(const std::vector<py::array_t<int>>& matrices, 
                                          bool check_input, bool check_output) {
    for (const auto& matrix : matrices) {
        if (check_input || check_output) {
            if (!has_chess_pattern(matrix) && !has_antichess_pattern(matrix)) {
                return false;
            }
        }
    }
    return true;
}

std::optional<std::vector<int>> ChessSolverCpp::find_optimal_colors(const py::array_t<int>& matrix) {
    auto colors = get_unique_colors(matrix);
    int total_colors = colors.size();
    
    for (int cnt = total_colors; cnt >= 2; cnt--) {
        std::vector<int> q_colors(cnt, -1);
        
        for (int c : colors) {
            auto pattern_indices = get_pattern_indices(matrix, c, cnt, false);
            
            if (pattern_indices.size() > 1) {
                continue;
            }
            
            if (pattern_indices.empty()) {
                continue;
            }
            
            int index = *pattern_indices.begin();
            q_colors[index] = c;
        }
        
        // Check if all positions are filled
        bool all_filled = true;
        for (int color : q_colors) {
            if (color == -1) {
                all_filled = false;
                break;
            }
        }
        
        if (all_filled) {
            return q_colors;
        }
    }
    
    return std::nullopt;
}

std::vector<py::array_t<int>> ChessSolverCpp::predict_chess_patterns(const py::array_t<int>& input_matrix) {
    auto q_colors_opt = find_optimal_colors(input_matrix);
    std::vector<int> q_colors;
    
    if (!q_colors_opt.has_value()) {
        // Fallback: use most frequent colors
        auto colors = get_unique_colors(input_matrix);
        std::vector<std::pair<int, int>> color_counts;
        
        for (int color : colors) {
            auto buf = input_matrix.request();
            int* ptr = static_cast<int*>(buf.ptr);
            int count = 0;
            
            for (py::ssize_t i = 0; i < buf.size; i++) {
                if (ptr[i] == color) {
                    count++;
                }
            }
            color_counts.push_back({count, color});
        }
        
        std::sort(color_counts.begin(), color_counts.end());
        q_colors = {color_counts[0].second, color_counts[1].second};
    } else {
        q_colors = q_colors_opt.value();
    }
    
    std::vector<py::array_t<int>> results;
    int counts = q_colors.size();
    
    for (int i = 0; i < counts; i++) {
        auto result = create_chess_pattern(input_matrix, q_colors, i);
        results.push_back(result);
        
        // Rotate colors for next pattern
        std::rotate(q_colors.begin(), q_colors.begin() + 1, q_colors.end());
    }
    
    return results;
}

py::array_t<int> ChessSolverCpp::apply_grid_filter(const py::array_t<int>& matrix) {
    auto [grid_color, cols, rows] = detect_grid(matrix);
    
    if (grid_color == -1) {
        return create_matrix_copy(matrix);
    }
    
    // Simplified grid filtering - for full implementation, would need get_cells equivalent
    // For now, return copy of original matrix
    return create_matrix_copy(matrix);
}

std::tuple<int, std::vector<int>, std::vector<int>> ChessSolverCpp::detect_grid(const py::array_t<int>& matrix) {
    auto buf = matrix.request();
    int* ptr = static_cast<int*>(buf.ptr);
    int rows = buf.shape[0];
    int cols = buf.shape[1];
    
    if (rows < 3 || cols < 3) {
        return {-1, {}, {}};
    }
    
    std::unordered_map<int, std::vector<int>> row_lines;
    std::unordered_map<int, std::vector<int>> col_lines;
    
    // Check for uniform rows
    for (int i = 0; i < rows; i++) {
        int first_val = ptr[i * cols];
        bool uniform = true;
        
        for (int j = 1; j < cols; j++) {
            if (ptr[i * cols + j] != first_val) {
                uniform = false;
                break;
            }
        }
        
        if (uniform) {
            row_lines[first_val].push_back(i);
        }
    }
    
    // Check for uniform columns
    for (int j = 0; j < cols; j++) {
        int first_val = ptr[j];
        bool uniform = true;
        
        for (int i = 1; i < rows; i++) {
            if (ptr[i * cols + j] != first_val) {
                uniform = false;
                break;
            }
        }
        
        if (uniform) {
            col_lines[first_val].push_back(j);
        }
    }
    
    // Find grid color that appears in both rows and columns
    for (const auto& [color, row_indices] : row_lines) {
        if (col_lines.find(color) != col_lines.end()) {
            const auto& col_indices = col_lines[color];
            
            // Check if spacing is consistent (> 1)
            bool valid_row_spacing = true;
            for (size_t i = 1; i < row_indices.size(); i++) {
                if (row_indices[i] - row_indices[i-1] <= 1) {
                    valid_row_spacing = false;
                    break;
                }
            }
            
            bool valid_col_spacing = true;
            for (size_t i = 1; i < col_indices.size(); i++) {
                if (col_indices[i] - col_indices[i-1] <= 1) {
                    valid_col_spacing = false;
                    break;
                }
            }
            
            if (valid_row_spacing && valid_col_spacing) {
                return {color, col_indices, row_indices};
            }
        }
    }
    
    return {-1, {}, {}};
}

int ChessSolverCpp::get_mode_color(const py::array_t<int>& matrix) {
    std::unordered_map<int, int> color_counts;
    auto buf = matrix.request();
    int* ptr = static_cast<int*>(buf.ptr);
    
    for (py::ssize_t i = 0; i < buf.size; i++) {
        color_counts[ptr[i]]++;
    }
    
    int mode_color = 0;
    int max_count = 0;
    
    for (const auto& [color, count] : color_counts) {
        if (count > max_count) {
            max_count = count;
            mode_color = color;
        }
    }
    
    return mode_color;
}

std::vector<int> ChessSolverCpp::get_unique_colors(const py::array_t<int>& matrix) {
    std::set<int> unique_set;
    auto buf = matrix.request();
    int* ptr = static_cast<int*>(buf.ptr);
    
    for (py::ssize_t i = 0; i < buf.size; i++) {
        unique_set.insert(ptr[i]);
    }
    
    return std::vector<int>(unique_set.begin(), unique_set.end());
}

std::unordered_set<int> ChessSolverCpp::get_pattern_indices(const py::array_t<int>& matrix, 
                                                            int color, int num_colors, 
                                                            bool is_antichess) {
    std::unordered_set<int> indices;
    auto buf = matrix.request();
    int* ptr = static_cast<int*>(buf.ptr);
    int rows = buf.shape[0];
    int cols = buf.shape[1];
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (ptr[i * cols + j] == color) {
                int index;
                if (is_antichess) {
                    index = (rows - i + j - 1) % num_colors;
                } else {
                    index = (i + j) % num_colors;
                }
                indices.insert(index);
            }
        }
    }
    
    return indices;
}

py::array_t<int> ChessSolverCpp::create_matrix_copy(const py::array_t<int>& source) {
    auto buf = source.request();
    
    auto result = py::array_t<int>(
        {buf.shape[0], buf.shape[1]},
        {sizeof(int) * buf.shape[1], sizeof(int)}
    );
    
    auto result_buf = result.request();
    std::memcpy(result_buf.ptr, buf.ptr, buf.size * sizeof(int));
    
    return result;
}

py::array_t<int> ChessSolverCpp::create_chess_pattern(const py::array_t<int>& template_matrix, 
                                                      const std::vector<int>& colors, 
                                                      int offset) {
    auto buf = template_matrix.request();
    int rows = buf.shape[0];
    int cols = buf.shape[1];
    
    auto result = py::array_t<int>(
        {rows, cols},
        {sizeof(int) * cols, sizeof(int)}
    );
    
    auto result_buf = result.request();
    int* result_ptr = static_cast<int*>(result_buf.ptr);
    int num_colors = colors.size();
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int pattern_index = (i + j + offset) % num_colors;
            result_ptr[i * cols + j] = colors[pattern_index];
        }
    }
    
    return result;
} 