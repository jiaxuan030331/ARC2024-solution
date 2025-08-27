#pragma once
#include <vector>
#include <queue>
#include <memory>
#include "core/dag.hpp"
#include "core/state.hpp"

namespace arc::piece {

// ============================================================================
// Piece结构 - 对应icecuber的Piece3
// ============================================================================

struct Piece {
    std::uint32_t memoryIndex;  // 对应icecuber的memi - 在内存中的索引
    std::uint16_t depth;        // 对应icecuber的depth - 搜索深度
    
    Piece() : memoryIndex(0), depth(0) {}
    Piece(std::uint32_t memIdx, std::uint16_t d) : memoryIndex(memIdx), depth(d) {}
};

// ============================================================================
// Pieces集合 - 对应icecuber的Pieces结构
// ============================================================================

class PieceCollection {
public:
    // 对应icecuber的vector<DAG> dag
    std::vector<std::unique_ptr<arc::core::DAG>> dags;
    
    // 对应icecuber的vector<Piece3> piece  
    std::vector<Piece> pieces;
    
    // 对应icecuber的vector<int> mem - 存储所有piece的节点索引
    std::vector<arc::core::NodeID> memory;
    
    PieceCollection() = default;
    PieceCollection(const PieceCollection&) = delete;
    PieceCollection& operator=(const PieceCollection&) = delete;
    PieceCollection(PieceCollection&&) = default;
    PieceCollection& operator=(PieceCollection&&) = default;
    
    // 获取指定piece在指定DAG中的节点ID
    arc::core::NodeID getPieceNodeId(std::size_t pieceIndex, std::size_t dagIndex) const;
    
    // 获取指定piece在指定DAG中的状态
    const arc::core::State& getPieceState(std::size_t pieceIndex, std::size_t dagIndex) const;
    
    // 获取指定piece在指定DAG中的图像
    const arc::core::Grid& getPieceImage(std::size_t pieceIndex, std::size_t dagIndex) const;
    
    // 获取piece数量
    std::size_t getPieceCount() const { return pieces.size(); }
    
    // 获取DAG数量
    std::size_t getDAGCount() const { return dags.size(); }
    
    // 验证piece集合的一致性
    bool validate() const;
    
    // 获取统计信息
    struct Statistics {
        std::size_t totalNodes = 0;
        std::size_t totalPieces = 0;
        std::size_t maxDepth = 0;
        std::size_t memoryUsage = 0;
    };
    Statistics getStatistics() const;
};

// ============================================================================
// Piece提取器 - 对应icecuber的makePieces2
// ============================================================================

class PieceExtractor {
public:
    // 配置参数
    struct Config {
        std::uint16_t maxDepth;          // 最大搜索深度
        std::uint32_t maxPieces;         // 最大piece数量
        bool enableParallelExtraction;   // 启用并行提取
        bool validateConsistency;        // 验证一致性
        
        Config() : maxDepth(10), maxPieces(100000), enableParallelExtraction(true), validateConsistency(true) {}
    };
    
    PieceExtractor(const Config& config = Config());
    
    // 从多个DAG中提取pieces - 对应icecuber的makePieces2
    PieceCollection extractPieces(std::vector<std::unique_ptr<arc::core::DAG>> dags);
    
    // 从训练数据构建pieces
    PieceCollection buildFromTraining(
        const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs,
        const arc::core::Grid& testInput,
        const std::vector<arc::core::Point>& outputSizes = {}
    );
    
    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config) { config_ = config; }
    
private:
    Config config_;
    
    // 内部哈希函数 - 对应icecuber的hashVec
    std::uint64_t hashVector(const std::vector<arc::core::NodeID>& nodeIds) const;
    
    // 添加piece候选到队列 - 对应icecuber的add lambda
    bool addPieceCandidate(
        const std::vector<arc::core::NodeID>& nodeIds,
        std::uint16_t depth,
        arc::core::CompactHashMap& seenPieces,
        std::vector<std::queue<std::uint32_t>>& depthQueues,
        std::vector<arc::core::NodeID>& memory,
        std::vector<std::uint16_t>& depthMemory
    );
    
    // 检查节点是否可以作为piece - 对应icecuber的ispiece检查
    bool isValidPiece(const std::vector<std::unique_ptr<arc::core::DAG>>& dags,
                     const std::vector<arc::core::NodeID>& nodeIds) const;
    
    // 获取子节点组合 - 对应icecuber的child遍历逻辑
    std::vector<std::pair<std::uint16_t, std::vector<arc::core::NodeID>>> 
    getChildCombinations(const std::vector<std::unique_ptr<arc::core::DAG>>& dags,
                        const std::vector<arc::core::NodeID>& parentNodes) const;
    
    // 验证piece的深度一致性
    bool validateDepthConsistency(const std::vector<std::unique_ptr<arc::core::DAG>>& dags,
                                 const std::vector<arc::core::NodeID>& nodeIds,
                                 std::uint16_t expectedDepth) const;
};

// ============================================================================
// 辅助函数
// ============================================================================

// 创建简单的piece集合用于测试
PieceCollection createTestPieceCollection(
    const std::vector<arc::core::Grid>& inputs,
    const std::vector<arc::core::Grid>& outputs
);

// 打印piece统计信息
void printPieceStatistics(const PieceCollection& pieces);

// 验证piece集合的有效性
bool validatePieceCollection(const PieceCollection& pieces);

} // namespace arc::piece 