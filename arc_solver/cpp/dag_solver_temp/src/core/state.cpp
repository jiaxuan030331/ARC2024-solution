#include "core/state.hpp"

namespace arc::core {

// 计算Grid的哈希值 - 对应icecuber的hashImage
std::uint64_t hashGrid(const Grid& grid) {
    constexpr std::uint64_t BASE = 137;
    std::uint64_t hash = 1543;
    
    // 包含尺寸和位置信息
    hash = hash * BASE + static_cast<std::uint64_t>(grid.width);
    hash = hash * BASE + static_cast<std::uint64_t>(grid.height);
    hash = hash * BASE + static_cast<std::uint64_t>(grid.x);
    hash = hash * BASE + static_cast<std::uint64_t>(grid.y);
    
    // 包含像素数据
    for (std::uint8_t pixel : grid.pixels) {
        hash = hash * BASE + static_cast<std::uint64_t>(pixel);
    }
    
    return hash;
}

// 计算State的哈希值 - 对应icecuber State::hash()
std::uint64_t State::hash() const {
    std::uint64_t hash = isVector ? 1ULL : 0ULL;
    
    // 包含所有图像的哈希
    for (const Grid& grid : images) {
        hash += hashGrid(grid) * 123413491ULL; // 对应icecuber的魔法数字
    }
    
    // 包含深度信息
    hash = hash * 31ULL + static_cast<std::uint64_t>(depth);
    
    return hash;
}

} // namespace arc::core 