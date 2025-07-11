"""
Repeating pattern solver for ARC tasks.
"""

import numpy as np
from typing import List, Optional
from abc import ABC, abstractmethod

from ..data.task import Task
from ..utils.core_functions import get_objects


class BaseSolver(ABC):
    """Base class for all ARC solvers."""
    
    @abstractmethod
    def can_solve(self, task: Task) -> bool:
        """Check if this solver can handle the given task."""
        pass
    
    @abstractmethod
    def solve(self, task: Task) -> List[np.ndarray]:
        """Solve the task and return candidate outputs."""
        pass


class RepeatingSolver(BaseSolver):
    """
    Solver for repeating patterns (chess, anti-chess, etc.).
    
    This solver detects and generates repeating patterns like:
    - Chess patterns
    - Anti-chess patterns
    - Other periodic patterns
    """
    
    def can_solve(self, task: Task) -> bool:
        """
        Check if the task involves repeating patterns.
        
        Args:
            task: The ARC task to analyze
            
        Returns:
            True if the task appears to involve repeating patterns
        """
        # Convert Task to dict format for compatibility
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        xs, ys = get_objects(task_dict)
        for y in ys:
            if self._has_chess_pattern(y) or self._has_anti_chess_pattern(y):
                return True
        return False
    
    def solve(self, task: Task) -> List[np.ndarray]:
        """
        Generate repeating pattern predictions for test inputs.
        
        Args:
            task: The ARC task to solve
            
        Returns:
            List of candidate output arrays
        """
        if not self.can_solve(task):
            return []
        
        candidates = []
        
        for test_input in task.test:
            # Try chess patterns
            chess_candidates = self._generate_chess_patterns(test_input)
            candidates.extend(chess_candidates)
            
            # Try anti-chess patterns
            anti_chess_candidates = self._generate_anti_chess_patterns(test_input)
            candidates.extend(anti_chess_candidates)
        
        return candidates
    
    def _find_colors(self, g: np.ndarray) -> Optional[np.ndarray]:
        """Find chess colors in matrix."""
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
    
    def _has_chess_pattern(self, matrix: np.ndarray) -> bool:
        """Check if matrix has a chess pattern."""
        colors = np.unique(matrix)
        if len(colors) < 2:
            return False
        
        for color in colors:
            positions = np.where(matrix == color)
            if len(positions[0]) == 0:
                continue
            
            # Check if positions follow chess pattern
            pattern_values = set()
            for x, y in zip(positions[0], positions[1]):
                pattern_values.add((x + y) % len(colors))
            
            if len(pattern_values) > 1:
                return False
        
        return True
    
    def _has_anti_chess_pattern(self, matrix: np.ndarray) -> bool:
        """Check if matrix has an anti-chess pattern."""
        # Similar to chess but with different pattern
        colors = np.unique(matrix)
        if len(colors) < 2:
            return False
        
        for color in colors:
            positions = np.where(matrix == color)
            if len(positions[0]) == 0:
                continue
            
            # Check anti-chess pattern
            pattern_values = set()
            for x, y in zip(positions[0], positions[1]):
                pattern_values.add((x - y) % len(colors))
            
            if len(pattern_values) > 1:
                return False
        
        return True
    
    def _generate_chess_patterns(self, input_matrix: np.ndarray) -> List[np.ndarray]:
        """Generate chess pattern candidates."""
        return self._predict_chess(input_matrix)
    
    def _generate_anti_chess_patterns(self, input_matrix: np.ndarray) -> List[np.ndarray]:
        """Generate anti-chess pattern candidates."""
        candidates = []
        colors = np.unique(input_matrix)
        
        if len(colors) < 2:
            return candidates
        
        for color in colors:
            # Create anti-chess pattern based on this color
            pattern = np.zeros_like(input_matrix)
            for i in range(input_matrix.shape[0]):
                for j in range(input_matrix.shape[1]):
                    pattern[i, j] = colors[(i - j) % len(colors)]
            candidates.append(pattern)
        
        return candidates 