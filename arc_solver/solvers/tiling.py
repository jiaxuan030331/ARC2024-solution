"""Tiling pattern solver for ARC tasks."""

import numpy as np
from typing import List, Dict, Any, Optional, Tuple
from .base import BaseSolver
from ..data.task import Task
from ..utils.core_functions import get_objects


class TilingSolver(BaseSolver):
    """Solver for tiling patterns."""
    
    def can_solve(self, task: Task) -> bool:
        """Check if task involves tiling patterns."""
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        patterns = []
        xs, ys = get_objects(task_dict)
        for x, y in zip(xs, ys):
            o_pattern = self._has_tiles(y, -1)
            if len(np.unique(x)) < 2 or o_pattern is None:
                return False
            found = False
            for c in [-1] + list(np.unique(x)):
                pattern = self._has_tiles_shape(x, o_pattern.shape, c)
                if pattern is not None:
                    found = True
                    patterns.append(pattern)
                    break
            if not found:
                return False
        return True
    
    def solve(self, task: Task) -> List[np.ndarray]:
        """Generate tiling pattern predictions."""
        if not self.can_solve(task):
            return []
        
        candidates = []
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        for test_input in task.test:
            tiling_candidates = self._predict_tiles_shape(task_dict, test_input)
            candidates.extend(tiling_candidates)
        
        return candidates
    
    def _has_tiles(self, g: np.ndarray, ignore: int = 0) -> Optional[np.ndarray]:
        """Check if matrix has tiling pattern."""
        for size0b, size1b in [(g.shape[0], int(0.6*g.shape[1])), (int(0.6*g.shape[0]), g.shape[1])]:
            t = np.full((g.shape[0]+size0b, g.shape[1]+size1b), -1)
            t[:-size0b, :-size1b] = g
            t[t == ignore] = -1
            box_trim = self._trim_matrix_box(g, [ignore])
            min_size0 = 1
            min_size1 = 1
            if box_trim is not None and ignore != -1:
                xmin, ymin, xmax, ymax = box_trim
                t[xmin:xmax, ymin:ymax] = g[xmin:xmax, ymin:ymax]
                min_size0 = xmax - xmin
                min_size1 = ymax - ymin
            for size0 in range(min_size0, size0b+1):
                for size1 in range(min_size1, size1b+1):
                    pattern = t[:size0, :size1].copy()
                    found = True
                    for d0 in range(0, t.shape[0]-size0, size0):
                        for d1 in range(0, t.shape[1]-size1, size1):
                            test = t[d0:d0+size0, d1:d1+size1]
                            mask = (test != -1) & (pattern != -1)
                            if np.array_equal(test[mask], pattern[mask]):
                                ind = test != -1
                                pattern[ind] = test[ind]
                            else:
                                found = False
                                break
                    if found:
                        return pattern
        return None
    
    def _has_tiles_shape(self, g: np.ndarray, shape: Tuple[int, ...], ignore: int = 0) -> Optional[np.ndarray]:
        """Check if matrix has tiling pattern with specific shape."""
        for size0b, size1b in [(g.shape[0], int(0.6*g.shape[1])), (int(0.6*g.shape[0]), g.shape[1])]:
            t = np.full((g.shape[0]+size0b, g.shape[1]+size1b), -1)
            t[:-size0b, :-size1b] = g
            t[t == ignore] = -1
            box_trim = self._trim_matrix_box(g, [ignore])
            min_size0 = 1
            min_size1 = 1
            if box_trim is not None and ignore != -1:
                xmin, ymin, xmax, ymax = box_trim
                t[xmin:xmax, ymin:ymax] = g[xmin:xmax, ymin:ymax]
                min_size0 = xmax - xmin
                min_size1 = ymax - ymin
            size0 = shape[0]
            size1 = shape[1]
            pattern = t[:size0, :size1].copy()
            found = True
            for d0 in range(0, t.shape[0]-size0, size0):
                for d1 in range(0, t.shape[1]-size1, size1):
                    test = t[d0:d0+size0, d1:d1+size1]
                    mask = (test != -1) & (pattern != -1)
                    if np.array_equal(test[mask], pattern[mask]):
                        ind = test != -1
                        pattern[ind] = test[ind]
                    else:
                        found = False
                        break
            if found:
                return pattern
        return None
    
    def _trim_matrix_box(self, g: np.ndarray, mask: List[int]) -> Optional[Tuple[int, int, int, int]]:
        """Get bounding box of non-mask elements."""
        if len(np.unique(g)) == 1:
            return None
        for c in mask:
            xs, ys = np.where(g != c)
            if len(xs) == 0:
                continue
            xmin, ymin, xmax, ymax = min(xs), min(ys), max(xs)+1, max(ys)+1
            if xmin > 0 or ymin > 0 or xmax < g.shape[0] or ymax < g.shape[1]:
                return (xmin, ymin, xmax, ymax)
        return None
    
    def _predict_tiles_shape(self, task: Dict[str, Any], test_input: np.ndarray) -> List[np.ndarray]:
        """Predict tiling patterns with specific shapes."""
        has_transforms = set()
        has_shapes = set()
        xs, ys = get_objects(task)
        
        for x, y in zip(xs, ys):
            o_pattern = self._has_tiles(y, -1)
            if len(np.unique(x)) < 2 or o_pattern is None:
                return []
            found = False
            for c in [-1] + list(np.unique(x)):
                pattern = self._has_tiles_shape(x, o_pattern.shape, c)
                if pattern is not None:
                    pattern[pattern == -1] = c
                    for transform in self._get_all_transforms():
                        transformed_pattern = transform(pattern)
                        pred = np.tile(transformed_pattern, (x.shape[0]//transformed_pattern.shape[0]+2, x.shape[1]//transformed_pattern.shape[1]+2))
                        pred = pred[:x.shape[0], :x.shape[1]]
                        if np.array_equal(pred, y):
                            found = True
                            has_transforms.add(transform)
                            has_shapes.add(o_pattern.shape)
                            break
            if not found:
                return []
        
        preds = []
        for c in np.unique(test_input):
            for shape in has_shapes:
                pattern = self._has_tiles_shape(test_input, shape, c)
                if pattern is None:
                    continue
                pattern[pattern == -1] = c
                
                pred = np.tile(pattern, (test_input.shape[0]//pattern.shape[0]+2, test_input.shape[1]//pattern.shape[1]+2))
                for transform in has_transforms:
                    transformed_pattern = transform(pattern)
                    pred = np.tile(transformed_pattern, (test_input.shape[0]//transformed_pattern.shape[0]+2, test_input.shape[1]//transformed_pattern.shape[1]+2))
                    pred = pred[:test_input.shape[0], :test_input.shape[1]]
                    preds.append(pred)
        
        return preds
    
    def _get_all_transforms(self):
        """Get all transformation functions."""
        from ..utils.transforms import get_all_transforms
        return get_all_transforms() 