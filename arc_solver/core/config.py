"""
Configuration classes for the ARC Solver.
"""

from dataclasses import dataclass
from typing import Optional


@dataclass
class SolverConfig:
    """Configuration for the ARC Solver."""
    
    # Solver enablement flags
    enable_repeating_solver: bool = True
    enable_grid_solver: bool = True
    enable_chess_solver: bool = True
    enable_tiling_solver: bool = True
    enable_submatrix_solver: bool = True
    enable_submask_solver: bool = True
    enable_ml_solver: bool = True
    enable_symmetry_solver: bool = True
    enable_color_counter_solver: bool = True
    
    # Performance settings
    max_candidates: int = 5
    max_depth: int = 4
    timeout_seconds: Optional[int] = None
    
    # ML settings
    ml_model_type: str = "decision_tree"  # "decision_tree", "xgboost", "ensemble"
    feature_extraction_method: str = "comprehensive"
    
    # Visualization settings
    enable_visualization: bool = True
    save_intermediate_results: bool = False
    
    # Integration settings
    enable_icecube_integration: bool = True
    icecube_timeout: int = 300  # seconds 