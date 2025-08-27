#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "core/state.hpp"

namespace arc::transform {

// 变换函数类型定义 - 对应icecuber的函数签名
using TransformFunction = std::function<bool(const arc::core::State&, arc::core::State&)>;

// 函数信息结构
struct FunctionInfo {
    std::string name;
    TransformFunction func;
    std::uint8_t cost{1};
    bool isListed{true};
};

// 变换函数库 - 对应icecuber的Functions3
class TransformLibrary {
public:
    static TransformLibrary& instance();
    
    std::uint16_t registerFunction(const std::string& name, 
                                  TransformFunction func, 
                                  std::uint8_t cost = 1,
                                  bool isListed = true);
    
    const FunctionInfo& getFunction(std::uint16_t id) const;
    std::uint16_t findFunction(const std::string& name) const;
    const std::vector<std::uint16_t>& getListedFunctions() const { return listedFunctions_; }
    std::size_t getFunctionCount() const { return functions_.size(); }
    
private:
    std::vector<FunctionInfo> functions_;
    std::vector<std::uint16_t> listedFunctions_;
    std::unordered_map<std::string, std::uint16_t> nameToId_;
};

// ============================================================================
// 核心辅助函数 - 对应icecuber的core_functions
// ============================================================================

namespace core {
    // 颜色操作
    int colMask(const arc::core::Grid& img);
    int countCols(const arc::core::Grid& img, bool include0 = false);
    int count(const arc::core::Grid& img);
    std::uint8_t majorityCol(const arc::core::Grid& img, bool include0 = false);
    
    // 图像创建
    arc::core::Grid full(const arc::core::Point& pos, const arc::core::Point& size, std::uint8_t filling = 1);
    arc::core::Grid full(const arc::core::Point& size, std::uint8_t filling = 1);
    arc::core::Grid empty(const arc::core::Point& pos, const arc::core::Point& size);
    arc::core::Grid empty(const arc::core::Point& size);
    
    // 几何操作
    bool isRectangle(const arc::core::Grid& img);
    int countComponents(const arc::core::Grid& img);
    arc::core::Grid subImage(const arc::core::Grid& img, const arc::core::Point& pos, const arc::core::Point& size);
    
    // 颜色分割
    std::vector<std::pair<arc::core::Grid, int>> splitCols(const arc::core::Grid& img, bool include0 = false);
}

// ============================================================================
// 基础图像变换函数 - 对应icecuber的image_functions
// ============================================================================

// 基础形状创建函数
arc::core::Grid createCol(int colorId);
arc::core::Grid createPos(int dx, int dy);
arc::core::Grid createSquare(int size);
arc::core::Grid createLine(int orientation, int length);

// 位置和尺寸提取
arc::core::Grid getPos(const arc::core::Grid& img);
arc::core::Grid getSize(const arc::core::Grid& img);
arc::core::Grid getHull(const arc::core::Grid& img);
arc::core::Grid toOrigin(arc::core::Grid img);

// 颜色过滤
arc::core::Grid filterCol(const arc::core::Grid& img, int colorId);
arc::core::Grid filterCol(const arc::core::Grid& img, const arc::core::Grid& palette);
arc::core::Grid invert(arc::core::Grid img);

// 形状操作
arc::core::Grid colShape(const arc::core::Grid& shape, int colorId);
arc::core::Grid colShape(const arc::core::Grid& color, const arc::core::Grid& shape);
arc::core::Grid compress(const arc::core::Grid& img, const arc::core::Grid& bg = createCol(0));

// 几何变换 - 对应icecuber的rigid
arc::core::Grid rigid(const arc::core::Grid& img, int transformId);

// 组合操作
arc::core::Grid broadcast(const arc::core::Grid& color, const arc::core::Grid& shape, bool include0 = true);
arc::core::Grid compose(const arc::core::Grid& a, const arc::core::Grid& b, int mode = 0);
arc::core::Grid align(const arc::core::Grid& a, const arc::core::Grid& b, int dx = 0, int dy = 0);

// 移动和嵌入
arc::core::Grid move(arc::core::Grid img, const arc::core::Grid& offset);
arc::core::Grid embed(const arc::core::Grid& img, const arc::core::Grid& shape);

// 切割和分割
std::vector<arc::core::Grid> cut(const arc::core::Grid& img);
std::vector<arc::core::Grid> splitCols(const arc::core::Grid& img, bool include0 = false);

// 模式操作
arc::core::Grid repeat(const arc::core::Grid& pattern, const arc::core::Grid& area, int padding = 0);
arc::core::Grid mirror(const arc::core::Grid& pattern, const arc::core::Grid& area, int padding = 0);

// 统计操作
arc::core::Grid majorityCol(const arc::core::Grid& img);

// ============================================================================
// 高级变换函数注册
// ============================================================================

// 初始化所有变换函数 - 对应icecuber的initFuncs3
void initializeTransformFunctions();

} // namespace arc::transform 