#pragma once
#include <vector>
#include <functional>
#include <memory>
#include "candidate/candidate.hpp"
#include "piece/piece.hpp"
#include "core/state.hpp"

namespace arc::scoring {

// ============================================================================
// 基础评分函数 - 对应icecuber的核心评分逻辑
// ============================================================================

// 像素差异计算 - 基础的像素级比较
float pixelDiff(const arc::core::Grid& predicted, const arc::core::Grid& target);

// 形状奖励 - 对应icecuber对正确形状的奖励
float shapeBonus(const arc::core::Grid& predicted, const arc::core::Grid& target);

// 尺寸匹配奖励
float sizeMatchBonus(const arc::core::Grid& predicted, const arc::core::Grid& target);

// 颜色分布相似度
float colorDistributionSimilarity(const arc::core::Grid& predicted, const arc::core::Grid& target);

// ============================================================================
// 候选解评分器 - 对应icecuber的scoreCands
// ============================================================================

class CandidateScorer {
public:
    // 评分配置
    struct Config {
        float pixelWeight = 1.0f;           // 像素匹配权重
        float shapeWeight = 0.5f;           // 形状匹配权重
        float sizeWeight = 0.3f;            // 尺寸匹配权重
        float colorWeight = 0.2f;           // 颜色分布权重
        float complexityPenalty = 0.01f;    // 复杂度惩罚系数 - 对应icecuber的0.01
        float priorWeight = 1e-3f;          // 先验权重 - 对应icecuber的1e-3
        bool enableNormalization = true;    // 启用分数归一化
        bool penalizeInvalidAnswers = true; // 惩罚无效答案
    };

    CandidateScorer(const Config& config = {});

    // 主要评分函数 - 对应icecuber的scoreCands
    int scoreCandidates(
        const std::vector<arc::candidate::Candidate>& candidates,
        const arc::core::Grid& testInput,
        const arc::core::Grid& testOutput
    );

    // 训练集匹配评分 - 对应icecuber的goods计算
    float scoreTrainingMatch(
        const arc::candidate::Candidate& candidate,
        const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs
    );

    // 单个候选解评分 - 综合评分函数
    float scoreSingleCandidate(
        const arc::candidate::Candidate& candidate,
        const arc::core::Grid& target,
        const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs = {}
    );

    // 答案验证 - 对应icecuber的答案有效性检查
    bool validateAnswer(const arc::core::Grid& answer) const;

    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config) { config_ = config; }

private:
    Config config_;

    // 计算复杂度分数 - 对应icecuber的prior计算
    float calculateComplexityScore(const arc::candidate::Candidate& candidate) const;

    // 计算像素级精确匹配分数
    float calculatePixelMatchScore(const arc::core::Grid& predicted, const arc::core::Grid& target) const;

    // 计算结构相似度分数
    float calculateStructuralSimilarity(const arc::core::Grid& predicted, const arc::core::Grid& target) const;
};

// ============================================================================
// 答案评分器 - 对应icecuber的scoreAnswers
// ============================================================================

class AnswerScorer {
public:
    // 多答案评分 - 对应icecuber的scoreAnswers
    int scoreAnswers(
        const std::vector<arc::core::Grid>& answers,
        const arc::core::Grid& testInput,
        const arc::core::Grid& testOutput
    );

    // 精确匹配检查 - 对应icecuber的精确比较
    bool exactMatch(const arc::core::Grid& answer, const arc::core::Grid& target) const;

    // 部分匹配评分 - 扩展功能
    float partialMatch(const arc::core::Grid& answer, const arc::core::Grid& target) const;

private:
    // 尺寸和内容双重验证
    bool sizeAndContentMatch(const arc::core::Grid& answer, const arc::core::Grid& target) const;
};

// ============================================================================
// Piece评分器 - 对应icecuber的scorePieces（虽然代码中没有实现）
// ============================================================================

class PieceScorer {
public:
    // 评分配置
    struct Config {
        float qualityWeight = 1.0f;         // 质量权重
        float depthPenalty = 0.05f;         // 深度惩罚
        float diversityBonus = 0.1f;        // 多样性奖励
        bool favorLowDepth = true;          // 偏好低深度pieces
    };

    PieceScorer(const Config& config = {});

    // Pieces集合评分
    float scorePieces(
        const arc::piece::PieceCollection& pieces,
        const arc::core::Grid& testInput,
        const arc::core::Grid& testOutput,
        const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs
    );

    // 单个piece质量评分
    float scoreSinglePiece(
        const arc::piece::Piece& piece,
        const arc::piece::PieceCollection& collection,
        std::size_t pieceIndex
    );

    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config) { config_ = config; }

private:
    Config config_;

    // 计算piece的覆盖度
    float calculateCoverage(
        const arc::piece::Piece& piece,
        const arc::piece::PieceCollection& collection,
        std::size_t pieceIndex,
        const std::vector<arc::core::Grid>& targets
    );

    // 计算piece的多样性
    float calculateDiversity(
        const arc::piece::Piece& piece,
        const arc::piece::PieceCollection& collection,
        std::size_t pieceIndex
    );
};

// ============================================================================
// 综合评分系统 - 整合所有评分组件
// ============================================================================

class IntegratedScorer {
public:
    // 系统配置
    struct Config {
        CandidateScorer::Config candidateConfig;
        PieceScorer::Config pieceConfig;
        
        float candidateWeight = 0.7f;       // 候选解权重
        float pieceWeight = 0.3f;           // Piece权重
        bool enableMultiObjective = true;   // 启用多目标优化
        std::size_t maxReturnedAnswers = 3; // 最大返回答案数 - 对应icecuber的assert
    };

    IntegratedScorer(const Config& config = {});

    // 综合评分和排序
    std::vector<arc::candidate::Candidate> scoreAndRank(
        std::vector<arc::candidate::Candidate> candidates,
        const arc::core::Grid& testInput,
        const arc::core::Grid& testOutput,
        const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs,
        const arc::piece::PieceCollection* pieces = nullptr
    );

    // 获取最佳答案 - 对应icecuber的返回逻辑
    std::vector<arc::core::Grid> getBestAnswers(
        const std::vector<arc::candidate::Candidate>& rankedCandidates,
        std::size_t maxAnswers = 3
    );

    // 评分统计信息
    struct ScoringStatistics {
        float bestScore = 0.0f;
        float averageScore = 0.0f;
        float scoreVariance = 0.0f;
        std::size_t validCandidates = 0;
        std::size_t totalCandidates = 0;
        std::size_t exactMatches = 0;
    };

    ScoringStatistics getLastScoringStatistics() const { return lastStats_; }

    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config) { config_ = config; }

private:
    Config config_;
    CandidateScorer candidateScorer_;
    AnswerScorer answerScorer_;
    PieceScorer pieceScorer_;
    ScoringStatistics lastStats_;

    // 计算评分统计
    void calculateStatistics(const std::vector<arc::candidate::Candidate>& candidates);

    // 多目标评分融合
    float fuseMultiObjectiveScores(
        float candidateScore,
        float pieceScore,
        const arc::candidate::Candidate& candidate
    );
};

// ============================================================================
// 评分策略 - 高级评分算法
// ============================================================================

class AdvancedScoringStrategy {
public:
    // 策略类型
    enum class StrategyType {
        ExactMatch,      // 精确匹配 - 对应icecuber的基础策略
        StructuralSim,   // 结构相似度
        ProgressiveEval, // 渐进式评估
        EnsembleScoring  // 集成评分
    };

    // 策略配置
    struct Config {
        StrategyType primaryStrategy = StrategyType::ExactMatch;
        std::vector<StrategyType> fallbackStrategies = {
            StrategyType::StructuralSim,
            StrategyType::ProgressiveEval
        };
        float strategyBlendWeight = 0.3f;   // 策略融合权重
        bool enableAdaptiveWeighting = true; // 自适应权重调整
    };

    AdvancedScoringStrategy(const Config& config = {});

    // 高级评分
    float advancedScore(
        const arc::candidate::Candidate& candidate,
        const arc::core::Grid& target,
        const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs
    );

    // 策略特定评分
    float scoreWithStrategy(
        const arc::candidate::Candidate& candidate,
        const arc::core::Grid& target,
        StrategyType strategy
    );

private:
    Config config_;

    // 各种评分策略实现
    float exactMatchScore(const arc::candidate::Candidate& candidate, const arc::core::Grid& target);
    float structuralSimilarityScore(const arc::candidate::Candidate& candidate, const arc::core::Grid& target);
    float progressiveEvalScore(const arc::candidate::Candidate& candidate, const arc::core::Grid& target);
    float ensembleScore(const arc::candidate::Candidate& candidate, const arc::core::Grid& target);
};

// ============================================================================
// 辅助函数
// ============================================================================

// 创建默认评分器
std::unique_ptr<IntegratedScorer> createDefaultScorer();

// 快速评分函数 - 对应icecuber的简单接口
float quickScore(
    const arc::candidate::Candidate& candidate,
    const arc::core::Grid& target
);

// 批量评分
std::vector<float> batchScore(
    const std::vector<arc::candidate::Candidate>& candidates,
    const arc::core::Grid& target
);

// 评分结果验证
bool validateScoringResults(
    const std::vector<arc::candidate::Candidate>& candidates,
    const std::vector<float>& scores
);

// 打印评分统计
void printScoringStatistics(const IntegratedScorer::ScoringStatistics& stats);

} // namespace arc::scoring 