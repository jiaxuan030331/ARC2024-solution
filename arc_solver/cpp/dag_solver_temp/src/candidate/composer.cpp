#include "candidate/candidate.hpp"
#include "transform/transform.hpp"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <cstring>
#include <map>

namespace arc::candidate {

// ============================================================================
// Candidate 方法实现
// ============================================================================

bool Candidate::isValid() const {
    if (images.empty()) return false;
    
    // 检查最后一个图像（答案）的有效性
    const auto& answer = images.back();
    if (answer.width > 30 || answer.height > 30 || answer.width * answer.height == 0) {
        return false;
    }
    
    // 检查像素值范围
    for (int i = 0; i < answer.height; ++i) {
        for (int j = 0; j < answer.width; ++j) {
            std::uint8_t pixel = answer(i, j);
            if (pixel >= 10) { // ARC颜色范围0-9
                return false;
            }
        }
    }
    
    return true;
}

double Candidate::getComplexity() const {
    if (maxDepth >= 0 && pieceCount >= 0) {
        return maxDepth + pieceCount * 0.001; // 对应icecuber的prior计算
    }
    return 0.0;
}

// ============================================================================
// CompactBitset 实现
// ============================================================================

CompactBitset::CompactBitset(std::size_t size) : size_(size) {
    data_.resize(getBlockCount(), 0);
}

bool CompactBitset::operator[](std::size_t index) const {
    if (index >= size_) return false;
    
    std::size_t blockIndex = index / BITS_PER_BLOCK;
    std::size_t bitIndex = index % BITS_PER_BLOCK;
    
    return (data_[blockIndex] >> bitIndex) & 1;
}

void CompactBitset::set(std::size_t index, bool value) {
    if (index >= size_) return;
    
    std::size_t blockIndex = index / BITS_PER_BLOCK;
    std::size_t bitIndex = index % BITS_PER_BLOCK;
    
    if (value) {
        data_[blockIndex] |= (1ULL << bitIndex);
    } else {
        data_[blockIndex] &= ~(1ULL << bitIndex);
    }
}

std::uint64_t CompactBitset::hash() const {
    std::uint64_t result = 1;
    constexpr std::uint64_t magic = 137139; // icecuber中的magic number
    
    for (std::uint64_t block : data_) {
        result = result * magic + block;
    }
    
    return result;
}

// ============================================================================
// GreedyComposer 实现
// ============================================================================

GreedyComposer::GreedyComposer(const Config& config) : config_(config) {}

// 对应icecuber的popcount64d
int GreedyComposer::popcount64(std::uint64_t value) {
    int count = 0;
    while (value) {
        value &= value - 1; // 清除最低位的1
        count++;
    }
    return count;
}

// 对应icecuber的greedyFillBlack - 简化版本
arc::core::Grid GreedyComposer::greedyFillBlack(const arc::core::Grid& image) {
    arc::core::Grid result = image;
    
    // 简单的贪心填充：将所有0像素保持为0，其他未填充区域用最常见的颜色填充
    std::map<std::uint8_t, int> colorCounts;
    
    for (int i = 0; i < image.height; ++i) {
        for (int j = 0; j < image.width; ++j) {
            std::uint8_t pixel = image(i, j);
            if (pixel < 10) { // 有效颜色
                colorCounts[pixel]++;
            }
        }
    }
    
    // 找到最常见的非零颜色
    std::uint8_t fillColor = 0;
    int maxCount = 0;
    for (const auto& [color, count] : colorCounts) {
        if (color != 0 && count > maxCount) {
            fillColor = color;
            maxCount = count;
        }
    }
    
    // 填充未定义的像素
    for (int i = 0; i < result.height; ++i) {
        for (int j = 0; j < result.width; ++j) {
            if (result(i, j) >= 10) { // 未定义的像素
                result(i, j) = 0; // 填充为背景色
            }
        }
    }
    
    return result;
}

std::uint64_t GreedyComposer::hashImage(const arc::core::Grid& image) const {
    std::uint64_t hash = 1;
    constexpr std::uint64_t magic = 1000000007;
    
    hash = hash * magic + image.width;
    hash = hash * magic + image.height;
    
    for (std::uint8_t pixel : image.pixels) {
        hash = hash * magic + pixel;
    }
    
    return hash;
}

// 预处理pieces数据 - 对应icecuber的复杂内存预处理
void GreedyComposer::preprocessPieces(
    const arc::piece::PieceCollection& pieces,
    const std::vector<arc::core::Grid>& targets,
    const std::vector<arc::core::Grid>& initialImages,
    std::vector<std::uint64_t>& activeMem,
    std::vector<std::uint64_t>& badMem,
    std::vector<std::size_t>& activeIndices,
    std::vector<std::size_t>& badIndices,
    std::vector<std::size_t>& imageIndices,
    std::vector<std::size_t>& imageSizes
) {
    std::size_t numPieces = pieces.getPieceCount();
    std::size_t numDAGs = pieces.getDAGCount();
    
    // 计算图像尺寸
    imageSizes.clear();
    std::size_t totalSize = 0;
    for (std::size_t dagIdx = 0; dagIdx < numDAGs; ++dagIdx) {
        const auto& img = (dagIdx < targets.size()) ? targets[dagIdx] : initialImages[dagIdx];
        std::size_t imageSize = img.width * img.height;
        imageSizes.push_back(imageSize);
        totalSize += imageSize;
    }
    
    const std::size_t blocksPerBitset = (totalSize + 63) / 64;
    
    // 为每个piece预处理数据
    activeMem.reserve(numPieces * blocksPerBitset);
    badMem.reserve(numPieces * blocksPerBitset);
    activeIndices.reserve(numPieces);
    badIndices.reserve(numPieces);
    imageIndices.reserve(numPieces);
    
    for (std::size_t pieceIdx = 0; pieceIdx < numPieces; ++pieceIdx) {
        CompactBitset activeBitset(totalSize);
        CompactBitset badBitset(totalSize);
        
        std::size_t globalIndex = 0;
        
        for (std::size_t dagIdx = 0; dagIdx < numDAGs; ++dagIdx) {
            try {
                const arc::core::Grid& pieceImage = pieces.getPieceImage(pieceIdx, dagIdx);
                const arc::core::Grid& targetImage = (dagIdx < targets.size()) ? 
                    targets[dagIdx] : initialImages[dagIdx];
                
                std::size_t imageSize = imageSizes[dagIdx];
                
                // 确保图像尺寸匹配
                if (pieceImage.width * pieceImage.height == imageSize &&
                    targetImage.width * targetImage.height == imageSize) {
                    
                    for (std::size_t pixelIdx = 0; pixelIdx < imageSize; ++pixelIdx) {
                        std::uint8_t piecePixel = pieceImage.pixels[pixelIdx];
                        std::uint8_t targetPixel = targetImage.pixels[pixelIdx];
                        
                        // active: piece中的非零像素
                        activeBitset.set(globalIndex, piecePixel != 0);
                        
                        // bad: piece与target不匹配的像素
                        badBitset.set(globalIndex, piecePixel != targetPixel);
                        
                        globalIndex++;
                    }
                } else {
                    // 尺寸不匹配，跳过这些像素
                    globalIndex += imageSize;
                }
            } catch (const std::exception&) {
                // 如果无法获取piece图像，跳过
                globalIndex += imageSizes[dagIdx];
            }
        }
        
        // 存储预处理的数据
        imageIndices.push_back(pieceIdx);
        
        activeIndices.push_back(activeMem.size());
        const auto& activeData = activeBitset.getData();
        activeMem.insert(activeMem.end(), activeData.begin(), activeData.end());
        
        badIndices.push_back(badMem.size());
        const auto& badData = badBitset.getData();
        badMem.insert(badMem.end(), badData.begin(), badData.end());
    }
}

// 核心贪心组合算法 - 对应icecuber的greedyComposeCore
int GreedyComposer::greedyComposeCore(
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
) {
    const std::size_t numBlocks = current.getData().size();
    
    // 找到稀疏的关心区域
    std::vector<std::size_t> sparseBlocks;
    const auto& currentData = current.getData();
    const auto& careMaskData = careMask.getData();
    
    for (std::size_t blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
        if (~currentData[blockIdx] & careMaskData[blockIdx]) {
            sparseBlocks.push_back(blockIdx);
        }
    }
    
    std::vector<std::uint64_t> bestActive(numBlocks);
    int bestPieceIndex = -1;
    std::pair<int, int> bestCount = {0, 0};
    
    // 遍历所有pieces寻找最佳匹配
    for (std::size_t i = 0; i < imageIndices.size(); ++i) {
        std::size_t pieceIdx = imageIndices[i];
        
        // 检查深度限制
        if (pieceIdx < pieces.getPieceCount()) {
            const auto& piece = pieces.pieces[pieceIdx];
            if (piece.depth > pieceDepthThreshold) {
                continue;
            }
        }
        
        // 尝试不同的变换模式 (对应icecuber的k=0,1,2)
        for (int mode = 0; mode < 3; ++mode) {
            const std::uint64_t* activeData = &activeMem[activeIndices[i]];
            const std::uint64_t* badData = &badMem[badIndices[i]];
            
            std::uint64_t flip = (mode == 0) ? ~0ULL : 0ULL;
            std::uint64_t full = (mode == 2) ? ~0ULL : 0ULL;
            
            // 检查是否有冲突
            bool hasConflict = false;
            for (std::size_t blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
                std::uint64_t active = (activeData[blockIdx] ^ flip) | full;
                if (~currentData[blockIdx] & badData[blockIdx] & active) {
                    hasConflict = true;
                    break;
                }
            }
            
            if (hasConflict) continue;
            
            // 计算覆盖分数
            int coverageCount = 0;
            for (std::size_t blockIdx : sparseBlocks) {
                std::uint64_t active = (activeData[blockIdx] ^ flip) | full;
                coverageCount += popcount64(active & ~currentData[blockIdx] & careMaskData[blockIdx]);
            }
            
            if (std::make_pair(coverageCount, 0) > bestCount) {
                bestCount = std::make_pair(coverageCount, 0);
                bestPieceIndex = static_cast<int>(i);
                
                // 保存最佳active模式
                for (std::size_t blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
                    if (mode == 0) {
                        bestActive[blockIdx] = ~activeData[blockIdx];
                    } else if (mode == 1) {
                        bestActive[blockIdx] = activeData[blockIdx];
                    } else {
                        bestActive[blockIdx] = ~0ULL;
                    }
                }
            }
        }
    }
    
    if (bestPieceIndex == -1) {
        return -1; // 没有找到合适的piece
    }
    
    // 应用最佳piece
    std::size_t selectedPieceIdx = imageIndices[bestPieceIndex];
    int depth = 0;
    
    if (selectedPieceIdx < pieces.getPieceCount()) {
        depth = pieces.pieces[selectedPieceIdx].depth;
        
        // 更新结果图像
        std::size_t globalBitIndex = 0;
        for (std::size_t dagIdx = 0; dagIdx < result.size(); ++dagIdx) {
            try {
                const arc::core::Grid& pieceImage = pieces.getPieceImage(selectedPieceIdx, dagIdx);
                std::size_t imageSize = imageSizes[dagIdx];
                
                for (std::size_t pixelIdx = 0; pixelIdx < imageSize && pixelIdx < pieceImage.pixels.size(); ++pixelIdx) {
                    std::size_t blockIdx = globalBitIndex / 64;
                    std::size_t bitIdx = globalBitIndex % 64;
                    
                    if (blockIdx < bestActive.size() && 
                        (bestActive[blockIdx] >> bitIdx & 1) && 
                        result[dagIdx].pixels[pixelIdx] == 10) { // 未填充的像素
                        
                        result[dagIdx].pixels[pixelIdx] = pieceImage.pixels[pixelIdx];
                    }
                    
                    globalBitIndex++;
                }
            } catch (const std::exception&) {
                // 如果无法应用piece，跳过
                globalBitIndex += imageSizes[dagIdx];
            }
        }
    }
    
    // 更新current bitset
    for (std::size_t bitIdx = 0; bitIdx < current.size(); ++bitIdx) {
        std::size_t blockIdx = bitIdx / 64;
        std::size_t inBlockIdx = bitIdx % 64;
        
        if (blockIdx < bestActive.size() && (bestActive[blockIdx] >> inBlockIdx & 1)) {
            current.set(bitIdx, true);
        }
    }
    
    return depth;
}

// 主要组合函数 - 对应icecuber的greedyCompose2
std::vector<Candidate> GreedyComposer::compose(
    arc::piece::PieceCollection& pieces,
    const std::vector<arc::core::Grid>& targets,
    const std::vector<arc::core::Point>& outputSizes
) {
    if (pieces.getPieceCount() == 0) {
        return {};
    }
    
    std::vector<Candidate> results;
    
    // 创建初始图像
    std::vector<arc::core::Grid> initialImages;
    std::vector<std::size_t> imageSizes;
    
    for (std::size_t i = 0; i < pieces.getDAGCount(); ++i) {
        arc::core::Point size = (i < outputSizes.size()) ? outputSizes[i] : arc::core::Point{10, 10};
        arc::core::Grid initialImg(size.x, size.y);
        
        // 填充为未定义值（用10表示）
        std::fill(initialImg.pixels.begin(), initialImg.pixels.end(), 10);
        
        initialImages.push_back(initialImg);
        imageSizes.push_back(size.x * size.y);
    }
    
    // 预处理pieces数据
    std::vector<std::uint64_t> activeMem, badMem;
    std::vector<std::size_t> activeIndices, badIndices, imageIndices;
    
    preprocessPieces(pieces, targets, initialImages, activeMem, badMem,
                    activeIndices, badIndices, imageIndices, imageSizes);
    
    // 计算总的位数
    std::size_t totalBits = 0;
    for (std::size_t size : imageSizes) {
        totalBits += size;
    }
    
    // 找到最大piece深度
    int maxPieceDepth = 0;
    for (const auto& piece : pieces.pieces) {
        maxPieceDepth = std::max(maxPieceDepth, static_cast<int>(piece.depth));
    }
    
    // 多种策略组合
    for (int pieceDepthThreshold = maxPieceDepth % 10; 
         pieceDepthThreshold <= maxPieceDepth; 
         pieceDepthThreshold += 10) {
        
        for (int iteration = 0; iteration < 10; ++iteration) {
            for (int mask = 1; mask < std::min(1 << targets.size(), 1 << 5); ++mask) {
                std::vector<int> maskVector;
                for (std::size_t j = 0; j < targets.size(); ++j) {
                    if (mask >> j & 1) {
                        maskVector.push_back(static_cast<int>(j));
                    }
                }
                
                if (iteration >= maskVector.size()) {
                    continue;
                }
                
                int careMask = 1 << maskVector[iteration];
                
                // 初始化位集合
                CompactBitset current(totalBits);
                CompactBitset careMaskBitset(totalBits);
                
                // 设置关心的区域
                std::size_t baseBit = 0;
                for (std::size_t j = 0; j < imageSizes.size(); ++j) {
                    if (!(mask >> j & 1)) {
                        // 不关心的区域标记为已填充
                        for (std::size_t k = 0; k < imageSizes[j]; ++k) {
                            current.set(baseBit + k, true);
                        }
                    }
                    if (careMask >> j & 1) {
                        // 关心的区域
                        for (std::size_t k = 0; k < imageSizes[j]; ++k) {
                            careMaskBitset.set(baseBit + k, true);
                        }
                    }
                    baseBit += imageSizes[j];
                }
                
                // 贪心组合
                std::vector<arc::core::Grid> candidateResult = initialImages;
                int pieceCount = 0;
                int sumDepth = 0;
                int maxDepth = 0;
                
                for (int iter = 0; iter < config_.maxIterations; ++iter) {
                    int depth = greedyComposeCore(current, careMaskBitset, pieceDepthThreshold,
                                                candidateResult, pieces, imageSizes,
                                                activeMem, badMem, activeIndices, 
                                                badIndices, imageIndices);
                    
                    if (depth == -1) break;
                    
                    pieceCount++;
                    sumDepth += depth;
                    maxDepth = std::max(maxDepth, depth);
                    
                    // 应用贪心填充
                    if (config_.enableGreedyFill) {
                        std::vector<arc::core::Grid> filledResult = candidateResult;
                        
                        // 填充未定义的像素并验证
                        bool isValid = true;
                        for (auto& img : filledResult) {
                            img = greedyFillBlack(img);
                            if (img.width * img.height == 0) {
                                isValid = false;
                                break;
                            }
                        }
                        
                        if (isValid) {
                            results.emplace_back(filledResult, pieceCount, sumDepth, maxDepth);
                            
                            if (results.size() >= config_.maxCandidates) {
                                goto composition_complete;
                            }
                        }
                    }
                }
                
                // 添加未完全填充的候选解
                results.emplace_back(candidateResult, pieceCount, sumDepth, maxDepth);
                
                if (results.size() >= config_.maxCandidates) {
                    goto composition_complete;
                }
            }
        }
    }
    
composition_complete:
    std::cout << "贪心组合完成，生成了 " << results.size() << " 个候选解" << std::endl;
    return results;
}

// ============================================================================
// CandidateComposer 实现
// ============================================================================

std::vector<Candidate> CandidateComposer::composePieces(
    arc::piece::PieceCollection& pieces,
    const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs,
    const std::vector<arc::core::Point>& outputSizes
) {
    std::vector<arc::core::Grid> targets;
    for (const auto& [input, output] : trainingPairs) {
        targets.push_back(output);
    }
    
    return greedyComposer_.compose(pieces, targets, outputSizes);
}

double CandidateComposer::calculatePriorScore(const Candidate& candidate) const {
    // 对应icecuber的prior计算
    return candidate.maxDepth + candidate.pieceCount * 0.001;
}

bool CandidateComposer::validateAnswerImage(const arc::core::Grid& answer) const {
    return answer.width <= 30 && answer.height <= 30 && answer.width * answer.height > 0;
}

int CandidateComposer::calculateTrainingMatches(
    const std::vector<arc::core::Grid>& candidateImages,
    const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs
) const {
    int matches = 0;
    
    std::size_t minSize = std::min(candidateImages.size(), trainingPairs.size());
    for (std::size_t i = 0; i < minSize; ++i) {
        const arc::core::Grid& candidateOutput = candidateImages[i];
        const arc::core::Grid& expectedOutput = trainingPairs[i].second;
        
        // 简单的像素级比较
        if (candidateOutput.width == expectedOutput.width &&
            candidateOutput.height == expectedOutput.height) {
            
            bool isMatch = true;
            for (int y = 0; y < candidateOutput.height; ++y) {
                for (int x = 0; x < candidateOutput.width; ++x) {
                    if (candidateOutput(y, x) != expectedOutput(y, x)) {
                        isMatch = false;
                        break;
                    }
                }
                if (!isMatch) break;
            }
            
            if (isMatch) matches++;
        }
    }
    
    return matches;
}

std::vector<Candidate> CandidateComposer::evaluateCandidates(
    const std::vector<Candidate>& candidates,
    const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs
) {
    std::vector<Candidate> evaluatedCandidates;
    
    for (const Candidate& candidate : candidates) {
        if (candidate.maxDepth < 0 || candidate.pieceCount < 0) {
            continue; // 跳过无效的候选解
        }
        
        double prior = calculatePriorScore(candidate);
        int matches = calculateTrainingMatches(candidate.images, trainingPairs);
        
        // 对应icecuber的score计算
        double score = matches - prior * 0.01;
        
        // 验证答案图像
        if (!candidate.images.empty()) {
            const arc::core::Grid& answer = candidate.images.back();
            if (!validateAnswerImage(answer)) {
                continue; // 跳过无效的答案
            }
        }
        
        if (matches > 0) {
            Candidate evaluatedCandidate(candidate.images, score);
            evaluatedCandidate.pieceCount = candidate.pieceCount;
            evaluatedCandidate.sumDepth = candidate.sumDepth;
            evaluatedCandidate.maxDepth = candidate.maxDepth;
            
            evaluatedCandidates.push_back(evaluatedCandidate);
        }
    }
    
    // 按分数排序 - 对应icecuber的sort
    std::sort(evaluatedCandidates.begin(), evaluatedCandidates.end());
    
    return evaluatedCandidates;
}

// ============================================================================
// AdvancedCandidateGenerator 实现
// ============================================================================

AdvancedCandidateGenerator::AdvancedCandidateGenerator(const Strategy& strategy) 
    : strategy_(strategy) {}

std::vector<Candidate> AdvancedCandidateGenerator::generateCandidates(
    arc::piece::PieceCollection& pieces,
    const std::vector<std::pair<arc::core::Grid, arc::core::Grid>>& trainingPairs,
    const arc::core::Grid& testInput,
    const std::vector<arc::core::Point>& outputSizes
) {
    std::vector<std::vector<Candidate>> candidateSets;
    
    // 1. 贪心组合策略
    if (strategy_.useGreedyComposition) {
        auto greedyCandidates = composer_.composePieces(pieces, trainingPairs, outputSizes);
        candidateSets.push_back(std::move(greedyCandidates));
    }
    
    // 2. Piece枚举策略
    if (strategy_.usePieceEnumeration) {
        auto pieceCandidates = generateFromPieces(pieces);
        candidateSets.push_back(std::move(pieceCandidates));
    }
    
    // 3. 融合所有候选解
    return fuseCandidates(candidateSets);
}

std::vector<Candidate> AdvancedCandidateGenerator::generateFromPieces(
    const arc::piece::PieceCollection& pieces
) {
    std::vector<Candidate> candidates;
    
    // 从pieces直接生成候选解
    for (std::size_t pieceIdx = 0; pieceIdx < pieces.getPieceCount(); ++pieceIdx) {
        try {
            std::vector<arc::core::Grid> images;
            
            for (std::size_t dagIdx = 0; dagIdx < pieces.getDAGCount(); ++dagIdx) {
                images.push_back(pieces.getPieceImage(pieceIdx, dagIdx));
            }
            
            const auto& piece = pieces.pieces[pieceIdx];
            candidates.emplace_back(images, 1, piece.depth, piece.depth);
            
        } catch (const std::exception&) {
            // 跳过无法获取的pieces
            continue;
        }
    }
    
    return candidates;
}

std::vector<Candidate> AdvancedCandidateGenerator::filterByDepth(
    const std::vector<Candidate>& candidates,
    int maxDepth
) {
    std::vector<Candidate> filtered;
    
    for (const Candidate& candidate : candidates) {
        if (candidate.maxDepth <= maxDepth) {
            filtered.push_back(candidate);
        }
    }
    
    return filtered;
}

std::vector<Candidate> AdvancedCandidateGenerator::filterByScore(
    const std::vector<Candidate>& candidates,
    double minScore,
    std::size_t maxCount
) {
    std::vector<Candidate> filtered = candidates;
    
    // 按分数排序
    std::sort(filtered.begin(), filtered.end());
    
    // 过滤低分和数量限制
    std::vector<Candidate> result;
    for (const Candidate& candidate : filtered) {
        if (candidate.score >= minScore && result.size() < maxCount) {
            result.push_back(candidate);
        }
    }
    
    return result;
}

std::vector<Candidate> AdvancedCandidateGenerator::fuseCandidates(
    const std::vector<std::vector<Candidate>>& candidateSets
) {
    std::vector<Candidate> allCandidates;
    
    // 合并所有候选解集合
    for (const auto& candidateSet : candidateSets) {
        allCandidates.insert(allCandidates.end(), candidateSet.begin(), candidateSet.end());
    }
    
    // 应用过滤策略
    if (strategy_.useDepthFiltering) {
        allCandidates = filterByDepth(allCandidates, strategy_.maxDepthRange);
    }
    
    if (strategy_.useScoreFiltering) {
        allCandidates = filterByScore(allCandidates, -10.0, strategy_.maxCandidatesPerStrategy);
    }
    
    return allCandidates;
}

// ============================================================================
// 辅助函数实现
// ============================================================================

std::vector<Candidate> createTestCandidates(const std::vector<arc::core::Grid>& images) {
    std::vector<Candidate> candidates;
    
    if (!images.empty()) {
        candidates.emplace_back(images, 1.0);
    }
    
    return candidates;
}

void printCandidateStatistics(const std::vector<Candidate>& candidates) {
    std::cout << "=== Candidate统计信息 ===" << std::endl;
    std::cout << "候选解数量: " << candidates.size() << std::endl;
    
    if (candidates.empty()) return;
    
    double totalScore = 0.0;
    int validCount = 0;
    
    for (const Candidate& candidate : candidates) {
        if (candidate.isValid()) {
            validCount++;
            totalScore += candidate.score;
        }
    }
    
    std::cout << "有效候选解: " << validCount << std::endl;
    
    if (validCount > 0) {
        std::cout << "平均分数: " << (totalScore / validCount) << std::endl;
        std::cout << "最高分数: " << candidates[0].score << std::endl;
    }
}

bool validateCandidates(const std::vector<Candidate>& candidates) {
    for (const Candidate& candidate : candidates) {
        if (!candidate.isValid()) {
            return false;
        }
    }
    return true;
}

} // namespace arc::candidate 