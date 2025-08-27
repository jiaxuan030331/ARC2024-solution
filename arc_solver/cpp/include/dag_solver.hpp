#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace arc_solver {

// 核心数据结构
struct Grid {
    int width, height;
    std::vector<std::uint8_t> pixels;
    
    Grid(int w = 0, int h = 0) : width(w), height(h), pixels(w * h, 0) {}
    
    std::uint8_t& operator()(int row, int col) {
        return pixels[row * width + col];
    }
    
    const std::uint8_t& operator()(int row, int col) const {
        return pixels[row * width + col];
    }
    
    std::uint8_t safe(int row, int col) const {
        if (row < 0 || col < 0 || row >= height || col >= width) return 0;
        return pixels[row * width + col];
    }
    
    bool operator==(const Grid& other) const {
        return width == other.width && height == other.height && pixels == other.pixels;
    }
};

struct State {
    std::vector<Grid> images;
    std::uint8_t depth = 0;
    bool isVector = false;
};

// DAG配置
struct DAGConfig {
    std::size_t maxDepth = 25;
    std::size_t maxNodes = 100000;
    std::size_t maxPixels = 40*40*5;
    double timeLimit = 60.0;
};

// 求解配置
struct SolverConfig {
    int maxDepth = 20;
    int maxSide = 100;
    int maxArea = 1600;
    int maxPixels = 8000;
    float complexityPenalty = 0.01f;
    std::size_t maxAnswers = 3;
    bool printTimes = false;
    bool printMemory = false;
    bool printNodes = false;
    bool enableVisualization = false;
};

// 求解结果
struct SolveResult {
    std::vector<Grid> answers;
    double solvingTime = 0.0;
    std::size_t totalPieces = 0;
    std::size_t totalCandidates = 0;
    float bestScore = 0.0f;
    bool success = false;
    enum class Verdict { Nothing = 0, Dimensions = 1, Candidate = 2, Correct = 3 };
    Verdict verdict = Verdict::Nothing;
};

// 任务定义
struct ARCExample {
    Grid input;
    Grid output;
    
    ARCExample(const Grid& in, const Grid& out) : input(in), output(out) {}
};

struct ARCTask {
    std::string taskId;
    std::vector<ARCExample> training;
    Grid testInput;
};

// DAG Solver主类
class DAGSolverCpp {
public:
    explicit DAGSolverCpp(const SolverConfig& config = SolverConfig());
    ~DAGSolverCpp();
    
    // 主要接口方法
    bool can_solve(const std::vector<std::vector<std::vector<int>>>& train_inputs,
                   const std::vector<std::vector<std::vector<int>>>& train_outputs);
    
    std::vector<std::vector<std::vector<int>>> solve(
        const std::vector<std::vector<std::vector<int>>>& train_inputs,
        const std::vector<std::vector<std::vector<int>>>& train_outputs,
        const std::vector<std::vector<std::vector<int>>>& test_inputs);
    
    // DAG特有的方法
    SolveResult solveSingle(const ARCTask& task);
    std::vector<SolveResult> solveBatch(const std::vector<ARCTask>& tasks);
    
    // 配置和统计
    SolverConfig getConfig() const { return config_; }
    std::vector<std::string> getAvailableFunctions() const;
    
    // 测试单个变换函数
    Grid testTransform(const std::string& funcName, const Grid& input);
    
private:
    SolverConfig config_;
    
    // 内部实现类（前向声明）
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // 辅助方法
    Grid convertFromVector(const std::vector<std::vector<int>>& input);
    std::vector<std::vector<int>> convertToVector(const Grid& grid);
    ARCTask convertTask(const std::vector<std::vector<std::vector<int>>>& train_inputs,
                       const std::vector<std::vector<std::vector<int>>>& train_outputs,
                       const std::vector<std::vector<int>>& test_input);
};

} // namespace arc_solver 