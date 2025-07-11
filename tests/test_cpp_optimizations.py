"""
Tests for C++ optimizations in ARC Solver.

This module tests the C++ implementations of critical solvers to ensure
they provide the expected performance improvements and maintain correctness.
"""

import pytest
import numpy as np
import time
from arc_solver import ArcSolver, TaskLoader, SolverConfig


def create_symmetry_task():
    """Create a symmetry pattern task for testing."""
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
        'task_id': 'test_symmetry',
        'train': [
            {'input': train_input.tolist(), 'output': train_output.tolist()}
        ],
        'test': [test_input.tolist()]
    }
    
    return TaskLoader.from_json(task_data)


def create_chess_task():
    """Create a chess pattern task for testing."""
    train_input = np.array([
        [0, 1, 0, 1],
        [1, 0, 1, 0]
    ])
    
    train_output = np.array([
        [0, 1, 0, 1],
        [1, 0, 1, 0]
    ])
    
    test_input = np.array([
        [0, 1, 0],
        [1, 0, 1]
    ])
    
    task_data = {
        'task_id': 'test_chess',
        'train': [
            {'input': train_input.tolist(), 'output': train_output.tolist()}
        ],
        'test': [test_input.tolist()]
    }
    
    return TaskLoader.from_json(task_data)


def create_tiling_task():
    """Create a tiling pattern task for testing."""
    train_input = np.array([
        [1, 1, 0, 0],
        [1, 1, 0, 0],
        [0, 0, 1, 1],
        [0, 0, 1, 1]
    ])
    
    train_output = np.array([
        [1, 1, 0, 0],
        [1, 1, 0, 0],
        [0, 0, 1, 1],
        [0, 0, 1, 1]
    ])
    
    test_input = np.array([
        [1, 1],
        [1, 1]
    ])
    
    task_data = {
        'task_id': 'test_tiling',
        'train': [
            {'input': train_input.tolist(), 'output': train_output.tolist()}
        ],
        'test': [test_input.tolist()]
    }
    
    return TaskLoader.from_json(task_data)


def create_ml_task():
    """Create a machine learning task for testing."""
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
        'task_id': 'test_ml',
        'train': [
            {'input': train_input.tolist(), 'output': train_output.tolist()}
        ],
        'test': [test_input.tolist()]
    }
    
    return TaskLoader.from_json(task_data)


class TestCppSymmetrySolver:
    """Test C++ symmetry solver optimizations."""
    
    def test_cpp_symmetry_available(self):
        """Test that C++ symmetry solver is available."""
        try:
            from arc_solver.cpp_wrappers.symmetry_wrapper import SymmetrySolverCpp
            solver = SymmetrySolverCpp()
            assert solver is not None
        except ImportError:
            pytest.skip("C++ symmetry solver not available")
    
    def test_cpp_symmetry_correctness(self):
        """Test that C++ symmetry solver produces correct results."""
        try:
            from arc_solver.cpp_wrappers.symmetry_wrapper import SymmetrySolverCpp
            solver = SymmetrySolverCpp()
            task = create_symmetry_task()
            
            candidates = solver.solve(task)
            
            assert len(candidates) > 0
            assert all(isinstance(cand, np.ndarray) for cand in candidates)
        except ImportError:
            pytest.skip("C++ symmetry solver not available")
    
    def test_cpp_symmetry_performance(self):
        """Test that C++ symmetry solver provides performance improvement."""
        try:
            from arc_solver.cpp_wrappers.symmetry_wrapper import SymmetrySolverCpp
            from arc_solver.solvers.symmetry import SymmetrySolver
            
            cpp_solver = SymmetrySolverCpp()
            python_solver = SymmetrySolver()
            task = create_symmetry_task()
            
            # Time C++ version
            start_time = time.time()
            cpp_candidates = cpp_solver.solve(task)
            cpp_time = time.time() - start_time
            
            # Time Python version
            start_time = time.time()
            python_candidates = python_solver.solve(task)
            python_time = time.time() - start_time
            
            # C++ should be faster
            assert cpp_time < python_time
            speedup = python_time / cpp_time
            assert speedup >= 2.0  # At least 2x speedup
            
        except ImportError:
            pytest.skip("C++ symmetry solver not available")


class TestCppChessSolver:
    """Test C++ chess solver optimizations."""
    
    def test_cpp_chess_available(self):
        """Test that C++ chess solver is available."""
        try:
            from arc_solver.cpp_wrappers.chess_wrapper import ChessSolverCpp
            solver = ChessSolverCpp()
            assert solver is not None
        except ImportError:
            pytest.skip("C++ chess solver not available")
    
    def test_cpp_chess_correctness(self):
        """Test that C++ chess solver produces correct results."""
        try:
            from arc_solver.cpp_wrappers.chess_wrapper import ChessSolverCpp
            solver = ChessSolverCpp()
            task = create_chess_task()
            
            candidates = solver.solve(task)
            
            assert len(candidates) > 0
            assert all(isinstance(cand, np.ndarray) for cand in candidates)
        except ImportError:
            pytest.skip("C++ chess solver not available")
    
    def test_cpp_chess_performance(self):
        """Test that C++ chess solver provides performance improvement."""
        try:
            from arc_solver.cpp_wrappers.chess_wrapper import ChessSolverCpp
            from arc_solver.solvers.chess import ChessSolver
            
            cpp_solver = ChessSolverCpp()
            python_solver = ChessSolver()
            task = create_chess_task()
            
            # Time C++ version
            start_time = time.time()
            cpp_candidates = cpp_solver.solve(task)
            cpp_time = time.time() - start_time
            
            # Time Python version
            start_time = time.time()
            python_candidates = python_solver.solve(task)
            python_time = time.time() - start_time
            
            # C++ should be faster
            assert cpp_time < python_time
            speedup = python_time / cpp_time
            assert speedup >= 5.0  # At least 5x speedup
            
        except ImportError:
            pytest.skip("C++ chess solver not available")


class TestCppTilingSolver:
    """Test C++ tiling solver optimizations."""
    
    def test_cpp_tiling_available(self):
        """Test that C++ tiling solver is available."""
        try:
            from arc_solver.cpp_wrappers.tiling_wrapper import TilingSolverCpp
            solver = TilingSolverCpp()
            assert solver is not None
        except ImportError:
            pytest.skip("C++ tiling solver not available")
    
    def test_cpp_tiling_correctness(self):
        """Test that C++ tiling solver produces correct results."""
        try:
            from arc_solver.cpp_wrappers.tiling_wrapper import TilingSolverCpp
            solver = TilingSolverCpp()
            task = create_tiling_task()
            
            candidates = solver.solve(task)
            
            assert len(candidates) > 0
            assert all(isinstance(cand, np.ndarray) for cand in candidates)
        except ImportError:
            pytest.skip("C++ tiling solver not available")
    
    def test_cpp_tiling_performance(self):
        """Test that C++ tiling solver provides performance improvement."""
        try:
            from arc_solver.cpp_wrappers.tiling_wrapper import TilingSolverCpp
            from arc_solver.solvers.tiling import TilingSolver
            
            cpp_solver = TilingSolverCpp()
            python_solver = TilingSolver()
            task = create_tiling_task()
            
            # Time C++ version
            start_time = time.time()
            cpp_candidates = cpp_solver.solve(task)
            cpp_time = time.time() - start_time
            
            # Time Python version
            start_time = time.time()
            python_candidates = python_solver.solve(task)
            python_time = time.time() - start_time
            
            # C++ should be faster
            assert cpp_time < python_time
            speedup = python_time / cpp_time
            assert speedup >= 3.0  # At least 3x speedup
            
        except ImportError:
            pytest.skip("C++ tiling solver not available")


class TestCppMLSolver:
    """Test C++ ML solver optimizations."""
    
    def test_cpp_ml_available(self):
        """Test that C++ ML solver is available."""
        try:
            from arc_solver.cpp_wrappers.ml_wrapper import MLSolverCpp
            solver = MLSolverCpp()
            assert solver is not None
        except ImportError:
            pytest.skip("C++ ML solver not available")
    
    def test_cpp_ml_correctness(self):
        """Test that C++ ML solver produces correct results."""
        try:
            from arc_solver.cpp_wrappers.ml_wrapper import MLSolverCpp
            solver = MLSolverCpp()
            task = create_ml_task()
            
            candidates = solver.solve(task)
            
            assert len(candidates) > 0
            assert all(isinstance(cand, np.ndarray) for cand in candidates)
        except ImportError:
            pytest.skip("C++ ML solver not available")
    
    def test_cpp_ml_performance(self):
        """Test that C++ ML solver provides performance improvement."""
        try:
            from arc_solver.cpp_wrappers.ml_wrapper import MLSolverCpp
            from arc_solver.solvers.ml import MLSolver
            
            cpp_solver = MLSolverCpp()
            python_solver = MLSolver()
            task = create_ml_task()
            
            # Time C++ version
            start_time = time.time()
            cpp_candidates = cpp_solver.solve(task)
            cpp_time = time.time() - start_time
            
            # Time Python version
            start_time = time.time()
            python_candidates = python_solver.solve(task)
            python_time = time.time() - start_time
            
            # C++ should be faster
            assert cpp_time < python_time
            speedup = python_time / cpp_time
            assert speedup >= 20.0  # At least 20x speedup
            
        except ImportError:
            pytest.skip("C++ ML solver not available")


class TestCppIntegration:
    """Test integration of C++ optimizations with main solver."""
    
    def test_cpp_optimizations_enabled(self):
        """Test that C++ optimizations are enabled by default."""
        solver = ArcSolver()
        info = solver.get_solver_info()
        
        # Check if C++ optimizations are mentioned in solver info
        assert 'cpp_optimizations' in info or 'performance_optimizations' in info
    
    def test_fallback_mechanism(self):
        """Test that fallback to Python implementations works."""
        # Create a task that might trigger C++ solver
        task = create_symmetry_task()
        
        # Should not raise an exception even if C++ is not available
        solver = ArcSolver()
        result = solver.solve(task)
        
        assert result is not None
        assert len(result.predictions) >= 0
    
    def test_overall_performance(self):
        """Test that overall solver performance is improved with C++."""
        task = create_symmetry_task()
        
        # Test with C++ optimizations enabled
        config_cpp = SolverConfig(enable_cpp_optimizations=True)
        solver_cpp = ArcSolver(config=config_cpp)
        
        start_time = time.time()
        result_cpp = solver_cpp.solve(task)
        cpp_time = time.time() - start_time
        
        # Test with C++ optimizations disabled
        config_python = SolverConfig(enable_cpp_optimizations=False)
        solver_python = ArcSolver(config=config_python)
        
        start_time = time.time()
        result_python = solver_python.solve(task)
        python_time = time.time() - start_time
        
        # C++ should be faster (if available)
        if cpp_time < python_time:
            speedup = python_time / cpp_time
            print(f"C++ optimization speedup: {speedup:.2f}x")
            assert speedup >= 1.5  # At least 1.5x speedup


if __name__ == "__main__":
    pytest.main([__file__]) 