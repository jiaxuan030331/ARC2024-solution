#include "../include/symmetry_solver.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <functional>

SymmetrySolverCpp::SymmetrySolverCpp() {
    // Constructor
}

bool SymmetrySolverCpp::can_solve(const std::vector<py::array_t<int>>& train_inputs, 
                                  const std::vector<py::array_t<int>>& train_outputs) {
    // Check if any training example shows symmetry patterns
    for (const auto& x : train_inputs) {
        if (has_symmetry_pattern(x)) return true;
    }
    for (const auto& y : train_outputs) {
        if (has_symmetry_pattern(y)) return true;
    }
    return false;
}

std::vector<py::array_t<int>> SymmetrySolverCpp::solve(
    const std::vector<py::array_t<int>>& train_inputs,
    const std::vector<py::array_t<int>>& train_outputs,
    const std::vector<py::array_t<int>>& test_inputs) {
    
    if (!can_solve(train_inputs, train_outputs)) {
        return {};
    }
    
    std::vector<py::array_t<int>> all_candidates;
    
    for (const auto& test_input : test_inputs) {
        auto candidates = symmetry_repair(train_inputs, train_outputs, test_input);
        all_candidates.insert(all_candidates.end(), candidates.begin(), candidates.end());
    }
    
    return all_candidates;
}

bool SymmetrySolverCpp::has_symmetry_pattern(const py::array_t<int>& matrix) {
    // Check for basic symmetries using function pointers instead of lambdas to avoid type deduction issues
    std::vector<std::function<bool(const py::array_t<int>&)>> funcs = {
        [this](const py::array_t<int>& x) -> bool { return !translation_sym(x).empty(); },
        [this](const py::array_t<int>& x) -> bool { return !translation1d_sym(x).empty(); },
        [this](const py::array_t<int>& x) -> bool { return !horizontal_sym(x).empty(); },
        [this](const py::array_t<int>& x) -> bool { return !vertical_sym(x).empty(); },
        [this](const py::array_t<int>& x) -> bool { return !nw_sym(x).empty(); },
        [this](const py::array_t<int>& x) -> bool { return !ne_sym(x).empty(); },
        [this](const py::array_t<int>& x) -> bool { return !rotate90_sym(x).empty(); },
        [this](const py::array_t<int>& x) -> bool { return !rotate180_sym(x).empty(); }
    };
    
    for (const auto& func : funcs) {
        if (func(matrix)) return true;
    }
    return false;
}

// Parameter calculation functions
std::tuple<std::vector<int>, std::vector<int>, double> 
SymmetrySolverCpp::horizontal_sym_params(const py::array_t<int>& x, int badcolor) {
    auto buf = x.unchecked<2>();
    int n = buf.shape(0);
    std::vector<int> possible_r;
    
    for (int r = 1; r < 2*n-2; ++r) {
        bool possible = true;
        for (int i = 0; i < n && possible; ++i) {
            for (int j = 0; j < buf.shape(1) && possible; ++j) {
                int i1 = r - i;
                if (i1 < 0 || i1 >= n) continue;
                if (buf(i, j) != buf(i1, j) && buf(i, j) != badcolor && buf(i1, j) != badcolor) {
                    possible = false;
                }
            }
        }
        if (possible) possible_r.push_back(r);
    }
    
    if (possible_r.empty()) {
        return {{}, {}, 0.0};
    }
    
    std::vector<std::pair<int, int>> scores;
    for (int r : possible_r) {
        scores.push_back({std::abs(r - n + 1), r});
    }
    std::sort(scores.begin(), scores.end());
    
    std::vector<int> ans, penalty;
    for (size_t i = 0; i < std::min(size_t(3), scores.size()); ++i) {
        ans.push_back(scores[i].second);
        penalty.push_back(scores[i].first);
    }
    
    double sym_level = 0.0;
    if (!ans.empty()) {
        int r = ans[0];
        sym_level = 1.0 - std::abs(r - n + 1) / double(n);
    }
    
    return {ans, penalty, sym_level};
}

std::tuple<std::vector<int>, std::vector<int>, double> 
SymmetrySolverCpp::vertical_sym_params(const py::array_t<int>& x, int badcolor) {
    auto buf = x.unchecked<2>();
    int k = buf.shape(1);
    std::vector<int> possible_s;
    
    for (int s = 1; s < 2*k-2; ++s) {
        bool possible = true;
        for (int i = 0; i < buf.shape(0) && possible; ++i) {
            for (int j = 0; j < k && possible; ++j) {
                int j1 = s - j;
                if (j1 < 0 || j1 >= k) continue;
                if (buf(i, j) != buf(i, j1) && buf(i, j) != badcolor && buf(i, j1) != badcolor) {
                    possible = false;
                }
            }
        }
        if (possible) possible_s.push_back(s);
    }
    
    if (possible_s.empty()) {
        return {{}, {}, 0.0};
    }
    
    std::vector<std::pair<int, int>> scores;
    for (int s : possible_s) {
        scores.push_back({std::abs(s - k + 1), s});
    }
    std::sort(scores.begin(), scores.end());
    
    std::vector<int> ans, penalty;
    for (size_t i = 0; i < std::min(size_t(3), scores.size()); ++i) {
        ans.push_back(scores[i].second);
        penalty.push_back(scores[i].first);
    }
    
    double sym_level = 0.0;
    if (!ans.empty()) {
        int s = ans[0];
        sym_level = 1.0 - std::abs(s - k + 1) / double(k);
    }
    
    return {ans, penalty, sym_level};
}

std::tuple<std::vector<int>, std::vector<int>, double> 
SymmetrySolverCpp::nw_sym_params(const py::array_t<int>& x, int badcolor) {
    auto buf = x.unchecked<2>();
    int n = buf.shape(0), k = buf.shape(1);
    std::vector<int> possible_s;
    
    for (int s = -k+2; s < n-1; ++s) {
        bool possible = true;
        for (int i = 0; i < n && possible; ++i) {
            for (int j = 0; j < k && possible; ++j) {
                int i1 = s + j;
                int j1 = -s + i;
                if (i1 < 0 || i1 >= n || j1 < 0 || j1 >= k) continue;
                if (buf(i, j) != buf(i1, j1)) {
                    possible = false;
                }
            }
        }
        if (possible) possible_s.push_back(s);
    }
    
    if (possible_s.empty()) {
        return {{}, {}, 0.0};
    }
    
    std::vector<std::pair<int, int>> scores;
    for (int s : possible_s) {
        scores.push_back({std::abs(s), s});
    }
    std::sort(scores.begin(), scores.end());
    
    std::vector<int> ans, penalty;
    for (size_t i = 0; i < std::min(size_t(3), scores.size()); ++i) {
        ans.push_back(scores[i].second);
        penalty.push_back(scores[i].first);
    }
    
    double sym_level = 0.0;
    if (!ans.empty()) {
        int s = ans[0];
        sym_level = 1.0 - std::abs(s) / double(n + k);
    }
    
    return {ans, penalty, sym_level};
}

std::tuple<std::vector<int>, std::vector<int>, double> 
SymmetrySolverCpp::ne_sym_params(const py::array_t<int>& x, int badcolor) {
    auto buf = x.unchecked<2>();
    int n = buf.shape(0), k = buf.shape(1);
    std::vector<int> possible_s;
    
    for (int s = 2; s < n+k-3; ++s) {
        bool possible = true;
        for (int i = 0; i < n && possible; ++i) {
            for (int j = 0; j < k && possible; ++j) {
                int i1 = s - j;
                int j1 = s - i;
                if (i1 < 0 || i1 >= n || j1 < 0 || j1 >= k) continue;
                if (buf(i, j) != buf(i1, j1) && buf(i, j) != badcolor && buf(i1, j1) != badcolor) {
                    possible = false;
                }
            }
        }
        if (possible) possible_s.push_back(s);
    }
    
    if (possible_s.empty()) {
        return {{}, {}, 0.0};
    }
    
    std::vector<std::pair<int, int>> scores;
    for (int s : possible_s) {
        scores.push_back({std::abs(2*s - n - k - 2), s});
    }
    std::sort(scores.begin(), scores.end());
    
    std::vector<int> ans, penalty;
    for (size_t i = 0; i < std::min(size_t(3), scores.size()); ++i) {
        ans.push_back(scores[i].second);
        penalty.push_back(scores[i].first);
    }
    
    double sym_level = 0.0;
    if (!ans.empty()) {
        int s = ans[0];
        sym_level = 1.0 - std::abs(2*s - n - k - 2) / double(n + k);
    }
    
    return {ans, penalty, sym_level};
}

// Equivalence class calculation functions
EquivClassList SymmetrySolverCpp::horizontal_sym_eq(const py::array_t<int>& x, int param) {
    auto buf = x.unchecked<2>();
    int n = buf.shape(0), k = buf.shape(1);
    
    std::unordered_map<int, EquivClass> equiv_map;
    
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < k; ++j) {
            int i1 = param - i;
            if (i1 < 0 || i1 >= n) continue;
            
            int key = std::min(i, i1);
            equiv_map[key].push_back({i, j});
            if (i != i1) {
                equiv_map[key].push_back({i1, j});
            }
        }
    }
    
    EquivClassList classes;
    for (const auto& pair : equiv_map) {
        if (pair.second.size() > 1) {
            classes.push_back(pair.second);
        }
    }
    
    return classes;
}

EquivClassList SymmetrySolverCpp::vertical_sym_eq(const py::array_t<int>& x, int param) {
    auto buf = x.unchecked<2>();
    int n = buf.shape(0), k = buf.shape(1);
    
    std::unordered_map<int, EquivClass> equiv_map;
    
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < k; ++j) {
            int j1 = param - j;
            if (j1 < 0 || j1 >= k) continue;
            
            int key = std::min(j, j1);
            equiv_map[key].push_back({i, j});
            if (j != j1) {
                equiv_map[key].push_back({i, j1});
            }
        }
    }
    
    EquivClassList classes;
    for (const auto& pair : equiv_map) {
        if (pair.second.size() > 1) {
            classes.push_back(pair.second);
        }
    }
    
    return classes;
}

// Core make_picture function
std::optional<py::array_t<int>> SymmetrySolverCpp::make_picture(
    const py::array_t<int>& x,
    const EquivClassList& relations,
    int badcolor) {
    
    auto buf = x.unchecked<2>();
    int n = buf.shape(0), k = buf.shape(1);
    
    // Create output array
    py::array_t<int> result = py::array_t<int>({n, k});
    auto result_buf = result.mutable_unchecked<2>();
    
    // Copy original data
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < k; ++j) {
            result_buf(i, j) = buf(i, j);
        }
    }
    
    // Build equivalence graph using Union-Find
    UnionFind uf;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < k; ++j) {
            uf[{i, j}] = {i, j};
        }
    }
    
    // Merge equivalence classes
    union_find_merge(uf, relations);
    
    // Build final equivalence classes
    std::unordered_map<std::tuple<int, int>, EquivClass> final_classes;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < k; ++j) {
            auto root = find_root(uf, {i, j});
            final_classes[root].push_back({i, j});
        }
    }
    
    // Color each equivalence class
    for (const auto& pair : final_classes) {
        const auto& equiv_class = pair.second;
        std::unordered_set<int> colors;
        
        for (const auto& pos : equiv_class) {
            int i = std::get<0>(pos), j = std::get<1>(pos);
            colors.insert(buf(i, j));
        }
        
        if (colors.size() <= 1) continue; // Nothing to recolor
        if (colors.size() > 2) return std::nullopt; // Collision
        
        if (colors.size() == 2 && colors.find(badcolor) == colors.end()) {
            return std::nullopt; // Collision
        }
        
        int fillcolor;
        if (colors.size() == 2 && colors.find(badcolor) != colors.end()) {
            for (int c : colors) {
                if (c != badcolor) {
                    fillcolor = c;
                    break;
                }
            }
        } else {
            fillcolor = *colors.begin();
        }
        
        for (const auto& pos : equiv_class) {
            int i = std::get<0>(pos), j = std::get<1>(pos);
            result_buf(i, j) = fillcolor;
        }
    }
    
    return result;
}

// Union-Find helper functions
void SymmetrySolverCpp::union_find_merge(UnionFind& uf, const EquivClassList& relations) {
    for (const auto& equiv_class : relations) {
        if (equiv_class.size() < 2) continue;
        
        for (size_t i = 1; i < equiv_class.size(); ++i) {
            auto root1 = find_root(uf, equiv_class[0]);
            auto root2 = find_root(uf, equiv_class[i]);
            
            if (root1 != root2) {
                if (root1 < root2) {
                    uf[root2] = root1;
                } else {
                    uf[root1] = root2;
                }
            }
        }
    }
}

std::tuple<int, int> SymmetrySolverCpp::find_root(UnionFind& uf, const std::tuple<int, int>& pos) {
    auto current = pos;
    while (uf[current] != current) {
        current = uf[current];
    }
    return current;
}

// Utility functions
bool SymmetrySolverCpp::is_uniform(const py::array_t<int>& picture) {
    auto buf = picture.unchecked<2>();
    if (buf.size() == 0) return true;
    
    int first_val = buf(0, 0);
    for (int i = 0; i < buf.shape(0); ++i) {
        for (int j = 0; j < buf.shape(1); ++j) {
            if (buf(i, j) != first_val) return false;
        }
    }
    return true;
}

std::vector<std::vector<int>> SymmetrySolverCpp::get_solvable_combinations() {
    return {
        {TRANSLATION}, {TRANSLATION1D}, {HORIZONTAL}, {VERTICAL},
        {NW_DIAGONAL}, {NE_DIAGONAL}, {ROTATE90}, {ROTATE180},
        {TRANSLATION, TRANSLATION1D}, {HORIZONTAL, VERTICAL},
        {NW_DIAGONAL, NE_DIAGONAL}, {ROTATE90, ROTATE180}
    };
}

bool SymmetrySolverCpp::is_solvable_by_symmetry(const std::vector<py::array_t<int>>& xs,
                                               const std::vector<py::array_t<int>>& ys) {
    // Simplified check - in practice would be more sophisticated
    return !xs.empty() && !ys.empty();
}

double SymmetrySolverCpp::sym_score(const py::array_t<int>& x, const std::vector<int>& first_p) {
    double score = 0.0;
    for (int s : first_p) {
        switch (s) {
            case HORIZONTAL:
                score += std::get<2>(horizontal_sym_params(x));
                break;
            case VERTICAL:
                score += std::get<2>(vertical_sym_params(x));
                break;
            case NW_DIAGONAL:
                score += std::get<2>(nw_sym_params(x));
                break;
            case NE_DIAGONAL:
                score += std::get<2>(ne_sym_params(x));
                break;
            // Add other cases as needed
        }
    }
    return score;
}

// Main symmetry repair function
std::vector<py::array_t<int>> SymmetrySolverCpp::symmetry_repair(
    const std::vector<py::array_t<int>>& xs,
    const std::vector<py::array_t<int>>& ys,
    const py::array_t<int>& test_input) {
    
    if (!is_solvable_by_symmetry(xs, ys)) {
        return {};
    }
    
    // Find disappearing colors
    std::vector<int> colors;
    for (size_t i = 0; i < xs.size(); ++i) {
        auto x_buf = xs[i].unchecked<2>();
        auto y_buf = ys[i].unchecked<2>();
        
        if (x_buf.shape(0) != y_buf.shape(0) || x_buf.shape(1) != y_buf.shape(1)) {
            return {};
        }
        
        std::unordered_set<int> disappearing;
        for (int row = 0; row < x_buf.shape(0); ++row) {
            for (int col = 0; col < x_buf.shape(1); ++col) {
                int color1 = x_buf(row, col);
                int color2 = y_buf(row, col);
                if (color2 != color1) {
                    disappearing.insert(color1);
                }
            }
        }
        
        if (disappearing.size() > 1) return {};
        if (disappearing.size() == 1) {
            int c = *disappearing.begin();
            if (std::find(colors.begin(), colors.end(), c) == colors.end()) {
                colors.push_back(c);
            }
        }
    }
    
    // Get candidate bad colors
    std::vector<int> c2;
    if (colors.size() == 1) {
        c2 = colors;
    } else {
        auto test_buf = test_input.unchecked<2>();
        std::unordered_set<int> unique_colors;
        for (int i = 0; i < test_buf.shape(0); ++i) {
            for (int j = 0; j < test_buf.shape(1); ++j) {
                unique_colors.insert(test_buf(i, j));
            }
        }
        c2.assign(unique_colors.begin(), unique_colors.end());
    }
    
    // Try different symmetry combinations
    std::vector<py::array_t<int>> ans;
    std::vector<int> first_p;
    auto solvable_combinations = get_solvable_combinations();
    
    for (const auto& p : solvable_combinations) {
        for (int c : c2) {
            auto candidates = proba_symmetry(xs, ys, test_input, c, p);
            if (!candidates.empty()) {
                ans.insert(ans.end(), candidates.begin(), candidates.end());
                if (ans.size() >= 6) break;
            }
        }
        if (!ans.empty()) {
            first_p = p;
            break;
        }
    }
    
    if (ans.empty()) return {};
    
    // Score and sort candidates
    std::vector<std::pair<double, py::array_t<int>>> scored_candidates;
    for (const auto& picture : ans) {
        double score = sym_score(picture, first_p);
        scored_candidates.push_back({score, picture});
    }
    
    std::sort(scored_candidates.begin(), scored_candidates.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Remove duplicates
    std::vector<py::array_t<int>> result;
    for (const auto& pair : scored_candidates) {
        bool is_duplicate = false;
        for (const auto& existing : result) {
            // Simple duplicate check - could be improved
            if (existing.size() == pair.second.size()) {
                auto buf1 = existing.unchecked<2>();
                auto buf2 = pair.second.unchecked<2>();
                bool equal = true;
                if (buf1.shape(0) == buf2.shape(0) && buf1.shape(1) == buf2.shape(1)) {
                    for (int i = 0; i < buf1.shape(0) && equal; ++i) {
                        for (int j = 0; j < buf1.shape(1) && equal; ++j) {
                            if (buf1(i, j) != buf2(i, j)) equal = false;
                        }
                    }
                    if (equal) {
                        is_duplicate = true;
                        break;
                    }
                }
            }
        }
        if (!is_duplicate) {
            result.push_back(pair.second);
        }
    }
    
    return std::vector<py::array_t<int>>(result.begin(), 
                                        result.begin() + std::min(size_t(3), result.size()));
}

// Proba symmetry function
std::vector<py::array_t<int>> SymmetrySolverCpp::proba_symmetry(
    const std::vector<py::array_t<int>>& xs,
    const std::vector<py::array_t<int>>& ys,
    const py::array_t<int>& test_input,
    int bad_color,
    const std::vector<int>& first_p) {
    
    std::vector<py::array_t<int>> ans;
    
    if (first_p.size() == 1) {
        int s = first_p[0];
        
        // Get parameters and equivalence classes based on symmetry type
        EquivClassList equiv_classes;
        
        switch (s) {
            case HORIZONTAL: {
                auto params = std::get<0>(horizontal_sym_params(test_input, bad_color));
                for (int p : params) {
                    auto eq_classes = horizontal_sym_eq(test_input, p);
                    auto picture = make_picture(test_input, eq_classes, bad_color);
                    if (picture && !is_uniform(*picture)) {
                        ans.push_back(*picture);
                    }
                }
                break;
            }
            case VERTICAL: {
                auto params = std::get<0>(vertical_sym_params(test_input, bad_color));
                for (int p : params) {
                    auto eq_classes = vertical_sym_eq(test_input, p);
                    auto picture = make_picture(test_input, eq_classes, bad_color);
                    if (picture && !is_uniform(*picture)) {
                        ans.push_back(*picture);
                    }
                }
                break;
            }
            case NW_DIAGONAL: {
                auto params = std::get<0>(nw_sym_params(test_input, bad_color));
                for (int p : params) {
                    auto eq_classes = nw_sym_eq(test_input, p);
                    auto picture = make_picture(test_input, eq_classes, bad_color);
                    if (picture && !is_uniform(*picture)) {
                        ans.push_back(*picture);
                    }
                }
                break;
            }
            case NE_DIAGONAL: {
                auto params = std::get<0>(ne_sym_params(test_input, bad_color));
                for (int p : params) {
                    auto eq_classes = ne_sym_eq(test_input, p);
                    auto picture = make_picture(test_input, eq_classes, bad_color);
                    if (picture && !is_uniform(*picture)) {
                        ans.push_back(*picture);
                    }
                }
                break;
            }
        }
    }
    // Handle double symmetry case (first_p.size() == 2) would go here
    
    return ans;
}

// Placeholder implementations for missing symmetry functions
EquivClassList SymmetrySolverCpp::translation_sym(const py::array_t<int>& x) {
    // Simplified implementation
    return {};
}

EquivClassList SymmetrySolverCpp::translation1d_sym(const py::array_t<int>& x) {
    // Simplified implementation  
    return {};
}

EquivClassList SymmetrySolverCpp::nw_sym_eq(const py::array_t<int>& x, int param) {
    // Simplified implementation
    return {};
}

EquivClassList SymmetrySolverCpp::ne_sym_eq(const py::array_t<int>& x, int param) {
    // Simplified implementation
    return {};
}

EquivClassList SymmetrySolverCpp::nw_sym(const py::array_t<int>& x) {
    return {};
}

EquivClassList SymmetrySolverCpp::ne_sym(const py::array_t<int>& x) {
    return {};
}

EquivClassList SymmetrySolverCpp::horizontal_sym(const py::array_t<int>& x) {
    // Get horizontal symmetry parameters
    auto params_result = horizontal_sym_params(x);
    auto params = std::get<0>(params_result);
    if (params.empty()) return {};
    
    // Use the first (best) parameter to generate equivalence classes
    return horizontal_sym_eq(x, params[0]);
}

EquivClassList SymmetrySolverCpp::vertical_sym(const py::array_t<int>& x) {
    // Get vertical symmetry parameters  
    auto params_result = vertical_sym_params(x);
    auto params = std::get<0>(params_result);
    if (params.empty()) return {};
    
    // Use the first (best) parameter to generate equivalence classes
    return vertical_sym_eq(x, params[0]);
}

EquivClassList SymmetrySolverCpp::rotate90_sym(const py::array_t<int>& x) {
    return {};
}

EquivClassList SymmetrySolverCpp::rotate180_sym(const py::array_t<int>& x) {
    return {};
}

// Other placeholder implementations
std::tuple<std::vector<int>, std::vector<int>, double> 
SymmetrySolverCpp::translation_params(const py::array_t<int>& x, int badcolor) {
    return {{}, {}, 0.0};
}

std::tuple<std::vector<std::tuple<int, int>>, std::vector<int>, double> 
SymmetrySolverCpp::translation1d_params(const py::array_t<int>& x, int badcolor) {
    return {{}, {}, 0.0};
}

std::tuple<std::vector<std::tuple<int, int>>, std::vector<int>, double> 
SymmetrySolverCpp::rotate90_sym_params(const py::array_t<int>& x, int badcolor) {
    return {{}, {}, 0.0};
}

std::tuple<std::vector<std::tuple<int, int>>, std::vector<int>, double> 
SymmetrySolverCpp::rotate180_sym_params(const py::array_t<int>& x, int badcolor) {
    return {{}, {}, 0.0};
}

EquivClassList SymmetrySolverCpp::translation_eq(const py::array_t<int>& x, int param) {
    return {};
}

EquivClassList SymmetrySolverCpp::translation1d_eq(const py::array_t<int>& x, const std::tuple<int, int>& param) {
    return {};
}

EquivClassList SymmetrySolverCpp::rotate90_sym_eq(const py::array_t<int>& x, const std::tuple<int, int>& param) {
    return {};
}

EquivClassList SymmetrySolverCpp::rotate180_sym_eq(const py::array_t<int>& x, const std::tuple<int, int>& param) {
    return {};
} 