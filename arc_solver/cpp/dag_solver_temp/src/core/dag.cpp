#include "core/dag.hpp"
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <cstring>

namespace arc::core {

// ============================================================================
// CompactHashMap 实现 - 对应icecuber的TinyHashMap
// ============================================================================

CompactHashMap::CompactHashMap() {
    constexpr std::size_t INITIAL_SIZE = 1024;
    table_.resize(INITIAL_SIZE, INVALID_NODE);
    mask_ = INITIAL_SIZE - 1;
    entries_.reserve(INITIAL_SIZE);
}

void CompactHashMap::resize() {
    std::size_t newSize = table_.size() * 2;
    std::vector<NodeID> newTable(newSize, INVALID_NODE);
    std::uint64_t newMask = newSize - 1;
    
    // 重新哈希所有条目
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].value != INVALID_NODE) {
            std::uint64_t slot = entries_[i].key & newMask;
            entries_[i].next = newTable[slot];
            newTable[slot] = static_cast<NodeID>(i);
        }
    }
    
    table_ = std::move(newTable);
    mask_ = newMask;
}

std::pair<NodeID, bool> CompactHashMap::insert(std::uint64_t key, NodeID value) {
    // 首先查找是否已存在
    NodeID existing = find(key);
    if (existing != INVALID_NODE) {
        return {existing, false};
    }
    
    // 检查是否需要扩容
    if (entries_.size() >= table_.size() * 1.5) {
        resize();
    }
    
    // 插入新条目
    std::uint64_t slot = key & mask_;
    NodeID entryId = static_cast<NodeID>(entries_.size());
    
    entries_.push_back({key, value, table_[slot]});
    table_[slot] = entryId;
    
    return {value, true};
}

NodeID CompactHashMap::find(std::uint64_t key) const {
    std::uint64_t slot = key & mask_;
    NodeID entryId = table_[slot];
    
    while (entryId != INVALID_NODE) {
        const Entry& entry = entries_[entryId];
        if (entry.key == key) {
            return entry.value;
        }
        entryId = entry.next;
    }
    
    return INVALID_NODE;
}

void CompactHashMap::clear() {
    entries_.clear();
    std::fill(table_.begin(), table_.end(), INVALID_NODE);
}

// ============================================================================
// CompactChildren 实现 - 对应icecuber的TinyChildren
// ============================================================================

CompactChildren::CompactChildren() : dense_(nullptr) {}

CompactChildren::~CompactChildren() {
    clear();
}

CompactChildren::CompactChildren(CompactChildren&& other) noexcept 
    : dense_(other.dense_), size_(other.size_), capacity_(other.capacity_), isDense_(other.isDense_) {
    other.dense_ = nullptr;
    other.size_ = other.capacity_ = 0;
}

CompactChildren& CompactChildren::operator=(CompactChildren&& other) noexcept {
    if (this != &other) {
        clear();
        dense_ = other.dense_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        isDense_ = other.isDense_;
        
        other.dense_ = nullptr;
        other.size_ = other.capacity_ = 0;
    }
    return *this;
}

void CompactChildren::convertToDense() {
    if (isDense_) return;
    
    // 分配密集数组
    constexpr std::size_t MAX_FUNC_ID = 1000; // 假设函数ID不超过1000
    NodeID* newDense = new NodeID[MAX_FUNC_ID];
    std::fill(newDense, newDense + MAX_FUNC_ID, NONE);
    
    // 复制稀疏数据到密集数组
    for (std::uint16_t i = 0; i < size_; ++i) {
        std::uint16_t funcId = sparse_[i].first;
        NodeID nodeId = sparse_[i].second;
        if (funcId < MAX_FUNC_ID) {
            newDense[funcId] = nodeId;
        }
    }
    
    // 清理旧数据
    delete[] sparse_;
    dense_ = newDense;
    capacity_ = MAX_FUNC_ID;
    isDense_ = true;
}

void CompactChildren::add(std::uint16_t funcId, NodeID nodeId) {
    if (!isDense_ && size_ >= DENSE_THRESHOLD) {
        convertToDense();
    }
    
    if (isDense_) {
        if (funcId < capacity_ && dense_[funcId] == NONE) {
            dense_[funcId] = nodeId;
            ++size_;
        }
    } else {
        // 稀疏模式：扩容并添加
        if (size_ >= capacity_) {
            std::size_t newCapacity = capacity_ == 0 ? 4 : capacity_ * 2;
            auto* newSparse = new std::pair<std::uint16_t, NodeID>[newCapacity];
            
            if (sparse_) {
                std::memcpy(newSparse, sparse_, size_ * sizeof(std::pair<std::uint16_t, NodeID>));
                delete[] sparse_;
            }
            
            sparse_ = newSparse;
            capacity_ = newCapacity;
        }
        
        sparse_[size_++] = {funcId, nodeId};
    }
}

NodeID CompactChildren::get(std::uint16_t funcId) const {
    if (isDense_) {
        return (funcId < capacity_) ? dense_[funcId] : NONE;
    } else {
        for (std::uint16_t i = 0; i < size_; ++i) {
            if (sparse_[i].first == funcId) {
                return sparse_[i].second;
            }
        }
        return NONE;
    }
}

void CompactChildren::clear() {
    if (dense_) {
        delete[] dense_;
        dense_ = nullptr;
    }
    size_ = capacity_ = 0;
    isDense_ = false;
}

// ============================================================================
// FunctionRegistry 实现 - 对应icecuber的Functions3
// ============================================================================

std::uint16_t FunctionRegistry::registerFunction(const std::string& name,
                                                TransformFunction func,
                                                std::uint8_t cost,
                                                bool isListed) {
    std::uint16_t id = static_cast<std::uint16_t>(functions_.size());
    
    functions_.push_back({name, std::move(func), cost, isListed});
    nameToId_[name] = id;
    
    if (isListed) {
        listedFunctions_.push_back(id);
    }
    
    return id;
}

const FunctionRegistry::FunctionInfo& FunctionRegistry::getFunction(std::uint16_t id) const {
    if (id >= functions_.size()) {
        throw std::out_of_range("Invalid function ID");
    }
    return functions_[id];
}

std::uint16_t FunctionRegistry::findFunction(const std::string& name) const {
    auto it = nameToId_.find(name);
    if (it == nameToId_.end()) {
        throw std::runtime_error("Function not found: " + name);
    }
    return it->second;
}

// ============================================================================
// DAG 实现 - 对应icecuber的DAG
// ============================================================================

DAG::DAG(const Config& config) : config_(config) {
    nodes_.reserve(1000); // 预分配节点空间
}

NodeID DAG::addNode(const State& state, bool force) {
    if (!state.isValid()) {
        return INVALID_NODE;
    }
    
    if (!force && !isValidExpansion(state)) {
        return INVALID_NODE;
    }
    
    // 检查重复
    std::uint64_t stateHash = state.hash();
    auto [existing, isNew] = hashMap_.insert(stateHash, static_cast<NodeID>(nodes_.size()));
    
    if (!isNew) {
        ++duplicateHits_;
        return existing;
    }
    
    // 创建新节点
    NodeID nodeId = static_cast<NodeID>(nodes_.size());
    nodes_.push_back(std::make_unique<Node>(state));
    
    return nodeId;
}

NodeID DAG::addRootNode(const State& state) {
    NodeID nodeId = addNode(state, true);
    if (nodeId != INVALID_NODE) {
        ++givenNodes_;
    }
    return nodeId;
}

std::vector<NodeID> DAG::expandNode(NodeID nodeId) {
    ++expandCalls_;
    
    if (nodeId >= nodes_.size()) {
        return {};
    }
    
    const Node& parentNode = *nodes_[nodeId];
    if (parentNode.state.depth >= config_.maxDepth) {
        return {};
    }
    
    std::vector<NodeID> newNodes;
    newNodes.reserve(functions_.getListedCount());
    
    // 对每个已注册的函数尝试变换
    for (std::uint16_t funcId : functions_.getListedFunctions()) {
        NodeID childId = applyFunction(nodeId, funcId);
        if (childId != INVALID_NODE) {
            newNodes.push_back(childId);
        }
    }
    
    return newNodes;
}

NodeID DAG::applyFunction(NodeID nodeId, std::uint16_t funcId) {
    if (nodeId >= nodes_.size() || funcId >= functions_.getFunctionCount()) {
        return INVALID_NODE;
    }
    
    Node& parentNode = *nodes_[nodeId];
    
    // 检查是否已经计算过这个变换
    NodeID existingChild = parentNode.children.get(funcId);
    if (existingChild != CompactChildren::NONE) {
        return existingChild;
    }
    
    // 应用变换函数
    const auto& funcInfo = functions_.getFunction(funcId);
    State newState;
    newState.depth = parentNode.state.depth + funcInfo.cost;
    
    bool success = funcInfo.func(parentNode.state, newState);
    if (!success) {
        return INVALID_NODE;
    }
    
    // 创建新节点
    NodeID childId = addNode(newState);
    if (childId != INVALID_NODE) {
        // 设置父子关系
        nodes_[childId]->parent = nodeId;
        nodes_[childId]->functionId = funcId;
        parentNode.children.add(funcId, childId);
    }
    
    return childId;
}

void DAG::buildDAG() {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::vector<NodeID> currentLevel;
    
    // 收集当前所有根节点
    for (std::size_t i = 0; i < givenNodes_ && i < nodes_.size(); ++i) {
        currentLevel.push_back(static_cast<NodeID>(i));
    }
    
    // 层次化构建DAG
    while (!currentLevel.empty() && nodes_.size() < config_.maxNodes) {
        std::vector<NodeID> nextLevel;
        
        for (NodeID nodeId : currentLevel) {
            auto newNodes = expandNode(nodeId);
            nextLevel.insert(nextLevel.end(), newNodes.begin(), newNodes.end());
            
            // 检查节点数限制
            if (nodes_.size() >= config_.maxNodes) {
                break;
            }
        }
        
        currentLevel = std::move(nextLevel);
        
        // 检查时间限制
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<double>(currentTime - startTime).count();
        if (elapsed > config_.timeLimit) {
            break;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    buildTime_ = std::chrono::duration<double>(endTime - startTime).count();
}

bool DAG::isValidExpansion(const State& newState) const {
    // 检查像素数限制
    if (newState.totalPixels() > config_.maxPixels) {
        return false;
    }
    
    // 检查深度限制
    if (newState.depth > config_.maxDepth) {
        return false;
    }
    
    // 检查图像有效性
    for (const auto& grid : newState.images) {
        if (grid.width <= 0 || grid.height <= 0 || 
            grid.width > 100 || grid.height > 100) { // 合理的尺寸限制
            return false;
        }
    }
    
    return true;
}

const Node& DAG::getNode(NodeID id) const {
    if (id >= nodes_.size()) {
        throw std::out_of_range("Invalid node ID");
    }
    return *nodes_[id];
}

Node& DAG::getNode(NodeID id) {
    if (id >= nodes_.size()) {
        throw std::out_of_range("Invalid node ID");
    }
    return *nodes_[id];
}

Grid DAG::getNodeImage(NodeID nodeId) const {
    const Node& node = getNode(nodeId);
    if (node.state.images.empty()) {
        return Grid{}; // 返回空Grid
    }
    return node.state.images[0]; // 返回第一个图像
}

State DAG::getNodeState(NodeID nodeId) const {
    return getNode(nodeId).state;
}

DAG::Statistics DAG::getStatistics() const {
    Statistics stats;
    stats.totalNodes = nodes_.size();
    stats.expandCalls = expandCalls_;
    stats.duplicateHits = duplicateHits_;
    stats.duplicateRate = expandCalls_ > 0 ? 
        static_cast<double>(duplicateHits_) / expandCalls_ : 0.0;
    stats.buildTime = buildTime_;
    stats.functionCount = functions_.getFunctionCount();
    
    return stats;
}

void DAG::clear() {
    nodes_.clear();
    hashMap_.clear();
    givenNodes_ = 0;
    expandCalls_ = duplicateHits_ = 0;
    buildTime_ = 0.0;
}

} // namespace arc::core 