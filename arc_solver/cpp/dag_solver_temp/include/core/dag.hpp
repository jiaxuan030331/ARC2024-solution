#pragma once
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <functional>
#include "core/state.hpp"

namespace arc::core {

using NodeID = std::uint32_t;
static constexpr NodeID INVALID_NODE = static_cast<NodeID>(-1);

// 简化版TinyHashMap - 对应icecuber的TinyHashMap
class CompactHashMap {
public:
    struct Entry {
        std::uint64_t key{0};
        NodeID value{INVALID_NODE};
        NodeID next{INVALID_NODE};
    };
    
private:
    std::vector<Entry> entries_;
    std::vector<NodeID> table_;
    std::uint64_t mask_{0};
    
    void resize();
    
public:
    CompactHashMap();
    
    // 插入键值对，返回(节点ID, 是否新插入)
    std::pair<NodeID, bool> insert(std::uint64_t key, NodeID value);
    
    NodeID find(std::uint64_t key) const;
    std::size_t size() const { return entries_.size(); }
    void clear();
};

// 紧凑的子节点存储 - 对应icecuber的TinyChildren
class CompactChildren {
public:
    static constexpr int DENSE_THRESHOLD = 10;
    static constexpr NodeID NONE = static_cast<NodeID>(-2);
    
private:
    union {
        NodeID* dense_;      // 密集存储：funcId -> nodeId
        std::pair<std::uint16_t, NodeID>* sparse_; // 稀疏存储：(funcId, nodeId)
    };
    std::uint16_t size_{0};
    std::uint16_t capacity_{0};
    bool isDense_{false};
    
    void convertToDense();
    
public:
    CompactChildren();
    ~CompactChildren();
    
    // 不可复制，只能移动
    CompactChildren(const CompactChildren&) = delete;
    CompactChildren& operator=(const CompactChildren&) = delete;
    CompactChildren(CompactChildren&& other) noexcept;
    CompactChildren& operator=(CompactChildren&& other) noexcept;
    
    void add(std::uint16_t funcId, NodeID nodeId);
    NodeID get(std::uint16_t funcId) const;
    void clear();
    std::uint16_t size() const { return size_; }
};

// DAG节点 - 对应icecuber的Node/TinyNode
struct Node {
    State state;                    // 节点状态
    std::uint16_t functionId{0xFFFF}; // 生成此节点的变换函数ID
    NodeID parent{INVALID_NODE};    // 父节点ID
    CompactChildren children;       // 子节点映射：funcId -> nodeId
    bool isPiece{false};           // 是否为piece节点
    
    Node() = default;
    Node(const State& s, std::uint16_t fid = 0xFFFF, NodeID p = INVALID_NODE)
        : state(s), functionId(fid), parent(p) {}
    
    // 移动构造和赋值
    Node(Node&&) = default;
    Node& operator=(Node&&) = default;
    
    // 禁止复制
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
};

// 函数定义 - 对应icecuber的Functions3
class FunctionRegistry {
public:
    using TransformFunction = std::function<bool(const State&, State&)>;
    
    struct FunctionInfo {
        std::string name;
        TransformFunction func;
        std::uint8_t cost{1};
        bool isListed{true}; // 是否在搜索中使用
    };
    
private:
    std::vector<FunctionInfo> functions_;
    std::vector<std::uint16_t> listedFunctions_; // 搜索中使用的函数ID列表
    std::unordered_map<std::string, std::uint16_t> nameToId_;
    
public:
    std::uint16_t registerFunction(const std::string& name, 
                                  TransformFunction func, 
                                  std::uint8_t cost = 1, 
                                  bool isListed = true);
    
    const FunctionInfo& getFunction(std::uint16_t id) const;
    std::uint16_t findFunction(const std::string& name) const;
    const std::vector<std::uint16_t>& getListedFunctions() const { return listedFunctions_; }
    std::size_t getFunctionCount() const { return functions_.size(); }
    std::size_t getListedCount() const { return listedFunctions_.size(); }
};

// DAG主类 - 对应icecuber的DAG
class DAG {
public:
    // 配置参数
    struct Config {
        std::size_t maxDepth;          // 最大搜索深度
        std::size_t maxNodes;          // 最大节点数
        std::size_t maxPixels;         // 最大总像素数
        double timeLimit;              // 时间限制(秒)
        
        Config() : maxDepth(25), maxNodes(100000), maxPixels(40*40*5), timeLimit(60.0) {}
    };
    
private:
    Config config_;
    std::vector<std::unique_ptr<Node>> nodes_;  // 节点存储
    CompactHashMap hashMap_;                    // 去重哈希表
    FunctionRegistry functions_;               // 函数注册表
    std::size_t givenNodes_{0};               // 给定的输入节点数量
    Point targetSize_{0, 0};                  // 目标输出尺寸
    
    // 统计信息
    mutable std::size_t expandCalls_{0};
    mutable std::size_t duplicateHits_{0};
    mutable double buildTime_{0.0};
    
public:
    explicit DAG(const Config& config = Config());
    
    // 节点操作
    NodeID addNode(const State& state, bool force = false);
    NodeID addRootNode(const State& state);
    
    // 扩展操作
    std::vector<NodeID> expandNode(NodeID nodeId);
    void buildDAG(); // 全面构建DAG到指定深度
    
    // 函数注册
    std::uint16_t registerFunction(const std::string& name,
                                  FunctionRegistry::TransformFunction func,
                                  std::uint8_t cost = 1,
                                  bool isListed = true) {
        return functions_.registerFunction(name, func, cost, isListed);
    }
    
    // 访问接口
    const Node& getNode(NodeID id) const;
    Node& getNode(NodeID id);
    std::size_t getNodeCount() const { return nodes_.size(); }
    const FunctionRegistry& getFunctions() const { return functions_; }
    
    // 查询接口
    Grid getNodeImage(NodeID nodeId) const; // 获取节点的主图像
    State getNodeState(NodeID nodeId) const; // 获取节点状态
    
    // 配置和状态
    const Config& getConfig() const { return config_; }
    void setTargetSize(const Point& size) { targetSize_ = size; }
    void setGivenNodes(std::size_t count) { givenNodes_ = count; }
    
    // 统计信息
    struct Statistics {
        std::size_t totalNodes;
        std::size_t expandCalls;
        std::size_t duplicateHits;
        double duplicateRate;
        double buildTime;
        std::size_t functionCount;
    };
    
    Statistics getStatistics() const;
    
    // 清理
    void clear();
    
private:
    bool isValidExpansion(const State& newState) const;
    NodeID applyFunction(NodeID nodeId, std::uint16_t funcId);
};

} // namespace arc::core 