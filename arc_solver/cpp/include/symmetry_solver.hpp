#pragma once

#include <vector>
#include <tuple>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using EquivClass = std::vector<std::tuple<int, int>>;
using EquivClassList = std::vector<EquivClass>;

class SymmetrySolverCpp {
public:
    SymmetrySolverCpp();
    
    // Main interface functions
    bool can_solve(const std::vector<py::array_t<int>>& train_inputs, 
                   const std::vector<py::array_t<int>>& train_outputs);
    
    std::vector<py::array_t<int>> solve(
        const std::vector<py::array_t<int>>& train_inputs,
        const std::vector<py::array_t<int>>& train_outputs,
        const std::vector<py::array_t<int>>& test_inputs
    );

private:
    // Symmetry detection functions
    bool has_symmetry_pattern(const py::array_t<int>& matrix);
    EquivClassList translation_sym(const py::array_t<int>& x);
    EquivClassList translation1d_sym(const py::array_t<int>& x);
    EquivClassList horizontal_sym(const py::array_t<int>& x);
    EquivClassList vertical_sym(const py::array_t<int>& x);
    EquivClassList nw_sym(const py::array_t<int>& x);
    EquivClassList ne_sym(const py::array_t<int>& x);
    EquivClassList rotate90_sym(const py::array_t<int>& x);
    EquivClassList rotate180_sym(const py::array_t<int>& x);
    
    // Parameter calculation functions
    std::tuple<std::vector<int>, std::vector<int>, double> 
    translation_params(const py::array_t<int>& x, int badcolor = 20);
    
    std::tuple<std::vector<std::tuple<int, int>>, std::vector<int>, double> 
    translation1d_params(const py::array_t<int>& x, int badcolor = 20);
    
    std::tuple<std::vector<int>, std::vector<int>, double> 
    horizontal_sym_params(const py::array_t<int>& x, int badcolor = 20);
    
    std::tuple<std::vector<int>, std::vector<int>, double> 
    vertical_sym_params(const py::array_t<int>& x, int badcolor = 20);
    
    std::tuple<std::vector<int>, std::vector<int>, double> 
    nw_sym_params(const py::array_t<int>& x, int badcolor = 20);
    
    std::tuple<std::vector<int>, std::vector<int>, double> 
    ne_sym_params(const py::array_t<int>& x, int badcolor = 20);
    
    std::tuple<std::vector<std::tuple<int, int>>, std::vector<int>, double> 
    rotate90_sym_params(const py::array_t<int>& x, int badcolor = 20);
    
    std::tuple<std::vector<std::tuple<int, int>>, std::vector<int>, double> 
    rotate180_sym_params(const py::array_t<int>& x, int badcolor = 20);
    
    // Equivalence class calculation functions
    EquivClassList translation_eq(const py::array_t<int>& x, int param);
    EquivClassList translation1d_eq(const py::array_t<int>& x, const std::tuple<int, int>& param);
    EquivClassList horizontal_sym_eq(const py::array_t<int>& x, int param);
    EquivClassList vertical_sym_eq(const py::array_t<int>& x, int param);
    EquivClassList nw_sym_eq(const py::array_t<int>& x, int param);
    EquivClassList ne_sym_eq(const py::array_t<int>& x, int param);
    EquivClassList rotate90_sym_eq(const py::array_t<int>& x, const std::tuple<int, int>& param);
    EquivClassList rotate180_sym_eq(const py::array_t<int>& x, const std::tuple<int, int>& param);
    
    // Core algorithm functions
    std::vector<py::array_t<int>> symmetry_repair(
        const std::vector<py::array_t<int>>& xs,
        const std::vector<py::array_t<int>>& ys,
        const py::array_t<int>& test_input
    );
    
    std::vector<py::array_t<int>> proba_symmetry(
        const std::vector<py::array_t<int>>& xs,
        const std::vector<py::array_t<int>>& ys,
        const py::array_t<int>& test_input,
        int bad_color,
        const std::vector<int>& first_p
    );
    
    std::optional<py::array_t<int>> make_picture(
        const py::array_t<int>& x,
        const EquivClassList& relations,
        int badcolor
    );
    
    bool is_uniform(const py::array_t<int>& picture);
    double sym_score(const py::array_t<int>& x, const std::vector<int>& first_p);
    
    // Utility functions
    std::vector<std::vector<int>> get_solvable_combinations();
    bool is_solvable_by_symmetry(const std::vector<py::array_t<int>>& xs,
                                 const std::vector<py::array_t<int>>& ys);
    
    // Helper functions for make_picture
    using UnionFind = std::unordered_map<std::tuple<int, int>, std::tuple<int, int>>;
    void union_find_merge(UnionFind& uf, const EquivClassList& relations);
    std::tuple<int, int> find_root(UnionFind& uf, const std::tuple<int, int>& pos);
    
    // Function indices for symmetry types
    static constexpr int TRANSLATION = 0;
    static constexpr int TRANSLATION1D = 1;
    static constexpr int HORIZONTAL = 2;
    static constexpr int VERTICAL = 3;
    static constexpr int NW_DIAGONAL = 4;
    static constexpr int NE_DIAGONAL = 5;
    static constexpr int ROTATE90 = 6;
    static constexpr int ROTATE180 = 7;
};

// Hash function for std::tuple<int, int> to use in unordered_map
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