"""
Submatrix pattern solver for ARC tasks.
"""

import numpy as np
from typing import List, Dict, Any
from .base import BaseSolver
from ..data.task import Task
from ..utils.core_functions import get_objects, find_sub


class SubmatrixSolver(BaseSolver):
    """Solver for submatrix patterns."""
    
    def can_solve(self, task: Task) -> bool:
        """Check if task involves submatrix patterns."""
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        xs, ys = get_objects(task_dict)
        for x, y in zip(xs, ys):
            positions = find_sub(x, y)
            if len(positions) == 0:
                return False
        return True
    
    def solve(self, task: Task) -> List[np.ndarray]:
        """Generate submatrix pattern predictions."""
        if not self.can_solve(task):
            return []
        
        candidates = []
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        for test_input in task.test:
            # Find all possible submatrices
            submatrix_candidates = self._find_submatrices(task_dict, test_input)
            candidates.extend(submatrix_candidates)
        
        return candidates
    
    def _find_submatrices(self, task: Dict[str, Any], test_input: np.ndarray) -> List[np.ndarray]:
        """Find all possible submatrices in test input."""
        candidates = []
        xs, ys = get_objects(task)
        
        # Use the first training example's output as template
        if ys:
            template = ys[0]
            positions = find_sub(test_input, template)
            
            for x, y, x_end, y_end in positions:
                candidate = test_input[x:x_end, y:y_end]
                if candidate.shape == template.shape:
                    candidates.append(candidate)
        
        return candidates


class SubmaskSolver(BaseSolver):
    """Solver for submask patterns."""
    
    def can_solve(self, task: Task) -> bool:
        """Check if task involves submask patterns."""
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        # Check if all inputs and outputs have same size
        xs, ys = get_objects(task_dict)
        for x, y in zip(xs, ys):
            if x.shape == y.shape:
                return False
        
        # Check submask condition
        for x, y in zip(xs, ys):
            colors, counts = np.unique(x, return_counts=True)
            found = 0
            for c, area in zip(colors, counts):
                cxs, cys = np.where(x == c)
                if len(cxs) == 0:
                    continue
                xmin, ymin, xmax, ymax = min(cxs), min(cys), max(cxs)+1, max(cys)+1
                shape = (xmax-xmin, ymax-ymin)
                if shape == y.shape and area == np.prod(y.shape):
                    found += 1
            if found != 1:
                return False
        return True
    
    def solve(self, task: Task) -> List[np.ndarray]:
        """Generate submask pattern predictions."""
        if not self.can_solve(task):
            return []
        
        candidates = []
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        for test_input in task.test:
            submask_candidates = self._predict_submask(test_input)
            candidates.extend(submask_candidates)
        
        return candidates
    
    def _predict_submask(self, test_input: np.ndarray) -> List[np.ndarray]:
        """Predict submask patterns."""
        candidates = []
        colors, counts = np.unique(test_input, return_counts=True)
        
        for c, area in zip(colors, counts):
            cxs, cys = np.where(test_input == c)
            if len(cxs) == 0:
                continue
            xmin, ymin, xmax, ymax = min(cxs), min(cys), max(cxs)+1, max(cys)+1
            candidate = test_input[xmin:xmax, ymin:ymax]
            candidates.append(candidate)
        
        return candidates 