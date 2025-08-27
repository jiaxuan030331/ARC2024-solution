#pragma once
#include <cstdint>
#include <vector>
#include <cassert>

namespace arc::core {

// 基础点结构 - 对应icecuber的point
struct Point {
    int x{0}, y{0};
    
    Point() = default;
    Point(int x_, int y_) : x(x_), y(y_) {}
    
    Point operator+(const Point& other) const { return {x + other.x, y + other.y}; }
    Point operator-(const Point& other) const { return {x - other.x, y - other.y}; }
    Point operator*(int factor) const { return {x * factor, y * factor}; }
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Point& other) const { return !(*this == other); }
};

// 图像结构 - 对应icecuber的Image
struct Grid {
    union {
        Point pos;
        struct { int x, y; };
    };
    union {
        Point size;
        struct { int width, height; };
    };
    std::vector<std::uint8_t> pixels; // 行优先存储
    
    Grid() : x(0), y(0), width(0), height(0) {}
    Grid(int w, int h) : x(0), y(0), width(w), height(h) {
        pixels.resize(w * h, 0);
    }
    Grid(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {
        pixels.resize(w * h, 0);
    }
    
    // 访问像素 - 对应icecuber的operator()
    std::uint8_t& operator()(int row, int col) {
        assert(row >= 0 && row < height && col >= 0 && col < width);
        return pixels[row * width + col];
    }
    
    const std::uint8_t& operator()(int row, int col) const {
        assert(row >= 0 && row < height && col >= 0 && col < width);
        return pixels[row * width + col];
    }
    
    // 安全访问 - 对应icecuber的safe()
    std::uint8_t safe(int row, int col) const {
        if (row < 0 || col < 0 || row >= height || col >= width) return 0;
        return pixels[row * width + col];
    }
    
    // 比较操作
    bool operator==(const Grid& other) const {
        return pos == other.pos && size == other.size && pixels == other.pixels;
    }
    bool operator!=(const Grid& other) const { return !(*this == other); }
};

using GridVector = std::vector<Grid>;

// 计算Grid的哈希值 - 对应icecuber的hashImage
std::uint64_t hashGrid(const Grid& grid);

// 状态结构 - 对应icecuber的State
struct State {
    GridVector images;     // 对应icecuber的vimg
    std::uint8_t depth{0}; // 变换深度
    bool isVector{false};  // 对应icecuber的isvec，表示是否为图像向量模式
    
    State() = default;
    State(const GridVector& imgs, bool isVec, std::uint8_t d) 
        : images(imgs), depth(d), isVector(isVec) {}
    State(const Grid& img, std::uint8_t d = 0) 
        : images{img}, depth(d), isVector(false) {}
    
    // 计算状态哈希值 - 对应icecuber的hash()
    std::uint64_t hash() const;
    
    // 获取总像素数
    std::size_t totalPixels() const {
        std::size_t total = 0;
        for (const auto& img : images) {
            total += img.width * img.height;
        }
        return total;
    }
    
    // 检查状态有效性
    bool isValid() const {
        return !images.empty() && depth < 128; // 最大深度限制
    }
};

} // namespace arc::core 