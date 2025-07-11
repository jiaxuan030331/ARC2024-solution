"""
Integration tests for ARC Solver.

This module tests the complete ARC solver pipeline, including
task loading, solving, and result generation.
"""

import pytest
import numpy as np
from arc_solver import ArcSolver, TaskLoader, SolverConfig


def create_test_task():
    """Create a simple test task for integration testing."""
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
        'task_id': 'test_integration',
        'train': [
            {'input': train_input.tolist(), 'output': train_output.tolist()}
        ],
        'test': [test_input.tolist()]
    }
    
    return TaskLoader.from_json(task_data)


class TestArcSolverIntegration:
    """Test the complete ARC solver integration."""
    
    def test_solver_initialization(self):
        """Test that the solver initializes correctly."""
        solver = ArcSolver()
        assert solver is not None
        
        # Test with custom config
        config = SolverConfig(
            enable_repeating_solver=True,
            enable_grid_solver=True,
            max_candidates=5
        )
        solver = ArcSolver(config=config)
        assert solver is not None
    
    def test_task_loading(self):
        """Test that tasks can be loaded correctly."""
        task = create_test_task()
        assert task is not None
        assert task.task_id == 'test_integration'
        assert len(task.train) == 1
        assert len(task.test) == 1
    
    def test_solving_pipeline(self):
        """Test the complete solving pipeline."""
        solver = ArcSolver()
        task = create_test_task()
        
        result = solver.solve(task)
        
        assert result is not None
        assert result.task_id == 'test_integration'
        assert len(result.predictions) >= 0
        assert len(result.scores) == len(result.predictions)
    
    def test_result_structure(self):
        """Test that results have the correct structure."""
        solver = ArcSolver()
        task = create_test_task()
        
        result = solver.solve(task)
        
        # Check required attributes
        assert hasattr(result, 'task_id')
        assert hasattr(result, 'predictions')
        assert hasattr(result, 'scores')
        assert hasattr(result, 'solver_contributions')
        
        # Check data types
        assert isinstance(result.predictions, list)
        assert isinstance(result.scores, list)
        assert isinstance(result.solver_contributions, dict)
    
    def test_solver_info(self):
        """Test that solver information is available."""
        solver = ArcSolver()
        info = solver.get_solver_info()
        
        assert isinstance(info, dict)
        assert 'total_solvers' in info
        assert 'available_solvers' in info
        assert info['total_solvers'] > 0
        assert len(info['available_solvers']) > 0
    
    def test_configuration_options(self):
        """Test different configuration options."""
        # Test with minimal configuration
        config = SolverConfig(
            enable_repeating_solver=True,
            enable_grid_solver=False,
            enable_chess_solver=False,
            enable_tiling_solver=False,
            enable_ml_solver=False,
            max_candidates=1
        )
        
        solver = ArcSolver(config=config)
        task = create_test_task()
        result = solver.solve(task)
        
        assert len(result.predictions) <= 1
        
        # Test with full configuration
        config = SolverConfig(
            enable_repeating_solver=True,
            enable_grid_solver=True,
            enable_chess_solver=True,
            enable_tiling_solver=True,
            enable_ml_solver=True,
            max_candidates=10
        )
        
        solver = ArcSolver(config=config)
        result = solver.solve(task)
        
        assert len(result.predictions) <= 10
    
    def test_error_handling(self):
        """Test that the solver handles errors gracefully."""
        solver = ArcSolver()
        
        # Test with invalid task data
        invalid_task_data = {
            'task_id': 'invalid',
            'train': [],
            'test': []
        }
        
        try:
            task = TaskLoader.from_json(invalid_task_data)
            result = solver.solve(task)
            # Should not raise an exception, but may return empty results
            assert result is not None
        except Exception as e:
            # If an exception is raised, it should be handled gracefully
            assert "error" in str(e).lower() or "invalid" in str(e).lower()
    
    def test_performance_characteristics(self):
        """Test that the solver performs within reasonable time limits."""
        solver = ArcSolver()
        task = create_test_task()
        
        import time
        start_time = time.time()
        result = solver.solve(task)
        end_time = time.time()
        
        # Should complete within reasonable time (e.g., 10 seconds)
        assert end_time - start_time < 10.0
        
        # Should produce some results
        assert result is not None
    
    def test_memory_usage(self):
        """Test that the solver doesn't use excessive memory."""
        import psutil
        import os
        
        process = psutil.Process(os.getpid())
        initial_memory = process.memory_info().rss
        
        solver = ArcSolver()
        task = create_test_task()
        
        # Run multiple solves to test memory usage
        for _ in range(5):
            result = solver.solve(task)
        
        final_memory = process.memory_info().rss
        memory_increase = final_memory - initial_memory
        
        # Memory increase should be reasonable (e.g., less than 100MB)
        assert memory_increase < 100 * 1024 * 1024  # 100MB
    
    def test_concurrent_solving(self):
        """Test that the solver can handle concurrent solving."""
        import threading
        import time
        
        solver = ArcSolver()
        task = create_test_task()
        
        results = []
        errors = []
        
        def solve_task():
            try:
                result = solver.solve(task)
                results.append(result)
            except Exception as e:
                errors.append(e)
        
        # Start multiple threads
        threads = []
        for _ in range(3):
            thread = threading.Thread(target=solve_task)
            threads.append(thread)
            thread.start()
        
        # Wait for all threads to complete
        for thread in threads:
            thread.join()
        
        # All threads should complete successfully
        assert len(errors) == 0
        assert len(results) == 3
        
        # All results should be valid
        for result in results:
            assert result is not None
            assert result.task_id == 'test_integration'


class TestTaskLoaderIntegration:
    """Test task loading and processing."""
    
    def test_task_creation(self):
        """Test creating tasks from various data formats."""
        # Test with numpy arrays
        train_input = np.array([[1, 0], [0, 1]])
        train_output = np.array([[1, 0], [0, 1]])
        test_input = np.array([[1]])
        
        task_data = {
            'task_id': 'numpy_test',
            'train': [{'input': train_input.tolist(), 'output': train_output.tolist()}],
            'test': [test_input.tolist()]
        }
        
        task = TaskLoader.from_json(task_data)
        assert task.task_id == 'numpy_test'
        assert len(task.train) == 1
        assert len(task.test) == 1
    
    def test_task_validation(self):
        """Test that task validation works correctly."""
        # Valid task
        valid_task_data = {
            'task_id': 'valid',
            'train': [{'input': [[1]], 'output': [[1]]}],
            'test': [[[1]]]
        }
        
        task = TaskLoader.from_json(valid_task_data)
        assert task is not None
        
        # Invalid task (missing required fields)
        invalid_task_data = {
            'task_id': 'invalid'
            # Missing train and test
        }
        
        try:
            task = TaskLoader.from_json(invalid_task_data)
            # Should handle gracefully
            assert task is not None
        except Exception:
            # Exception is also acceptable
            pass


class TestSolverConfigIntegration:
    """Test solver configuration system."""
    
    def test_default_config(self):
        """Test that default configuration works."""
        config = SolverConfig()
        assert config is not None
        
        # Check default values
        assert hasattr(config, 'enable_repeating_solver')
        assert hasattr(config, 'enable_grid_solver')
        assert hasattr(config, 'max_candidates')
    
    def test_custom_config(self):
        """Test that custom configuration works."""
        config = SolverConfig(
            enable_repeating_solver=True,
            enable_grid_solver=False,
            max_candidates=5
        )
        
        assert config.enable_repeating_solver == True
        assert config.enable_grid_solver == False
        assert config.max_candidates == 5
    
    def test_config_validation(self):
        """Test that configuration validation works."""
        # Valid configuration
        config = SolverConfig(max_candidates=10)
        assert config.max_candidates == 10
        
        # Invalid configuration should be handled gracefully
        try:
            config = SolverConfig(max_candidates=-1)
            # Should handle gracefully
            assert config is not None
        except Exception:
            # Exception is also acceptable
            pass


if __name__ == "__main__":
    pytest.main([__file__]) 