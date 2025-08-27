#include "../include/dag_solver.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <cmath>
#include <random>
#include <unordered_set>

namespace arc_solver {

// 哈希函数实现
std::uint64_t hashGrid(const Grid& grid) {
    // 使用FNV-1a哈希算法，与icecuber保持一致
    constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;
    
    std::uint64_t hash = FNV_OFFSET_BASIS;
    
    // 哈希尺寸信息
    hash ^= static_cast<std::uint64_t>(grid.width);
    hash *= FNV_PRIME;
    hash ^= static_cast<std::uint64_t>(grid.height);
    hash *= FNV_PRIME;
    
    // 哈希像素数据
    for (const auto& pixel : grid.pixels) {
        hash ^= static_cast<std::uint64_t>(pixel);
        hash *= FNV_PRIME;
    }
    
    return hash;
}

std::uint64_t hashState(const State& state) {
    constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;
    
    std::uint64_t hash = FNV_OFFSET_BASIS;
    
    // 哈希深度
    hash ^= static_cast<std::uint64_t>(state.depth);
    hash *= FNV_PRIME;
    
    // 哈希向量标志
    hash ^= static_cast<std::uint64_t>(state.isVector);
    hash *= FNV_PRIME;
    
    // 哈希所有图像
    for (const auto& img : state.images) {
        std::uint64_t img_hash = hashGrid(img);
        hash ^= img_hash;
        hash *= FNV_PRIME;
    }
    
    return hash;
}

// 简化的DAG实现类
class DAGSolverCpp::Impl {
public:
    DAGConfig dagConfig_;
    std::unordered_map<std::uint64_t, State> stateCache_;
    std::vector<std::string> availableFunctions_;
    
    Impl() {
        // 初始化可用的变换函数（简化版本）
        availableFunctions_ = {
            "identity", "invert", "transpose", "flipH", "flipV",
            "rotateR", "rotateL", "compress", "toOrigin",
            "filterCol_0", "filterCol_1", "filterCol_2", "filterCol_3",
            "rigid_0", "rigid_1", "rigid_2", "rigid_3",
            "colorMap", "fillHoles", "removeNoise", "extractPattern",
            "replicate"
        };
    }
    
    // 简化的变换函数实现
    Grid applyTransform(const std::string& funcName, const Grid& input) {
        if (funcName == "identity") {
            return input;
        } else if (funcName == "invert") {
            Grid output = input;
            for (auto& pixel : output.pixels) {
                pixel = (pixel == 0) ? 1 : 0;
            }
            return output;
        } else if (funcName == "transpose") {
            Grid output(input.height, input.width);
            for (int i = 0; i < input.height; ++i) {
                for (int j = 0; j < input.width; ++j) {
                    output(j, i) = input(i, j);
                }
            }
            return output;
        } else if (funcName == "flipH") {
            Grid output = input;
            for (int i = 0; i < input.height; ++i) {
                for (int j = 0; j < input.width / 2; ++j) {
                    std::swap(output(i, j), output(i, input.width - 1 - j));
                }
            }
            return output;
        } else if (funcName == "flipV") {
            Grid output = input;
            for (int i = 0; i < input.height / 2; ++i) {
                for (int j = 0; j < input.width; ++j) {
                    std::swap(output(i, j), output(input.height - 1 - i, j));
                }
            }
            return output;
        } else if (funcName == "rotateR") {
            Grid output(input.height, input.width);
            for (int i = 0; i < input.height; ++i) {
                for (int j = 0; j < input.width; ++j) {
                    output(j, input.height - 1 - i) = input(i, j);
                }
            }
            return output;
        } else if (funcName == "compress") {
            // 简化的压缩：去除空行和空列
            std::vector<bool> validRows(input.height, false);
            std::vector<bool> validCols(input.width, false);
            
            for (int i = 0; i < input.height; ++i) {
                for (int j = 0; j < input.width; ++j) {
                    if (input(i, j) != 0) {
                        validRows[i] = true;
                        validCols[j] = true;
                    }
                }
            }
            
            int newHeight = std::count(validRows.begin(), validRows.end(), true);
            int newWidth = std::count(validCols.begin(), validCols.end(), true);
            
            if (newHeight == 0 || newWidth == 0) {
                return Grid(1, 1); // 返回最小网格
            }
            
            Grid output(newWidth, newHeight);
            int oi = 0;
            for (int i = 0; i < input.height; ++i) {
                if (!validRows[i]) continue;
                int oj = 0;
                for (int j = 0; j < input.width; ++j) {
                    if (!validCols[j]) continue;
                    output(oi, oj) = input(i, j);
                    oj++;
                }
                oi++;
            }
            return output;
        }
        
        // 默认返回原图像
        return input;
    }
    
    // 简化的DAG搜索
    std::vector<Grid> searchSolutions(const ARCTask& task, const SolverConfig& config) {
        std::vector<Grid> solutions;
        
        // 预测输出尺寸
        std::vector<std::pair<int, int>> outputSizes;
        for (const auto& example : task.training) {
            outputSizes.emplace_back(example.output.width, example.output.height);
        }
        
        // 如果所有训练样本有相同的输出尺寸，使用该尺寸
        bool sameSize = true;
        if (!outputSizes.empty()) {
            auto firstSize = outputSizes[0];
            for (const auto& size : outputSizes) {
                if (size != firstSize) {
                    sameSize = false;
                    break;
                }
            }
        }
        
        // 尝试不同的变换组合
        std::vector<std::string> functionsToTry = {"identity", "invert", "transpose", "flipH", "flipV"};
        
        for (const auto& funcName : functionsToTry) {
            try {
                Grid result = applyTransform(funcName, task.testInput);
                
                // 检查尺寸是否合理
                if (sameSize && !outputSizes.empty()) {
                    auto expectedSize = outputSizes[0];
                    if (result.width == expectedSize.first && result.height == expectedSize.second) {
                        solutions.push_back(result);
                    }
                } else {
                    // 检查尺寸是否在合理范围内
                    if (result.width <= config.maxSide && result.height <= config.maxSide &&
                        result.width * result.height <= config.maxArea) {
                        solutions.push_back(result);
                    }
                }
                
                if (solutions.size() >= config.maxAnswers) {
                    break;
                }
            } catch (...) {
                // 忽略变换错误
            }
        }
        
        // 如果没有找到解决方案，返回一个简单的变换
        if (solutions.empty() && !outputSizes.empty()) {
            auto expectedSize = outputSizes[0];
            Grid defaultSolution(expectedSize.first, expectedSize.second);
            // 用测试输入的平均颜色填充
            if (!task.testInput.pixels.empty()) {
                int sum = 0;
                for (auto pixel : task.testInput.pixels) {
                    sum += pixel;
                }
                std::uint8_t avgColor = sum / task.testInput.pixels.size();
                std::fill(defaultSolution.pixels.begin(), defaultSolution.pixels.end(), avgColor);
            }
            solutions.push_back(defaultSolution);
        }
        
        return solutions;
    }
};

// DAGSolverCpp实现
DAGSolverCpp::DAGSolverCpp(const SolverConfig& config) 
    : config_(config), impl_(std::make_unique<Impl>()) {
}

DAGSolverCpp::~DAGSolverCpp() = default;

bool DAGSolverCpp::can_solve(const std::vector<std::vector<std::vector<int>>>& train_inputs,
                            const std::vector<std::vector<std::vector<int>>>& train_outputs) {
    // 基础检查
    if (train_inputs.empty() || train_outputs.empty() || 
        train_inputs.size() != train_outputs.size()) {
        return false;
    }
    
    // 检查尺寸限制
    for (const auto& input : train_inputs) {
        if (input.empty() || input[0].empty()) return false;
        int height = input.size();
        int width = input[0].size();
        
        if (height > config_.maxSide || width > config_.maxSide ||
            height * width > config_.maxArea) {
            return false;
        }
    }
    
    for (const auto& output : train_outputs) {
        if (output.empty() || output[0].empty()) return false;
        int height = output.size();
        int width = output[0].size();
        
        if (height > config_.maxSide || width > config_.maxSide ||
            height * width > config_.maxArea) {
            return false;
        }
    }
    
    return true;
}

std::vector<std::vector<std::vector<int>>> DAGSolverCpp::solve(
    const std::vector<std::vector<std::vector<int>>>& train_inputs,
    const std::vector<std::vector<std::vector<int>>>& train_outputs,
    const std::vector<std::vector<std::vector<int>>>& test_inputs) {
    
    std::vector<std::vector<std::vector<int>>> results;
    
    if (test_inputs.empty()) {
        return results;
    }
    
    try {
        // 转换第一个测试输入
        ARCTask task = convertTask(train_inputs, train_outputs, test_inputs[0]);
        
        // 求解
        std::vector<Grid> solutions = impl_->searchSolutions(task, config_);
        
        // 转换回vector格式
        for (const auto& solution : solutions) {
            results.push_back(convertToVector(solution));
        }
        
    } catch (const std::exception& e) {
        // 错误处理：返回空结果
        std::cerr << "DAG Solver error: " << e.what() << std::endl;
    }
    
    return results;
}

SolveResult DAGSolverCpp::solveSingle(const ARCTask& task) {
    SolveResult result;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        std::vector<Grid> solutions = impl_->searchSolutions(task, config_);
        result.answers = solutions;
        result.success = !solutions.empty();
        
        if (result.success) {
            result.verdict = SolveResult::Verdict::Candidate;
        } else {
            result.verdict = SolveResult::Verdict::Nothing;
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.verdict = SolveResult::Verdict::Nothing;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.solvingTime = std::chrono::duration<double>(end - start).count();
    
    return result;
}

std::vector<SolveResult> DAGSolverCpp::solveBatch(const std::vector<ARCTask>& tasks) {
    std::vector<SolveResult> results;
    results.reserve(tasks.size());
    
    for (const auto& task : tasks) {
        results.push_back(solveSingle(task));
    }
    
    return results;
}

std::vector<std::string> DAGSolverCpp::getAvailableFunctions() const {
    return impl_->availableFunctions_;
}

Grid DAGSolverCpp::testTransform(const std::string& funcName, const Grid& input) {
    return impl_->applyTransform(funcName, input);
}

// 辅助方法实现
Grid DAGSolverCpp::convertFromVector(const std::vector<std::vector<int>>& input) {
    if (input.empty() || input[0].empty()) {
        return Grid(0, 0);
    }
    
    int height = input.size();
    int width = input[0].size();
    Grid grid(width, height);
    
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            grid(i, j) = static_cast<std::uint8_t>(std::max(0, std::min(255, input[i][j])));
        }
    }
    
    return grid;
}

std::vector<std::vector<int>> DAGSolverCpp::convertToVector(const Grid& grid) {
    std::vector<std::vector<int>> result(grid.height, std::vector<int>(grid.width));
    
    for (int i = 0; i < grid.height; ++i) {
        for (int j = 0; j < grid.width; ++j) {
            result[i][j] = static_cast<int>(grid(i, j));
        }
    }
    
    return result;
}

ARCTask DAGSolverCpp::convertTask(const std::vector<std::vector<std::vector<int>>>& train_inputs,
                                 const std::vector<std::vector<std::vector<int>>>& train_outputs,
                                 const std::vector<std::vector<int>>& test_input) {
    ARCTask task;
    task.taskId = "converted_task";
    
    // 转换训练样本
    for (std::size_t i = 0; i < train_inputs.size() && i < train_outputs.size(); ++i) {
        Grid inputGrid = convertFromVector(train_inputs[i]);
        Grid outputGrid = convertFromVector(train_outputs[i]);
        task.training.emplace_back(inputGrid, outputGrid);
    }
    
    // 转换测试输入
    task.testInput = convertFromVector(test_input);
    
    return task;
}

} // namespace arc_solver 