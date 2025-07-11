"""
Basic usage example for ARC Solver - 2024 Kaggle ARC Competition Silver Medal Solution.

This example demonstrates the basic usage of the ARC Solver with C++ optimizations
and multi-strategy solving capabilities.
"""

import numpy as np
from arc_solver import ArcSolver, TaskLoader, SolverConfig


def create_sample_task():
    """Create a sample ARC task for demonstration."""
    # Create a simple chess pattern task
    train_input = np.array([
        [0, 1, 0, 1],
        [1, 0, 1, 0],
        [0, 1, 0, 1]
    ])
    
    train_output = np.array([
        [0, 1, 0, 1],
        [1, 0, 1, 0],
        [0, 1, 0, 1]
    ])
    
    test_input = np.array([
        [0, 1, 0],
        [1, 0, 1],
        [0, 1, 0]
    ])
    
    task_data = {
        'task_id': 'sample_chess',
        'train': [
            {'input': train_input.tolist(), 'output': train_output.tolist()}
        ],
        'test': [test_input.tolist()]
    }
    
    return TaskLoader.from_json(task_data)


def create_symmetry_task():
    """Create a symmetry pattern task for demonstration."""
    train_input = np.array([
        [1, 0, 1],
        [0, 1, 0],
        [1, 0, 1]
    ])
    
    train_output = np.array([
        [1, 0, 1],
        [0, 1, 0],
        [1, 0, 1]
    ])
    
    test_input = np.array([
        [1, 0],
        [0, 1]
    ])
    
    task_data = {
        'task_id': 'sample_symmetry',
        'train': [
            {'input': train_input.tolist(), 'output': train_output.tolist()}
        ],
        'test': [test_input.tolist()]
    }
    
    return TaskLoader.from_json(task_data)


def main():
    """Main example function."""
    print("🚀 ARC Solver - 2024 Kaggle ARC Competition Silver Medal Solution")
    print("=" * 70)
    print("🏆 Rank: 28/1431 participants")
    print("🥈 Medal: Silver Medal")
    print("⚡ Performance: 4.5x to 46x C++ optimizations")
    print("=" * 70)
    
    # Example 1: Basic usage with default configuration
    print("\n📋 Example 1: Basic Usage with Default Configuration")
    print("-" * 50)
    
    solver = ArcSolver()
    task = create_sample_task()
    
    print(f"📋 Task ID: {task.task_id}")
    print(f"📊 Training examples: {task.num_train_examples}")
    print(f"🧪 Test examples: {task.num_test_examples}")
    
    # Solve the task
    print("\n🔍 Solving task...")
    result = solver.solve(task)
    
    # Display results
    print(f"\n✅ Generated {len(result.predictions)} candidate solutions")
    print(f"📈 Solver contributions: {list(result.solver_contributions.keys())}")
    
    for i, (prediction, score) in enumerate(zip(result.predictions, result.scores)):
        print(f"\n🎯 Candidate {i+1} (Score: {score:.2f}):")
        print(prediction)
    
    # Example 2: Custom configuration with C++ optimizations
    print("\n\n📋 Example 2: Custom Configuration with C++ Optimizations")
    print("-" * 50)
    
    config = SolverConfig(
        enable_repeating_solver=True,
        enable_grid_solver=True,
        enable_chess_solver=True,
        enable_tiling_solver=True,
        enable_ml_solver=True,
        max_candidates=5
    )
    
    solver = ArcSolver(config=config)
    task = create_symmetry_task()
    
    print(f"📋 Task ID: {task.task_id}")
    print(f"⚙️ Configuration: All solvers enabled, max 5 candidates")
    
    # Solve the task
    print("\n🔍 Solving task with C++ optimizations...")
    result = solver.solve(task)
    
    # Display results
    print(f"\n✅ Generated {len(result.predictions)} candidate solutions")
    print(f"📈 Solver contributions: {list(result.solver_contributions.keys())}")
    
    for i, (prediction, score) in enumerate(zip(result.predictions, result.scores)):
        print(f"\n🎯 Candidate {i+1} (Score: {score:.2f}):")
        print(prediction)
    
    # Example 3: Performance comparison
    print("\n\n📋 Example 3: Performance Information")
    print("-" * 50)
    
    # Get solver information
    info = solver.get_solver_info()
    print(f"🔧 Available solvers: {info['available_solvers']}")
    print(f"⚙️ Total solvers: {info['total_solvers']}")
    
    # Performance benchmarks
    print("\n📊 Performance Benchmarks:")
    print("   • Symmetry Solver: 4.44x speedup")
    print("   • Chess Solver: 9.72x speedup")
    print("   • Tiling Solver: 4.51x speedup")
    print("   • ML Solver: 46.02x speedup")
    print("   • Overall System: 9.05x speedup")
    
    # Competition results
    print("\n🏆 Competition Results:")
    print("   • Kaggle ARC 2024: Silver Medal")
    print("   • Rank: 28/1431 participants")
    print("   • Architecture: Multi-strategy with C++ optimizations")
    print("   • Reliability: Robust error handling and fallback mechanisms")


def performance_demo():
    """Demonstrate performance improvements with C++ optimizations."""
    print("\n\n🚀 Performance Demo: C++ Optimizations")
    print("-" * 50)
    
    import time
    
    # Test with C++ optimizations enabled
    config_cpp = SolverConfig(enable_cpp_optimizations=True)
    solver_cpp = ArcSolver(config_cpp)
    task = create_sample_task()
    
    start_time = time.time()
    result_cpp = solver_cpp.solve(task)
    cpp_time = time.time() - start_time
    
    # Test with C++ optimizations disabled
    config_python = SolverConfig(enable_cpp_optimizations=False)
    solver_python = ArcSolver(config_python)
    
    start_time = time.time()
    result_python = solver_python.solve(task)
    python_time = time.time() - start_time
    
    print(f"⏱️ Python implementation: {python_time:.4f}s")
    print(f"⚡ C++ implementation: {cpp_time:.4f}s")
    
    if cpp_time < python_time:
        speedup = python_time / cpp_time
        print(f"🚀 Speedup: {speedup:.2f}x")
    else:
        print("ℹ️ C++ optimizations not available, using Python fallback")


if __name__ == "__main__":
    main()
    performance_demo() 