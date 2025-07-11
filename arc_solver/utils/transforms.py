"""
Image transformation functions for ARC solver.
"""

import numpy as np
from typing import List, Callable, Optional, Tuple


def roll_color(g: np.ndarray) -> np.ndarray:
    """
    Roll colors in the array.
    
    Args:
        g: Input array
        
    Returns:
        Array with rolled colors
    """
    from_values = np.unique(g)
    to_values = np.roll(from_values, 1)
    
    sort_idx = np.argsort(from_values)
    idx = np.searchsorted(from_values, g, sorter=sort_idx)
    return to_values[sort_idx][idx]


def get_all_transforms() -> List[Callable[[np.ndarray], np.ndarray]]:
    """
    Get all available transformation functions.
    
    Returns:
        List of transformation functions
    """
    return [
        lambda x: roll_color(x),
        lambda x: np.roll(x, -1, axis=0),
        lambda x: np.roll(x, 1, axis=0),
        lambda x: np.roll(x, -1, axis=1),
        lambda x: np.roll(x, 1, axis=1),
        lambda x: np.rot90(x.T, k=1),
        lambda x: np.rot90(x.T, k=2),
        lambda x: np.rot90(x.T, k=3),
        lambda x: np.rot90(x.T, k=4),
        lambda x: np.rot90(x, k=1),
        lambda x: np.rot90(x, k=2),
        lambda x: np.rot90(x, k=3),
        lambda x: x,
    ]


def get_transform(xs: List[np.ndarray], ys: List[np.ndarray]) -> Optional[Callable[[np.ndarray], np.ndarray]]:
    """
    Find transformation that maps xs to ys.
    
    Args:
        xs: List of input arrays
        ys: List of output arrays
        
    Returns:
        Transformation function or None if not found
    """
    for transform in get_all_transforms():
        transformed = True
        for x, y in zip(xs, ys):
            if transform(x).shape != y.shape:
                transformed = False
                break
            if not np.equal(transform(x), y).all():
                transformed = False
        if transformed:
            return transform
    return None


def get_transforms(xs: List[np.ndarray], ys: List[np.ndarray]) -> Optional[Callable[[np.ndarray], np.ndarray] | np.ndarray]:
    """
    Find transformations with ratio scaling.
    
    Args:
        xs: List of input arrays
        ys: List of output arrays
        
    Returns:
        Transformation function or None if not found
    """
    from .core_functions import get_ratio, mul_ratios
    
    fn = get_transform(xs, ys)
    if fn:
        return fn
    
    ratio = get_ratio(xs, ys)
    if ratio:
        x_ratio, y_ratio = ratio
        xs_ = []
        ys_ = []
        for x, y in zip(xs, ys):
            x, y = mul_ratios(x, y, x_ratio, y_ratio)
            xs_.append(x)
            ys_.append(y)
        
        fn = get_transform(xs_, ys_)
        if fn:
            return fn
        
        fns = np.full(x_ratio, object)
        for i, j in np.ndindex(x_ratio):
            ys_ = []
            for y in ys:
                m1 = y.shape[0] // x_ratio[0]
                m2 = y.shape[1] // x_ratio[1]
                ys_.append(y[i*m1:(i+1)*m1, j*m2:(j+1)*m2])
            fn = get_transform(xs, ys_)
            if fn:
                fns[i, j] = fn
            else:
                return None
        return fns
    
    return None


def predict_transforms(xs: List[np.ndarray], ys: List[np.ndarray], test: np.ndarray) -> List[np.ndarray]:
    """
    Predict transformations for test input.
    
    Args:
        xs: List of input arrays
        ys: List of output arrays
        test: Test input array
        
    Returns:
        List of predicted outputs
    """
    from .core_functions import get_ratio, mul_ratios, mul_ratio
    
    fn = get_transform(xs, ys)
    if fn:
        return [fn(test)]
    
    ratio = get_ratio(xs, ys)
    if ratio:
        x_ratio, y_ratio = ratio
        xs_ = []
        ys_ = []
        for x, y in zip(xs, ys):
            x, y = mul_ratios(x, y, x_ratio, y_ratio)
            xs_.append(x)
            ys_.append(y)
        
        fn = get_transform(xs_, ys_)
        if fn:
            test = mul_ratio(test, x_ratio)
            return [fn(test)]
        
        fns = np.full(x_ratio, object)
        for i, j in np.ndindex(x_ratio):
            ys_ = []
            for y in ys:
                m1 = y.shape[0] // x_ratio[0]
                m2 = y.shape[1] // x_ratio[1]
                ys_.append(y[i*m1:(i+1)*m1, j*m2:(j+1)*m2])
            fn = get_transform(xs, ys_)
            if fn:
                fns[i, j] = fn
            else:
                return []
        
        return [np.concatenate([np.concatenate([fns[i, j](test) for i in range(x_ratio[0])], axis=0) for j in range(x_ratio[1])], axis=1)]
    
    return []


def predict_transforms_2x(xs: List[np.ndarray], ys: List[np.ndarray], test: np.ndarray) -> List[np.ndarray]:
    """
    Predict 2x transformations for test input.
    
    Args:
        xs: List of input arrays
        ys: List of output arrays
        test: Test input array
        
    Returns:
        List of predicted outputs
    """
    predictions = []
    transforms = [
        lambda x: np.rot90(x.T, k=1),
        lambda x: np.rot90(x.T, k=3),
        lambda x: np.rot90(x, k=2),
        lambda x: x,
    ]
    quads = [(1, 3, 2, 0), (3, 1, 0, 2), (2, 0, 1, 3)]  # 3 full symmetrical shapes
    
    for f1, f2, f3, f4 in quads:
        fns = np.array([[transforms[f1], transforms[f2]], [transforms[f3], transforms[f4]]])
        x_ = np.concatenate([np.concatenate([fns[i, j](test) for i in range(2)], axis=0) for j in range(2)], axis=1)
        predictions.append(x_)
    
    return predictions 