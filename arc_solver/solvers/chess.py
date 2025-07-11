"""Chess pattern solver for ARC tasks."""

import numpy as np
from typing import List, Optional
from .base import BaseSolver
from ..data.task import Task
from ..utils.core_functions import get_objects, get_grid, grid_filter


class ChessSolver(BaseSolver):
    """Solver for chess patterns."""
    
    def can_solve(self, task: Task) -> bool:
        """Check if task involves chess patterns."""
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        # Check if grid patterns exist and outputs have chess patterns
        if not self._check_grid(task_dict):
            return False
        
        return self._check_chess(task_dict, input=False, output=True)
    
    def solve(self, task: Task) -> List[np.ndarray]:
        """Generate chess pattern predictions."""
        if not self.can_solve(task):
            return []
        
        candidates = []
        for test_input in task.test:
            # Apply grid filter and predict chess patterns
            filtered_input = grid_filter(test_input)
            chess_candidates = self._predict_chess(filtered_input)
            candidates.extend(chess_candidates)
        
        return candidates
    
    def _check_grid(self, task_dict: dict) -> bool:
        """Check if task has grid patterns."""
        xs, ys = get_objects(task_dict)
        for x in xs:
            color_of_grid, cols, rows = get_grid(x)
            if color_of_grid != -1:
                return True
        return False
    
    def _check_chess(self, task_dict: dict, input: bool = False, output: bool = True) -> bool:
        """Check if task has chess patterns in input/output."""
        xs, ys = get_objects(task_dict)
        
        if input:
            for x in xs:
                if not self._has_chess(x) and not self._has_antichess(x):
                    return False
        
        if output:
            for y in ys:
                if not self._has_chess(y) and not self._has_antichess(y):
                    return False
        
        return True
    
    def _has_chess(self, g: np.ndarray) -> bool:
        """Check if matrix has chess pattern."""
        colors = np.unique(g)
        counts = len(colors)
        if counts < 2:
            return False
        
        indexes = np.zeros(counts, bool)
        for c in colors:
            pts = np.where(g == c)
            s = set([(x+y) % counts for x, y in zip(*pts)])
            if len(s) > 1:
                return False
            index = s.pop()
            if indexes[index]:
                return False
            indexes[index] = True
        return True
    
    def _has_antichess(self, g: np.ndarray) -> bool:
        """Check if matrix has anti-chess pattern."""
        colors = np.unique(g)
        counts = len(colors)
        if counts < 2:
            return False
        
        indexes = np.zeros(counts, bool)
        for c in colors:
            pts = np.where(g == c)
            s = set([(g.shape[0]-x+y-1) % counts for x, y in zip(*pts)])
            if len(s) > 1:
                return False
            index = s.pop()
            if indexes[index]:
                return False
            indexes[index] = True
        return True
    
    def _find_colors(self, g: np.ndarray) -> Optional[np.ndarray]:
        """Find optimal color arrangement for chess pattern."""
        colors = np.unique(g)
        counts = len(colors)
        
        for cnt in range(counts, 1, -1):
            q_colors = np.full(cnt, -1, int)
            for c in colors:
                pts = np.where(g == c)
                s = set([(x+y) % cnt for x, y in zip(*pts)])
                if len(s) > 1:
                    continue
                index = s.pop()
                q_colors[index] = c
            
            if -1 not in q_colors:
                return q_colors
        return None
    
    def _predict_chess(self, g: np.ndarray) -> List[np.ndarray]:
        """Generate chess pattern predictions."""
        q_colors = self._find_colors(g)
        if q_colors is None:
            colors, counts = np.unique(g, return_counts=True)
            q_colors = colors[np.argsort(counts)][:2]
        
        results = []
        counts = len(q_colors)
        for i in range(counts):
            result = g.copy()
            for x, y in np.ndindex(g.shape):
                result[x, y] = q_colors[(x+y) % counts]
            results.append(result)
            q_colors = np.roll(q_colors, 1)
        
        return results 