import numpy as np
from arc_solver.data.task import Task, TaskExample
from arc_solver.solvers.chess import ChessSolver

def test_comprehensive_chess_solver():
    """全面测试ChessSolver的各种情况"""
    
    print("=" * 60)
    print("ChessSolver 综合测试")
    print("=" * 60)
    
    # 测试1: 标准的grid+chess模式
    print("\n测试1: 网格结构 + Chess Pattern输出")
    test_grid_chess_pattern()
    
    # 测试2: chess pattern检测
    print("\n测试2: Chess Pattern 检测")
    test_pattern_detection()
    
    # 测试3: chess pattern预测
    print("\n测试3: Chess Pattern 预测")
    test_pattern_prediction()
    
    # 测试4: 非chess任务
    print("\n测试4: 非Chess任务处理")
    test_non_chess_task()

def test_grid_chess_pattern():
    """测试网格+chess pattern的标准情况"""
    
    # 7x7网格，颜色2作为网格线
    train_input = np.array([
        [2, 2, 2, 2, 2, 2, 2],
        [2, 0, 1, 2, 0, 1, 2],
        [2, 1, 0, 2, 1, 0, 2],
        [2, 2, 2, 2, 2, 2, 2],
        [2, 0, 1, 2, 0, 1, 2],
        [2, 1, 0, 2, 1, 0, 2],
        [2, 2, 2, 2, 2, 2, 2]
    ])
    
    # 标准chess pattern
    train_output = np.array([
        [0, 1, 0, 1],
        [1, 0, 1, 0],
        [0, 1, 0, 1],
        [1, 0, 1, 0]
    ])
    
    test_input = np.array([
        [2, 2, 2, 2, 2],
        [2, 1, 0, 1, 2],
        [2, 0, 1, 0, 2],
        [2, 2, 2, 2, 2]
    ])
    
    train = [TaskExample(input=train_input, output=train_output)]
    test = [test_input]
    task = Task(task_id="grid_chess", train=train, test=test)
    
    solver = ChessSolver()
    
    print(f"  Can solve: {solver.can_solve(task)}")
    
    if solver.can_solve(task):
        results = solver.solve(task)
        print(f"  生成了 {len(results)} 个chess pattern候选")
        for i, result in enumerate(results):
            print(f"  候选 {i+1}: shape {result.shape}")
            print(f"    {result.tolist()}")
    else:
        print("  无法解决此任务")

def test_pattern_detection():
    """测试不同模式的检测能力"""
    
    solver = ChessSolver()
    
    # 标准2色chess
    chess_2color = np.array([
        [0, 1, 0, 1],
        [1, 0, 1, 0],
        [0, 1, 0, 1]
    ])
    
    # 3色chess
    chess_3color = np.array([
        [0, 1, 2],
        [1, 2, 0],
        [2, 0, 1]
    ])
    
    # 非chess模式
    non_chess = np.array([
        [0, 1, 2],
        [1, 1, 0],
        [2, 0, 1]
    ])
    
    # 反chess模式（简化测试）
    anti_chess = np.array([
        [0, 0, 1],
        [0, 1, 1],
        [1, 1, 2]
    ])
    
    print(f"  2色chess: {solver._has_chess(chess_2color)}")
    print(f"  3色chess: {solver._has_chess(chess_3color)}")
    print(f"  非chess: {solver._has_chess(non_chess)}")
    print(f"  反chess: {solver._has_antichess(anti_chess)}")

def test_pattern_prediction():
    """测试chess pattern的预测生成"""
    
    solver = ChessSolver()
    
    # 测试矩阵
    test_matrix = np.array([
        [0, 1, 0],
        [1, 0, 1],
        [0, 1, 0]
    ])
    
    predictions = solver._predict_chess(test_matrix)
    print(f"  输入矩阵: {test_matrix.shape}")
    print(f"  生成 {len(predictions)} 个预测:")
    
    for i, pred in enumerate(predictions):
        print(f"    预测 {i+1}:")
        for row in pred:
            print(f"      {row.tolist()}")

def test_non_chess_task():
    """测试非chess任务的处理"""
    
    # 创建一个没有网格结构的任务
    train_input = np.array([
        [0, 1, 2],
        [1, 2, 0],
        [2, 0, 1]
    ])
    
    train_output = np.array([
        [1, 2, 0],
        [2, 0, 1],
        [0, 1, 2]
    ])
    
    test_input = np.array([
        [0, 1, 2],
        [1, 2, 0],
        [2, 0, 1]
    ])
    
    train = [TaskExample(input=train_input, output=train_output)]
    test = [test_input]
    task = Task(task_id="non_chess", train=train, test=test)
    
    solver = ChessSolver()
    can_solve = solver.can_solve(task)
    
    print(f"  Can solve: {can_solve}")
    print(f"  预期: False (因为没有网格结构)")

if __name__ == "__main__":
    test_comprehensive_chess_solver() 