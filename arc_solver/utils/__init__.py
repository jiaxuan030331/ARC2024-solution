"""
Utility modules for ARC solver.

This package contains various utility functions and classes
for data processing, visualization, and other common operations.
"""

from .core_functions import (
    get_objects, find_sub, get_grid, get_cells, get_mode_color,
    grid_filter, mul_ratio, mul_ratios, get_ratio
)

from .transforms import (
    get_transform, get_transforms, get_all_transforms,
    predict_transforms, predict_transforms_2x
)

from .candidate_selector import CandidateSelector

from .ml_functions import (
    make_features, format_features, tree1
)

from .visualization import (
    ARCVisualizer,
    plot_pic, plot_task, plot_objects,
    CMAP, NORM, COLOR_LIST
)

from .data_processing import (
    DataProcessor, TaskValidator,
    defensive_copy, create_task, flattener
)

__all__ = [
    # Core functions
    'get_objects', 'find_sub', 'get_grid', 'get_cells', 'get_mode_color',
    'grid_filter', 'mul_ratio', 'mul_ratios', 'get_ratio',
    
    # Transforms
    'get_transform', 'get_transforms', 'get_all_transforms',
    'predict_transforms', 'predict_transforms_2x',
    
    # Candidate selection
    'CandidateSelector',
    
    # ML functions
    'make_features', 'format_features', 'tree1',
    
    # Visualization
    'ARCVisualizer', 'plot_pic', 'plot_task', 'plot_objects', 
    'CMAP', 'NORM', 'COLOR_LIST',
    
    # Data processing
    'DataProcessor', 'TaskValidator', 'defensive_copy', 'create_task', 
    'flattener'
] 