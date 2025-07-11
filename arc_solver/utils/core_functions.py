"""
Core utility functions for ARC solver.
"""

import numpy as np
from typing import List, Tuple, Optional, Dict, Any
from collections import defaultdict


def flattener(pred: np.ndarray) -> str:
    """
    Convert prediction array to string format.
    
    Args:
        pred: Prediction array
        
    Returns:
        String representation of the prediction
    """
    str_pred = str([row for row in pred])
    str_pred = str_pred.replace(', ', '')
    str_pred = str_pred.replace('[[', '|')
    str_pred = str_pred.replace('][', '|')
    str_pred = str_pred.replace(']]', '|')
    return str_pred


def get_objects(task: Dict[str, Any]) -> Tuple[List[np.ndarray], List[np.ndarray]]:
    """
    Extract input and output arrays from task.
    
    Args:
        task: Task dictionary
        
    Returns:
        Tuple of (input_arrays, output_arrays)
    """
    xs, ys = [], []
    for obj in task['train']:
        xs.append(np.array(obj['input']))
        ys.append(np.array(obj['output']))
    return xs, ys


def find_sub(matrix: np.ndarray, sub: np.ndarray, ignore: Optional[int] = None) -> List[Tuple[int, int, int, int]]:
    """
    Find all positions of sub-matrix in matrix.
    
    Args:
        matrix: Main matrix
        sub: Sub-matrix to find
        ignore: Value to ignore in comparison
        
    Returns:
        List of (x, y, x_end, y_end) positions
    """
    positions = []
    mask = sub != ignore
    sub_ = sub[mask]
    for x in range(matrix.shape[0]-sub.shape[0]+1):
        for y in range(matrix.shape[1]-sub.shape[1]+1):
            if np.array_equal(matrix[x:x+sub.shape[0], y:y+sub.shape[1]][mask], sub_):
                positions.append((x, y, x+sub.shape[0], y+sub.shape[1]))
    return positions


def get_mode_color(ar: np.ndarray) -> int:
    """
    Get the most frequent color in array.
    
    Args:
        ar: Input array
        
    Returns:
        Most frequent color value
    """
    colors, counts = np.unique(ar, return_counts=True)
    return colors[np.argmax(counts)]


def get_cells(x: np.ndarray, cols: List[int], rows: List[int]) -> np.ndarray:
    """
    Extract cells from grid based on column and row indices.
    
    Args:
        x: Input matrix
        cols: Column indices
        rows: Row indices
        
    Returns:
        Array of cells
    """
    if cols[0] != 0:
        cols = [-1] + cols
    if rows[0] != 0:
        rows = [-1] + rows
    if cols[-1] != x.shape[0]-1:
        cols.append(x.shape[0])
    if rows[-1] != x.shape[1]-1:
        rows.append(x.shape[1])
    
    cells = np.full((len(cols)-1, len(rows)-1), object)
    for i in range(len(cols)-1):
        for j in range(len(rows)-1):
            cells[i][j] = x[cols[i]+1:cols[i+1], rows[j]+1:rows[j+1]]
    return cells


def get_grid(x: np.ndarray) -> Tuple[int, List[int], List[int]]:
    """
    Detect grid structure in matrix.
    
    Args:
        x: Input matrix
        
    Returns:
        Tuple of (grid_color, column_indices, row_indices)
    """
    cols = defaultdict(list)
    rows = defaultdict(list)
    
    if x.shape[0] < 3 or x.shape[1] < 3:
        return -1, [], []
    
    for i in range(x.shape[0]):
        if len(np.unique(x[i])) == 1:
            cols[x[i,0]].append(i)
    
    for i in range(x.shape[1]):
        if len(np.unique(x[:,i])) == 1:
            rows[x[0,i]].append(i)
    
    for c in cols:
        if c in rows and all(np.diff(cols[c]) > 1) and all(np.diff(rows[c]) > 1):
            return c, cols[c], rows[c]
    
    return -1, [], []


def grid_filter(x: np.ndarray) -> np.ndarray:
    """
    Filter matrix to extract grid structure.
    
    Args:
        x: Input matrix
        
    Returns:
        Filtered matrix
    """
    color_of_grid, cols, rows = get_grid(x)
    if color_of_grid == -1:
        return x
    
    cells = get_cells(x, cols, rows)
    return np.array([get_mode_color(cell) for cell in cells.reshape(-1)]).reshape(cells.shape)


def mul_ratio(x: np.ndarray, x_ratio: Tuple[int, int]) -> np.ndarray:
    """
    Multiply matrix by ratio.
    
    Args:
        x: Input matrix
        x_ratio: Ratio tuple (height_ratio, width_ratio)
        
    Returns:
        Scaled matrix
    """
    x_shape = (x.shape[0] * x_ratio[0], x.shape[1] * x_ratio[1])
    x_ = np.array([x[i//x_ratio[0]][j//x_ratio[1]] for i, j in np.ndindex(x_shape)]).reshape(x_shape)
    return x_


def mul_ratios(x: np.ndarray, y: np.ndarray, x_ratio: Tuple[int, int], y_ratio: Tuple[int, int]) -> Tuple[np.ndarray, np.ndarray]:
    """
    Multiply two matrices by their respective ratios.
    
    Args:
        x: First matrix
        y: Second matrix
        x_ratio: Ratio for first matrix
        y_ratio: Ratio for second matrix
        
    Returns:
        Tuple of scaled matrices
    """
    x_shape = (x.shape[0] * x_ratio[0], x.shape[1] * x_ratio[1])
    x_ = np.array([x[i//x_ratio[0]][j//x_ratio[1]] for i, j in np.ndindex(x_shape)]).reshape(x_shape)
    
    y_shape = (y.shape[0] * y_ratio[0], y.shape[1] * y_ratio[1])
    y_ = np.array([y[i//y_ratio[0]][j//y_ratio[1]] for i, j in np.ndindex(y_shape)]).reshape(y_shape)
    
    return x_, y_


def get_ratio(xs: List[np.ndarray], ys: List[np.ndarray]) -> Optional[Tuple[Tuple[int, int], Tuple[int, int]]]:
    """
    Calculate scaling ratios between input and output arrays.
    
    Args:
        xs: List of input arrays
        ys: List of output arrays
        
    Returns:
        Tuple of (x_ratio, y_ratio) or None if no consistent ratio
    """
    x_ratio = []
    y_ratio = []
    
    for i in range(2):
        if all(x.shape[i] % y.shape[i] == 0 for x, y in zip(xs, ys)):
            if len(set(x.shape[i] // y.shape[i] for x, y in zip(xs, ys))) == 1:
                x_ratio.append(1)
                y_ratio.append(xs[0].shape[i] // ys[0].shape[i])
        elif all(y.shape[i] % x.shape[i] == 0 for x, y in zip(xs, ys)):
            if len(set(y.shape[i] // x.shape[i] for x, y in zip(xs, ys))) == 1:
                x_ratio.append(ys[0].shape[i] // xs[0].shape[i])
                y_ratio.append(1)
    
    if len(x_ratio) != 2:
        return None
    
    return tuple(x_ratio), tuple(y_ratio) 