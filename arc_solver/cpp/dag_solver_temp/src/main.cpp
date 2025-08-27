#include <iostream>
#include <cstdlib>
#include <string>
#include "solver.hpp"

using namespace arc::solver;

void printUsage(const char* programName) {
    std::cout << "用法: " << programName << " [选项]" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -h, --help     显示帮助信息" << std::endl;
    std::cout << "  -d DEPTH       设置最大搜索深度 (默认: 20)" << std::endl;
    std::cout << "  -f, --fast     使用快速模式" << std::endl;
    std::cout << "  -a, --accurate 使用高精度模式" << std::endl;
    std::cout << "  -t, --times    显示计时信息" << std::endl;
    std::cout << "  -m, --memory   显示内存使用信息" << std::endl;
    std::cout << "  --demo         运行演示" << std::endl;
}

ARCTask createDemoTask() {
    // 创建一个简单的演示任务
    arc::core::Grid input1(2, 2);
    input1(0, 0) = 1; input1(0, 1) = 0;
    input1(1, 0) = 0; input1(1, 1) = 1;
    
    arc::core::Grid output1(2, 2);
    output1(0, 0) = 2; output1(0, 1) = 0;
    output1(1, 0) = 0; output1(1, 1) = 2;
    
    arc::core::Grid input2(2, 2);
    input2(0, 0) = 3; input2(0, 1) = 0;
    input2(1, 0) = 0; input2(1, 1) = 3;
    
    arc::core::Grid output2(2, 2);
    output2(0, 0) = 4; output2(0, 1) = 0;
    output2(1, 0) = 0; output2(1, 1) = 4;
    
    arc::core::Grid testInput(2, 2);
    testInput(0, 0) = 5; testInput(0, 1) = 0;
    testInput(1, 0) = 0; testInput(1, 1) = 5;
    
    arc::core::Grid testOutput(2, 2);
    testOutput(0, 0) = 6; testOutput(0, 1) = 0;
    testOutput(1, 0) = 0; testOutput(1, 1) = 6;
    
    std::vector<std::pair<arc::core::Grid, arc::core::Grid>> examples = {
        {input1, output1},
        {input2, output2}
    };
    
    return TaskLoader::createTestTask(examples, testInput, testOutput);
}

void runDemo() {
    std::cout << "=== ARC DAG Solver 演示 ===" << std::endl;
    
    // 创建演示任务
    auto task = createDemoTask();
    
    std::cout << "\n创建的演示任务:" << std::endl;
    std::cout << "- 任务ID: " << task.taskId << std::endl;
    std::cout << "- 训练样本数: " << task.getTrainingCount() << std::endl;
    std::cout << "- 测试输入尺寸: " << task.testInput.width << "x" << task.testInput.height << std::endl;
    
    // 显示任务内容
    for (std::size_t i = 0; i < task.trainingExamples.size(); ++i) {
        const auto& example = task.trainingExamples[i];
        std::cout << "\n训练样本 " << (i+1) << ":" << std::endl;
        std::cout << "输入: ";
        for (int y = 0; y < example.input.height; ++y) {
            for (int x = 0; x < example.input.width; ++x) {
                std::cout << static_cast<int>(example.input(y, x)) << " ";
            }
            std::cout << (y < example.input.height-1 ? "| " : "");
        }
        std::cout << " -> 输出: ";
        for (int y = 0; y < example.output.height; ++y) {
            for (int x = 0; x < example.output.width; ++x) {
                std::cout << static_cast<int>(example.output(y, x)) << " ";
            }
            std::cout << (y < example.output.height-1 ? "| " : "");
        }
        std::cout << std::endl;
    }
    
    std::cout << "\n测试输入: ";
    for (int y = 0; y < task.testInput.height; ++y) {
        for (int x = 0; x < task.testInput.width; ++x) {
            std::cout << static_cast<int>(task.testInput(y, x)) << " ";
        }
        std::cout << (y < task.testInput.height-1 ? "| " : "");
    }
    std::cout << std::endl;
    
    // 创建求解器并求解
    std::cout << "\n开始求解..." << std::endl;
    auto solver = SolverFactory::createDefault();
    auto result = solver->solve(task);
    
    // 显示结果
    std::cout << "\n=== 求解结果 ===" << std::endl;
    printResult(0, task.taskId, result);
    
    if (result.hasAnswers()) {
        std::cout << "\n预测的答案:" << std::endl;
        for (std::size_t i = 0; i < result.answers.size(); ++i) {
            const auto& answer = result.answers[i];
            std::cout << "答案 " << (i+1) << ": ";
            for (int y = 0; y < answer.height; ++y) {
                for (int x = 0; x < answer.width; ++x) {
                    std::cout << static_cast<int>(answer(y, x)) << " ";
                }
                std::cout << (y < answer.height-1 ? "| " : "");
            }
            std::cout << std::endl;
        }
        
        // 验证答案
        if (task.hasTestOutput()) {
            std::cout << "\n正确答案: ";
            for (int y = 0; y < task.testOutput.height; ++y) {
                for (int x = 0; x < task.testOutput.width; ++x) {
                    std::cout << static_cast<int>(task.testOutput(y, x)) << " ";
                }
                std::cout << (y < task.testOutput.height-1 ? "| " : "");
            }
            std::cout << std::endl;
            
            if (result.verdict == SolveResult::Verdict::Correct) {
                std::cout << colorGreen("✓ 求解成功！") << std::endl;
            } else {
                std::cout << colorYellow("! 求解部分成功") << std::endl;
            }
        }
    } else {
        std::cout << colorRed("✗ 未找到答案") << std::endl;
    }
    
    // 显示统计信息
    auto stats = solver->getStatistics();
    printStatistics(stats);
}

int main(int argc, char* argv[]) {
    std::cout << "ARC DAG Solver - 基于icecuber核心逻辑" << std::endl;
    
    // 解析命令行参数
    SolverConfig config;
    bool showHelp = false;
    bool runDemoMode = false;
    bool fastMode = false;
    bool accurateMode = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            showHelp = true;
        } else if (arg == "-d" && i + 1 < argc) {
            config.maxDepth = std::atoi(argv[++i]);
            std::cout << "设置最大深度: " << config.maxDepth << std::endl;
        } else if (arg == "-f" || arg == "--fast") {
            fastMode = true;
        } else if (arg == "-a" || arg == "--accurate") {
            accurateMode = true;
        } else if (arg == "-t" || arg == "--times") {
            config.printTimes = true;
        } else if (arg == "-m" || arg == "--memory") {
            config.printMemory = true;
        } else if (arg == "--demo") {
            runDemoMode = true;
        } else {
            std::cout << "未知参数: " << arg << std::endl;
            showHelp = true;
        }
    }
    
    if (showHelp) {
        printUsage(argv[0]);
        return 0;
    }
    
    if (runDemoMode) {
        runDemo();
        return 0;
    }
    
    // 创建求解器
    std::unique_ptr<ARCSolver> solver;
    
    if (fastMode) {
        std::cout << "使用快速模式" << std::endl;
        solver = SolverFactory::createFast();
    } else if (accurateMode) {
        std::cout << "使用高精度模式" << std::endl;
        solver = SolverFactory::createAccurate();
    } else {
        std::cout << "使用默认模式" << std::endl;
        solver = SolverFactory::createFromConfig(config);
    }
    
    // 如果没有指定演示模式，运行默认演示
    if (!runDemoMode) {
        std::cout << "\n未指定输入文件，运行默认演示" << std::endl;
        std::cout << "使用 --demo 参数查看详细演示，使用 -h 查看帮助" << std::endl;
        
        // 运行简单测试
        auto task = createDemoTask();
        auto result = solver->solve(task);
        
        std::cout << "\n快速测试结果:" << std::endl;
        printResult(0, task.taskId, result);
        
        auto stats = solver->getStatistics();
        printStatistics(stats);
    }
    
    return 0;
} 