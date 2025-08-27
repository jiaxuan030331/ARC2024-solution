#include "solver.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <set>
#include <fstream>
#include <sstream>

namespace arc::solver {

// ============================================================================
// ARCSolver 实现
// ============================================================================

ARCSolver::ARCSolver(const SolverConfig& config) : config_(config) {
    // 初始化变换函数库
    arc::transform::initializeTransformFunctions();
    transformLib_ = std::make_unique<arc::transform::TransformLibrary>(
        arc::transform::TransformLibrary::instance()
    );
    
    // 初始化核心组件
    arc::piece::PieceExtractor::Config pieceConfig;
    pieceConfig.maxDepth = config_.maxDepth;
    pieceConfig.maxPieces = config_.maxPieces;
    pieceExtractor_ = std::make_unique<arc::piece::PieceExtractor>(pieceConfig);
    
    candidateComposer_ = std::make_unique<arc::candidate::CandidateComposer>();
    
    // 配置候选解组合器
    auto& greedyConfig = candidateComposer_->getGreedyConfig();
    greedyConfig.maxIterations = config_.maxIterations;
    greedyConfig.enableGreedyFill = config_.enableGreedyFill;
    greedyConfig.maxCandidates = config_.maxCandidates;
    
    arc::scoring::IntegratedScorer::Config scoringConfig;
    scoringConfig.candidateConfig.complexityPenalty = config_.complexityPenalty;
    scoringConfig.maxReturnedAnswers = config_.maxAnswers;
    scorer_ = std::make_unique<arc::scoring::IntegratedScorer>(scoringConfig);
}

// 主求解函数 - 对应icecuber的核心求解流程
SolveResult ARCSolver::solve(const ARCTask& task) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    SolveResult result;
    result.success = false;
    
    try {
        if (config_.printTimes) {
            std::cout << "开始求解任务: " << task.taskId << std::endl;
        }
        
        // 1. 尺寸预测 - 对应icecuber的bruteSize
        auto stepStart = std::chrono::high_resolution_clock::now();
        auto outputSizes = predictOutputSizes(task.testInput, task.trainingExamples);
        auto stepEnd = std::chrono::high_resolution_clock::now();
        
        if (config_.printTimes) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stepEnd - stepStart);
            printProgress("尺寸预测", duration.count() / 1000.0);
        }
        
        // 2. 构建DAG和提取pieces - 对应icecuber的brutePieces2 + makePieces2
        stepStart = std::chrono::high_resolution_clock::now();
        auto pieces = buildPieces(task.testInput, task.trainingExamples, outputSizes);
        stepEnd = std::chrono::high_resolution_clock::now();
        
        result.totalPieces = pieces.getPieceCount();
        
        if (config_.printTimes) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stepEnd - stepStart);
            printProgress("Piece构建", duration.count() / 1000.0);
        }
        
        if (config_.printMemory) {
            printMemoryUsage(pieces);
        }
        
        // 3. 组合候选解 - 对应icecuber的composePieces2
        stepStart = std::chrono::high_resolution_clock::now();
        auto candidates = generateCandidates(pieces, task.trainingExamples, outputSizes);
        stepEnd = std::chrono::high_resolution_clock::now();
        
        result.totalCandidates = candidates.size();
        
        if (config_.printTimes) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stepEnd - stepStart);
            printProgress("候选解生成", duration.count() / 1000.0);
        }
        
        // 4. 评估和排序 - 对应icecuber的evaluateCands
        stepStart = std::chrono::high_resolution_clock::now();
        auto rankedCandidates = evaluateAndRank(std::move(candidates), task.trainingExamples);
        stepEnd = std::chrono::high_resolution_clock::now();
        
        if (config_.printTimes) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stepEnd - stepStart);
            printProgress("候选解评估", duration.count() / 1000.0);
        }
        
        // 5. 选择最佳答案 - 对应icecuber的答案过滤
        result.answers = selectBestAnswers(rankedCandidates);
        
        if (!rankedCandidates.empty()) {
            result.bestScore = rankedCandidates[0].score;
        }
        
        // 计算verdict
        result.verdict = calculateVerdict(result.answers, task);
        result.success = (result.verdict != SolveResult::Verdict::Nothing);
        
    } catch (const std::exception& e) {
        if (config_.printTimes) {
            std::cout << "求解过程中出现异常: " << e.what() << std::endl;
        }
        result.success = false;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    result.solvingTime = totalDuration.count() / 1000.0;
    
    updateStatistics(result);
    
    return result;
}

// 批量求解
std::vector<SolveResult> ARCSolver::solveBatch(const std::vector<ARCTask>& tasks) {
    std::vector<SolveResult> results;
    results.reserve(tasks.size());
    
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        if (config_.printTimes) {
            std::cout << "\n处理任务 " << (i + 1) << "/" << tasks.size() << std::endl;
        }
        
        auto result = solve(tasks[i]);
        results.push_back(result);
        
        if (config_.printTimes) {
            printResult(static_cast<int>(i), tasks[i].taskId, result);
        }
    }
    
    return results;
}

// 1. 尺寸预测 - 简化版的bruteSize
std::vector<arc::core::Point> ARCSolver::predictOutputSizes(
    const arc::core::Grid& testInput,
    const std::vector<ARCExample>& training
) {
    std::vector<arc::core::Point> outputSizes;
    
    // 收集训练样本的输出尺寸
    for (const auto& example : training) {
        outputSizes.push_back({example.output.width, example.output.height});
    }
    
    // 简单的尺寸预测策略
    if (!training.empty()) {
        // 策略1: 使用最常见的输出尺寸
        std::map<std::pair<int, int>, int> sizeCount;
        for (const auto& example : training) {
            sizeCount[{example.output.width, example.output.height}]++;
        }
        
        auto mostCommon = std::max_element(sizeCount.begin(), sizeCount.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        
        if (mostCommon != sizeCount.end()) {
            outputSizes.push_back({mostCommon->first.first, mostCommon->first.second});
        } else {
            // 备用策略：使用测试输入的尺寸
            outputSizes.push_back({testInput.width, testInput.height});
        }
    } else {
        // 没有训练样本时，使用测试输入尺寸
        outputSizes.push_back({testInput.width, testInput.height});
    }
    
    return outputSizes;
}

// 2. 构建DAG和提取pieces
arc::piece::PieceCollection ARCSolver::buildPieces(
    const arc::core::Grid& testInput,
    const std::vector<ARCExample>& training,
    const std::vector<arc::core::Point>& outputSizes
) {
    // 准备训练对
    std::vector<std::pair<arc::core::Grid, arc::core::Grid>> trainingPairs;
    for (const auto& example : training) {
        trainingPairs.emplace_back(example.input, example.output);
    }
    
    // 使用piece提取器构建pieces
    return pieceExtractor_->buildFromTraining(trainingPairs, testInput, outputSizes);
}

// 3. 组合候选解
std::vector<arc::candidate::Candidate> ARCSolver::generateCandidates(
    arc::piece::PieceCollection& pieces,
    const std::vector<ARCExample>& training,
    const std::vector<arc::core::Point>& outputSizes
) {
    // 准备训练对
    std::vector<std::pair<arc::core::Grid, arc::core::Grid>> trainingPairs;
    for (const auto& example : training) {
        trainingPairs.emplace_back(example.input, example.output);
    }
    
    // 使用候选解组合器生成候选解
    return candidateComposer_->composePieces(pieces, trainingPairs, outputSizes);
}

// 4. 评估和排序
std::vector<arc::candidate::Candidate> ARCSolver::evaluateAndRank(
    std::vector<arc::candidate::Candidate> candidates,
    const std::vector<ARCExample>& training
) {
    // 准备训练对
    std::vector<std::pair<arc::core::Grid, arc::core::Grid>> trainingPairs;
    for (const auto& example : training) {
        trainingPairs.emplace_back(example.input, example.output);
    }
    
    // 使用候选解组合器评估
    return candidateComposer_->evaluateCandidates(candidates, trainingPairs);
}

// 5. 选择最佳答案 - 对应icecuber的答案过滤逻辑
std::vector<arc::core::Grid> ARCSolver::selectBestAnswers(
    const std::vector<arc::candidate::Candidate>& rankedCandidates
) {
    std::vector<arc::core::Grid> answers;
    std::set<std::vector<std::uint8_t>> seenAnswers; // 去重
    
    for (const auto& candidate : rankedCandidates) {
        if (answers.size() >= config_.maxAnswers) {
            break; // 对应icecuber的assert(answers.size() <= 3)
        }
        
        if (!candidate.images.empty()) {
            const auto& answer = candidate.images.back();
            
            // 验证答案有效性（对应icecuber的验证逻辑）
            if (answer.width > 0 && answer.height > 0 && 
                answer.width <= config_.maxSide && answer.height <= config_.maxSide &&
                answer.width * answer.height <= config_.maxArea) {
                
                // 去重检查
                if (seenAnswers.find(answer.pixels) == seenAnswers.end()) {
                    answers.push_back(answer);
                    seenAnswers.insert(answer.pixels);
                }
            }
        }
    }
    
    return answers;
}

// 辅助函数实现

void ARCSolver::updateStatistics(const SolveResult& result) {
    statistics_.totalTasks++;
    statistics_.totalTime += result.solvingTime;
    statistics_.averageSolvingTime = statistics_.totalTime / statistics_.totalTasks;
    
    switch (result.verdict) {
        case SolveResult::Verdict::Correct:
            statistics_.correctSolutions++;
            break;
        case SolveResult::Verdict::Candidate:
            statistics_.candidateSolutions++;
            break;
        case SolveResult::Verdict::Dimensions:
            statistics_.dimensionMatches++;
            break;
        default:
            break;
    }
}

SolveResult::Verdict ARCSolver::calculateVerdict(
    const std::vector<arc::core::Grid>& answers,
    const ARCTask& task
) {
    if (answers.empty()) {
        return SolveResult::Verdict::Nothing;
    }
    
    if (!task.hasTestOutput()) {
        return SolveResult::Verdict::Candidate; // 无法验证正确性
    }
    
    // 检查是否有完全正确的答案
    for (const auto& answer : answers) {
        if (answer.width == task.testOutput.width && 
            answer.height == task.testOutput.height &&
            answer.pixels == task.testOutput.pixels) {
            return SolveResult::Verdict::Correct;
        }
    }
    
    // 检查是否有尺寸正确的答案
    for (const auto& answer : answers) {
        if (answer.width == task.testOutput.width && 
            answer.height == task.testOutput.height) {
            return SolveResult::Verdict::Dimensions;
        }
    }
    
    return SolveResult::Verdict::Candidate;
}

void ARCSolver::printProgress(const std::string& stage, double elapsedTime) {
    std::cout << "  " << stage << ": " << elapsedTime << "s" << std::endl;
}

void ARCSolver::printMemoryUsage(const arc::piece::PieceCollection& pieces) {
    auto stats = pieces.getStatistics();
    std::cout << "内存使用: " 
              << "节点=" << stats.totalNodes 
              << ", pieces=" << stats.totalPieces 
              << ", 内存=" << (stats.memoryUsage / 1024.0 / 1024.0) << "MB" 
              << std::endl;
}

// ============================================================================
// TaskLoader 实现
// ============================================================================

ARCTask TaskLoader::createTestTask(
    const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& examples,
    const arc::core::Grid& testInput,
    const arc::core::Grid& testOutput
) {
    ARCTask task;
    task.taskId = "test_task";
    
    for (const auto& [input, output] : examples) {
        task.trainingExamples.emplace_back(input, output);
    }
    
    task.testInput = testInput;
    task.testOutput = testOutput;
    
    return task;
}

// 简化的JSON加载（实际项目中应使用JSON库）
ARCTask TaskLoader::loadFromJson(const std::string& jsonStr) {
    // 这里应该使用真正的JSON解析库
    // 为简化起见，创建一个测试任务
    ARCTask task;
    task.taskId = "json_task";
    
    // 创建简单的测试数据
    arc::core::Grid input(2, 2);
    input(0, 0) = 1; input(0, 1) = 0;
    input(1, 0) = 0; input(1, 1) = 1;
    
    arc::core::Grid output(2, 2);
    output(0, 0) = 2; output(0, 1) = 0;
    output(1, 0) = 0; output(1, 1) = 2;
    
    task.trainingExamples.emplace_back(input, output);
    task.testInput = input;
    task.testOutput = output;
    
    return task;
}

ARCTask TaskLoader::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + filepath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromJson(buffer.str());
}

std::vector<ARCTask> TaskLoader::loadFromDirectory(const std::string& dirPath) {
    // 简化实现：返回单个测试任务
    std::vector<ARCTask> tasks;
    tasks.push_back(loadFromJson("{}"));
    return tasks;
}

// ============================================================================
// SolverFactory 实现
// ============================================================================

std::unique_ptr<ARCSolver> SolverFactory::createDefault() {
    SolverConfig config;
    config.maxDepth = 20;
    config.maxCandidates = 1000;
    config.printTimes = true;
    return std::make_unique<ARCSolver>(config);
}

std::unique_ptr<ARCSolver> SolverFactory::createFast() {
    SolverConfig config;
    config.maxDepth = 10;
    config.maxCandidates = 100;
    config.maxIterations = 5;
    config.printTimes = false;
    return std::make_unique<ARCSolver>(config);
}

std::unique_ptr<ARCSolver> SolverFactory::createAccurate() {
    SolverConfig config;
    config.maxDepth = 30;
    config.maxCandidates = 5000;
    config.maxIterations = 20;
    config.printTimes = true;
    config.printMemory = true;
    return std::make_unique<ARCSolver>(config);
}

std::unique_ptr<ARCSolver> SolverFactory::createFromConfig(const SolverConfig& config) {
    return std::make_unique<ARCSolver>(config);
}

// ============================================================================
// 辅助函数实现
// ============================================================================

std::string colorGreen(const std::string& text) {
    return "\033[1;32m" + text + "\033[0m";
}

std::string colorBlue(const std::string& text) {
    return "\033[1;34m" + text + "\033[0m";
}

std::string colorYellow(const std::string& text) {
    return "\033[1;33m" + text + "\033[0m";
}

std::string colorRed(const std::string& text) {
    return "\033[1;31m" + text + "\033[0m";
}

void printResult(int taskIndex, const std::string& taskId, const SolveResult& result) {
    std::cout << "任务 #" << taskIndex << " (" << taskId << "): ";
    
    switch (result.verdict) {
        case SolveResult::Verdict::Correct:
            std::cout << colorGreen("正确") << std::endl;
            break;
        case SolveResult::Verdict::Candidate:
            std::cout << colorYellow("候选解") << std::endl;
            break;
        case SolveResult::Verdict::Dimensions:
            std::cout << colorBlue("尺寸匹配") << std::endl;
            break;
        case SolveResult::Verdict::Nothing:
            std::cout << colorRed("无解") << std::endl;
            break;
    }
    
    std::cout << "  用时: " << result.solvingTime << "s, "
              << "Pieces: " << result.totalPieces << ", "
              << "候选解: " << result.totalCandidates << ", "
              << "答案: " << result.answers.size() << std::endl;
}

void printStatistics(const ARCSolver::Statistics& stats) {
    std::cout << "\n=== 求解统计 ===" << std::endl;
    std::cout << "总任务数: " << stats.totalTasks << std::endl;
    std::cout << "正确解: " << stats.correctSolutions 
              << " (" << (100.0 * stats.correctSolutions / stats.totalTasks) << "%)" << std::endl;
    std::cout << "候选解: " << stats.candidateSolutions 
              << " (" << (100.0 * stats.candidateSolutions / stats.totalTasks) << "%)" << std::endl;
    std::cout << "尺寸匹配: " << stats.dimensionMatches 
              << " (" << (100.0 * stats.dimensionMatches / stats.totalTasks) << "%)" << std::endl;
    std::cout << "平均用时: " << stats.averageSolvingTime << "s" << std::endl;
    std::cout << "总用时: " << stats.totalTime << "s" << std::endl;
}

} // namespace arc::solver 