"""
Symmetry solver for ARC tasks.

This solver implements symmetry-based pattern recognition and repair,
including translation, reflection, rotation, and diagonal symmetries.
"""

import numpy as np
import pandas as pd
from typing import List, Dict, Any, Optional, Tuple, Set
from .base import BaseSolver
from ..data.task import Task
from ..utils.core_functions import get_objects


class SymmetrySolver(BaseSolver):
    """
    Solver for symmetry patterns and repair.
    
    Implements various symmetry detection and repair algorithms:
    - Translation symmetry
    - Reflection symmetry (horizontal, vertical)
    - Diagonal symmetry (NW-SE, NE-SW)
    - Rotation symmetry (90°, 180°)
    - Symmetry repair and color filling
    """
    
    def __init__(self):
        """Initialize the symmetry solver."""
        self.symmetry_functions = [
            self._translation_sym,
            self._translation1d_sym,
            self._horizontal_sym,
            self._vertical_sym,
            self._nw_sym,
            self._ne_sym,
            self._rotate90_sym,
            self._rotate180_sym
        ]
        
        self.symmetry_params_functions = [
            self._translation_params,
            self._translation1d_params,
            self._horizontal_sym_params,
            self._vertical_sym_params,
            self._nw_sym_params,
            self._ne_sym_params,
            self._rotate90_sym_params,
            self._rotate180_sym_params
        ]
        
        self.symmetry_eq_functions = [
            self._translation_eq,
            self._translation1d_eq,
            self._horizontal_sym_eq,
            self._vertical_sym_eq,
            self._nw_sym_eq,
            self._ne_sym_eq,
            self._rotate90_sym_eq,
            self._rotate180_sym_eq
        ]
    
    def can_solve(self, task: Task) -> bool:
        """
        Check if task involves symmetry patterns.
        
        Args:
            task: The ARC task to analyze
            
        Returns:
            True if the task appears to involve symmetry patterns
        """
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        # Check if any training example shows symmetry patterns
        xs, ys = get_objects(task_dict)
        for x, y in zip(xs, ys):
            if self._has_symmetry_pattern(x) or self._has_symmetry_pattern(y):
                return True
        
        return False
    
    def solve(self, task: Task) -> List[np.ndarray]:
        """
        Generate symmetry-based predictions for test inputs.
        
        Args:
            task: The ARC task to solve
            
        Returns:
            List of candidate output arrays
        """
        if not self.can_solve(task):
            return []
        
        candidates = []
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        for test_input in task.test:
            # Try symmetry repair
            symmetry_candidates = self._symmetry_repair(task_dict, test_input)
            candidates.extend(symmetry_candidates)
        
        return candidates
    
    def _has_symmetry_pattern(self, matrix: np.ndarray) -> bool:
        """Check if matrix has any symmetry pattern."""
        # Check for basic symmetries
        for sym_func in self.symmetry_functions:
            if sym_func(matrix):
                return True
        return False
    
    def _translation_sym(self, x: np.ndarray) -> List[List]:
        """Detect translation symmetry."""
        n, k = x.shape
        best_r, best_s = n, k
        
        # Find best translation parameters
        for r in range(1, n):
            if np.array_equal(x[:n-r, :], x[r:, :]):
                best_r = r
                break
        
        for s in range(1, k):
            if np.array_equal(x[:, :k-s], x[:, s:]):
                best_s = s
                break
        
        if (best_r, best_s) == (n, k):
            return []
        
        # Build equivalence classes
        E = {}
        for i in range(n):
            for j in range(k):
                u, v = i % best_r, j % best_s
                p = (u, v)
                if p not in E:
                    E[p] = [(i, j)]
                else:
                    E[p].append((i, j))
        
        # Return classes with more than one element
        ans = []
        for p in E:
            if len(E[p]) > 1:
                ans.append(E[p])
        
        return ans
    
    def _translation1d_sym(self, x: np.ndarray) -> List[List]:
        """Detect 1D translation symmetry."""
        n, k = x.shape
        possible_s = []
        
        for r in range(-n+1, n):
            for s in range(-k+1, k):
                if s == 0 and r == 0:
                    continue
                
                equiv_colors = {}
                possible = True
                
                for i in range(n):
                    if not possible:
                        break
                    for j in range(k):
                        u = i * s - j * r
                        v = (i * r + j * s + 100 * (r*r + s*s)) % (r*r + s*s)
                        color = x[i, j]
                        
                        if (u, v) not in equiv_colors:
                            equiv_colors[(u, v)] = color
                        elif color != equiv_colors[(u, v)]:
                            possible = False
                            break
                
                if possible:
                    possible_s.append((r, s))
        
        if not possible_s:
            return []
        
        # Choose best parameters
        scores = [(abs(r) + abs(s), (r, s)) for r, s in possible_s]
        scores.sort()
        best_r, best_s = scores[0][1]
        
        # Build equivalence classes
        equiv_colors = {}
        for i in range(n):
            for j in range(k):
                u = i * best_s - j * best_r
                v = (i * best_r + j * best_s + 100 * (best_r*best_r + best_s*best_s)) % (best_r*best_r + best_s*best_s)
                if (u, v) not in equiv_colors:
                    equiv_colors[(u, v)] = []
                equiv_colors[(u, v)].append((i, j))
        
        return [equiv_colors[key] for key in equiv_colors if len(equiv_colors[key]) > 1]
    
    def _horizontal_sym(self, x: np.ndarray) -> List[List]:
        """Detect horizontal symmetry."""
        n, k = x.shape
        possible_r = []
        
        for r in range(1, 2*n-2):
            possible = True
            for i in range(n):
                for j in range(k):
                    i1 = r - i
                    if i1 < 0 or i1 >= n:
                        continue
                    if x[i, j] != x[i1, j]:
                        possible = False
                        break
                if not possible:
                    break
            if possible:
                possible_r.append(r)
        
        if not possible_r:
            return []
        
        # Choose best parameter
        scores = [(abs(r - n + 1), r) for r in possible_r]
        scores.sort()
        best_r = scores[0][1]
        
        # Build equivalence classes
        ans = []
        for i in range(n):
            for j in range(k):
                i1 = best_r - i
                if i1 < 0 or i1 >= n:
                    continue
                a, b = (i, j), (i1, j)
                if [a, b] not in ans and [b, a] not in ans and a != b:
                    ans.append([a, b])
        
        return ans
    
    def _vertical_sym(self, x: np.ndarray) -> List[List]:
        """Detect vertical symmetry."""
        n, k = x.shape
        possible_s = []
        
        for s in range(1, 2*k-2):
            possible = True
            for i in range(n):
                for j in range(k):
                    j1 = s - j
                    if j1 < 0 or j1 >= k:
                        continue
                    if x[i, j] != x[i, j1]:
                        possible = False
                        break
                if not possible:
                    break
            if possible:
                possible_s.append(s)
        
        if not possible_s:
            return []
        
        # Choose best parameter
        scores = [(abs(s - k + 1), s) for s in possible_s]
        scores.sort()
        best_s = scores[0][1]
        
        # Build equivalence classes
        ans = []
        for i in range(n):
            for j in range(k):
                j1 = best_s - j
                if j1 < 0 or j1 >= k:
                    continue
                a, b = (i, j), (i, j1)
                if [a, b] not in ans and [b, a] not in ans and a != b:
                    ans.append([a, b])
        
        return ans
    
    def _nw_sym(self, x: np.ndarray) -> List[List]:
        """Detect NW-SE diagonal symmetry."""
        n, k = x.shape
        possible_s = []
        
        for s in range(-k+2, n-1):
            possible = True
            for i in range(n):
                for j in range(k):
                    i1 = s + j
                    j1 = -s + i
                    if i1 < 0 or i1 >= n or j1 < 0 or j1 >= k:
                        continue
                    if x[i, j] != x[i1, j1]:
                        possible = False
                        break
                if not possible:
                    break
            if possible:
                possible_s.append(s)
        
        if not possible_s:
            return []
        
        # Choose best parameter
        scores = [(abs(s), s) for s in possible_s]
        scores.sort()
        best_s = scores[0][1]
        
        # Build equivalence classes
        ans = []
        for i in range(n):
            for j in range(k):
                i1 = best_s + j
                j1 = -best_s + i
                if i1 < 0 or i1 >= n or j1 < 0 or j1 >= k:
                    continue
                a, b = (i, j), (i1, j1)
                if [a, b] not in ans and [b, a] not in ans and a != b:
                    ans.append([a, b])
        
        return ans
    
    def _ne_sym(self, x: np.ndarray) -> List[List]:
        """Detect NE-SW diagonal symmetry."""
        n, k = x.shape
        possible_s = []
        
        for s in range(2, n+k-3):
            possible = True
            for i in range(n):
                for j in range(k):
                    i1 = s - j
                    j1 = s - i
                    if i1 < 0 or i1 >= n or j1 < 0 or j1 >= k:
                        continue
                    if x[i, j] != x[i1, j1]:
                        possible = False
                        break
                if not possible:
                    break
            if possible:
                possible_s.append(s)
        
        if not possible_s:
            return []
        
        # Choose best parameter
        scores = [(abs(2*s - n - k - 2), s) for s in possible_s]
        scores.sort()
        best_s = scores[0][1]
        
        # Build equivalence classes
        ans = []
        for i in range(n):
            for j in range(k):
                i1 = best_s - j
                j1 = best_s - i
                if i1 < 0 or i1 >= n or j1 < 0 or j1 >= k:
                    continue
                a, b = (i, j), (i1, j1)
                if [a, b] not in ans and [b, a] not in ans and a != b:
                    ans.append([a, b])
        
        return ans
    
    def _rotate90_sym(self, x: np.ndarray) -> List[List]:
        """Detect 90-degree rotation symmetry."""
        n, k = x.shape
        possible_s = []
        
        for r in range(1, 2*n-2):
            for s in range(1, 2*k-2):
                if (s + r) % 2 != 0:
                    continue
                u = (r - s) // 2
                v = (r + s) // 2
                possible = True
                
                for i in range(n):
                    if not possible:
                        break
                    for j in range(k):
                        neig = [(v-j, -u+i), (r-i, s-j), (j+u, v-i)]
                        for i1, j1 in neig:
                            if j1 < 0 or j1 >= k or i1 < 0 or i1 >= n:
                                continue
                            if x[i, j] != x[i1, j1]:
                                possible = False
                                break
                        if not possible:
                            break
                
                if possible:
                    possible_s.append((r, s))
        
        if not possible_s:
            return []
        
        # Choose best parameters
        scores = [(abs(r-n+1) + abs(s-k+1), (r, s)) for r, s in possible_s]
        scores.sort()
        best_r, best_s = scores[0][1]
        u = (best_r - best_s) // 2
        v = (best_r + best_s) // 2
        
        # Build equivalence classes
        ans = []
        for i in range(n):
            for j in range(k):
                neig = [(v-j, -u+i), (best_r-i, best_s-j), (j+u, v-i)]
                n2 = [(i, j)]
                for i1, j1 in neig:
                    if i1 < 0 or i1 >= n or j1 < 0 or j1 >= k:
                        continue
                    n2.append((i1, j1))
                n2.sort()
                if len(n2) == 1 or n2 in ans:
                    continue
                ans.append(n2)
        
        return ans
    
    def _rotate180_sym(self, x: np.ndarray) -> List[List]:
        """Detect 180-degree rotation symmetry."""
        n, k = x.shape
        possible_s = []
        
        for r in range(1, 2*n-2):
            for s in range(1, 2*k-2):
                possible = True
                for i in range(n):
                    if not possible:
                        break
                    for j in range(k):
                        i1 = r - i
                        j1 = s - j
                        if j1 < 0 or j1 >= k or i1 < 0 or i1 >= n:
                            continue
                        if x[i, j] != x[i1, j1]:
                            possible = False
                            break
                    if not possible:
                        break
                
                if possible:
                    possible_s.append((r, s))
        
        if not possible_s:
            return []
        
        # Choose best parameters
        scores = [(abs(r-n+1) + abs(s-k+1), (r, s)) for r, s in possible_s]
        scores.sort()
        best_r, best_s = scores[0][1]
        
        # Build equivalence classes
        ans = []
        for i in range(n):
            for j in range(k):
                i1 = best_r - i
                j1 = best_s - j
                if i1 < 0 or i1 >= n or j1 < 0 or j1 >= k:
                    continue
                a, b = (i, j), (i1, j1)
                if [a, b] not in ans and [b, a] not in ans and a != b:
                    ans.append([a, b])
        
        return ans
    
    # Parameter functions (simplified versions)
    def _translation_params(self, x: np.ndarray, badcolor: int = 20) -> Tuple[List, List, float]:
        """Get translation parameters."""
        result = self._translation_sym(x)
        if not result:
            return [], [], 0
        return [1], [0], 1.0
    
    def _translation1d_params(self, x: np.ndarray, badcolor: int = 20) -> Tuple[List, List, float]:
        """Get 1D translation parameters."""
        result = self._translation1d_sym(x)
        if not result:
            return [], [], 0
        return [1], [0], 1.0
    
    def _horizontal_sym_params(self, x: np.ndarray, badcolor: int = 20) -> Tuple[List, List, float]:
        """Get horizontal symmetry parameters."""
        n = x.shape[0]
        possible_r = []
        
        for r in range(1, 2*n-2):
            possible = True
            for i in range(n):
                for j in range(x.shape[1]):
                    i1 = r - i
                    if i1 < 0 or i1 >= n:
                        continue
                    if x[i, j] != x[i1, j] and x[i, j] != badcolor and x[i1, j] != badcolor:
                        possible = False
                        break
                if not possible:
                    break
            if possible:
                possible_r.append(r)
        
        if not possible_r:
            return [], [], 0
        
        scores = [(abs(r - n + 1), r) for r in possible_r]
        scores.sort()
        ans = [item[1] for item in scores]
        penalty = [item[0] for item in scores]
        
        sym_level = 0
        if ans:
            r = ans[0]
            sym_level = 1 - abs(r - n + 1) / n
        
        return ans[:3], penalty[:3], sym_level
    
    def _vertical_sym_params(self, x: np.ndarray, badcolor: int = 20) -> Tuple[List, List, float]:
        """Get vertical symmetry parameters."""
        k = x.shape[1]
        possible_s = []
        
        for s in range(1, 2*k-2):
            possible = True
            for i in range(x.shape[0]):
                for j in range(k):
                    j1 = s - j
                    if j1 < 0 or j1 >= k:
                        continue
                    if x[i, j] != x[i, j1] and x[i, j] != badcolor and x[i, j1] != badcolor:
                        possible = False
                        break
                if not possible:
                    break
            if possible:
                possible_s.append(s)
        
        if not possible_s:
            return [], [], 0
        
        scores = [(abs(s - k + 1), s) for s in possible_s]
        scores.sort()
        ans = [item[1] for item in scores]
        penalty = [item[0] for item in scores]
        
        sym_level = 0
        if ans:
            s = ans[0]
            sym_level = 1 - abs(s - k + 1) / k
        
        return ans[:3], penalty[:3], sym_level
    
    def _nw_sym_params(self, x: np.ndarray, badcolor: int = 20) -> Tuple[List, List, float]:
        """Get NW symmetry parameters."""
        n, k = x.shape
        possible_s = []
        
        for s in range(-k+2, n-1):
            possible = True
            for i in range(n):
                for j in range(k):
                    i1 = s + j
                    j1 = -s + i
                    if i1 < 0 or i1 >= n or j1 < 0 or j1 >= k:
                        continue
                    if x[i, j] != x[i1, j1]:
                        possible = False
                        break
                if not possible:
                    break
            if possible:
                possible_s.append(s)
        
        if not possible_s:
            return [], [], 0
        
        scores = [(abs(s), s) for s in possible_s]
        scores.sort()
        ans = [item[1] for item in scores]
        penalty = [item[0] for item in scores]
        
        sym_level = 0
        if ans:
            s = ans[0]
            sym_level = 1 - abs(s) / (n + k)
        
        return ans[:3], penalty[:3], sym_level
    
    def _ne_sym_params(self, x: np.ndarray, badcolor: int = 20) -> Tuple[List, List, float]:
        """Get NE symmetry parameters."""
        n, k = x.shape
        possible_s = []
        
        for s in range(2, n+k-3):
            possible = True
            for i in range(n):
                for j in range(k):
                    i1 = s - j
                    j1 = s - i
                    if i1 < 0 or i1 >= n or j1 < 0 or j1 >= k:
                        continue
                    if x[i, j] != x[i1, j1] and x[i, j] != badcolor and x[i1, j1] != badcolor:
                        possible = False
                        break
                if not possible:
                    break
            if possible:
                possible_s.append(s)
        
        if not possible_s:
            return [], [], 0
        
        scores = [(abs(2*s - n - k - 2), s) for s in possible_s]
        scores.sort()
        ans = [item[1] for item in scores]
        penalty = [item[0] for item in scores]
        
        sym_level = 0
        if ans:
            s = ans[0]
            sym_level = 1 - abs(2*s - n - k - 2) / (n + k)
        
        return ans[:3], penalty[:3], sym_level
    
    def _rotate90_sym_params(self, x: np.ndarray, badcolor: int = 20) -> Tuple[List, List, float]:
        """Get 90-degree rotation parameters."""
        n, k = x.shape
        possible_s = []
        
        for r in range(1, 2*n-2):
            for s in range(1, 2*k-2):
                if (s + r) % 2 != 0:
                    continue
                u = (r - s) // 2
                v = (r + s) // 2
                possible = True
                
                for i in range(n):
                    if not possible:
                        break
                    for j in range(k):
                        neig = [(v-j, -u+i), (r-i, s-j), (j+u, v-i)]
                        for i1, j1 in neig:
                            if j1 < 0 or j1 >= k or i1 < 0 or i1 >= n:
                                continue
                            if x[i, j] != x[i1, j1] and x[i, j] != badcolor and x[i1, j1] != badcolor:
                                possible = False
                                break
                        if not possible:
                            break
                
                if possible:
                    possible_s.append((r, s))
        
        if not possible_s:
            return [], [], 0
        
        scores = [(abs(r-n+1) + abs(s-k+1), (r, s)) for r, s in possible_s]
        scores.sort()
        ans = [item[1] for item in scores]
        penalty = [item[0] for item in scores]
        
        sym_level = 0
        if ans:
            r, s = ans[0]
            sym_level = 1 - (abs(r-n+1) + abs(s-k+1)) / (n + k)
        
        return ans[:3], penalty[:3], sym_level
    
    def _rotate180_sym_params(self, x: np.ndarray, badcolor: int = 20) -> Tuple[List, List, float]:
        """Get 180-degree rotation parameters."""
        n, k = x.shape
        possible_s = []
        
        for r in range(1, 2*n-2):
            for s in range(1, 2*k-2):
                possible = True
                for i in range(n):
                    if not possible:
                        break
                    for j in range(k):
                        i1 = r - i
                        j1 = s - j
                        if j1 < 0 or j1 >= k or i1 < 0 or i1 >= n:
                            continue
                        if x[i, j] != x[i1, j1] and x[i, j] != badcolor and x[i1, j1] != badcolor:
                            possible = False
                            break
                    if not possible:
                        break
                
                if possible:
                    possible_s.append((r, s))
        
        if not possible_s:
            return [], [], 0
        
        scores = [(abs(r-n+1) + abs(s-k+1), (r, s)) for r, s in possible_s]
        scores.sort()
        ans = [item[1] for item in scores]
        penalty = [item[0] for item in scores]
        
        sym_level = 0
        if ans:
            r, s = ans[0]
            sym_level = 1 - (abs(r-n+1) + abs(s-k+1)) / (n + k)
        
        return ans[:3], penalty[:3], sym_level
    
    # Equivalence functions
    def _translation_eq(self, x: np.ndarray, param: int) -> List[List]:
        """Get translation equivalence classes."""
        return self._translation_sym(x)
    
    def _translation1d_eq(self, x: np.ndarray, param: Tuple[int, int]) -> List[List]:
        """Get 1D translation equivalence classes."""
        return self._translation1d_sym(x)
    
    def _horizontal_sym_eq(self, x: np.ndarray, param: int) -> List[List]:
        """Get horizontal symmetry equivalence classes."""
        return self._horizontal_sym(x)
    
    def _vertical_sym_eq(self, x: np.ndarray, param: int) -> List[List]:
        """Get vertical symmetry equivalence classes."""
        return self._vertical_sym(x)
    
    def _nw_sym_eq(self, x: np.ndarray, param: int) -> List[List]:
        """Get NW symmetry equivalence classes."""
        return self._nw_sym(x)
    
    def _ne_sym_eq(self, x: np.ndarray, param: int) -> List[List]:
        """Get NE symmetry equivalence classes."""
        return self._ne_sym(x)
    
    def _rotate90_sym_eq(self, x: np.ndarray, param: Tuple[int, int]) -> List[List]:
        """Get 90-degree rotation equivalence classes."""
        return self._rotate90_sym(x)
    
    def _rotate180_sym_eq(self, x: np.ndarray, param: Tuple[int, int]) -> List[List]:
        """Get 180-degree rotation equivalence classes."""
        return self._rotate180_sym(x)
    
    def _sym_score(self, x: np.ndarray, first_p: List[int]) -> float:
        """Calculate symmetry score."""
        score = 0
        for s in first_p:
            if s < len(self.symmetry_params_functions):
                func = self.symmetry_params_functions[s]
                value = func(x)[2]  # Get symmetry level
                score += value
        return score
    
    def _make_picture(self, x: np.ndarray, relations: List[List], badcolor: int) -> Optional[np.ndarray]:
        """Make picture from symmetry relations."""
        n, k = x.shape
        x0 = x.copy()
        
        # Build equivalence graph
        G = {}
        for i in range(n):
            for j in range(k):
                G[(i, j)] = (i, j)
        
        # Union-find to merge equivalence classes
        for class_list in relations:
            for p in class_list:
                for q in class_list:
                    if G[p] == G[q]:
                        continue
                    a, b = G[p], G[q]
                    if a < b:
                        G[q] = a
                    else:
                        G[p] = b
        
        # Build final equivalence classes
        H = {}
        for i in range(n):
            for j in range(k):
                p = (i, j)
                while G[p] != p:
                    p = G[p]
                if p not in H:
                    H[p] = [(i, j)]
                else:
                    H[p].append((i, j))
        
        # Color each equivalence class
        for key in H:
            item = H[key]
            colors = []
            for i, j in item:
                c = x[i, j]
                if c not in colors:
                    colors.append(c)
            
            if len(colors) <= 1:
                continue  # Nothing to recolor
            
            if len(colors) > 2:
                return None  # Collision
            
            if len(colors) == 2 and badcolor not in colors:
                return None  # Collision
            
            if len(colors) == 2 and badcolor == colors[0]:
                fillcolor = colors[1]
            else:
                fillcolor = colors[0]
            
            for i, j in item:
                x0[i, j] = fillcolor
        
        return x0
    
    def _symmetry_repair(self, task: Dict[str, Any], test_input: np.ndarray) -> List[np.ndarray]:
        """Main symmetry repair function."""
        xs, ys = get_objects(task)
        
        # Check if task is solvable by symmetry
        if not self._is_solvable_by_symmetry(xs, ys):
            return []
        
        # Find disappearing colors
        colors = []
        for x, y in zip(xs, ys):
            if x.shape != y.shape:
                return []
            
            disappearing = []
            for i in range(x.shape[0]):
                for j in range(x.shape[1]):
                    color1 = x[i, j]
                    color2 = y[i, j]
                    if color2 != color1 and color1 not in disappearing:
                        disappearing.append(color1)
            
            if len(disappearing) > 1:
                return []
            if len(disappearing) == 1:
                c = disappearing[0]
                if c not in colors:
                    colors.append(c)
        
        # Get candidate bad colors
        if len(colors) == 1:
            c2 = [colors[0]]
        else:
            c2 = []
            for row in test_input:
                for c in row:
                    if c not in c2:
                        c2.append(c)
        
        # Try different symmetry combinations
        ans = []
        first_p = None
        
        # Get solvable symmetry combinations
        solvable_combinations = self._get_solvable_combinations()
        
        for p in solvable_combinations:
            for c in c2:
                candidates = self._proba_symmetry(xs, ys, test_input, c, p)
                if candidates:
                    ans.extend(candidates[:6])
            if ans:
                first_p = p
                break
        
        if not ans:
            return []
        
        # Score and sort candidates
        scores = []
        for picture in ans:
            if first_p is not None:
                s = self._sym_score(picture, first_p)
                scores.append((s, picture))
        
        # Sort by score (first element of tuple)
        scores.sort(key=lambda x: x[0], reverse=True)
        ans2 = []
        for _, picture in scores:
            if not any(np.array_equal(picture, p) for p in ans2):
                ans2.append(picture)
        
        return ans2[:3]
    
    def _is_solvable_by_symmetry(self, xs: List[np.ndarray], ys: List[np.ndarray]) -> bool:
        """Check if task can be solved by symmetry."""
        # Simplified check - in practice would be more sophisticated
        return len(xs) > 0 and len(ys) > 0
    
    def _get_solvable_combinations(self) -> List[List[int]]:
        """Get solvable symmetry combinations."""
        # Simplified - return common combinations
        return [[0], [1], [0, 1], [2], [3], [2, 3], [4], [5], [4, 5], [6], [7]]
    
    def _proba_symmetry(self, xs: List[np.ndarray], ys: List[np.ndarray], 
                       test_input: np.ndarray, bad_color: int, first_p: List[int]) -> List[np.ndarray]:
        """Generate symmetry-based predictions."""
        ans = []
        
        if len(first_p) == 1:
            s = first_p[0]
            if s < len(self.symmetry_params_functions):
                f = self.symmetry_params_functions[s]
                g = self.symmetry_eq_functions[s]
                params = f(test_input, bad_color)[0]
                
                for p in params:
                    a = g(test_input, p)
                    picture = self._make_picture(test_input, a, bad_color)
                    if picture is not None and not self._is_uniform(picture):
                        ans.append(picture)
        
        elif len(first_p) == 2:
            s1, s2 = first_p[0], first_p[1]
            if s1 < len(self.symmetry_params_functions) and s2 < len(self.symmetry_params_functions):
                f1 = self.symmetry_params_functions[s1]
                f2 = self.symmetry_params_functions[s2]
                g1 = self.symmetry_eq_functions[s1]
                g2 = self.symmetry_eq_functions[s2]
                
                params1 = f1(test_input, bad_color)[0]
                params2 = f2(test_input, bad_color)[0]
                
                for k in range(6):
                    for i in range(len(params1)):
                        for j in range(len(params2)):
                            if i + j == k:
                                p1 = params1[i]
                                p2 = params2[j]
                                a1 = g1(test_input, p1)
                                a2 = g2(test_input, p2)
                                a = a1 + a2
                                picture = self._make_picture(test_input, a, bad_color)
                                if picture is not None and not self._is_uniform(picture):
                                    ans.append(picture)
        
        return ans
    
    def _is_uniform(self, picture: np.ndarray) -> bool:
        """Check if picture is uniform (all same color)."""
        return np.min(picture) == np.max(picture) 