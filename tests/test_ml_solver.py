import numpy as np
from arc_solver.data.task import Task, TaskExample
from arc_solver.solvers.ml import MLSolver

def test_ml_solver():
    # 构造一个input包含output子块的简单任务
    train = [
        TaskExample(input=np.array([[1,2,3],[4,5,6],[7,8,9]]), output=np.array([[5,6],[8,9]])),
        TaskExample(input=np.array([[9,8,7],[6,5,4],[3,2,1]]), output=np.array([[5,4],[2,1]]))
    ]
    test = [np.array([[1,2,3],[4,5,6],[7,8,9]])]
    task = Task(task_id="dummy", train=train, test=test)
    solver = MLSolver()
    assert solver.can_solve(task)
    results = solver.solve(task)
    print("Test input:")
    print(test[0])
    print("Predicted output:")
    for res in results:
        print(res)

def test_ml_solver_fail():
    # 构造一个input不包含output子块的任务
    train = [
        TaskExample(input=np.array([[1,1,1],[1,1,1],[1,1,1]]), output=np.array([[2,2],[2,2]]))
    ]
    test = [np.array([[1,1,1],[1,1,1],[1,1,1]])]
    task = Task(task_id="fail", train=train, test=test)
    solver = MLSolver()
    assert not solver.can_solve(task)
    results = solver.solve(task)
    assert results == []
    print("Fail test passed.")

if __name__ == "__main__":
    test_ml_solver()
    test_ml_solver_fail() 