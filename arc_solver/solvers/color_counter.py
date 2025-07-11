"""Color counter solver for ARC tasks."""

import numpy as np
from typing import List
from .base import BaseSolver
from ..data.task import Task


class ColorCounterSolver(BaseSolver):
    """Solver based on color counting patterns."""
    
    def can_solve(self, task: Task) -> bool:
        """Check if task involves color counting."""
        inputs = [ex.input for ex in task.train]
        outputs = [ex.output for ex in task.train]
        for x, y in zip(inputs, outputs):
            if not self._color_hist_equal(x, y):
                return False
        return True
    
    def solve(self, task: Task) -> List[np.ndarray]:
        """Generate color counting predictions."""
        if not self.can_solve(task):
            return []
        outputs = [ex.output for ex in task.train]
        color_hist = self._get_color_hist(outputs[0])
        for y in outputs[1:]:
            color_hist = self._merge_hist(color_hist, self._get_color_hist(y))
        results = []
        for test_in in task.test:
            recolored = self._recolor_to_hist(test_in, color_hist)
            results.append(recolored)
        return results

    def _color_hist_equal(self, x: np.ndarray, y: np.ndarray) -> bool:
        """Check if two color histograms are equal."""
        x_hist = self._get_color_hist(x)
        y_hist = self._get_color_hist(y)
        return np.array_equal(x_hist, y_hist)

    def _get_color_hist(self, arr: np.ndarray) -> np.ndarray:
        """Return a 10-element color histogram for the given array."""
        hist = np.zeros(10, dtype=int)
        for c in np.unique(arr):
            hist[c] = np.sum(arr == c)
        return hist

    def _merge_hist(self, h1: np.ndarray, h2: np.ndarray) -> np.ndarray:
        """Merge two color histograms by taking the maximum of each bin."""
        return np.maximum(h1, h2)

    def _recolor_to_hist(self, arr: np.ndarray, hist: np.ndarray) -> np.ndarray:
        """Recolor the given array to match the target histogram."""
        flat = arr.flatten()
        unique, counts = np.unique(flat, return_counts=True)
        target_colors = np.where(hist > 0)[0]
        recolor_map = {}
        for i, c in enumerate(unique):
            recolor_map[c] = target_colors[i % len(target_colors)]
        recolored = np.vectorize(lambda x: recolor_map[x])(arr)
        return recolored 