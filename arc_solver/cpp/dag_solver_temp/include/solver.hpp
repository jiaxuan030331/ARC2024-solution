#pragma once
#include <string>
#include <vector>
#include <memory>
#include "core/state.hpp"
#include "transform/transform.hpp"
#include "piece/piece.hpp"
#include "candidate/candidate.hpp"
#include "scoring/score.hpp"

namespace arc::solver {

// ============================================================================
// ARC任务数据结构
// ============================================================================

struct ARCExample {
    arc::core::Grid input;
    arc::core::Grid output;
    
    ARCExample(const arc::core::Grid& in, const arc::core::Grid& out) 
        : input(in), output(out) {}
};

struct ARCTask {
    std::string taskId;
    std::vector<ARCExample> trainingExamples;
    arc::core::Grid testInput;
    arc::core::Grid testOutput; // 用于评估（实际求解时为空）
    
    std::size_t getTrainingCount() const { return trainingExamples.size(); }
    bool hasTestOutput() const { return testOutput.width > 0 && testOutput.height > 0; }
};

// ============================================================================
// 求解器配置 - 对应icecuber的参数设置
// ============================================================================

struct SolverConfig {
    // DAG构建参数
    int maxDepth = 20;              // 对应icecuber的MAXDEPTH
    int maxSide = 100;              // 对应icecuber的MAXSIDE
    int maxArea = 1600;             // 对应icecuber的MAXAREA (40*40)
    int maxPixels = 8000;           // 对应icecuber的MAXPIXELS
    
    // Piece提取参数
    std::size_t maxPieces = 100000; // 最大piece数量
    bool enablePieceOptimization = true;
    
    // 候选解生成参数
    std::size_t maxCandidates = 1000;
    int maxIterations = 10;
    bool enableGreedyFill = true;
    
    // 评分参数
    float complexityPenalty = 0.01f; // 对应icecuber的0.01
    std::size_t maxAnswers = 3;      // 对应icecuber的assert(answers.size() <= 3)
    
    // 调试参数
    bool printTimes = false;
    bool printMemory = false;
    bool printNodes = false;
    bool enableVisualization = false;
};

// ============================================================================
// 求解结果
// ============================================================================

struct SolveResult {
    std::vector<arc::core::Grid> answers;      // 最多3个答案
    double solvingTime = 0.0;                  // 求解时间（秒）
    std::size_t totalPieces = 0;               // 生成的piece数量
    std::size_t totalCandidates = 0;           // 生成的候选解数量
    float bestScore = 0.0f;                    // 最佳候选解分数
    bool success = false;                      // 是否成功求解
    
    // 对应icecuber的verdict系统
    enum class Verdict {
        Nothing = 0,     // 没有找到答案
        Dimensions = 1,  // 尺寸正确
        Candidate = 2,   // 有候选解
        Correct = 3      // 完全正确
    };
    Verdict verdict = Verdict::Nothing;
    
    bool hasAnswers() const { return !answers.empty(); }
    const arc::core::Grid& getBestAnswer() const { 
        if (answers.empty()) throw std::runtime_error("No answers available");
        return answers[0]; 
    }
};

// ============================================================================
// 主求解器 - 对应icecuber的runner核心逻辑
// ============================================================================

class ARCSolver {
public:
    ARCSolver(const SolverConfig& config = {});
    
    // 主求解函数 - 对应icecuber的run函数核心逻辑
    SolveResult solve(const ARCTask& task);
    
    // 批量求解 - 对应icecuber的批量处理
    std::vector<SolveResult> solveBatch(const std::vector<ARCTask>& tasks);
    
    // 获取统计信息
    struct Statistics {
        int totalTasks = 0;
        int correctSolutions = 0;
        int candidateSolutions = 0;
        int dimensionMatches = 0;
        double averageSolvingTime = 0.0;
        double totalTime = 0.0;
    };
    
    Statistics getStatistics() const { return statistics_; }
    void resetStatistics() { statistics_ = Statistics{}; }
    
    const SolverConfig& getConfig() const { return config_; }
    void setConfig(const SolverConfig& config) { config_ = config; }
    
private:
    SolverConfig config_;
    Statistics statistics_;
    
    // 核心求解组件
    std::unique_ptr<arc::transform::TransformLibrary> transformLib_;
    std::unique_ptr<arc::piece::PieceExtractor> pieceExtractor_;
    std::unique_ptr<arc::candidate::CandidateComposer> candidateComposer_;
    std::unique_ptr<arc::scoring::IntegratedScorer> scorer_;
    
    // 核心求解步骤 - 对应icecuber的主要流程
    
    // 1. 尺寸预测 - 对应icecuber的bruteSize
    std::vector<arc::core::Point> predictOutputSizes(
        const arc::core::Grid& testInput,
        const std::vector<ARCExample>& training
    );
    
    // 2. 构建DAG和提取pieces - 对应icecuber的brutePieces2 + makePieces2
    arc::piece::PieceCollection buildPieces(
        const arc::core::Grid& testInput,
        const std::vector<ARCExample>& training,
        const std::vector<arc::core::Point>& outputSizes
    );
    
    // 3. 组合候选解 - 对应icecuber的composePieces2
    std::vector<arc::candidate::Candidate> generateCandidates(
        arc::piece::PieceCollection& pieces,
        const std::vector<ARCExample>& training,
        const std::vector<arc::core::Point>& outputSizes
    );
    
    // 4. 评估和排序 - 对应icecuber的evaluateCands
    std::vector<arc::candidate::Candidate> evaluateAndRank(
        std::vector<arc::candidate::Candidate> candidates,
        const std::vector<ARCExample>& training
    );
    
    // 5. 选择最佳答案 - 对应icecuber的答案过滤逻辑
    std::vector<arc::core::Grid> selectBestAnswers(
        const std::vector<arc::candidate::Candidate>& rankedCandidates
    );
    
    // 辅助函数
    void updateStatistics(const SolveResult& result);
    SolveResult::Verdict calculateVerdict(
        const std::vector<arc::core::Grid>& answers,
        const ARCTask& task
    );
    void printProgress(const std::string& stage, double elapsedTime = 0.0);
    void printMemoryUsage(const arc::piece::PieceCollection& pieces);
};

// ============================================================================
// 任务加载器 - 对应icecuber的读取逻辑
// ============================================================================

class TaskLoader {
public:
    // 从JSON文件加载单个任务
    static ARCTask loadFromFile(const std::string& filepath);
    
    // 从JSON字符串加载任务
    static ARCTask loadFromJson(const std::string& jsonStr);
    
    // 批量加载任务目录
    static std::vector<ARCTask> loadFromDirectory(const std::string& dirPath);
    
    // 创建简单测试任务
    static ARCTask createTestTask(
        const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& examples,
        const arc::core::Grid& testInput,
        const arc::core::Grid& testOutput = arc::core::Grid(0, 0)
    );
};

// ============================================================================
// 求解器工厂
// ============================================================================

class SolverFactory {
public:
    // 创建默认求解器 - 对应icecuber的默认配置
    static std::unique_ptr<ARCSolver> createDefault();
    
    // 创建快速求解器 - 低延迟配置
    static std::unique_ptr<ARCSolver> createFast();
    
    // 创建高精度求解器 - 高质量配置
    static std::unique_ptr<ARCSolver> createAccurate();
    
    // 从配置创建求解器
    static std::unique_ptr<ARCSolver> createFromConfig(const SolverConfig& config);
};

// ============================================================================
// 辅助函数
// ============================================================================

// 颜色化输出 - 对应icecuber的颜色函数
std::string colorGreen(const std::string& text);
std::string colorBlue(const std::string& text);
std::string colorYellow(const std::string& text);
std::string colorRed(const std::string& text);

// 打印求解结果 - 对应icecuber的writeVerdict
void printResult(int taskIndex, const std::string& taskId, const SolveResult& result);

// 打印统计摘要
void printStatistics(const ARCSolver::Statistics& stats);

} // namespace arc::solver 