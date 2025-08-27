#include "transform/transform.hpp"
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <queue>
#include <cassert>

namespace arc::transform {

// ============================================================================
// TransformLibrary实现
// ============================================================================

TransformLibrary& TransformLibrary::instance() {
    static TransformLibrary lib;
    return lib;
}

std::uint16_t TransformLibrary::registerFunction(const std::string& name,
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

const FunctionInfo& TransformLibrary::getFunction(std::uint16_t id) const {
    if (id >= functions_.size()) {
        throw std::out_of_range("Invalid function ID");
    }
    return functions_[id];
}

std::uint16_t TransformLibrary::findFunction(const std::string& name) const {
    auto it = nameToId_.find(name);
    if (it == nameToId_.end()) {
        throw std::runtime_error("Function not found: " + name);
    }
    return it->second;
}

// ============================================================================
// 核心辅助函数实现 - 对应icecuber的core_functions
// ============================================================================

namespace core {

// 计算颜色掩码 - 对应icecuber的colMask
int colMask(const arc::core::Grid& img) {
    int mask = 0;
    for (std::uint8_t pixel : img.pixels) {
        if (pixel < 10) {
            mask |= (1 << pixel);
        }
    }
    return mask;
}

// 计算颜色数量
int countCols(const arc::core::Grid& img, bool include0) {
    int mask = colMask(img);
    if (!include0) {
        mask &= ~1; // 清除第0位
    }
    return __builtin_popcount(mask);
}

// 计算非零像素数量
int count(const arc::core::Grid& img) {
    int cnt = 0;
    for (std::uint8_t pixel : img.pixels) {
        if (pixel != 0) cnt++;
    }
    return cnt;
}

// 获取主要颜色 - 对应icecuber的majorityCol
std::uint8_t majorityCol(const arc::core::Grid& img, bool include0) {
    int counts[10] = {0};
    for (std::uint8_t pixel : img.pixels) {
        if (pixel < 10) {
            counts[pixel]++;
        }
    }
    
    std::uint8_t majorColor = include0 ? 0 : 1;
    int maxCount = counts[majorColor];
    
    for (int c = include0 ? 0 : 1; c < 10; ++c) {
        if (counts[c] > maxCount) {
            majorColor = c;
            maxCount = counts[c];
        }
    }
    
    return majorColor;
}

// 创建填充图像 - 对应icecuber的full
arc::core::Grid full(const arc::core::Point& pos, const arc::core::Point& size, std::uint8_t filling) {
    arc::core::Grid grid(pos.x, pos.y, size.x, size.y);
    std::fill(grid.pixels.begin(), grid.pixels.end(), filling);
    return grid;
}

arc::core::Grid full(const arc::core::Point& size, std::uint8_t filling) {
    return full({0, 0}, size, filling);
}

// 创建空图像 - 对应icecuber的empty
arc::core::Grid empty(const arc::core::Point& pos, const arc::core::Point& size) {
    return full(pos, size, 0);
}

arc::core::Grid empty(const arc::core::Point& size) {
    return full(size, 0);
}

// 检查是否为矩形
bool isRectangle(const arc::core::Grid& img) {
    if (img.width <= 0 || img.height <= 0) return false;
    
    // 找边界
    int minX = img.width, maxX = -1, minY = img.height, maxY = -1;
    bool hasNonZero = false;
    
    for (int i = 0; i < img.height; ++i) {
        for (int j = 0; j < img.width; ++j) {
            if (img(i, j) != 0) {
                hasNonZero = true;
                minX = std::min(minX, j);
                maxX = std::max(maxX, j);
                minY = std::min(minY, i);
                maxY = std::max(maxY, i);
            }
        }
    }
    
    if (!hasNonZero) return true;
    
    // 检查边界内是否全为非零
    for (int i = minY; i <= maxY; ++i) {
        for (int j = minX; j <= maxX; ++j) {
            if (img(i, j) == 0) return false;
        }
    }
    
    return true;
}

// 计算连通分量数量
int countComponents(const arc::core::Grid& img) {
    if (img.width <= 0 || img.height <= 0) return 0;
    
    std::vector<std::vector<bool>> visited(img.height, std::vector<bool>(img.width, false));
    int components = 0;
    
    for (int i = 0; i < img.height; ++i) {
        for (int j = 0; j < img.width; ++j) {
            if (img(i, j) != 0 && !visited[i][j]) {
                // BFS
                std::queue<std::pair<int, int>> q;
                q.push({i, j});
                visited[i][j] = true;
                components++;
                
                while (!q.empty()) {
                    auto [y, x] = q.front();
                    q.pop();
                    
                    // 检查4连通邻居
                    for (auto [dy, dx] : std::vector<std::pair<int, int>>{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}) {
                        int ny = y + dy, nx = x + dx;
                        if (ny >= 0 && ny < img.height && nx >= 0 && nx < img.width &&
                            img(ny, nx) != 0 && !visited[ny][nx]) {
                            visited[ny][nx] = true;
                            q.push({ny, nx});
                        }
                    }
                }
            }
        }
    }
    
    return components;
}

// 提取子图像 - 对应icecuber的subImage
arc::core::Grid subImage(const arc::core::Grid& img, const arc::core::Point& pos, const arc::core::Point& size) {
    arc::core::Grid result(size.x, size.y);
    
    for (int i = 0; i < size.y; ++i) {
        for (int j = 0; j < size.x; ++j) {
            int srcY = pos.y + i;
            int srcX = pos.x + j;
            
            if (srcY >= 0 && srcY < img.height && srcX >= 0 && srcX < img.width) {
                result(i, j) = img(srcY, srcX);
            } else {
                result(i, j) = 0;
            }
        }
    }
    
    return result;
}

// 按颜色分割 - 对应icecuber的splitCols
std::vector<std::pair<arc::core::Grid, int>> splitCols(const arc::core::Grid& img, bool include0) {
    std::vector<std::pair<arc::core::Grid, int>> result;
    
    for (int color = include0 ? 0 : 1; color < 10; ++color) {
        bool hasColor = false;
        for (std::uint8_t pixel : img.pixels) {
            if (pixel == color) {
                hasColor = true;
                break;
            }
        }
        
        if (hasColor) {
            arc::core::Grid colorImg(img.width, img.height);
            colorImg.x = img.x;
            colorImg.y = img.y;
            
            for (int i = 0; i < img.height; ++i) {
                for (int j = 0; j < img.width; ++j) {
                    colorImg(i, j) = (img(i, j) == color) ? color : 0;
                }
            }
            
            result.emplace_back(std::move(colorImg), color);
        }
    }
    
    return result;
}

} // namespace core

// ============================================================================
// 基础图像变换函数实现 - 对应icecuber的image_functions
// ============================================================================

// 创建单色图像 - 对应icecuber的Col
arc::core::Grid createCol(int colorId) {
    assert(colorId >= 0 && colorId < 10);
    return core::full({1, 1}, colorId);
}

// 创建位置图像 - 对应icecuber的Pos
arc::core::Grid createPos(int dx, int dy) {
    return core::full({dx, dy}, {1, 1}, 1);
}

// 创建正方形 - 对应icecuber的Square
arc::core::Grid createSquare(int size) {
    assert(size >= 1);
    return core::full({size, size}, 1);
}

// 创建线条 - 对应icecuber的Line
arc::core::Grid createLine(int orientation, int length) {
    assert(length >= 1);
    int width = length, height = 1;
    if (orientation) std::swap(width, height);
    return core::full({width, height}, 1);
}

// 获取位置 - 对应icecuber的getPos
arc::core::Grid getPos(const arc::core::Grid& img) {
    return core::full({img.x, img.y}, {1, 1}, core::majorityCol(img));
}

// 获取尺寸 - 对应icecuber的getSize
arc::core::Grid getSize(const arc::core::Grid& img) {
    return core::full({img.width, img.height}, core::majorityCol(img));
}

// 获取外壳 - 对应icecuber的hull
arc::core::Grid getHull(const arc::core::Grid& img) {
    return core::full({img.x, img.y}, {img.width, img.height}, core::majorityCol(img));
}

// 移动到原点 - 对应icecuber的toOrigin
arc::core::Grid toOrigin(arc::core::Grid img) {
    img.x = 0;
    img.y = 0;
    return img;
}

// 颜色过滤 - 对应icecuber的filterCol
arc::core::Grid filterCol(const arc::core::Grid& img, int colorId) {
    assert(colorId >= 0 && colorId < 10);
    if (colorId == 0) return invert(img);
    
    arc::core::Grid result = img;
    for (int i = 0; i < img.height; ++i) {
        for (int j = 0; j < img.width; ++j) {
            if (img(i, j) != colorId) {
                result(i, j) = 0;
            }
        }
    }
    return result;
}

arc::core::Grid filterCol(const arc::core::Grid& img, const arc::core::Grid& palette) {
    arc::core::Grid result = img;
    int palMask = core::colMask(palette);
    
    for (int i = 0; i < img.height; ++i) {
        for (int j = 0; j < img.width; ++j) {
            if ((palMask >> img(i, j) & 1) == 0) {
                result(i, j) = 0;
            }
        }
    }
    return result;
}

// 颜色反转 - 对应icecuber的invert
arc::core::Grid invert(arc::core::Grid img) {
    for (std::uint8_t& pixel : img.pixels) {
        pixel = (pixel == 0) ? 1 : 0;
    }
    return img;
}

// 压缩 - 对应icecuber的compress
arc::core::Grid compress(const arc::core::Grid& img, const arc::core::Grid& bg) {
    int bgMask = core::colMask(bg);
    
    int xMin = img.width, xMax = -1, yMin = img.height, yMax = -1;
    bool hasContent = false;
    
    for (int i = 0; i < img.height; ++i) {
        for (int j = 0; j < img.width; ++j) {
            if ((bgMask >> img(i, j) & 1) == 0) {
                hasContent = true;
                xMin = std::min(xMin, j);
                xMax = std::max(xMax, j);
                yMin = std::min(yMin, i);
                yMax = std::max(yMax, i);
            }
        }
    }
    
    if (!hasContent) {
        return arc::core::Grid(0, 0);
    }
    
    arc::core::Grid result(xMax - xMin + 1, yMax - yMin + 1);
    result.x = img.x + xMin;
    result.y = img.y + yMin;
    
    for (int i = yMin; i <= yMax; ++i) {
        for (int j = xMin; j <= xMax; ++j) {
            result(i - yMin, j - xMin) = img(i, j);
        }
    }
    
    return result;
}

// 几何变换 - 对应icecuber的rigid
arc::core::Grid rigid(const arc::core::Grid& img, int transformId) {
    arc::core::Grid result;
    
    switch (transformId) {
        case 0: // 不变换
            result = img;
            break;
        case 1: // 90度顺时针旋转
            result = arc::core::Grid(img.height, img.width);
            result.x = img.x;
            result.y = img.y;
            for (int i = 0; i < img.height; ++i) {
                for (int j = 0; j < img.width; ++j) {
                    result(j, img.height - 1 - i) = img(i, j);
                }
            }
            break;
        case 2: // 180度旋转
            result = arc::core::Grid(img.width, img.height);
            result.x = img.x;
            result.y = img.y;
            for (int i = 0; i < img.height; ++i) {
                for (int j = 0; j < img.width; ++j) {
                    result(img.height - 1 - i, img.width - 1 - j) = img(i, j);
                }
            }
            break;
        case 3: // 270度顺时针旋转
            result = arc::core::Grid(img.height, img.width);
            result.x = img.x;
            result.y = img.y;
            for (int i = 0; i < img.height; ++i) {
                for (int j = 0; j < img.width; ++j) {
                    result(img.width - 1 - j, i) = img(i, j);
                }
            }
            break;
        case 4: // 水平翻转
            result = img;
            for (int i = 0; i < img.height; ++i) {
                for (int j = 0; j < img.width; ++j) {
                    result(i, img.width - 1 - j) = img(i, j);
                }
            }
            break;
        case 5: // 垂直翻转
            result = img;
            for (int i = 0; i < img.height; ++i) {
                for (int j = 0; j < img.width; ++j) {
                    result(img.height - 1 - i, j) = img(i, j);
                }
            }
            break;
        case 6: // 转置
            result = arc::core::Grid(img.height, img.width);
            result.x = img.x;
            result.y = img.y;
            for (int i = 0; i < img.height; ++i) {
                for (int j = 0; j < img.width; ++j) {
                    result(j, i) = img(i, j);
                }
            }
            break;
        case 7: // 反转置
            result = arc::core::Grid(img.height, img.width);
            result.x = img.x;
            result.y = img.y;
            for (int i = 0; i < img.height; ++i) {
                for (int j = 0; j < img.width; ++j) {
                    result(img.width - 1 - j, img.height - 1 - i) = img(i, j);
                }
            }
            break;
        default:
            result = img;
            break;
    }
    
    return result;
}

// 广播 - 对应icecuber的broadcast
arc::core::Grid broadcast(const arc::core::Grid& color, const arc::core::Grid& shape, bool include0) {
    if (color.width * color.height == 0 || shape.width * shape.height == 0) {
        return arc::core::Grid(0, 0);
    }
    
    arc::core::Grid result = shape;
    
    // 简化版本：如果尺寸匹配，直接映射
    if (shape.width % color.width == 0 && shape.height % color.height == 0) {
        int dh = shape.height / color.height;
        int dw = shape.width / color.width;
        
        for (int ii = 0; ii < color.height; ++ii) {
            for (int jj = 0; jj < color.width; ++jj) {
                std::uint8_t c = color(ii, jj);
                for (int i = ii * dh; i < ii * dh + dh; ++i) {
                    for (int j = jj * dw; j < jj * dw + dw; ++j) {
                        result(i, j) = c;
                    }
                }
            }
        }
        return result;
    }
    
    // 一般情况：缩放映射
    double fh = color.height * 1.0 / shape.height;
    double fw = color.width * 1.0 / shape.width;
    
    for (int i = 0; i < shape.height; ++i) {
        for (int j = 0; j < shape.width; ++j) {
            int srcI = static_cast<int>(i * fh);
            int srcJ = static_cast<int>(j * fw);
            
            if (srcI < color.height && srcJ < color.width) {
                result(i, j) = color(srcI, srcJ);
            }
        }
    }
    
    return result;
}

// 组合 - 对应icecuber的compose
arc::core::Grid compose(const arc::core::Grid& a, const arc::core::Grid& b, int mode) {
    // 计算结果尺寸
    int minX = std::min(a.x, b.x);
    int minY = std::min(a.y, b.y);
    int maxX = std::max(a.x + a.width, b.x + b.width);
    int maxY = std::max(a.y + a.height, b.y + b.height);
    
    arc::core::Grid result(minX, minY, maxX - minX, maxY - minY);
    
    // 填充背景
    for (int i = 0; i < result.height; ++i) {
        for (int j = 0; j < result.width; ++j) {
            int globalX = result.x + j;
            int globalY = result.y + i;
            
            std::uint8_t pixelA = 0, pixelB = 0;
            
            // 从图像A获取像素
            if (globalX >= a.x && globalX < a.x + a.width &&
                globalY >= a.y && globalY < a.y + a.height) {
                pixelA = a(globalY - a.y, globalX - a.x);
            }
            
            // 从图像B获取像素
            if (globalX >= b.x && globalX < b.x + b.width &&
                globalY >= b.y && globalY < b.y + b.height) {
                pixelB = b(globalY - b.y, globalX - b.x);
            }
            
            // 根据模式组合
            switch (mode) {
                case 0: // B覆盖A（B非零时）
                    result(i, j) = (pixelB != 0) ? pixelB : pixelA;
                    break;
                case 1: // A覆盖B
                    result(i, j) = (pixelA != 0) ? pixelA : pixelB;
                    break;
                default:
                    result(i, j) = (pixelB != 0) ? pixelB : pixelA;
                    break;
            }
        }
    }
    
    return result;
}

// 移动 - 对应icecuber的Move
arc::core::Grid move(arc::core::Grid img, const arc::core::Grid& offset) {
    img.x += offset.x;
    img.y += offset.y;
    return img;
}

// 切割 - 对应icecuber的cut函数
std::vector<arc::core::Grid> cut(const arc::core::Grid& img) {
    std::vector<arc::core::Grid> result;
    
    if (img.width <= 0 || img.height <= 0) return result;
    
    std::vector<std::vector<bool>> visited(img.height, std::vector<bool>(img.width, false));
    
    for (int i = 0; i < img.height; ++i) {
        for (int j = 0; j < img.width; ++j) {
            if (img(i, j) != 0 && !visited[i][j]) {
                // 找到连通分量的边界
                int minX = j, maxX = j, minY = i, maxY = i;
                std::queue<std::pair<int, int>> q;
                q.push({i, j});
                visited[i][j] = true;
                
                std::vector<std::pair<int, int>> component;
                component.push_back({i, j});
                
                while (!q.empty()) {
                    auto [y, x] = q.front();
                    q.pop();
                    
                    // 检查4连通邻居
                    for (auto [dy, dx] : std::vector<std::pair<int, int>>{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}) {
                        int ny = y + dy, nx = x + dx;
                        if (ny >= 0 && ny < img.height && nx >= 0 && nx < img.width &&
                            img(ny, nx) != 0 && !visited[ny][nx]) {
                            visited[ny][nx] = true;
                            q.push({ny, nx});
                            component.push_back({ny, nx});
                            
                            minX = std::min(minX, nx);
                            maxX = std::max(maxX, nx);
                            minY = std::min(minY, ny);
                            maxY = std::max(maxY, ny);
                        }
                    }
                }
                
                // 创建子图像
                arc::core::Grid subImg(maxX - minX + 1, maxY - minY + 1);
                subImg.x = img.x + minX;
                subImg.y = img.y + minY;
                
                // 复制像素
                for (auto [y, x] : component) {
                    subImg(y - minY, x - minX) = img(y, x);
                }
                
                result.push_back(std::move(subImg));
            }
        }
    }
    
    return result;
}

// 按颜色分割图像
std::vector<arc::core::Grid> splitCols(const arc::core::Grid& img, bool include0) {
    std::vector<arc::core::Grid> result;
    auto splits = core::splitCols(img, include0);
    
    for (const auto& [grid, color] : splits) {
        result.push_back(grid);
    }
    
    return result;
}

// 其他缺失函数的简化实现
arc::core::Grid colShape(const arc::core::Grid& shape, int colorId) {
    arc::core::Grid result = shape;
    for (std::uint8_t& pixel : result.pixels) {
        if (pixel != 0) pixel = colorId;
    }
    return result;
}

arc::core::Grid colShape(const arc::core::Grid& color, const arc::core::Grid& shape) {
    return broadcast(color, shape);
}

arc::core::Grid align(const arc::core::Grid& a, const arc::core::Grid& b, int dx, int dy) {
    arc::core::Grid result = a;
    result.x = b.x + dx;
    result.y = b.y + dy;
    return result;
}

arc::core::Grid embed(const arc::core::Grid& img, const arc::core::Grid& shape) {
    return compose(shape, img, 0);
}

arc::core::Grid repeat(const arc::core::Grid& pattern, const arc::core::Grid& area, int padding) {
    // 简化实现：在指定区域重复模式
    if (pattern.width == 0 || pattern.height == 0) return area;
    
    arc::core::Grid result = area;
    
    for (int i = 0; i < area.height; ++i) {
        for (int j = 0; j < area.width; ++j) {
            int patY = i % pattern.height;
            int patX = j % pattern.width;
            
            if (pattern(patY, patX) != 0) {
                result(i, j) = pattern(patY, patX);
            }
        }
    }
    
    return result;
}

arc::core::Grid mirror(const arc::core::Grid& pattern, const arc::core::Grid& area, int padding) {
    // 简化实现：镜像重复
    return repeat(pattern, area, padding);
}

arc::core::Grid majorityCol(const arc::core::Grid& img) {
    std::uint8_t majColor = core::majorityCol(img);
    return core::full({img.width, img.height}, majColor);
}

// ============================================================================
// 变换函数注册
// ============================================================================

// 初始化所有变换函数 - 对应icecuber的initFuncs3
void initializeTransformFunctions() {
    auto& lib = TransformLibrary::instance();
    
    // 基础几何变换
    for (int i = 0; i < 8; ++i) {
        std::string name = "rigid_" + std::to_string(i);
        lib.registerFunction(name, [i](const arc::core::State& input, arc::core::State& output) {
            if (input.isVector || input.images.empty()) return false;
            
            output.images.resize(input.images.size());
            output.isVector = input.isVector;
            
            for (size_t j = 0; j < input.images.size(); ++j) {
                output.images[j] = rigid(input.images[j], i);
            }
            return true;
        }, 10);
    }
    
    // 颜色过滤
    for (int c = 0; c < 10; ++c) {
        std::string name = "filterCol_" + std::to_string(c);
        lib.registerFunction(name, [c](const arc::core::State& input, arc::core::State& output) {
            if (input.isVector || input.images.empty()) return false;
            
            output.images.resize(input.images.size());
            output.isVector = input.isVector;
            
            for (size_t i = 0; i < input.images.size(); ++i) {
                output.images[i] = filterCol(input.images[i], c);
            }
            return true;
        }, 10);
    }
    
    // 基础操作
    lib.registerFunction("compress", [](const arc::core::State& input, arc::core::State& output) {
        if (input.isVector || input.images.empty()) return false;
        
        output.images.resize(input.images.size());
        output.isVector = input.isVector;
        
        for (size_t i = 0; i < input.images.size(); ++i) {
            output.images[i] = compress(input.images[i]);
        }
        return true;
    }, 10);
    
    lib.registerFunction("toOrigin", [](const arc::core::State& input, arc::core::State& output) {
        if (input.isVector || input.images.empty()) return false;
        
        output.images.resize(input.images.size());
        output.isVector = input.isVector;
        
        for (size_t i = 0; i < input.images.size(); ++i) {
            output.images[i] = toOrigin(input.images[i]);
        }
        return true;
    }, 5);
    
    lib.registerFunction("invert", [](const arc::core::State& input, arc::core::State& output) {
        if (input.isVector || input.images.empty()) return false;
        
        output.images.resize(input.images.size());
        output.isVector = input.isVector;
        
        for (size_t i = 0; i < input.images.size(); ++i) {
            output.images[i] = invert(input.images[i]);
        }
        return true;
    }, 5);
    
    // 切割操作（生成向量）
    lib.registerFunction("cut", [](const arc::core::State& input, arc::core::State& output) {
        if (input.isVector || input.images.empty()) return false;
        
        output.images = cut(input.images[0]);
        output.isVector = true;
        return !output.images.empty();
    }, 15);
    
    lib.registerFunction("splitCols", [](const arc::core::State& input, arc::core::State& output) {
        if (input.isVector || input.images.empty()) return false;
        
        auto splits = core::splitCols(input.images[0], false);
        output.images.clear();
        for (const auto& [grid, color] : splits) {
            output.images.push_back(grid);
        }
        output.isVector = true;
        return !output.images.empty();
    }, 15);
}

} // namespace arc::transform 