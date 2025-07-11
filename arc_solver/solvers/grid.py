"""
Grid pattern solver for ARC tasks.
"""

import numpy as np
from typing import List, Dict, Any
from .base import BaseSolver
from ..data.task import Task
from ..utils.core_functions import get_objects, get_grid, grid_filter
from ..utils.transforms import predict_transforms, predict_transforms_2x


class GridSolver(BaseSolver):
    """Solver for grid-based patterns."""
    
    def can_solve(self, task: Task) -> bool:
        """Check if task involves grid patterns."""
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        xs, ys = get_objects(task_dict)
        for x in xs:
            color_of_grid, cols, rows = get_grid(x)
            if color_of_grid != -1:
                return True
        return False
    
    def solve(self, task: Task) -> List[np.ndarray]:
        """Generate grid pattern predictions."""
        if not self.can_solve(task):
            return []
        
        candidates = []
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        for test_input in task.test:
            # Try grid transforms
            grid_candidates = self._predict_grid_transforms(task_dict, test_input)
            candidates.extend(grid_candidates)
            
            # Try grid 2x transforms
            grid_2x_candidates = self._predict_grid_transforms_2x(task_dict, test_input)
            candidates.extend(grid_2x_candidates)
        
        return candidates
    
    def _predict_grid_transforms(self, task: Dict[str, Any], test_input: np.ndarray) -> List[np.ndarray]:
        """Predict grid transformations."""
        xs, ys = get_objects(task)
        xs = [grid_filter(x) for x in xs]
        return predict_transforms(xs, ys, grid_filter(test_input))
    
    def _predict_grid_transforms_2x(self, task: Dict[str, Any], test_input: np.ndarray) -> List[np.ndarray]:
        """Predict 2x grid transformations."""
        xs, ys = get_objects(task)
        xs = [grid_filter(x) for x in xs]
        return predict_transforms_2x(xs, ys, grid_filter(test_input)) 