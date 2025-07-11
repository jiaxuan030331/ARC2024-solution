"""
Pattern detection and prediction functions for ARC tasks.

This module contains all the pattern detection and prediction functions
from the main notebook, organized and compatible with the arc_solver framework.
"""

import numpy as np
from typing import List, Dict, Any, Optional, Tuple, Callable
from collections import defaultdict

from ..utils.core_functions import (
    get_objects, find_sub, get_grid, get_cells, get_mode_color, 
    grid_filter, mul_ratio, mul_ratios, get_ratio
)
from ..utils.transforms import (
    get_transform, get_transforms, get_all_transforms,
    predict_transforms, predict_transforms_2x
)


class PatternDetector:
    """
    Main pattern detection and prediction class.
    
    Contains all the pattern detection and prediction functions
    from the main notebook, adapted for use with the arc_solver framework.
    """
    
    def __init__(self):
        """Initialize pattern detector."""
        pass
    
    # ===========================================
    # REPEATING PATTERNS
    # ===========================================
    
    def has_repeating(self, g: np.ndarray, ignore: int = 0) -> Optional[Tuple[int, int, np.ndarray]]:
        """Check if grid has repeating patterns."""
        size0b = int(.6 * g.shape[0])
        size1b = int(.6 * g.shape[1])
        t = np.full((g.shape[0]+2*size0b, g.shape[1]+2*size1b), -1)
        t[size0b:-size0b, size1b:-size1b] = g
        t[t==ignore] = -1
        
        for size0 in range(2, size0b+1):
            for size1 in range(2, size1b+1):
                for shift0 in range(size0):
                    for shift1 in range(size1):
                        pattern = t[size0b+shift0:size0b+shift0+size0, size1b+shift1:size1b+shift1+size1].copy()
                        found = True
                        for d0 in range(size0b+shift0-size0, t.shape[0]-size0, size0):
                            for d1 in range(size1b+shift1-size1, t.shape[1]-size1, size1):
                                test = t[d0:d0+size0, d1:d1+size1]
                                mask = (test != -1) & (pattern != -1)
                                if np.array_equal(test[mask], pattern[mask]):
                                    ind = test != -1
                                    pattern[ind] = test[ind]
                                else:
                                    found = False
                                    break
                        if found:
                            return shift0, shift1, pattern
        return None
    
    def check_repeating(self, task: Dict[str, Any], has_complete: bool = False) -> bool:
        """Check if task involves repeating patterns."""
        patterns = []
        xs, ys = get_objects(task)
        
        for x, y in zip(xs, ys):
            if len(np.unique(x)) < 3 or not self.has_repeating(y, -1):
                return False
            result = None
            for c in np.unique(x):
                result = self.has_repeating(x, c)
                if result:
                    sh0, sh1, pattern = result
                    pattern[pattern == -1] = c
                    if has_complete:
                        pred = np.tile(pattern, (x.shape[0]//pattern.shape[0]+2, x.shape[1]//pattern.shape[1]+2))
                        pred1 = pred[sh0:sh0+x.shape[0], sh1:sh1+x.shape[1]]
                        pred2 = pred[sh0+1:sh0+1+x.shape[0], sh1:sh1+x.shape[1]]
                        pred3 = pred[sh0:sh0+x.shape[0], sh1+1:sh1+1+x.shape[1]]
                        if np.array_equal(pred1, y) or np.array_equal(pred2, y) or np.array_equal(pred3, y):
                            break
                        result = None
                    else:
                        break
            if not result:
                return False
        return True
    
    def predict_repeating(self, x: np.ndarray) -> List[np.ndarray]:
        """Predict repeating patterns."""
        for c in np.unique(x):
            result = self.has_repeating(x, c)
            if result:
                sh0, sh1, pattern = result
                pattern[pattern == -1] = c
                pred = np.tile(pattern, (x.shape[0]//pattern.shape[0]+2, x.shape[1]//pattern.shape[1]+2))
                pred1 = pred[sh0:sh0+x.shape[0], sh1:sh1+x.shape[1]]
                pred2 = pred[sh0+1:sh0+1+x.shape[0], sh1:sh1+x.shape[1]]
                pred3 = pred[sh0:sh0+x.shape[0], sh1+1:sh1+1+x.shape[1]]
                return [pred1, pred2, pred3]
        return []
    
    def predict_repeating_mask(self, x: np.ndarray) -> List[np.ndarray]:
        """Predict repeating patterns with mask."""
        predictions = self.predict_repeating(x)
        if len(predictions) > 0:
            rows, cols = np.where(predictions[0] != x)
            if len(rows) > 0 and len(cols) > 0:
                return [predictions[0][min(rows):max(rows)+1, min(cols):max(cols)+1]]
        return []
    
    # ===========================================
    # CHESS PATTERNS
    # ===========================================
    
    def has_chess(self, g: np.ndarray) -> bool:
        """Check if grid has chess pattern."""
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
    
    def has_antichess(self, g: np.ndarray) -> bool:
        """Check if grid has anti-chess pattern."""
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
    
    def find_colors(self, g: np.ndarray) -> Optional[np.ndarray]:
        """Find colors for chess pattern."""
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
    
    def predict_chess(self, g: np.ndarray) -> List[np.ndarray]:
        """Predict chess patterns."""
        q_colors = self.find_colors(g)
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
    
    def check_chess(self, task: Dict[str, Any], input: bool = False, output: bool = True) -> bool:
        """Check if task involves chess patterns."""
        xs, ys = get_objects(task)
        if input:
            for x in xs:
                if not self.has_chess(x) and not self.has_antichess(x):
                    return False
        if output:
            for y in ys:
                if not self.has_chess(y) and not self.has_antichess(y):
                    return False
        return True
    
    # ===========================================
    # TILING PATTERNS
    # ===========================================
    
    def trim_matrix(self, x: np.ndarray) -> np.ndarray:
        """Trim matrix to remove uniform borders."""
        if len(np.unique(x)) == 1:
            return x
        for c in np.unique(x):
            xs, ys = np.where(x != c)
            if len(xs) > 0:
                xmin, ymin, xmax, ymax = min(xs), min(ys), max(xs)+1, max(ys)+1
                if xmin > 0 or ymin > 0 or xmax < x.shape[0] or ymax < x.shape[1]:
                    return x[xmin:xmax, ymin:ymax]
        return x
    
    def trim_matrix_box(self, g: np.ndarray, mask: Optional[List[int]] = None) -> Optional[Tuple[int, int, int, int]]:
        """Get bounding box of non-mask elements."""
        if mask is None:
            mask = list(np.unique(g))
        if len(np.unique(g)) == 1:
            return None
        for c in mask:
            xs, ys = np.where(g != c)
            if len(xs) > 0:
                xmin, ymin, xmax, ymax = min(xs), min(ys), max(xs)+1, max(ys)+1
                if xmin > 0 or ymin > 0 or xmax < g.shape[0] or ymax < g.shape[1]:
                    return (xmin, ymin, xmax, ymax)
        return None
    
    def has_tiles(self, g: np.ndarray, ignore: int = 0) -> Optional[np.ndarray]:
        """Check if grid has tiling patterns."""
        for size0b, size1b in [(g.shape[0], int(0.6*g.shape[1])), (int(0.6*g.shape[0]), g.shape[1])]:
            t = np.full((g.shape[0]+size0b, g.shape[1]+size1b), -1)
            t[:-size0b, :-size1b] = g
            t[t==ignore] = -1
            box_trim = self.trim_matrix_box(g, [ignore])
            min_size0 = 1
            min_size1 = 1
            if box_trim is not None and ignore != -1:
                xmin, ymin, xmax, ymax = box_trim
                t[xmin:xmax, ymin:ymax] = g[xmin:xmax, ymin:ymax]
                min_size0 = xmax-xmin
                min_size1 = ymax-ymin
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
    
    def has_tiles_shape(self, g: np.ndarray, shape: Tuple[int, int], ignore: int = 0) -> Optional[np.ndarray]:
        """Check if grid has tiling patterns with specific shape."""
        for size0b, size1b in [(g.shape[0], int(0.6*g.shape[1])), (int(0.6*g.shape[0]), g.shape[1])]:
            t = np.full((g.shape[0]+size0b, g.shape[1]+size1b), -1)
            t[:-size0b, :-size1b] = g
            t[t==ignore] = -1
            box_trim = self.trim_matrix_box(g, [ignore])
            min_size0 = 1
            min_size1 = 1
            if box_trim is not None and ignore != -1:
                xmin, ymin, xmax, ymax = box_trim
                t[xmin:xmax, ymin:ymax] = g[xmin:xmax, ymin:ymax]
                min_size0 = xmax-xmin
                min_size1 = ymax-ymin
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
    
    def check_tiles_shape(self, task: Dict[str, Any], has_complete: int = 0) -> bool:
        """Check if task involves tiling patterns."""
        patterns = []
        xs, ys = get_objects(task)
        
        for x, y in zip(xs, ys):
            o_pattern = self.has_tiles(y, -1)
            if len(np.unique(x)) < 2 or o_pattern is None:
                return False
            found = False
            for c in [-1] + list(np.unique(x)):
                pattern = self.has_tiles_shape(x, o_pattern.shape, c)
                if pattern is not None:
                    pattern[pattern == -1] = c
                    if has_complete:
                        for transform in get_all_transforms():
                            transformed_pattern = transform(pattern)
                            pred = np.tile(transformed_pattern, (x.shape[0]//transformed_pattern.shape[0]+2, x.shape[1]//transformed_pattern.shape[1]+2))
                            pred = pred[:x.shape[0], :x.shape[1]]
                            if np.array_equal(pred, y):
                                found = True
                                patterns.append(pattern)
                                break
                    else:
                        found = True
                        patterns.append(pattern)
            if not found:
                return False
        return True
    
    def predict_tiles_shape(self, task: Dict[str, Any], test_input: np.ndarray) -> List[np.ndarray]:
        """Predict tiling patterns with specific shapes."""
        has_transforms = set()
        has_shapes = set()
        xs, ys = get_objects(task)
        
        for x, y in zip(xs, ys):
            o_pattern = self.has_tiles(y, -1)
            if len(np.unique(x)) < 2 or o_pattern is None:
                return []
            found = False
            for c in [-1] + list(np.unique(x)):
                pattern = self.has_tiles_shape(x, o_pattern.shape, c)
                if pattern is not None:
                    pattern[pattern == -1] = c
                    for transform in get_all_transforms():
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
                pattern = self.has_tiles_shape(test_input, shape, c)
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
    
    # ===========================================
    # GRID PATTERNS
    # ===========================================
    
    def check_grid(self, task: Dict[str, Any]) -> bool:
        """Check if task involves grid patterns."""
        xs, ys = get_objects(task)
        for x in xs:
            color_of_grid, cols, rows = get_grid(x)
            if color_of_grid == -1:
                return False
        return True
    
    def check_sub_grid_2x(self, task: Dict[str, Any]) -> bool:
        """Check if task involves sub-grid 2x patterns."""
        if self.check_samesize(task) or self.check_subitem(task):
            return False
        xs, ys = get_objects(task)
        for x, y in zip(xs, ys):
            color_of_grid, cols, rows = get_grid(x)
            if color_of_grid == -1:
                return False
            cells = grid_filter(x)
            if (cells.shape[0]*2 != y.shape[0] or cells.shape[1]*2 != y.shape[1]):
                return False
        return True
    
    def check_grid_transforms(self, task: Dict[str, Any]) -> bool:
        """Check if task involves grid transform patterns."""
        xs, ys = get_objects(task)
        xs = [grid_filter(x) for x in xs]
        return get_transforms(xs, ys) is not None
    
    def predict_grid_transforms(self, task: Dict[str, Any], test: np.ndarray) -> List[np.ndarray]:
        """Predict grid transformations."""
        xs, ys = get_objects(task)
        xs = [grid_filter(x) for x in xs]
        return predict_transforms(xs, ys, grid_filter(test))
    
    def predict_transforms_grid_2x(self, task: Dict[str, Any], test: np.ndarray) -> List[np.ndarray]:
        """Predict grid 2x transformations."""
        xs, ys = get_objects(task)
        xs = [grid_filter(x) for x in xs]
        return predict_transforms_2x(xs, ys, grid_filter(test))
    
    # ===========================================
    # SUBMATRIX PATTERNS
    # ===========================================
    
    def check_samesize(self, task: Dict[str, Any]) -> bool:
        """Check if all inputs and outputs have same size."""
        xs, ys = get_objects(task)
        for x, y in zip(xs, ys):
            if x.shape != y.shape:
                return False
        return True
    
    def check_subitem(self, task: Dict[str, Any]) -> bool:
        """Check if outputs are subitems of inputs."""
        xs, ys = get_objects(task)
        for x, y in zip(xs, ys):
            positions = find_sub(x, y)
            if len(positions) == 0:
                return False
        return True
    
    def check_sub_mask(self, task: Dict[str, Any]) -> bool:
        """Check if task involves sub-mask patterns."""
        if self.check_samesize(task):
            return False
        xs, ys = get_objects(task)
        for x, y in zip(xs, ys):
            colors, counts = np.unique(x, return_counts=True)
            found = 0
            for c, area in zip(colors, counts):
                cxs, cys = np.where(x == c)
                if len(cxs) > 0:
                    xmin, ymin, xmax, ymax = min(cxs), min(cys), max(cxs)+1, max(cys)+1
                    shape = (xmax-xmin, ymax-ymin)
                    if shape == y.shape and area == np.prod(y.shape):
                        found += 1
            if found != 1:
                return False
        return True


# Create global pattern detector instance
pattern_detector = PatternDetector()

# Export pattern detection functions
check_repeating = pattern_detector.check_repeating
predict_repeating = pattern_detector.predict_repeating
predict_repeating_mask = pattern_detector.predict_repeating_mask

check_chess = pattern_detector.check_chess
has_chess = pattern_detector.has_chess
has_antichess = pattern_detector.has_antichess
predict_chess = pattern_detector.predict_chess

check_tiles_shape = pattern_detector.check_tiles_shape
predict_tiles_shape = pattern_detector.predict_tiles_shape
has_tiles = pattern_detector.has_tiles
has_tiles_shape = pattern_detector.has_tiles_shape

check_grid = pattern_detector.check_grid
check_sub_grid_2x = pattern_detector.check_sub_grid_2x
check_grid_transforms = pattern_detector.check_grid_transforms
predict_grid_transforms = pattern_detector.predict_grid_transforms
predict_transforms_grid_2x = pattern_detector.predict_transforms_grid_2x

check_samesize = pattern_detector.check_samesize
check_subitem = pattern_detector.check_subitem
check_sub_mask = pattern_detector.check_sub_mask

# All pattern functions
__all__ = [
    'PatternDetector', 'pattern_detector',
    'check_repeating', 'predict_repeating', 'predict_repeating_mask',
    'check_chess', 'has_chess', 'has_antichess', 'predict_chess',
    'check_tiles_shape', 'predict_tiles_shape', 'has_tiles', 'has_tiles_shape',
    'check_grid', 'check_sub_grid_2x', 'check_grid_transforms', 
    'predict_grid_transforms', 'predict_transforms_grid_2x',
    'check_samesize', 'check_subitem', 'check_sub_mask'
] 