#include "piece/piece.hpp"
#include "transform/transform.hpp"
#include <algorithm>
#include <iostream>
#include <unordered_set>
#include <chrono>
#include <map>

namespace arc::piece {

// ============================================================================
// PieceCollection 实现
// ============================================================================

arc::core::NodeID PieceCollection::getPieceNodeId(std::size_t pieceIndex, std::size_t dagIndex) const {
    if (pieceIndex >= pieces.size() || dagIndex >= dags.size()) {
        throw std::out_of_range("Invalid piece or DAG index");
    }
    
    std::uint32_t memoryBase = pieces[pieceIndex].memoryIndex;
    return memory[memoryBase + dagIndex];
}

const arc::core::State& PieceCollection::getPieceState(std::size_t pieceIndex, std::size_t dagIndex) const {
    arc::core::NodeID nodeId = getPieceNodeId(pieceIndex, dagIndex);
    return dags[dagIndex]->getNodeState(nodeId);
}

const arc::core::Grid& PieceCollection::getPieceImage(std::size_t pieceIndex, std::size_t dagIndex) const {
    arc::core::NodeID nodeId = getPieceNodeId(pieceIndex, dagIndex);
    return dags[dagIndex]->getNodeImage(nodeId);
}

bool PieceCollection::validate() const {
    // 验证基本结构
    if (dags.empty() || pieces.empty()) {
        return false;
    }
    
    std::size_t dagCount = dags.size();
    
    // 验证每个piece的内存索引有效
    for (const Piece& piece : pieces) {
        std::uint32_t memoryBase = piece.memoryIndex;
        
        // 检查内存边界
        if (memoryBase + dagCount > memory.size()) {
            return false;
        }
        
        // 检查每个DAG中的节点ID有效性
        for (std::size_t dagIdx = 0; dagIdx < dagCount; ++dagIdx) {
            arc::core::NodeID nodeId = memory[memoryBase + dagIdx];
            if (nodeId >= dags[dagIdx]->getStatistics().totalNodes) {
                return false;
            }
        }
    }
    
    return true;
}

PieceCollection::Statistics PieceCollection::getStatistics() const {
    Statistics stats;
    stats.totalPieces = pieces.size();
    stats.memoryUsage = memory.size() * sizeof(arc::core::NodeID);
    
    for (const auto& dag : dags) {
        auto dagStats = dag->getStatistics();
        stats.totalNodes += dagStats.totalNodes;
    }
    
    if (!pieces.empty()) {
        stats.maxDepth = std::max_element(pieces.begin(), pieces.end(),
            [](const Piece& a, const Piece& b) { return a.depth < b.depth; })->depth;
    }
    
    return stats;
}

// ============================================================================
// PieceExtractor 实现
// ============================================================================

PieceExtractor::PieceExtractor(const Config& config) : config_(config) {}

// 对应icecuber的hashVec函数
std::uint64_t PieceExtractor::hashVector(const std::vector<arc::core::NodeID>& nodeIds) const {
    std::uint64_t hash = 1;
    constexpr std::uint64_t magic = 1069388789821391921ULL; // icecuber中的magic number
    
    for (arc::core::NodeID nodeId : nodeIds) {
        hash = hash * magic + nodeId;
    }
    
    return hash;
}

// 对应icecuber的add lambda函数
bool PieceExtractor::addPieceCandidate(
    const std::vector<arc::core::NodeID>& nodeIds,
    std::uint16_t depth,
    arc::core::CompactHashMap& seenPieces,
    std::vector<std::queue<std::uint32_t>>& depthQueues,
    std::vector<arc::core::NodeID>& memory,
    std::vector<std::uint16_t>& depthMemory
) {
    if (nodeIds.size() != depthQueues.size()) {
        return false; // DAG数量不匹配
    }
    
    std::uint64_t hash = hashVector(nodeIds);
    std::uint32_t memoryIndex = static_cast<std::uint32_t>(memory.size());
    
    auto [existingIndex, wasInserted] = seenPieces.insert(hash, memoryIndex);
    
    if (wasInserted) {
        // 新的piece候选
        for (arc::core::NodeID nodeId : nodeIds) {
            memory.push_back(nodeId);
        }
        depthMemory.push_back(depth);
    }
    
    // 如果是新的或者找到了更短的路径，加入队列
    std::uint32_t adjustedIndex = existingIndex / nodeIds.size();
    if (wasInserted || depthMemory[adjustedIndex] > depth) {
        depthMemory[adjustedIndex] = depth;
        
        // 确保队列足够大
        while (depthQueues.size() <= depth) {
            depthQueues.emplace_back();
        }
        
        depthQueues[depth].push(existingIndex);
        return true;
    }
    
    return false;
}

// 对应icecuber的ispiece检查
bool PieceExtractor::isValidPiece(const std::vector<std::unique_ptr<arc::core::DAG>>& dags,
                                 const std::vector<arc::core::NodeID>& nodeIds) const {
    if (nodeIds.size() != dags.size()) {
        return false;
    }
    
    for (std::size_t i = 0; i < dags.size(); ++i) {
        const arc::core::Node* node = dags[i]->getNode(nodeIds[i]);
        if (!node || !node->isPiece) {
            return false;
        }
    }
    
    return true;
}

// 验证深度一致性
bool PieceExtractor::validateDepthConsistency(const std::vector<std::unique_ptr<arc::core::DAG>>& dags,
                                             const std::vector<arc::core::NodeID>& nodeIds,
                                             std::uint16_t expectedDepth) const {
    std::uint16_t maxDepth = 0;
    
    for (std::size_t i = 0; i < dags.size(); ++i) {
        const arc::core::Node* node = dags[i]->getNode(nodeIds[i]);
        if (!node) return false;
        
        maxDepth = std::max(maxDepth, node->depth);
    }
    
    return maxDepth >= expectedDepth;
}

// 获取子节点组合 - 对应icecuber复杂的child遍历逻辑
std::vector<std::pair<std::uint16_t, std::vector<arc::core::NodeID>>> 
PieceExtractor::getChildCombinations(const std::vector<std::unique_ptr<arc::core::DAG>>& dags,
                                    const std::vector<arc::core::NodeID>& parentNodes) const {
    std::vector<std::pair<std::uint16_t, std::vector<arc::core::NodeID>>> combinations;
    
    if (parentNodes.size() != dags.size()) {
        return combinations;
    }
    
    // 获取每个DAG中父节点的子节点
    std::vector<std::vector<std::pair<std::uint16_t, arc::core::NodeID>>> allChildren(dags.size());
    
    for (std::size_t dagIdx = 0; dagIdx < dags.size(); ++dagIdx) {
        const arc::core::Node* parentNode = dags[dagIdx]->getNode(parentNodes[dagIdx]);
        if (!parentNode) {
            return combinations; // 无效的父节点
        }
        
        // 获取子节点列表
        const auto& children = parentNode->children;
        for (std::size_t childIdx = 0; childIdx < children.size(); ++childIdx) {
            std::uint16_t functionId = children.getFunctionId(childIdx);
            arc::core::NodeID childNodeId = children.getChildNodeId(childIdx);
            
            if (childNodeId != arc::core::INVALID_NODE) {
                allChildren[dagIdx].emplace_back(functionId, childNodeId);
            }
        }
        
        // 按函数ID排序，便于后续合并
        std::sort(allChildren[dagIdx].begin(), allChildren[dagIdx].end());
    }
    
    // 找到所有DAG都有的共同函数ID - 对应icecuber的复杂遍历逻辑
    std::vector<std::size_t> childIndices(dags.size(), 0);
    std::uint16_t currentFunctionId = 0;
    
    while (true) {
        // 找到当前最小的可用函数ID
        std::uint16_t minAvailableFunctionId = UINT16_MAX;
        bool foundAny = false;
        
        for (std::size_t dagIdx = 0; dagIdx < dags.size(); ++dagIdx) {
            // 跳过小于currentFunctionId的子节点
            while (childIndices[dagIdx] < allChildren[dagIdx].size() &&
                   allChildren[dagIdx][childIndices[dagIdx]].first < currentFunctionId) {
                childIndices[dagIdx]++;
            }
            
            if (childIndices[dagIdx] >= allChildren[dagIdx].size()) {
                // 这个DAG没有更多子节点了
                goto finish_combinations;
            }
            
            std::uint16_t availableFunctionId = allChildren[dagIdx][childIndices[dagIdx]].first;
            minAvailableFunctionId = std::min(minAvailableFunctionId, availableFunctionId);
            foundAny = true;
        }
        
        if (!foundAny) break;
        
        currentFunctionId = minAvailableFunctionId;
        
        // 检查所有DAG是否都有这个函数ID
        std::vector<arc::core::NodeID> childNodes(dags.size());
        bool allHaveFunction = true;
        
        for (std::size_t dagIdx = 0; dagIdx < dags.size(); ++dagIdx) {
            if (childIndices[dagIdx] < allChildren[dagIdx].size() &&
                allChildren[dagIdx][childIndices[dagIdx]].first == currentFunctionId) {
                childNodes[dagIdx] = allChildren[dagIdx][childIndices[dagIdx]].second;
            } else {
                allHaveFunction = false;
                break;
            }
        }
        
        if (allHaveFunction) {
            // 检查是否有无效节点
            bool hasInvalidNode = false;
            for (arc::core::NodeID nodeId : childNodes) {
                if (nodeId == arc::core::INVALID_NODE) {
                    hasInvalidNode = true;
                    break;
                }
            }
            
            if (!hasInvalidNode) {
                combinations.emplace_back(currentFunctionId, std::move(childNodes));
            }
        }
        
        currentFunctionId++;
    }
    
finish_combinations:
    return combinations;
}

// 主要的piece提取函数 - 对应icecuber的makePieces2
PieceCollection PieceExtractor::extractPieces(std::vector<std::unique_ptr<arc::core::DAG>> dags) {
    if (dags.empty()) {
        throw std::invalid_argument("DAG vector cannot be empty");
    }
    
    PieceCollection collection;
    collection.dags = std::move(dags);
    
    const std::size_t dagCount = collection.dags.size();
    
    // 初始化数据结构
    arc::core::CompactHashMap seenPieces;
    std::vector<std::queue<std::uint32_t>> depthQueues;
    std::vector<std::uint16_t> depthMemory;
    
    // 添加初始给定节点 - 对应icecuber的givens处理
    std::uint32_t initialGivens = collection.dags[0]->getStatistics().totalRootNodes;
    for (std::uint32_t i = 0; i < initialGivens; ++i) {
        std::vector<arc::core::NodeID> initialNodes(dagCount, i);
        
        // 获取深度 - 假设初始节点深度都为0
        std::uint16_t depth = 0;
        if (const arc::core::Node* node = collection.dags[0]->getNode(i)) {
            depth = node->depth;
        }
        
        addPieceCandidate(initialNodes, depth, seenPieces, depthQueues, 
                         collection.memory, depthMemory);
    }
    
    // 主循环 - 对应icecuber的深度优先遍历
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (std::uint16_t depth = 0; depth < depthQueues.size() && depth <= config_.maxDepth; ++depth) {
        while (!depthQueues[depth].empty()) {
            std::uint32_t memoryIndex = depthQueues[depth].front();
            depthQueues[depth].pop();
            
            // 检查深度是否仍然有效
            std::uint32_t adjustedIndex = memoryIndex / dagCount;
            if (depth > depthMemory[adjustedIndex]) {
                continue; // 已经找到更短路径
            }
            
            // 构建当前节点向量
            std::vector<arc::core::NodeID> currentNodes(
                collection.memory.begin() + memoryIndex,
                collection.memory.begin() + memoryIndex + dagCount
            );
            
            // 验证是否为有效piece
            if (isValidPiece(collection.dags, currentNodes) && 
                validateDepthConsistency(collection.dags, currentNodes, depth)) {
                
                Piece piece(memoryIndex, depth);
                collection.pieces.push_back(piece);
                
                // 检查piece数量限制
                if (collection.pieces.size() >= config_.maxPieces) {
                    std::cout << "达到最大piece数量限制: " << config_.maxPieces << std::endl;
                    goto extraction_complete;
                }
            }
            
            // 如果深度不匹配，跳过扩展
            if (!validateDepthConsistency(collection.dags, currentNodes, depth)) {
                continue;
            }
            
            // 获取子节点组合
            auto childCombinations = getChildCombinations(collection.dags, currentNodes);
            
            for (const auto& [functionId, childNodes] : childCombinations) {
                // 计算新深度
                std::uint16_t newDepth = 0;
                for (std::size_t dagIdx = 0; dagIdx < dagCount; ++dagIdx) {
                    const arc::core::Node* childNode = collection.dags[dagIdx]->getNode(childNodes[dagIdx]);
                    if (childNode) {
                        newDepth = std::max(newDepth, childNode->depth);
                    }
                }
                
                // 获取函数成本 - 简化版本，假设成本为1
                std::uint16_t functionCost = 1;
                if (!collection.dags.empty()) {
                    // 尝试从函数注册表获取成本
                    try {
                        auto& transformLib = arc::transform::TransformLibrary::instance();
                        if (functionId < transformLib.getFunctionCount()) {
                            const auto& funcInfo = transformLib.getFunction(functionId);
                            functionCost = funcInfo.cost;
                        }
                    } catch (...) {
                        // 如果获取失败，使用默认成本
                        functionCost = 1;
                    }
                }
                
                // 检查深度约束
                if (newDepth >= depth + functionCost) {
                    addPieceCandidate(childNodes, depth + functionCost, seenPieces, 
                                    depthQueues, collection.memory, depthMemory);
                }
            }
        }
    }
    
extraction_complete:
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Piece提取完成:" << std::endl;
    std::cout << "  - 总节点数: " << collection.getStatistics().totalNodes << std::endl;
    std::cout << "  - 提取的pieces: " << collection.pieces.size() << std::endl;
    std::cout << "  - 用时: " << duration.count() << "ms" << std::endl;
    
    // 验证结果
    if (config_.validateConsistency && !collection.validate()) {
        throw std::runtime_error("提取的piece集合验证失败");
    }
    
    return collection;
}

// 从训练数据构建pieces - 简化版本
PieceCollection PieceExtractor::buildFromTraining(
    const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs,
    const arc::core::Grid& testInput,
    const std::vector<arc::core::Point>& outputSizes
) {
    // 为每个训练样本和测试输入创建DAG
    std::vector<std::unique_ptr<arc::core::DAG>> dags;
    
    // 初始化变换函数
    arc::transform::initializeTransformFunctions();
    auto& transformLib = arc::transform::TransformLibrary::instance();
    
    // 为每个训练样本创建DAG
    for (const auto& [input, output] : trainingPairs) {
        auto dag = std::make_unique<arc::core::DAG>();
        
        // 添加输入作为根节点
        arc::core::State inputState(input, 0);
        dag->addRootNode(inputState);
        
        // 构建DAG
        dag->buildDAG(transformLib, 3); // 限制深度为3
        
        dags.push_back(std::move(dag));
    }
    
    // 为测试输入创建DAG
    auto testDAG = std::make_unique<arc::core::DAG>();
    arc::core::State testState(testInput, 0);
    testDAG->addRootNode(testState);
    testDAG->buildDAG(transformLib, 3);
    dags.push_back(std::move(testDAG));
    
    return extractPieces(std::move(dags));
}

// ============================================================================
// 辅助函数实现
// ============================================================================

PieceCollection createTestPieceCollection(
    const std::vector<arc::core::Grid>& inputs,
    const std::vector<arc::core::Grid>& outputs
) {
    if (inputs.size() != outputs.size()) {
        throw std::invalid_argument("输入和输出数量必须相等");
    }
    
    std::vector<std::pair<arc::core::Grid, arc::core::Grid>> trainingPairs;
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        trainingPairs.emplace_back(inputs[i], outputs[i]);
    }
    
    // 使用第一个输入作为测试输入
    arc::core::Grid testInput = inputs.empty() ? arc::core::Grid(1, 1) : inputs[0];
    
    PieceExtractor extractor;
    return extractor.buildFromTraining(trainingPairs, testInput);
}

void printPieceStatistics(const PieceCollection& pieces) {
    auto stats = pieces.getStatistics();
    
    std::cout << "=== Piece统计信息 ===" << std::endl;
    std::cout << "DAG数量: " << pieces.getDAGCount() << std::endl;
    std::cout << "Piece数量: " << stats.totalPieces << std::endl;
    std::cout << "总节点数: " << stats.totalNodes << std::endl;
    std::cout << "最大深度: " << stats.maxDepth << std::endl;
    std::cout << "内存使用: " << stats.memoryUsage << " bytes" << std::endl;
    
    // 深度分布
    std::map<std::uint16_t, std::size_t> depthDistribution;
    for (const Piece& piece : pieces.pieces) {
        depthDistribution[piece.depth]++;
    }
    
    std::cout << "深度分布:" << std::endl;
    for (const auto& [depth, count] : depthDistribution) {
        std::cout << "  深度 " << depth << ": " << count << " pieces" << std::endl;
    }
}

bool validatePieceCollection(const PieceCollection& pieces) {
    return pieces.validate();
}

} // namespace arc::piece 