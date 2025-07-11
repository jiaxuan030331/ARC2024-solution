"""
Core modules for ARC solver.
"""

from .solver import ArcSolver, SolverResult
from .config import SolverConfig
from .patterns import (
    PatternDetector, pattern_detector,
    check_repeating, predict_repeating, predict_repeating_mask,
    check_chess, has_chess, has_antichess, predict_chess,
    check_tiles_shape, predict_tiles_shape, has_tiles, has_tiles_shape,
    check_grid, check_sub_grid_2x, check_grid_transforms, 
    predict_grid_transforms, predict_transforms_grid_2x,
    check_samesize, check_subitem, check_sub_mask
)
from .main_flow import (
    MainFlowController, main_flow_controller,
    ganswer_answer, ganswer_answer_1, prn_plus, prn_select_2,
    run_main_solvers, solve_batch
)

__all__ = [
    "ArcSolver",
    "SolverResult",
    "SolverConfig",
    "PatternDetector",
    "pattern_detector",
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
    "MainFlowController",
    "main_flow_controller",
    "ganswer_answer",
    "ganswer_answer_1",
    "prn_plus",
    "prn_select_2",
    "run_main_solvers",
    "solve_batch",
] 