#include "../include/ml_solver.hpp"
#include <algorithm>
#include <unordered_map>
#include <set>
#include <iostream>
#include <cstring>
#include <cmath>
#include <queue>

MLSolverCpp::MLSolverCpp() {
    // Constructor
}

bool MLSolverCpp::can_solve(const std::vector<py::array_t<int>>& train_inputs, 
                            const std::vector<py::array_t<int>>& train_outputs) {
    // Check if all training examples have subitem relationship
    for (size_t i = 0; i < train_inputs.size(); i++) {
        if (!has_subitem(train_inputs[i], train_outputs[i])) {
            return false;
        }
    }
    return true;
}

std::vector<py::array_t<int>> MLSolverCpp::solve(
    const std::vector<py::array_t<int>>& train_inputs,
    const std::vector<py::array_t<int>>& train_outputs,
    const std::vector<py::array_t<int>>& test_inputs) {
    
    if (!can_solve(train_inputs, train_outputs)) {
        return {};
    }
    
    // Format training features
    auto train_features = format_features(train_inputs, train_outputs);
    
    // Train the model
    model.fit(train_features);
    
    std::vector<py::array_t<int>> results;
    
    // Process each test input
    for (const auto& test_input : test_inputs) {
        auto test_features = make_features(test_input);
        auto probabilities = model.predict_proba(test_features);
        
        // Find the feature with highest probability
        auto max_iter = std::max_element(probabilities.begin(), probabilities.end());
        if (max_iter != probabilities.end()) {
            size_t max_idx = std::distance(probabilities.begin(), max_iter);
            const auto& best_feature = test_features[max_idx];
            
            // Extract the corresponding submatrix
            auto result = extract_submatrix(test_input, 
                                          best_feature.xmin, best_feature.ymin,
                                          best_feature.xmax, best_feature.ymax);
            results.push_back(result);
        }
    }
    
    return results;
}

bool MLSolverCpp::has_subitem(const py::array_t<int>& matrix, const py::array_t<int>& sub_matrix) {
    auto matrix_buf = matrix.request();
    auto sub_buf = sub_matrix.request();
    
    int* matrix_ptr = static_cast<int*>(matrix_buf.ptr);
    int* sub_ptr = static_cast<int*>(sub_buf.ptr);
    
    int m_rows = matrix_buf.shape[0];
    int m_cols = matrix_buf.shape[1];
    int s_rows = sub_buf.shape[0];
    int s_cols = sub_buf.shape[1];
    
    // Check if sub_matrix can fit in matrix
    if (s_rows > m_rows || s_cols > m_cols) {
        return false;
    }
    
    // Search for sub_matrix in matrix
    for (int i = 0; i <= m_rows - s_rows; i++) {
        for (int j = 0; j <= m_cols - s_cols; j++) {
            bool match = true;
            
            // Check if sub_matrix matches at position (i,j)
            for (int si = 0; si < s_rows && match; si++) {
                for (int sj = 0; sj < s_cols && match; sj++) {
                    int matrix_val = matrix_ptr[(i + si) * m_cols + (j + sj)];
                    int sub_val = sub_ptr[si * s_cols + sj];
                    if (matrix_val != sub_val) {
                        match = false;
                    }
                }
            }
            
            if (match) {
                return true;
            }
        }
    }
    
    return false;
}

std::vector<FeatureRecord> MLSolverCpp::format_features(
    const std::vector<py::array_t<int>>& train_inputs,
    const std::vector<py::array_t<int>>& train_outputs) {
    
    std::vector<FeatureRecord> all_features;
    
    for (size_t i = 0; i < train_inputs.size(); i++) {
        const auto& input = train_inputs[i];
        const auto& output = train_outputs[i];
        
        // Generate features for this input
        auto features = make_features(input);
        
        // Mark all as negative labels initially
        for (auto& feature : features) {
            feature.label = false;
        }
        
        // Find positions where output matches and mark as positive
        auto positions = find_sub(input, output);
        for (const auto& pos : positions) {
            int xmin, ymin, xmax, ymax;
            std::tie(xmin, ymin, xmax, ymax) = pos;
            
            // Find matching feature and mark as positive
            for (auto& feature : features) {
                if (feature.xmin == xmin && feature.ymin == ymin &&
                    feature.xmax == xmax && feature.ymax == ymax) {
                    feature.label = true;
                }
            }
        }
        
        // Add to all features
        all_features.insert(all_features.end(), features.begin(), features.end());
    }
    
    return all_features;
}

std::vector<FeatureRecord> MLSolverCpp::make_features(const py::array_t<int>& matrix) {
    auto buf = matrix.request();
    int rows = buf.shape[0];
    int cols = buf.shape[1];
    
    std::vector<FeatureRecord> features;
    
    // Generate all possible submatrices
    for (int xmin = 0; xmin < rows; xmin++) {
        for (int ymin = 0; ymin < cols; ymin++) {
            for (int xmax = xmin + 1; xmax <= rows; xmax++) {
                for (int ymax = ymin + 1; ymax <= cols; ymax++) {
                    // Skip if this is the entire matrix
                    if (xmin == 0 && ymin == 0 && xmax == rows && ymax == cols) {
                        continue;
                    }
                    
                    // Extract submatrix
                    auto submatrix = extract_submatrix(matrix, xmin, ymin, xmax, ymax);
                    
                    // Calculate features
                    FeatureRecord record;
                    record.xmin = xmin;
                    record.ymin = ymin;
                    record.xmax = xmax;
                    record.ymax = ymax;
                    record.area = (xmax - xmin) * (ymax - ymin);
                    record.unique_colors = count_unique_colors(submatrix);
                    record.mode_color = get_mode_color(submatrix);
                    record.has_frame = has_frame(submatrix);
                    record.rps4 = has_region(submatrix, 1);
                    record.rps8 = has_region(submatrix, 2);
                    record.label = false; // Will be set later in format_features
                    
                    features.push_back(record);
                }
            }
        }
    }
    
    return features;
}

std::vector<std::tuple<int, int, int, int>> MLSolverCpp::find_sub(
    const py::array_t<int>& matrix, const py::array_t<int>& sub_matrix) {
    
    std::vector<std::tuple<int, int, int, int>> positions;
    
    auto matrix_buf = matrix.request();
    auto sub_buf = sub_matrix.request();
    
    int* matrix_ptr = static_cast<int*>(matrix_buf.ptr);
    int* sub_ptr = static_cast<int*>(sub_buf.ptr);
    
    int m_rows = matrix_buf.shape[0];
    int m_cols = matrix_buf.shape[1];
    int s_rows = sub_buf.shape[0];
    int s_cols = sub_buf.shape[1];
    
    // Search for all occurrences of sub_matrix in matrix
    for (int i = 0; i <= m_rows - s_rows; i++) {
        for (int j = 0; j <= m_cols - s_cols; j++) {
            bool match = true;
            
            // Check if sub_matrix matches at position (i,j)
            for (int si = 0; si < s_rows && match; si++) {
                for (int sj = 0; sj < s_cols && match; sj++) {
                    int matrix_val = matrix_ptr[(i + si) * m_cols + (j + sj)];
                    int sub_val = sub_ptr[si * s_cols + sj];
                    if (matrix_val != sub_val) {
                        match = false;
                    }
                }
            }
            
            if (match) {
                positions.push_back(std::make_tuple(i, j, i + s_rows, j + s_cols));
            }
        }
    }
    
    return positions;
}

int MLSolverCpp::get_mode_color(const py::array_t<int>& array) {
    auto color_counts = count_colors(array);
    
    int mode_color = 0;
    int max_count = 0;
    
    for (const auto& pair : color_counts) {
        if (pair.second > max_count) {
            max_count = pair.second;
            mode_color = pair.first;
        }
    }
    
    return mode_color;
}

int MLSolverCpp::has_frame(const py::array_t<int>& array) {
    auto buf = array.request();
    int rows = buf.shape[0];
    int cols = buf.shape[1];
    
    if (rows < 2 || cols < 2) {
        return 0;
    }
    
    int* ptr = static_cast<int*>(buf.ptr);
    
    // Get all border elements
    std::vector<int> border_elements;
    
    // Top and bottom rows
    for (int j = 0; j < cols; j++) {
        border_elements.push_back(ptr[0 * cols + j]);        // Top row
        border_elements.push_back(ptr[(rows-1) * cols + j]); // Bottom row
    }
    
    // Left and right columns (excluding corners already added)
    for (int i = 1; i < rows - 1; i++) {
        border_elements.push_back(ptr[i * cols + 0]);        // Left column
        border_elements.push_back(ptr[i * cols + (cols-1)]); // Right column
    }
    
    // Check if all border elements are the same
    if (border_elements.empty()) {
        return 0;
    }
    
    int first_element = border_elements[0];
    for (int element : border_elements) {
        if (element != first_element) {
            return 0;
        }
    }
    
    return 1;
}

int MLSolverCpp::has_region(const py::array_t<int>& array, int connectivity) {
    auto labeled = label_connected_components(array, -1, connectivity);
    auto labeled_buf = labeled.request();
    int* labeled_ptr = static_cast<int*>(labeled_buf.ptr);
    
    // Find maximum label (number of connected components)
    int max_label = 0;
    for (py::ssize_t i = 0; i < labeled_buf.size; i++) {
        max_label = std::max(max_label, labeled_ptr[i]);
    }
    
    return (max_label > 0) ? 1 : 0;
}

int MLSolverCpp::count_unique_colors(const py::array_t<int>& array) {
    auto color_counts = count_colors(array);
    return static_cast<int>(color_counts.size());
}

py::array_t<int> MLSolverCpp::extract_submatrix(const py::array_t<int>& matrix, 
                                                 int xmin, int ymin, int xmax, int ymax) {
    auto buf = matrix.request();
    int* ptr = static_cast<int*>(buf.ptr);
    int cols = buf.shape[1];
    
    int sub_rows = xmax - xmin;
    int sub_cols = ymax - ymin;
    
    auto result = py::array_t<int>(
        {sub_rows, sub_cols},
        {sizeof(int) * sub_cols, sizeof(int)}
    );
    
    auto result_buf = result.request();
    int* result_ptr = static_cast<int*>(result_buf.ptr);
    
    for (int i = 0; i < sub_rows; i++) {
        for (int j = 0; j < sub_cols; j++) {
            result_ptr[i * sub_cols + j] = ptr[(xmin + i) * cols + (ymin + j)];
        }
    }
    
    return result;
}

bool MLSolverCpp::arrays_equal(const py::array_t<int>& arr1, const py::array_t<int>& arr2) {
    auto buf1 = arr1.request();
    auto buf2 = arr2.request();
    
    if (buf1.size != buf2.size) {
        return false;
    }
    
    int* ptr1 = static_cast<int*>(buf1.ptr);
    int* ptr2 = static_cast<int*>(buf2.ptr);
    
    for (py::ssize_t i = 0; i < buf1.size; i++) {
        if (ptr1[i] != ptr2[i]) {
            return false;
        }
    }
    
    return true;
}

py::array_t<int> MLSolverCpp::label_connected_components(const py::array_t<int>& array, 
                                                        int background, int connectivity) {
    auto buf = array.request();
    int rows = buf.shape[0];
    int cols = buf.shape[1];
    int* ptr = static_cast<int*>(buf.ptr);
    
    // Convert to 2D vector for easier processing
    std::vector<std::vector<int>> input_array(rows, std::vector<int>(cols));
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            input_array[i][j] = ptr[i * cols + j];
        }
    }
    
    // Initialize labels array
    std::vector<std::vector<int>> labels(rows, std::vector<int>(cols, 0));
    int current_label = 1;
    
    // Connected component labeling using flood fill
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (labels[i][j] == 0 && input_array[i][j] != background) {
                flood_fill(labels, input_array, i, j, current_label, input_array[i][j], connectivity);
                current_label++;
            }
        }
    }
    
    // Convert back to numpy array
    auto result = py::array_t<int>(
        {rows, cols},
        {sizeof(int) * cols, sizeof(int)}
    );
    
    auto result_buf = result.request();
    int* result_ptr = static_cast<int*>(result_buf.ptr);
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            result_ptr[i * cols + j] = labels[i][j];
        }
    }
    
    return result;
}

void MLSolverCpp::flood_fill(std::vector<std::vector<int>>& labels, 
                             const std::vector<std::vector<int>>& array,
                             int x, int y, int label, int target_value, int connectivity) {
    int rows = array.size();
    int cols = array[0].size();
    
    std::queue<std::pair<int, int>> queue;
    queue.push({x, y});
    labels[x][y] = label;
    
    // Define neighbor offsets based on connectivity
    std::vector<std::pair<int, int>> offsets;
    if (connectivity == 1) { // 4-connectivity
        offsets = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    } else { // 8-connectivity
        offsets = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
    }
    
    while (!queue.empty()) {
        auto [cx, cy] = queue.front();
        queue.pop();
        
        for (const auto& [dx, dy] : offsets) {
            int nx = cx + dx;
            int ny = cy + dy;
            
            if (nx >= 0 && nx < rows && ny >= 0 && ny < cols &&
                labels[nx][ny] == 0 && array[nx][ny] == target_value) {
                labels[nx][ny] = label;
                queue.push({nx, ny});
            }
        }
    }
}

std::unordered_map<int, int> MLSolverCpp::count_colors(const py::array_t<int>& array) {
    std::unordered_map<int, int> color_counts;
    
    auto buf = array.request();
    int* ptr = static_cast<int*>(buf.ptr);
    
    for (py::ssize_t i = 0; i < buf.size; i++) {
        color_counts[ptr[i]]++;
    }
    
    return color_counts;
}

// ML Model implementation
void MLSolverCpp::MLModel::fit(const std::vector<FeatureRecord>& data) {
    training_data = data;
    
    // Calculate a simple threshold based on training data statistics
    // This is a simplified model compared to sklearn's BaggingClassifier
    double positive_score_sum = 0.0;
    double negative_score_sum = 0.0;
    int positive_count = 0;
    int negative_count = 0;
    
    for (const auto& record : data) {
        double score = calculate_score(record);
        if (record.label) {
            positive_score_sum += score;
            positive_count++;
        } else {
            negative_score_sum += score;
            negative_count++;
        }
    }
    
    double positive_avg = positive_count > 0 ? positive_score_sum / positive_count : 0.0;
    double negative_avg = negative_count > 0 ? negative_score_sum / negative_count : 0.0;
    
    threshold = (positive_avg + negative_avg) / 2.0;
}

std::vector<double> MLSolverCpp::MLModel::predict_proba(const std::vector<FeatureRecord>& test_data) {
    std::vector<double> probabilities;
    
    for (const auto& record : test_data) {
        double score = calculate_score(record);
        // Convert score to probability using sigmoid-like function
        double prob = 1.0 / (1.0 + std::exp(-(score - threshold)));
        probabilities.push_back(prob);
    }
    
    return probabilities;
}

double MLSolverCpp::MLModel::calculate_score(const FeatureRecord& record) {
    // Simple scoring function based on feature importance
    double score = 0.0;
    
    // Area contributes to score
    score += record.area * 0.1;
    
    // Frame presence is important
    score += record.has_frame * 2.0;
    
    // Region connectivity
    score += record.rps4 * 1.5;
    score += record.rps8 * 1.0;
    
    // Color diversity
    score += record.unique_colors * 0.5;
    
    // Position bias (prefer center regions)
    int center_bias = std::abs((record.xmax + record.xmin) / 2 - 5) + 
                     std::abs((record.ymax + record.ymin) / 2 - 5);
    score -= center_bias * 0.1;
    
    return score;
} 