#pragma once

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <tuple>
#include <set>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;

struct FeatureRecord {
    int xmin, ymin, xmax, ymax;
    int area;
    int unique_colors;
    int mode_color;
    int has_frame;
    int rps4;
    int rps8;
    bool label;
    
    FeatureRecord() : xmin(0), ymin(0), xmax(0), ymax(0), area(0), 
                      unique_colors(0), mode_color(0), has_frame(0), 
                      rps4(0), rps8(0), label(false) {}
};

class MLSolverCpp {
public:
    MLSolverCpp();
    
    // Main interface functions matching Python MLSolver
    bool can_solve(const std::vector<py::array_t<int>>& train_inputs, 
                   const std::vector<py::array_t<int>>& train_outputs);
    
    std::vector<py::array_t<int>> solve(
        const std::vector<py::array_t<int>>& train_inputs,
        const std::vector<py::array_t<int>>& train_outputs,
        const std::vector<py::array_t<int>>& test_inputs
    );

private:
    // Core ML processing functions
    bool has_subitem(const py::array_t<int>& matrix, const py::array_t<int>& sub_matrix);
    std::vector<FeatureRecord> format_features(const std::vector<py::array_t<int>>& train_inputs,
                                                const std::vector<py::array_t<int>>& train_outputs);
    std::vector<FeatureRecord> make_features(const py::array_t<int>& matrix);
    
    // Sub-matrix search functions
    std::vector<std::tuple<int, int, int, int>> find_sub(const py::array_t<int>& matrix, 
                                                          const py::array_t<int>& sub_matrix);
    
    // Feature calculation functions
    int get_mode_color(const py::array_t<int>& array);
    int has_frame(const py::array_t<int>& array);
    int has_region(const py::array_t<int>& array, int connectivity = 1);
    int count_unique_colors(const py::array_t<int>& array);
    
    // Matrix utilities
    py::array_t<int> extract_submatrix(const py::array_t<int>& matrix, 
                                       int xmin, int ymin, int xmax, int ymax);
    bool arrays_equal(const py::array_t<int>& arr1, const py::array_t<int>& arr2);
    
    // Connected component analysis
    py::array_t<int> label_connected_components(const py::array_t<int>& array, 
                                                int background = -1, 
                                                int connectivity = 1);
    void flood_fill(std::vector<std::vector<int>>& labels, 
                   const std::vector<std::vector<int>>& array,
                   int x, int y, int label, int target_value, int connectivity);
    
    // Simple ML prediction (without sklearn dependency)
    struct MLModel {
        std::vector<FeatureRecord> training_data;
        double threshold;
        
        MLModel() : threshold(0.5) {}
        
        void fit(const std::vector<FeatureRecord>& data);
        std::vector<double> predict_proba(const std::vector<FeatureRecord>& test_data);
        double calculate_score(const FeatureRecord& record);
    };
    
    MLModel model;
    
    // Helper functions for feature extraction
    std::vector<int> get_border_elements(const py::array_t<int>& array);
    bool all_elements_equal(const std::vector<int>& elements);
    std::unordered_map<int, int> count_colors(const py::array_t<int>& array);
}; 