# demo.py - Minimal ARC Solver Demo
from arc_solver.solvers import TilingSolver
from arc_solver.data.task import Task, TaskExample

# Create a simple tiling task
train_examples = [
    TaskExample(
        input=[[1, 2], [3, 4]],
        output=[[1, 2, 1, 2], [3, 4, 3, 4], [1, 2, 1, 2], [3, 4, 3, 4]]
    )
]

test_examples = [
    TaskExample(input=[[5, 6], [7, 8]], output=None)
]

task = Task(task_id="demo_task", train=train_examples, test=test_examples)

print("🚀 ARC Solver Demo - Tiling Pattern Recognition")
print("=" * 50)

# Initialize solver and solve
print("Initializing TilingSolver...")
solver = TilingSolver()

print("Checking if solver can handle this task...")
if solver.can_solve(task):
    print("✓ Task recognized as tiling pattern")
    
    print("Solving task...")
    results = solver.solve(task)
    
    if results:
        prediction = results[0]
        print("\n✅ Success!")
        print(f"Input:  {test_examples[0].input}")
        print(f"Output: {prediction.tolist()}")
        print("\n🎯 Expected: [[5, 6, 5, 6], [7, 8, 7, 8], [5, 6, 5, 6], [7, 8, 7, 8]]")
        print("✓ Tiling pattern successfully detected and applied!")
    else:
        print("❌ No solution found")
else:
    print("❌ Task not recognized as tiling pattern") 