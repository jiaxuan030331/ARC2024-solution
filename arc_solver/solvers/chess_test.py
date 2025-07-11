import numpy as np
from arc_solver.data.task import Task, TaskExample
from arc_solver.solvers.chess import ChessSolver

def test_chess_solver():
    # 构造一个有真正网格结构的输入：用颜色2作为网格线
    train_input = np.array([
        [2, 2, 2, 2, 2, 2, 2],  # 网格线(全为2)
        [2, 0, 1, 2, 0, 1, 2],  # 内容行
        [2, 1, 0, 2, 1, 0, 2],  # 内容行
        [2, 2, 2, 2, 2, 2, 2],  # 网格线(全为2)
        [2, 0, 1, 2, 0, 1, 2],  # 内容行
        [2, 1, 0, 2, 1, 0, 2],  # 内容行
        [2, 2, 2, 2, 2, 2, 2]   # 网格线(全为2)
    ])
    
    # 标准chess pattern output
    train_output = np.array([
        [0, 1, 0, 1],
        [1, 0, 1, 0],
        [0, 1, 0, 1],
        [1, 0, 1, 0]
    ])
    
    test_input = np.array([
        [2, 2, 2, 2, 2],  # 网格线(全为2)
        [2, 1, 0, 1, 2],  # 内容行
        [2, 0, 1, 0, 2],  # 内容行
        [2, 2, 2, 2, 2]   # 网格线(全为2)
    ])
    
    train = [TaskExample(input=train_input, output=train_output)]
    test = [test_input]
    task = Task(task_id="chess_test", train=train, test=test)
    
    solver = ChessSolver()
    print("Testing ChessSolver...")
    
    # 调试can_solve的每个步骤
    task_dict = {
        'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
    }
    
    print(f"Training input shape: {train_input.shape}")
    print(f"Grid check: {solver._check_grid(task_dict)}")
    print(f"Chess check: {solver._check_chess(task_dict, input=False, output=True)}")
    print(f"Output has chess: {solver._has_chess(train_output)}")
    print(f"Output has anti-chess: {solver._has_antichess(train_output)}")
    
    # Test can_solve
    can_solve = solver.can_solve(task)
    print(f"Can solve: {can_solve}")
    
    if can_solve:
        results = solver.solve(task)
        print("Test input:")
        print(test_input)
        print("Predicted chess patterns:")
        for i, res in enumerate(results):
            print(f"Pattern {i+1}:")
            print(res)
    else:
        print("Solver cannot handle this task")

def test_chess_pattern_detection():
    # 测试chess pattern检测
    solver = ChessSolver()
    
    # Valid chess pattern - 标准棋盘
    chess_matrix = np.array([
        [0, 1, 0, 1],
        [1, 0, 1, 0],
        [0, 1, 0, 1]
    ])
    
    # 三色chess pattern
    chess_3color = np.array([
        [0, 1, 2, 0],
        [1, 2, 0, 1],
        [2, 0, 1, 2]
    ])
    
    # Non-chess pattern
    random_matrix = np.array([
        [0, 1, 0],
        [1, 2, 1],
        [0, 1, 2]
    ])
    
    print("Chess pattern detection tests:")
    print(f"Chess matrix (2-color) has chess pattern: {solver._has_chess(chess_matrix)}")
    print(f"Chess matrix (3-color) has chess pattern: {solver._has_chess(chess_3color)}")
    print(f"Random matrix has chess pattern: {solver._has_chess(random_matrix)}")

def test_chess_prediction():
    # 测试chess pattern预测
    solver = ChessSolver()
    
    input_matrix = np.array([
        [0, 1, 0],
        [1, 0, 1],
        [0, 1, 0]
    ])
    
    predictions = solver._predict_chess(input_matrix)
    print("Chess pattern predictions:")
    for i, pred in enumerate(predictions):
        print(f"Prediction {i+1}:")
        print(pred)

if __name__ == "__main__":
    test_chess_pattern_detection()
    print()
    test_chess_prediction() 
    print()
    test_chess_solver() 