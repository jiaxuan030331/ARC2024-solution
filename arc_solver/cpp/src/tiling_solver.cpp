#include "../include/tiling_solver.hpp"
#include <algorithm>
#include <unordered_map>
#include <set>
#include <iostream>
#include <cstring>
#include <cmath>

// Hash function for std::tuple<int, int>
namespace std {
    template <>
    struct hash<std::tuple<int, int>> {
        size_t operator()(const std::tuple<int, int>& t) const {
            auto h1 = std::hash<int>{}(std::get<0>(t));
            auto h2 = std::hash<int>{}(std::get<1>(t));
            return h1 ^ (h2 << 1);
        }
    };
}

TilingSolverCpp::TilingSolverCpp() {
    // Constructor
}

bool TilingSolverCpp::can_solve(const std::vector<py::array_t<int>>& train_inputs, 
                                const std::vector<py::array_t<int>>& train_outputs) {
    // Check if task involves tiling patterns
    for (size_t i = 0; i < train_inputs.size(); i++) {
        const auto& x = train_inputs[i];
        const auto& y = train_outputs[i];
        
        auto o_pattern = has_tiles(y, -1);
        if (!o_pattern.has_value()) {
            return false;
        }
        
        auto colors = get_unique_colors(x);
        if (colors.size() < 2) {
            return false;
        }
        
        bool found = false;
        std::vector<int> check_colors = {-1};
        check_colors.insert(check_colors.end(), colors.begin(), colors.end());
        
        for (int c : check_colors) {
            auto pattern = has_tiles_shape(x, {o_pattern->shape(0), o_pattern->shape(1)}, c);
            if (pattern.has_value()) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            return false;
        }
    }
    
    return true;
}

std::vector<py::array_t<int>> TilingSolverCpp::solve(
    const std::vector<py::array_t<int>>& train_inputs,
    const std::vector<py::array_t<int>>& train_outputs,
    const std::vector<py::array_t<int>>& test_inputs) {
    
    if (!can_solve(train_inputs, train_outputs)) {
        return {};
    }
    
    std::vector<py::array_t<int>> candidates;
    
    for (const auto& test_input : test_inputs) {
        auto tiling_candidates = predict_tiles_shape(train_inputs, train_outputs, test_input);
        candidates.insert(candidates.end(), tiling_candidates.begin(), tiling_candidates.end());
    }
    
    return candidates;
}

std::optional<py::array_t<int>> TilingSolverCpp::has_tiles(const py::array_t<int>& matrix, int ignore) {
    auto buf = matrix.request();
    int* ptr = static_cast<int*>(buf.ptr);
    int rows = buf.shape[0];
    int cols = buf.shape[1];
    
    // Try different size combinations
    std::vector<std::pair<int, int>> size_combinations = {
        {rows, static_cast<int>(0.6 * cols)},
        {static_cast<int>(0.6 * rows), cols}
    };
    
    for (const auto& [size0b, size1b] : size_combinations) {
        // Create extended matrix
        auto t = create_full_matrix(rows + size0b, cols + size1b, -1);
        auto t_buf = t.request();
        int* t_ptr = static_cast<int*>(t_buf.ptr);
        int t_rows = t_buf.shape[0];
        int t_cols = t_buf.shape[1];
        
        // Copy original matrix
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                t_ptr[i * t_cols + j] = ptr[i * cols + j];
            }
        }
        
        // Set ignored values to -1
        for (int i = 0; i < t_rows * t_cols; i++) {
            if (t_ptr[i] == ignore) {
                t_ptr[i] = -1;
            }
        }
        
        // Get bounding box
        auto box_trim = trim_matrix_box(matrix, {ignore});
        int min_size0 = 1;
        int min_size1 = 1;
        
        if (box_trim.has_value() && ignore != -1) {
            auto [xmin, ymin, xmax, ymax] = box_trim.value();
            for (int i = xmin; i < xmax; i++) {
                for (int j = ymin; j < ymax; j++) {
                    t_ptr[i * t_cols + j] = ptr[i * cols + j];
                }
            }
            min_size0 = xmax - xmin;
            min_size1 = ymax - ymin;
        }
        
        // Use min_size0 and min_size1 to avoid unused variable warnings
        (void)min_size0;
        (void)min_size1;
        
        // Try different pattern sizes
        for (int size0 = min_size0; size0 <= size0b; size0++) {
            for (int size1 = min_size1; size1 <= size1b; size1++) {
                auto pattern = create_matrix_copy(t);
                auto pattern_buf = pattern.request();
                int* pattern_ptr = static_cast<int*>(pattern_buf.ptr);
                
                // Extract pattern
                for (int i = 0; i < size0; i++) {
                    for (int j = 0; j < size1; j++) {
                        pattern_ptr[i * size1 + j] = t_ptr[i * t_cols + j];
                    }
                }
                
                bool found = true;
                // Check if pattern tiles correctly
                for (int d0 = 0; d0 <= t_rows - size0; d0 += size0) {
                    for (int d1 = 0; d1 <= t_cols - size1; d1 += size1) {
                        // Create test pattern
                        auto test_pattern = create_full_matrix(size0, size1, -1);
                        auto test_buf = test_pattern.request();
                        int* test_ptr = static_cast<int*>(test_buf.ptr);
                        
                        for (int i = 0; i < size0; i++) {
                            for (int j = 0; j < size1; j++) {
                                test_ptr[i * size1 + j] = t_ptr[(d0 + i) * t_cols + (d1 + j)];
                            }
                        }
                        
                        // Check if patterns match - only compare non-ignored values
                        bool patterns_match = true;
                        for (int i = 0; i < size0; i++) {
                            for (int j = 0; j < size1; j++) {
                                int pattern_val = pattern_ptr[i * size1 + j];
                                int test_val = test_ptr[i * size1 + j];
                                
                                // Only compare if both values are not ignored
                                if (pattern_val != -1 && test_val != -1) {
                                    if (pattern_val != test_val) {
                                        patterns_match = false;
                                        break;
                                    }
                                }
                            }
                            if (!patterns_match) break;
                        }
                        
                        if (!patterns_match) {
                            found = false;
                            break;
                        }
                    }
                    if (!found) break;
                }
                
                if (found) {
                    return pattern;
                }
            }
        }
    }
    
    return std::nullopt;
}

std::optional<py::array_t<int>> TilingSolverCpp::has_tiles_shape(const py::array_t<int>& matrix, 
                                                                  const std::tuple<int, int>& shape, 
                                                                  int ignore) {
    auto buf = matrix.request();
    int* ptr = static_cast<int*>(buf.ptr);
    int rows = buf.shape[0];
    int cols = buf.shape[1];
    int size0 = std::get<0>(shape);
    int size1 = std::get<1>(shape);
    
    // Try different size combinations
    std::vector<std::pair<int, int>> size_combinations = {
        {rows, static_cast<int>(0.6 * cols)},
        {static_cast<int>(0.6 * rows), cols}
    };
    
    for (const auto& [size0b, size1b] : size_combinations) {
        // Create extended matrix
        auto t = create_full_matrix(rows + size0b, cols + size1b, -1);
        auto t_buf = t.request();
        int* t_ptr = static_cast<int*>(t_buf.ptr);
        int t_rows = t_buf.shape[0];
        int t_cols = t_buf.shape[1];
        
        // Copy original matrix
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                t_ptr[i * t_cols + j] = ptr[i * cols + j];
            }
        }
        
        // Set ignored values to -1
        for (int i = 0; i < t_rows * t_cols; i++) {
            if (t_ptr[i] == ignore) {
                t_ptr[i] = -1;
            }
        }
        
        // Get bounding box
        auto box_trim = trim_matrix_box(matrix, {ignore});
        int min_size0 = 1;
        int min_size1 = 1;
        
        if (box_trim.has_value() && ignore != -1) {
            auto [xmin, ymin, xmax, ymax] = box_trim.value();
            for (int i = xmin; i < xmax; i++) {
                for (int j = ymin; j < ymax; j++) {
                    t_ptr[i * t_cols + j] = ptr[i * cols + j];
                }
            }
            min_size0 = xmax - xmin;
            min_size1 = ymax - ymin;
        }
        
        // Extract pattern with specific shape
        auto pattern = create_full_matrix(size0, size1, -1);
        auto pattern_buf = pattern.request();
        int* pattern_ptr = static_cast<int*>(pattern_buf.ptr);
        
        for (int i = 0; i < size0; i++) {
            for (int j = 0; j < size1; j++) {
                pattern_ptr[i * size1 + j] = t_ptr[i * t_cols + j];
            }
        }
        
        bool found = true;
        // Check if pattern tiles correctly
        for (int d0 = 0; d0 <= t_rows - size0; d0 += size0) {
            for (int d1 = 0; d1 <= t_cols - size1; d1 += size1) {
                // Create test pattern
                auto test_pattern = create_full_matrix(size0, size1, -1);
                auto test_buf = test_pattern.request();
                int* test_ptr = static_cast<int*>(test_buf.ptr);
                
                for (int i = 0; i < size0; i++) {
                    for (int j = 0; j < size1; j++) {
                        test_ptr[i * size1 + j] = t_ptr[(d0 + i) * t_cols + (d1 + j)];
                    }
                }
                
                                        // Check if patterns match - only compare non-ignored values
                        bool patterns_match = true;
                        for (int i = 0; i < size0; i++) {
                            for (int j = 0; j < size1; j++) {
                                int pattern_val = pattern_ptr[i * size1 + j];
                                int test_val = test_ptr[i * size1 + j];
                                
                                // Only compare if both values are not ignored
                                if (pattern_val != -1 && test_val != -1) {
                                    if (pattern_val != test_val) {
                                        patterns_match = false;
                                        break;
                                    }
                                }
                            }
                            if (!patterns_match) break;
                        }
                        
                        if (!patterns_match) {
                            found = false;
                            break;
                        }
            }
            if (!found) break;
        }
        
        if (found) {
            return pattern;
        }
    }
    
    return std::nullopt;
}

std::vector<py::array_t<int>> TilingSolverCpp::predict_tiles_shape(
    const std::vector<py::array_t<int>>& train_inputs,
    const std::vector<py::array_t<int>>& train_outputs,
    const py::array_t<int>& test_input) {
    
    std::unordered_set<int> has_transforms;
    std::unordered_set<std::tuple<int, int>> has_shapes;
    
    // Analyze training examples
    for (size_t i = 0; i < train_inputs.size(); i++) {
        const auto& x = train_inputs[i];
        const auto& y = train_outputs[i];
        
        auto o_pattern = has_tiles(y, -1);
        if (!o_pattern.has_value()) {
            return {};
        }
        
        auto colors = get_unique_colors(x);
        if (colors.size() < 2) {
            return {};
        }
        
        bool found = false;
        std::vector<int> check_colors = {-1};
        check_colors.insert(check_colors.end(), colors.begin(), colors.end());
        
        for (int c : check_colors) {
            auto pattern = has_tiles_shape(x, {o_pattern->shape(0), o_pattern->shape(1)}, c);
            if (pattern.has_value()) {
                // Apply color to pattern
                auto pattern_buf = pattern->request();
                int* pattern_ptr = static_cast<int*>(pattern_buf.ptr);
                for (int j = 0; j < pattern_buf.size; j++) {
                    if (pattern_ptr[j] == -1) {
                        pattern_ptr[j] = c;
                    }
                }
                
                // Test transformations
                auto transforms = get_all_transforms();
                for (size_t t_idx = 0; t_idx < transforms.size(); t_idx++) {
                    auto transformed_pattern = transforms[t_idx](*pattern);
                    
                    // Create prediction
                    int pred_rows = x.shape(0) / transformed_pattern.shape(0) + 2;
                    int pred_cols = x.shape(1) / transformed_pattern.shape(1) + 2;
                    auto pred = create_full_matrix(pred_rows * transformed_pattern.shape(0), 
                                                 pred_cols * transformed_pattern.shape(1), 0);
                    
                    // Fill prediction with tiled pattern
                    auto pred_buf = pred.request();
                    int* pred_ptr = static_cast<int*>(pred_buf.ptr);
                    auto trans_buf = transformed_pattern.request();
                    int* trans_ptr = static_cast<int*>(trans_buf.ptr);
                    
                    for (int ti = 0; ti < pred_rows; ti++) {
                        for (int tj = 0; tj < pred_cols; tj++) {
                            for (int pi = 0; pi < transformed_pattern.shape(0); pi++) {
                                for (int pj = 0; pj < transformed_pattern.shape(1); pj++) {
                                    int pred_i = ti * transformed_pattern.shape(0) + pi;
                                    int pred_j = tj * transformed_pattern.shape(1) + pj;
                                    int trans_i = pi;
                                    int trans_j = pj;
                                    
                                    if (pred_i < pred.shape(0) && pred_j < pred.shape(1)) {
                                        pred_ptr[pred_i * pred.shape(1) + pred_j] = 
                                            trans_ptr[trans_i * transformed_pattern.shape(1) + trans_j];
                                    }
                                }
                            }
                        }
                    }
                    
                    // Trim to original size
                    auto trimmed_pred = create_full_matrix(x.shape(0), x.shape(1), 0);
                    auto trimmed_buf = trimmed_pred.request();
                    int* trimmed_ptr = static_cast<int*>(trimmed_buf.ptr);
                    
                    for (int i = 0; i < x.shape(0); i++) {
                        for (int j = 0; j < x.shape(1); j++) {
                            if (i < pred.shape(0) && j < pred.shape(1)) {
                                trimmed_ptr[i * x.shape(1) + j] = pred_ptr[i * pred.shape(1) + j];
                            }
                        }
                    }
                    
                    // Check if prediction matches output
                    if (patterns_match(trimmed_pred, y, create_full_matrix(x.shape(0), x.shape(1), 1))) {
                        found = true;
                        has_transforms.insert(t_idx);
                        has_shapes.insert({o_pattern->shape(0), o_pattern->shape(1)});
                        break;
                    }
                }
                
                if (found) break;
            }
        }
        
        if (!found) {
            return {};
        }
    }
    
    // Generate predictions for test input
    std::vector<py::array_t<int>> preds;
    auto test_colors = get_unique_colors(test_input);
    
    for (int c : test_colors) {
        for (const auto& shape : has_shapes) {
            auto pattern = has_tiles_shape(test_input, shape, c);
            if (!pattern.has_value()) {
                continue;
            }
            
            // Apply color to pattern
            auto pattern_buf = pattern->request();
            int* pattern_ptr = static_cast<int*>(pattern_buf.ptr);
            for (int j = 0; j < pattern_buf.size; j++) {
                if (pattern_ptr[j] == -1) {
                    pattern_ptr[j] = c;
                }
            }
            
            // Apply transformations
            auto transforms = get_all_transforms();
            for (int transform_idx : has_transforms) {
                if (transform_idx < static_cast<int>(transforms.size())) {
                    auto transformed_pattern = transforms[transform_idx](*pattern);
                    
                    // Create prediction
                    int pred_rows = test_input.shape(0) / transformed_pattern.shape(0) + 2;
                    int pred_cols = test_input.shape(1) / transformed_pattern.shape(1) + 2;
                    auto pred = create_full_matrix(pred_rows * transformed_pattern.shape(0), 
                                                 pred_cols * transformed_pattern.shape(1), 0);
                    
                    // Fill prediction with tiled pattern
                    auto pred_buf = pred.request();
                    int* pred_ptr = static_cast<int*>(pred_buf.ptr);
                    auto trans_buf = transformed_pattern.request();
                    int* trans_ptr = static_cast<int*>(trans_buf.ptr);
                    
                    for (int ti = 0; ti < pred_rows; ti++) {
                        for (int tj = 0; tj < pred_cols; tj++) {
                            for (int pi = 0; pi < transformed_pattern.shape(0); pi++) {
                                for (int pj = 0; pj < transformed_pattern.shape(1); pj++) {
                                    int pred_i = ti * transformed_pattern.shape(0) + pi;
                                    int pred_j = tj * transformed_pattern.shape(1) + pj;
                                    int trans_i = pi;
                                    int trans_j = pj;
                                    
                                    if (pred_i < pred.shape(0) && pred_j < pred.shape(1)) {
                                        pred_ptr[pred_i * pred.shape(1) + pred_j] = 
                                            trans_ptr[trans_i * transformed_pattern.shape(1) + trans_j];
                                    }
                                }
                            }
                        }
                    }
                    
                    // Trim to test input size
                    auto trimmed_pred = create_full_matrix(test_input.shape(0), test_input.shape(1), 0);
                    auto trimmed_buf = trimmed_pred.request();
                    int* trimmed_ptr = static_cast<int*>(trimmed_buf.ptr);
                    
                    for (int i = 0; i < test_input.shape(0); i++) {
                        for (int j = 0; j < test_input.shape(1); j++) {
                            if (i < pred.shape(0) && j < pred.shape(1)) {
                                trimmed_ptr[i * test_input.shape(1) + j] = pred_ptr[i * pred.shape(1) + j];
                            }
                        }
                    }
                    
                    preds.push_back(trimmed_pred);
                }
            }
        }
    }
    
    return preds;
}

std::optional<std::tuple<int, int, int, int>> TilingSolverCpp::trim_matrix_box(
    const py::array_t<int>& matrix, const std::vector<int>& mask) {
    
    auto buf = matrix.request();
    int* ptr = static_cast<int*>(buf.ptr);
    int rows = buf.shape[0];
    int cols = buf.shape[1];
    
    // Check if matrix has only one unique value
    std::set<int> unique_values;
    for (int i = 0; i < rows * cols; i++) {
        unique_values.insert(ptr[i]);
    }
    if (unique_values.size() == 1) {
        return std::nullopt;
    }
    
    for (int c : mask) {
        std::vector<int> xs, ys;
        
        // Find positions where value != c
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (ptr[i * cols + j] != c) {
                    xs.push_back(i);
                    ys.push_back(j);
                }
            }
        }
        
        if (xs.empty()) {
            continue;
        }
        
        int xmin = *std::min_element(xs.begin(), xs.end());
        int ymin = *std::min_element(ys.begin(), ys.end());
        int xmax = *std::max_element(xs.begin(), xs.end()) + 1;
        int ymax = *std::max_element(ys.begin(), ys.end()) + 1;
        
        if (xmin > 0 || ymin > 0 || xmax < rows || ymax < cols) {
            return std::make_tuple(xmin, ymin, xmax, ymax);
        }
    }
    
    return std::nullopt;
}

std::vector<int> TilingSolverCpp::get_unique_colors(const py::array_t<int>& matrix) {
    std::set<int> unique_set;
    auto buf = matrix.request();
    int* ptr = static_cast<int*>(buf.ptr);
    
    for (py::ssize_t i = 0; i < buf.size; i++) {
        unique_set.insert(ptr[i]);
    }
    
    return std::vector<int>(unique_set.begin(), unique_set.end());
}

py::array_t<int> TilingSolverCpp::create_matrix_copy(const py::array_t<int>& source) {
    auto buf = source.request();
    
    auto result = py::array_t<int>(
        {buf.shape[0], buf.shape[1]},
        {sizeof(int) * buf.shape[1], sizeof(int)}
    );
    
    auto result_buf = result.request();
    std::memcpy(result_buf.ptr, buf.ptr, buf.size * sizeof(int));
    
    return result;
}

py::array_t<int> TilingSolverCpp::create_full_matrix(int rows, int cols, int fill_value) {
    auto result = py::array_t<int>(
        {rows, cols},
        {sizeof(int) * cols, sizeof(int)}
    );
    
    auto result_buf = result.request();
    int* result_ptr = static_cast<int*>(result_buf.ptr);
    
    for (int i = 0; i < rows * cols; i++) {
        result_ptr[i] = fill_value;
    }
    
    return result;
}

bool TilingSolverCpp::patterns_match(const py::array_t<int>& pattern1, 
                                     const py::array_t<int>& pattern2, 
                                     const py::array_t<int>& mask) {
    auto buf1 = pattern1.request();
    auto buf2 = pattern2.request();
    auto mask_buf = mask.request();
    
    int* ptr1 = static_cast<int*>(buf1.ptr);
    int* ptr2 = static_cast<int*>(buf2.ptr);
    int* mask_ptr = static_cast<int*>(mask_buf.ptr);
    
    for (py::ssize_t i = 0; i < buf1.size; i++) {
        if (mask_ptr[i] && ptr1[i] != ptr2[i]) {
            return false;
        }
    }
    
    return true;
}

py::array_t<int> TilingSolverCpp::apply_transform(const py::array_t<int>& pattern, int transform_type) {
    auto buf = pattern.request();
    int rows = buf.shape[0];
    int cols = buf.shape[1];
    
    auto result = create_full_matrix(rows, cols, 0);
    auto result_buf = result.request();
    int* result_ptr = static_cast<int*>(result_buf.ptr);
    int* pattern_ptr = static_cast<int*>(buf.ptr);
    
    switch (transform_type) {
        case 0: // Identity
            std::memcpy(result_ptr, pattern_ptr, rows * cols * sizeof(int));
            break;
        case 1: // Rotate 90 degrees
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    result_ptr[j * rows + (rows - 1 - i)] = pattern_ptr[i * cols + j];
                }
            }
            break;
        case 2: // Rotate 180 degrees
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    result_ptr[(rows - 1 - i) * cols + (cols - 1 - j)] = pattern_ptr[i * cols + j];
                }
            }
            break;
        case 3: // Rotate 270 degrees
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    result_ptr[(cols - 1 - j) * rows + i] = pattern_ptr[i * cols + j];
                }
            }
            break;
        default:
            std::memcpy(result_ptr, pattern_ptr, rows * cols * sizeof(int));
            break;
    }
    
    return result;
}

std::vector<std::function<py::array_t<int>(const py::array_t<int>&)>> TilingSolverCpp::get_all_transforms() {
    std::vector<std::function<py::array_t<int>(const py::array_t<int>&)>> transforms;
    
    // Add basic transformations
    for (int i = 0; i < 4; i++) {
        transforms.push_back([this, i](const py::array_t<int>& pattern) {
            return apply_transform(pattern, i);
        });
    }
    
    return transforms;
} 