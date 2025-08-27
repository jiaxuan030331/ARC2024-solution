#include "scoring/score.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <iostream>
#include <unordered_map>

namespace arc::scoring {

// ============================================================================
// 基础评分函数实现
// ============================================================================

float pixelDiff(const arc::core::Grid& predicted, const arc::core::Grid& target) {
    if (predicted.width != target.width || predicted.height != target.height) {
        return 0.0f; // 尺寸不匹配，完全不相似
    }
    
    int totalPixels = predicted.width * predicted.height;
    if (totalPixels == 0) return 1.0f;
    
    int matchingPixels = 0;
    for (int i = 0; i < totalPixels; ++i) {
        if (predicted.pixels[i] == target.pixels[i]) {
            matchingPixels++;
        }
    }
    
    return static_cast<float>(matchingPixels) / totalPixels;
}

float shapeBonus(const arc::core::Grid& predicted, const arc::core::Grid& target) {
    // 基于非零像素的形状匹配
    if (predicted.width != target.width || predicted.height != target.height) {
        return 0.0f;
    }
    
    int totalNonZero = 0;
    int matchingShape = 0;
    
    for (int i = 0; i < predicted.height; ++i) {
        for (int j = 0; j < predicted.width; ++j) {
            bool predNonZero = predicted(i, j) != 0;
            bool targNonZero = target(i, j) != 0;
            
            if (targNonZero) totalNonZero++;
            if (predNonZero && targNonZero) matchingShape++;
        }
    }
    
    return totalNonZero > 0 ? static_cast<float>(matchingShape) / totalNonZero : 1.0f;
}

float sizeMatchBonus(const arc::core::Grid& predicted, const arc::core::Grid& target) {
    if (predicted.width == target.width && predicted.height == target.height) {
        return 1.0f;
    }
    
    // 计算尺寸相似度
    float widthRatio = std::min(predicted.width, target.width) / 
                      static_cast<float>(std::max(predicted.width, target.width));
    float heightRatio = std::min(predicted.height, target.height) / 
                       static_cast<float>(std::max(predicted.height, target.height));
    
    return (widthRatio + heightRatio) / 2.0f;
}

float colorDistributionSimilarity(const arc::core::Grid& predicted, const arc::core::Grid& target) {
    std::unordered_map<int, int> predCounts, targCounts;
    
    // 统计颜色分布
    for (std::uint8_t pixel : predicted.pixels) {
        predCounts[pixel]++;
    }
    for (std::uint8_t pixel : target.pixels) {
        targCounts[pixel]++;
    }
    
    // 计算分布相似度
    float similarity = 0.0f;
    float totalPred = predicted.width * predicted.height;
    float totalTarg = target.width * target.height;
    
    for (int color = 0; color < 10; ++color) {
        float predRatio = predCounts[color] / totalPred;
        float targRatio = targCounts[color] / totalTarg;
        similarity += 1.0f - std::abs(predRatio - targRatio);
    }
    
    return similarity / 10.0f;
}

// ============================================================================
// CandidateScorer 实现
// ============================================================================

CandidateScorer::CandidateScorer(const Config& config) : config_(config) {}

// 对应icecuber的scoreCands
int CandidateScorer::scoreCandidates(
    const std::vector<arc::candidate::Candidate>& candidates,
    const arc::core::Grid& testInput,
    const arc::core::Grid& testOutput
) {
    // 对应icecuber的精确匹配检查：if (cand.imgs.back() == test_out) return 1;
    for (const auto& candidate : candidates) {
        if (!candidate.images.empty()) {
            const arc::core::Grid& answer = candidate.images.back();
            
            // 精确匹配检查
            if (answer.width == testOutput.width && 
                answer.height == testOutput.height &&
                answer.pixels == testOutput.pixels) {
                return 1; // 找到精确匹配
            }
        }
    }
    
    return 0; // 没有找到精确匹配
}

float CandidateScorer::scoreTrainingMatch(
    const arc::candidate::Candidate& candidate,
    const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs
) {
    if (candidate.images.size() < trainingPairs.size()) {
        return 0.0f;
    }
    
    int matches = 0;
    
    // 对应icecuber的goods计算：goods += (imgs[i] == train[i].second);
    for (std::size_t i = 0; i < trainingPairs.size(); ++i) {
        const arc::core::Grid& candidateOutput = candidate.images[i];
        const arc::core::Grid& expectedOutput = trainingPairs[i].second;
        
        if (candidateOutput.width == expectedOutput.width &&
            candidateOutput.height == expectedOutput.height &&
            candidateOutput.pixels == expectedOutput.pixels) {
            matches++;
        }
    }
    
    return static_cast<float>(matches);
}

float CandidateScorer::calculateComplexityScore(const arc::candidate::Candidate& candidate) const {
    // 对应icecuber的prior计算：cand.max_depth+cand.cnt_pieces*1e-3
    if (candidate.maxDepth >= 0 && candidate.pieceCount >= 0) {
        return candidate.maxDepth + candidate.pieceCount * config_.priorWeight;
    }
    return 0.0f;
}

float CandidateScorer::calculatePixelMatchScore(const arc::core::Grid& predicted, const arc::core::Grid& target) const {
    return pixelDiff(predicted, target);
}

float CandidateScorer::calculateStructuralSimilarity(const arc::core::Grid& predicted, const arc::core::Grid& target) const {
    float shapeScore = shapeBonus(predicted, target);
    float sizeScore = sizeMatchBonus(predicted, target);
    float colorScore = colorDistributionSimilarity(predicted, target);
    
    return (shapeScore * config_.shapeWeight + 
            sizeScore * config_.sizeWeight + 
            colorScore * config_.colorWeight) / 
           (config_.shapeWeight + config_.sizeWeight + config_.colorWeight);
}

bool CandidateScorer::validateAnswer(const arc::core::Grid& answer) const {
    // 对应icecuber的答案验证：answer.w > 30 || answer.h > 30 || answer.w*answer.h == 0
    if (answer.width > 30 || answer.height > 30 || answer.width * answer.height == 0) {
        return false;
    }
    
    // 对应icecuber的像素值检查：answer(i,j) < 0 || answer(i,j) >= 10
    for (std::uint8_t pixel : answer.pixels) {
        if (pixel >= 10) {
            return false;
        }
    }
    
    return true;
}

float CandidateScorer::scoreSingleCandidate(
    const arc::candidate::Candidate& candidate,
    const arc::core::Grid& target,
    const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs
) {
    if (candidate.images.empty()) {
        return -1000.0f; // 无效候选解
    }
    
    const arc::core::Grid& answer = candidate.images.back();
    
    // 验证答案有效性
    if (!validateAnswer(answer)) {
        return -1000.0f;
    }
    
    // 计算训练匹配分数
    float trainingMatches = 0.0f;
    if (!trainingPairs.empty()) {
        trainingMatches = scoreTrainingMatch(candidate, trainingPairs);
    }
    
    // 计算复杂度惩罚
    float complexityPenalty = calculateComplexityScore(candidate);
    
    // 对应icecuber的最终分数计算：score = goods-prior*0.01
    float finalScore = trainingMatches - complexityPenalty * config_.complexityPenalty;
    
    return finalScore;
}

// ============================================================================
// AnswerScorer 实现
// ============================================================================

int AnswerScorer::scoreAnswers(
    const std::vector<arc::core::Grid>& answers,
    const arc::core::Grid& testInput,
    const arc::core::Grid& testOutput
) {
    // 对应icecuber的assert：assert(answers.size() <= 3);
    if (answers.size() > 3) {
        std::cerr << "Warning: Too many answers provided (expected <= 3)" << std::endl;
    }
    
    // 对应icecuber的循环检查：for (Image_ answer : answers)
    for (const auto& answer : answers) {
        if (sizeAndContentMatch(answer, testOutput)) {
            return 1; // 找到匹配
        }
    }
    
    return 0; // 没有找到匹配
}

bool AnswerScorer::exactMatch(const arc::core::Grid& answer, const arc::core::Grid& target) const {
    return answer.width == target.width &&
           answer.height == target.height &&
           answer.pixels == target.pixels;
}

bool AnswerScorer::sizeAndContentMatch(const arc::core::Grid& answer, const arc::core::Grid& target) const {
    // 对应icecuber的检查：answer.sz == test_out.sz && answer.mask == test_out.mask
    return exactMatch(answer, target);
}

float AnswerScorer::partialMatch(const arc::core::Grid& answer, const arc::core::Grid& target) const {
    if (answer.width != target.width || answer.height != target.height) {
        return 0.0f;
    }
    
    return pixelDiff(answer, target);
}

// ============================================================================
// PieceScorer 实现
// ============================================================================

PieceScorer::PieceScorer(const Config& config) : config_(config) {}

float PieceScorer::scorePieces(
    const arc::piece::PieceCollection& pieces,
    const arc::core::Grid& testInput,
    const arc::core::Grid& testOutput,
    const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs
) {
    if (pieces.getPieceCount() == 0) {
        return 0.0f;
    }
    
    float totalScore = 0.0f;
    std::size_t validPieces = 0;
    
    for (std::size_t i = 0; i < pieces.getPieceCount(); ++i) {
        float pieceScore = scoreSinglePiece(pieces.pieces[i], pieces, i);
        if (pieceScore > 0) {
            totalScore += pieceScore;
            validPieces++;
        }
    }
    
    return validPieces > 0 ? totalScore / validPieces : 0.0f;
}

float PieceScorer::scoreSinglePiece(
    const arc::piece::Piece& piece,
    const arc::piece::PieceCollection& collection,
    std::size_t pieceIndex
) {
    float score = config_.qualityWeight;
    
    // 深度惩罚
    if (config_.favorLowDepth) {
        score -= piece.depth * config_.depthPenalty;
    }
    
    // 多样性奖励
    float diversity = calculateDiversity(piece, collection, pieceIndex);
    score += diversity * config_.diversityBonus;
    
    return std::max(0.0f, score);
}

float PieceScorer::calculateCoverage(
    const arc::piece::Piece& piece,
    const arc::piece::PieceCollection& collection,
    std::size_t pieceIndex,
    const std::vector<arc::core::Grid>& targets
) {
    // 简化的覆盖度计算
    try {
        std::size_t totalCoverage = 0;
        for (std::size_t dagIdx = 0; dagIdx < collection.getDAGCount(); ++dagIdx) {
            const arc::core::Grid& pieceImage = collection.getPieceImage(pieceIndex, dagIdx);
            totalCoverage += pieceImage.width * pieceImage.height;
        }
        return static_cast<float>(totalCoverage);
    } catch (const std::exception&) {
        return 0.0f;
    }
}

float PieceScorer::calculateDiversity(
    const arc::piece::Piece& piece,
    const arc::piece::PieceCollection& collection,
    std::size_t pieceIndex
) {
    // 基于深度的多样性评估
    float diversityScore = 1.0f;
    
    // 检查与其他pieces的深度差异
    for (std::size_t i = 0; i < collection.getPieceCount(); ++i) {
        if (i != pieceIndex) {
            float depthDiff = std::abs(static_cast<float>(piece.depth) - 
                                     static_cast<float>(collection.pieces[i].depth));
            diversityScore += depthDiff * 0.1f;
        }
    }
    
    return diversityScore / collection.getPieceCount();
}

// ============================================================================
// IntegratedScorer 实现
// ============================================================================

IntegratedScorer::IntegratedScorer(const Config& config) 
    : config_(config), 
      candidateScorer_(config.candidateConfig),
      pieceScorer_(config.pieceConfig) {}

std::vector<arc::candidate::Candidate> IntegratedScorer::scoreAndRank(
    std::vector<arc::candidate::Candidate> candidates,
    const arc::core::Grid& testInput,
    const arc::core::Grid& testOutput,
    const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs,
    const arc::piece::PieceCollection* pieces
) {
    // 为每个候选解计算综合分数
    for (auto& candidate : candidates) {
        float candidateScore = candidateScorer_.scoreSingleCandidate(candidate, testOutput, trainingPairs);
        
        float pieceScore = 0.0f;
        if (pieces != nullptr) {
            pieceScore = pieceScorer_.scorePieces(*pieces, testInput, testOutput, trainingPairs);
        }
        
        if (config_.enableMultiObjective) {
            candidate.score = fuseMultiObjectiveScores(candidateScore, pieceScore, candidate);
        } else {
            candidate.score = candidateScore;
        }
    }
    
    // 排序 - 分数越高越好
    std::sort(candidates.begin(), candidates.end());
    
    // 计算统计信息
    calculateStatistics(candidates);
    
    return candidates;
}

std::vector<arc::core::Grid> IntegratedScorer::getBestAnswers(
    const std::vector<arc::candidate::Candidate>& rankedCandidates,
    std::size_t maxAnswers
) {
    std::vector<arc::core::Grid> answers;
    
    std::size_t count = std::min(maxAnswers, rankedCandidates.size());
    count = std::min(count, config_.maxReturnedAnswers); // 对应icecuber的assert(answers.size() <= 3)
    
    for (std::size_t i = 0; i < count; ++i) {
        const auto& candidate = rankedCandidates[i];
        if (!candidate.images.empty() && candidateScorer_.validateAnswer(candidate.images.back())) {
            answers.push_back(candidate.images.back());
        }
    }
    
    return answers;
}

void IntegratedScorer::calculateStatistics(const std::vector<arc::candidate::Candidate>& candidates) {
    lastStats_ = ScoringStatistics{};
    lastStats_.totalCandidates = candidates.size();
    
    if (candidates.empty()) return;
    
    float totalScore = 0.0f;
    float minScore = candidates[0].score;
    float maxScore = candidates[0].score;
    
    for (const auto& candidate : candidates) {
        if (candidateScorer_.validateAnswer(candidate.images.back())) {
            lastStats_.validCandidates++;
        }
        
        totalScore += candidate.score;
        minScore = std::min(minScore, candidate.score);
        maxScore = std::max(maxScore, candidate.score);
    }
    
    lastStats_.bestScore = candidates[0].score; // 已排序，第一个是最好的
    lastStats_.averageScore = totalScore / candidates.size();
    
    // 计算方差
    float variance = 0.0f;
    for (const auto& candidate : candidates) {
        float diff = candidate.score - lastStats_.averageScore;
        variance += diff * diff;
    }
    lastStats_.scoreVariance = variance / candidates.size();
}

float IntegratedScorer::fuseMultiObjectiveScores(
    float candidateScore,
    float pieceScore,
    const arc::candidate::Candidate& candidate
) {
    return candidateScore * config_.candidateWeight + pieceScore * config_.pieceWeight;
}

// ============================================================================
// AdvancedScoringStrategy 实现
// ============================================================================

AdvancedScoringStrategy::AdvancedScoringStrategy(const Config& config) : config_(config) {}

float AdvancedScoringStrategy::advancedScore(
    const arc::candidate::Candidate& candidate,
    const arc::core::Grid& target,
    const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs
) {
    float primaryScore = scoreWithStrategy(candidate, target, config_.primaryStrategy);
    
    if (config_.fallbackStrategies.empty()) {
        return primaryScore;
    }
    
    // 计算备选策略分数
    float fallbackScore = 0.0f;
    for (auto strategy : config_.fallbackStrategies) {
        fallbackScore += scoreWithStrategy(candidate, target, strategy);
    }
    fallbackScore /= config_.fallbackStrategies.size();
    
    // 融合分数
    return primaryScore * (1.0f - config_.strategyBlendWeight) + 
           fallbackScore * config_.strategyBlendWeight;
}

float AdvancedScoringStrategy::scoreWithStrategy(
    const arc::candidate::Candidate& candidate,
    const arc::core::Grid& target,
    StrategyType strategy
) {
    switch (strategy) {
        case StrategyType::ExactMatch:
            return exactMatchScore(candidate, target);
        case StrategyType::StructuralSim:
            return structuralSimilarityScore(candidate, target);
        case StrategyType::ProgressiveEval:
            return progressiveEvalScore(candidate, target);
        case StrategyType::EnsembleScoring:
            return ensembleScore(candidate, target);
        default:
            return 0.0f;
    }
}

float AdvancedScoringStrategy::exactMatchScore(const arc::candidate::Candidate& candidate, const arc::core::Grid& target) {
    if (candidate.images.empty()) return 0.0f;
    
    const arc::core::Grid& answer = candidate.images.back();
    return (answer.width == target.width && 
            answer.height == target.height && 
            answer.pixels == target.pixels) ? 1.0f : 0.0f;
}

float AdvancedScoringStrategy::structuralSimilarityScore(const arc::candidate::Candidate& candidate, const arc::core::Grid& target) {
    if (candidate.images.empty()) return 0.0f;
    
    const arc::core::Grid& answer = candidate.images.back();
    float pixelSim = pixelDiff(answer, target);
    float shapeSim = shapeBonus(answer, target);
    float sizeSim = sizeMatchBonus(answer, target);
    
    return (pixelSim + shapeSim + sizeSim) / 3.0f;
}

float AdvancedScoringStrategy::progressiveEvalScore(const arc::candidate::Candidate& candidate, const arc::core::Grid& target) {
    // 渐进式评估：先粗粒度，再细粒度
    float coarseScore = structuralSimilarityScore(candidate, target);
    if (coarseScore < 0.5f) return coarseScore;
    
    return exactMatchScore(candidate, target);
}

float AdvancedScoringStrategy::ensembleScore(const arc::candidate::Candidate& candidate, const arc::core::Grid& target) {
    float exactScore = exactMatchScore(candidate, target);
    float structScore = structuralSimilarityScore(candidate, target);
    
    return (exactScore + structScore) / 2.0f;
}

// ============================================================================
// 辅助函数实现
// ============================================================================

std::unique_ptr<IntegratedScorer> createDefaultScorer() {
    return std::make_unique<IntegratedScorer>();
}

float quickScore(const arc::candidate::Candidate& candidate, const arc::core::Grid& target) {
    if (candidate.images.empty()) return 0.0f;
    
    return pixelDiff(candidate.images.back(), target);
}

std::vector<float> batchScore(
    const std::vector<arc::candidate::Candidate>& candidates,
    const arc::core::Grid& target
) {
    std::vector<float> scores;
    scores.reserve(candidates.size());
    
    for (const auto& candidate : candidates) {
        scores.push_back(quickScore(candidate, target));
    }
    
    return scores;
}

bool validateScoringResults(
    const std::vector<arc::candidate::Candidate>& candidates,
    const std::vector<float>& scores
) {
    return candidates.size() == scores.size();
}

void printScoringStatistics(const IntegratedScorer::ScoringStatistics& stats) {
    std::cout << "=== Scoring统计信息 ===" << std::endl;
    std::cout << "总候选解数: " << stats.totalCandidates << std::endl;
    std::cout << "有效候选解数: " << stats.validCandidates << std::endl;
    std::cout << "精确匹配数: " << stats.exactMatches << std::endl;
    std::cout << "最佳分数: " << stats.bestScore << std::endl;
    std::cout << "平均分数: " << stats.averageScore << std::endl;
    std::cout << "分数方差: " << stats.scoreVariance << std::endl;
}

} // namespace arc::scoring 