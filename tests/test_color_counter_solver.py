import numpy as np
from arc_solver.data.task import Task, TaskExample
from arc_solver.solvers.color_counter import ColorCounterSolver

def test_color_counter_solver():
    # 构造一个颜色分布一致的简单任务
    train = [
        TaskExample(input=np.array([[1,2],[2,1]]), output=np.array([[2,1],[1,2]])),
        TaskExample(input=np.array([[2,1],[1,2]]), output=np.array([[1,2],[2,1]]))
    ]
    test = [np.array([[1,2],[2,1]])]
    task = Task(task_id="dummy", train=train, test=test)
    solver = ColorCounterSolver()
    assert solver.can_solve(task)
    results = solver.solve(task)
    print("Test input:")
    print(test[0])
    print("Predicted output:")
    for res in results:
        print(res)

def test_color_counter_solver_fail():
    # 构造一个颜色分布不一致的任务
    train = [
        TaskExample(input=np.array([[1,1],[2,2]]), output=np.array([[2,2],[2,2]]))
    ]
    test = [np.array([[1,2],[2,1]])]
    task = Task(task_id="dummy2", train=train, test=test)
    solver = ColorCounterSolver()
    assert not solver.can_solve(task)
    results = solver.solve(task)
    assert results == []
    print("Fail test passed.")

if __name__ == "__main__":
    test_color_counter_solver()
    test_color_counter_solver_fail() 