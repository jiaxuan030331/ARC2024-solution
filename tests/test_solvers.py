"""
Tests for ARC solvers.
"""

import pytest
import numpy as np
from arc_solver import ArcSolver, TaskLoader, SolverConfig
from arc_solver.solvers.repeating import RepeatingSolver


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


class TestRepeatingSolver:
    """Test the repeating pattern solver."""
    
    def test_can_solve_chess_pattern(self):
        """Test that chess patterns are detected correctly."""
        solver = RepeatingSolver()
        task = create_chess_task()
        
        assert solver.can_solve(task) == True
    
    def test_solve_chess_pattern(self):
        """Test that chess patterns are solved correctly."""
        solver = RepeatingSolver()
        task = create_chess_task()
        
        candidates = solver.solve(task)
        
        assert len(candidates) > 0
        assert all(isinstance(cand, np.ndarray) for cand in candidates)
    
    def test_has_chess_pattern(self):
        """Test chess pattern detection."""
        solver = RepeatingSolver()
        
        # Valid chess pattern
        chess_matrix = np.array([
            [0, 1, 0, 1],
            [1, 0, 1, 0],
            [0, 1, 0, 1]
        ])
        
        assert solver._has_chess_pattern(chess_matrix) == True
        
        # Invalid pattern
        invalid_matrix = np.array([
            [0, 0, 0],
            [1, 1, 1]
        ])
        
        assert solver._has_chess_pattern(invalid_matrix) == False


class TestArcSolver:
    """Test the main ARC solver."""
    
    def test_solver_initialization(self):
        """Test solver initialization with different configs."""
        # Default config
        solver = ArcSolver()
        info = solver.get_solver_info()
        assert info['total_solvers'] > 0
        
        # Custom config
        config = SolverConfig(enable_repeating_solver=True, enable_grid_solver=False)
        solver = ArcSolver(config=config)
        info = solver.get_solver_info()
        assert 'repeating' in info['available_solvers']
        assert 'grid' not in info['available_solvers']
    
    def test_solve_task(self):
        """Test solving a simple task."""
        config = SolverConfig(
            enable_repeating_solver=True,
            enable_grid_solver=False,
            enable_chess_solver=False,
            enable_tiling_solver=False,
            enable_ml_solver=False,
            max_candidates=2
        )
        
        solver = ArcSolver(config=config)
        task = create_chess_task()
        
        result = solver.solve(task)
        
        assert result.task_id == 'test_chess'
        assert len(result.predictions) <= 2
        assert len(result.scores) == len(result.predictions)
        assert 'repeating' in result.solver_contributions


if __name__ == "__main__":
    pytest.main([__file__]) 