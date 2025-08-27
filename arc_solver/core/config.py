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
    enable_dag_solver: bool = True  # DAG solver作为fallback
    
    # Performance settings
    max_candidates: int = 5
    max_depth: int = 4
    timeout_seconds: Optional[int] = None
    
    # DAG solver settings
    dag_high_confidence_threshold: float = 15.0  # 专用solver可信度阈值
    dag_max_depth: int = 20
    dag_enable_logging: bool = False
    
    # ML settings
    ml_model_type: str = "decision_tree"  # "decision_tree", "xgboost", "ensemble"
    feature_extraction_method: str = "comprehensive"
    
    # Visualization settings
    enable_visualization: bool = True
    save_intermediate_results: bool = False
    
    # Integration settings
    enable_icecube_integration: bool = True
    icecube_timeout: int = 300  # seconds 