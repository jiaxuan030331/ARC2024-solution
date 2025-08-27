# ARC DAG Solver Fallback Demo

## 概述

这个demo展示了ARC求解器中DAG fallback机制的完整工作流程，通过一个具体案例说明：
1. 专用solvers（Repeating、Chess、Tiling等）无法解决的复杂任务
2. 置信度阈值检查机制
3. DAG solver作为fallback的成功求解过程
4. 2层深度搜索树的可视化

## 文件说明

- `demo_dag_fallback.ipynb` - 主要的demo notebook
- `DAG_FALLBACK_DEMO.md` - 本说明文档

## Demo内容

### 1. 测试任务
- **规律**: 输入矩阵 → 转置 → 颜色映射(1→3, 2→4) → 输出
- **复杂性**: 需要2步变换，单一专用solver难以处理
- **可视化**: 彩色矩阵展示，与ARC标准格式一致

### 2. 求解对比
- **低阈值配置**: 专用solver优先，DAG不会被触发
- **高阈值配置**: 触发DAG fallback机制
- **详细日志**: 实时显示求解过程、耗时、分数等

### 3. 可视化组件
- **矩阵可视化**: 复用`main.ipynb`的绘制逻辑
- **DAG搜索树**: 2层深度的搜索过程图解
- **结果对比**: 不同配置下的求解结果展示

### 4. 关键指标
- **置信度阈值**: 控制fallback触发的关键参数
- **搜索深度**: 限制为2层，确保实时性能
- **候选评分**: 展示不同solver的分数对比

## 运行方法

### 环境要求
```bash
# 确保在arc_solver目录下
cd /path/to/arc_solver

# 安装依赖
pip install matplotlib numpy jupyter

# 启动Jupyter
jupyter notebook demo_dag_fallback.ipynb
```

### 运行步骤
1. **启动notebook**: 按顺序执行所有cells
2. **观察输出**: 查看专用solver vs DAG fallback的对比
3. **调整参数**: 可以修改置信度阈值来观察不同行为

## 核心演示点

### Fallback机制触发
```python
# 高阈值配置 - 确保触发DAG fallback
config_high_threshold.dag_high_confidence_threshold = 25.0

# 低阈值配置 - 专用solver优先
config_low_threshold.dag_high_confidence_threshold = 5.0
```

### DAG搜索可视化
- **绿色路径**: 成功的变换序列 (Input → Transpose → ColorMap → Output)
- **灰色路径**: 被探索但未成功的路径
- **层级结构**: 清晰展示2层搜索深度

### 性能对比
```
配置        | 低阈值 (专用优先) | 高阈值 (DAG fallback)
使用DAG     |      False       |        True
预测数      |        N         |         M
专用最高分  |       X.X        |        Y.Y
DAG最高分   |       0.0        |        Z.Z
```

## 预期效果

### 成功场景
- ✅ DAG fallback成功解决专用solver无法处理的复杂任务
- ✅ 通过2层搜索找到正确的变换序列
- ✅ 置信度阈值机制确保资源的高效利用

### 展示价值
- **智能补充**: DAG作为专用solver的智能后备
- **效率优化**: 避免不必要的复杂计算
- **灵活配置**: 可调节的阈值和搜索深度
- **可视化调试**: 直观的搜索过程展示

## 技术特点

### 数据流程
1. **任务输入**: 标准ARC JSON格式
2. **专用solver尝试**: Repeating、Chess、Tiling等
3. **置信度评估**: 基于配置阈值判断
4. **DAG fallback**: 多层搜索与候选生成
5. **结果合并**: 智能选择最佳答案

### 可视化技术
- **矩阵绘制**: matplotlib + ListedColormap
- **搜索树**: 节点-边图形化展示
- **动态对比**: 实时更新的结果比较

## 应用场景

### 适用任务
- 需要多步变换的复杂模式
- 专用solver覆盖不足的边缘案例
- 需要组合多种操作的任务

### 配置建议
- **实时应用**: 阈值 15-20，深度 2-3
- **精度优先**: 阈值 10-15，深度 3-4
- **演示用途**: 阈值 25+，深度 2

## 扩展可能

### 未来增强
- 更多测试案例
- 实时动画展示
- 交互式参数调节
- 性能基准测试

### 集成方向
- Web界面展示
- 批量任务测试
- 与其他solver的详细比较

---

🎯 **Demo目标**: 最大化让观众理解DAG solver在复杂ARC任务中的价值和工作机制 