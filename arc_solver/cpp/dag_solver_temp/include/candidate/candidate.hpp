#pragma once
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include "piece/piece.hpp"
#include "core/state.hpp"

namespace arc::candidate {

// ============================================================================
// Candidate结构 - 对应icecuber的Candidate
// ============================================================================

struct Candidate {
    // 对应icecuber的vImage imgs
    std::vector<arc::core::Grid> images;
    
    // 对应icecuber的double score
    double score = -1.0;
    
    // 对应icecuber的统计信息
    int pieceCount = -1;     // cnt_pieces
    int sumDepth = -1;       // sum_depth  
    int maxDepth = -1;       // max_depth
    
    // 构造函数 - 对应icecuber的两个构造函数
    Candidate(const std::vector<arc::core::Grid>& imgs, double score_)
        : images(imgs), score(score_) {}
        
    Candidate(const std::vector<arc::core::Grid>& imgs, int pieceCount_, int sumDepth_, int maxDepth_)
        : images(imgs), pieceCount(pieceCount_), sumDepth(sumDepth_), maxDepth(maxDepth_) {}
        
    // 获取候选解的最后一个图像（通常是测试结果）
    const arc::core::Grid& getResult() const {
        if (images.empty()) {
            throw std::runtime_error("候选解图像为空");
        }
        return images.back();
    }
    
    // 验证候选解的有效性
    bool isValid() const;
    
    // 获取候选解的复杂度分数
    double getComplexity() const;
};

// 候选解比较 - 对应icecuber的operator<，分数越高排序越前
inline bool operator<(const Candidate& a, const Candidate& b) {
    return a.score > b.score;
}

// ============================================================================
// 位集合工具 - 对应icecuber的mybitset
// ============================================================================

class CompactBitset {
public:
    explicit CompactBitset(std::size_t size);
    
    bool operator[](std::size_t index) const;
    void set(std::size_t index, bool value);
    std::uint64_t hash() const;
    
    const std::vector<std::uint64_t>& getData() const { return data_; }
    std::size_t size() const { return size_; }
    
private:
    std::vector<std::uint64_t> data_;
    std::size_t size_;
    
    static constexpr std::size_t BITS_PER_BLOCK = 64;
    std::size_t getBlockCount() const { return (size_ + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK; }
};

// ============================================================================
// 贪心组合器 - 对应icecuber的greedyCompose2
// ============================================================================

class GreedyComposer {
public:
    // 配置参数
    struct Config {
        int maxIterations = 10;           // 最大迭代次数
        int maxPieceDepth = 50;          // 最大piece深度
        bool enableGreedyFill = true;    // 启用贪心填充
        bool enableVariations = true;    // 启用变化组合
        std::size_t maxCandidates = 1000; // 最大候选数量
    };
    
    GreedyComposer(const Config& config = {});
    
    // 主要组合函数 - 对应icecuber的greedyCompose2
    std::vector<Candidate> compose(
        arc::piece::PieceCollection& pieces,
        const std::vector<arc::core::Grid>& targets,
        const std::vector<arc::core::Point>& outputSizes
    );
    
    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config) { config_ = config; }
    
private:
    Config config_;
    
    // 贪心组合核心算法 - 对应icecuber的greedyComposeCore
    int greedyComposeCore(
        CompactBitset& current,
        const CompactBitset& careMask,
        int pieceDepthThreshold,
        std::vector<arc::core::Grid>& result,
        const arc::piece::PieceCollection& pieces,
        const std::vector<std::size_t>& imageSizes,
        const std::vector<std::uint64_t>& activeMem,
        const std::vector<std::uint64_t>& badMem,
        const std::vector<std::size_t>& activeIndices,
        const std::vector<std::size_t>& badIndices,
        const std::vector<std::size_t>& imageIndices
    );
    
    // 预处理pieces数据 - 对应icecuber的内存预处理逻辑
    void preprocessPieces(
        const arc::piece::PieceCollection& pieces,
        const std::vector<arc::core::Grid>& targets,
        const std::vector<arc::core::Grid>& initialImages,
        std::vector<std::uint64_t>& activeMem,
        std::vector<std::uint64_t>& badMem,
        std::vector<std::size_t>& activeIndices,
        std::vector<std::size_t>& badIndices,
        std::vector<std::size_t>& imageIndices,
        std::vector<std::size_t>& imageSizes
    );
    
    // 贪心填充黑色区域 - 对应icecuber的greedyFillBlack
    arc::core::Grid greedyFillBlack(const arc::core::Grid& image);
    
    // 位计数工具 - 对应icecuber的popcount64d
    static int popcount64(std::uint64_t value);
    
    // 图像哈希 - 用于缓存
    std::uint64_t hashImage(const arc::core::Grid& image) const;
    
    // 填充缓存
    std::map<std::uint64_t, arc::core::Grid> greedyFillCache_;
};

// ============================================================================
// 候选解组合器 - 对应icecuber的composePieces2
// ============================================================================

class CandidateComposer {
public:
    // 从pieces生成候选解 - 对应icecuber的composePieces2
    std::vector<Candidate> composePieces(
        arc::piece::PieceCollection& pieces,
        const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs,
        const std::vector<arc::core::Point>& outputSizes
    );
    
    // 评估候选解 - 对应icecuber的evaluateCands
    std::vector<Candidate> evaluateCandidates(
        const std::vector<Candidate>& candidates,
        const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs
    );
    
    // 获取贪心组合器配置
    GreedyComposer::Config& getGreedyConfig() { return greedyComposer_.config_; }
    
private:
    GreedyComposer greedyComposer_;
    
    // 计算候选解的先验分数 - 对应icecuber的prior计算
    double calculatePriorScore(const Candidate& candidate) const;
    
    // 验证答案图像的有效性
    bool validateAnswerImage(const arc::core::Grid& answer) const;
    
    // 计算训练匹配分数
    int calculateTrainingMatches(
        const std::vector<arc::core::Grid>& candidateImages,
        const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs
    ) const;
};

// ============================================================================
// 高级候选解生成策略
// ============================================================================

class AdvancedCandidateGenerator {
public:
    // 多策略生成配置
    struct Strategy {
        bool useGreedyComposition = true;    // 使用贪心组合
        bool usePieceEnumeration = true;     // 使用piece枚举
        bool useDepthFiltering = true;       // 使用深度过滤
        bool useScoreFiltering = true;       // 使用分数过滤
        
        int maxDepthRange = 10;              // 最大深度范围
        std::size_t maxCandidatesPerStrategy = 500; // 每个策略的最大候选数
    };
    
    AdvancedCandidateGenerator(const Strategy& strategy = {});
    
    // 生成候选解集合
    std::vector<Candidate> generateCandidates(
        arc::piece::PieceCollection& pieces,
        const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs,
        const arc::core::Grid& testInput,
        const std::vector<arc::core::Point>& outputSizes = {}
    );
    
    // 融合和优化候选解
    std::vector<Candidate> fuseCandidates(
        const std::vector<std::vector<Candidate>>& candidateSets
    );
    
private:
    Strategy strategy_;
    CandidateComposer composer_;
    
    // 基于piece直接生成候选解
    std::vector<Candidate> generateFromPieces(
        const arc::piece::PieceCollection& pieces
    );
    
    // 按深度过滤候选解
    std::vector<Candidate> filterByDepth(
        const std::vector<Candidate>& candidates,
        int maxDepth
    );
    
    // 按分数过滤候选解
    std::vector<Candidate> filterByScore(
        const std::vector<Candidate>& candidates,
        double minScore,
        std::size_t maxCount
    );
};

// ============================================================================
// 辅助函数
// ============================================================================

// 创建测试候选解
std::vector<Candidate> createTestCandidates(
    const std::vector<arc::core::Grid>& images
);

// 打印候选解统计信息
void printCandidateStatistics(const std::vector<Candidate>& candidates);

// 验证候选解集合
bool validateCandidates(const std::vector<Candidate>& candidates);

} // namespace arc::candidate 