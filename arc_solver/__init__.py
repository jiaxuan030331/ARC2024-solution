"""
ARC Solver - Multi-Strategy Abstract Reasoning Corpus Solver

A comprehensive solution for the ARC challenge combining heuristic algorithms,
machine learning, and pattern recognition techniques.
"""

__version__ = "1.0.0"
__author__ = "Your Name"
__email__ = "your.email@example.com"

from .core.solver import ArcSolver
from .core.config import SolverConfig
from .data.task import Task, TaskExample, TaskLoader

# C++ optimized solvers
from .cpp_wrappers.symmetry_wrapper import SymmetrySolverWrapper
from .cpp_wrappers.chess_wrapper import ChessSolverWrapper
from .cpp_wrappers.tiling_wrapper import TilingSolverWrapper
from .cpp_wrappers.ml_wrapper import MLSolverWrapper
from .core.patterns import (
    PatternDetector,
    check_repeating, predict_repeating, predict_repeating_mask,
    check_chess, has_chess, has_antichess, predict_chess,
    check_tiles_shape, predict_tiles_shape, has_tiles, has_tiles_shape,
    check_grid, check_sub_grid_2x, check_grid_transforms, 
    predict_grid_transforms, predict_transforms_grid_2x,
    check_samesize, check_subitem, check_sub_mask
)
from .core.main_flow import (
    MainFlowController, main_flow_controller,
    ganswer_answer, ganswer_answer_1, prn_plus, prn_select_2,
    run_main_solvers, solve_batch
)

# Backward compatibility with notebook functions
from .utils.core_functions import (
    flattener, get_objects, find_sub, get_mode_color,
    get_cells, get_grid, grid_filter, mul_ratio, mul_ratios, get_ratio
)

from .utils.transforms import (
    roll_color, get_all_transforms, get_transform, get_transforms,
    predict_transforms, predict_transforms_2x
)

from .utils.ml_functions import (
    make_features, format_features, tree1
)

__all__ = [
    "ArcSolver",
    "SolverConfig", 
    "Task",
    "TaskExample",
    "TaskLoader",
    # C++ optimized solvers
    "SymmetrySolverWrapper",
    "ChessSolverWrapper",
    "TilingSolverWrapper",
    "MLSolverWrapper",
    "PatternDetector",
    # Pattern detection functions
    "check_repeating",
    "predict_repeating", 
    "predict_repeating_mask",
    "check_chess",
    "has_chess",
    "has_antichess",
    "predict_chess", 
    "check_tiles_shape",
    "predict_tiles_shape",
    "has_tiles",
    "has_tiles_shape",
    "check_grid",
    "check_sub_grid_2x", 
    "check_grid_transforms",
    "predict_grid_transforms",
    "predict_transforms_grid_2x",
    "check_samesize",
    "check_subitem",
    "check_sub_mask",
    # Main flow control functions
    "MainFlowController",
    "main_flow_controller",
    "ganswer_answer",
    "ganswer_answer_1",
    "prn_plus",
    "prn_select_2",
    "run_main_solvers",
    "solve_batch",
    # Core utility functions
    "flattener",
    "get_objects",
    "find_sub",
    "get_mode_color",
    "get_cells", 
    "get_grid",
    "grid_filter",
    "mul_ratio",
    "mul_ratios",
    "get_ratio",
    # Transform functions
    "roll_color",
    "get_all_transforms",
    "get_transform",
    "get_transforms", 
    "predict_transforms",
    "predict_transforms_2x",
    # ML functions
    "make_features",
    "format_features",
    "tree1",
] 