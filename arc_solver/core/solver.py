"""
Main ARC Solver class that orchestrates all solving strategies.
"""

import numpy as np
from typing import List, Dict, Any, Optional
from dataclasses import dataclass

from ..data.task import Task
from ..core.config import SolverConfig
from ..solvers import (
    RepeatingSolver,
    GridSolver,
    ChessSolver,
    TilingSolver,
    SubmatrixSolver,
    SubmaskSolver,
    MLSolver,
    SymmetrySolver,
    ColorCounterSolver
)
from ..utils.candidate_selector import CandidateSelector


@dataclass
class SolverResult:
    """Result of solving an ARC task."""
    task_id: str
    predictions: List[np.ndarray]
    scores: List[float]
    solver_contributions: Dict[str, List[np.ndarray]]
    metadata: Dict[str, Any]


class ArcSolver:
    """
    Main ARC Solver that combines multiple solving strategies.
    
    This class orchestrates the execution of various specialized solvers
    and combines their results to generate the best possible predictions.
    """
    
    def __init__(self, config: Optional[SolverConfig] = None):
        """
        Initialize the ARC Solver.
        
        Args:
            config: Configuration for the solver. If None, uses default config.
        """
        self.config = config or SolverConfig()
        self._initialize_solvers()
        self.candidate_selector = CandidateSelector()
        
    def _initialize_solvers(self):
        """Initialize all available solvers based on configuration."""
        self.solvers = {}
        
        if self.config.enable_repeating_solver:
            self.solvers['repeating'] = RepeatingSolver()
            
        if self.config.enable_grid_solver:
            self.solvers['grid'] = GridSolver()
            
        if self.config.enable_chess_solver:
            self.solvers['chess'] = ChessSolver()
            
        if self.config.enable_tiling_solver:
            self.solvers['tiling'] = TilingSolver()
            
        if self.config.enable_submatrix_solver:
            self.solvers['submatrix'] = SubmatrixSolver()
            
        if self.config.enable_submask_solver:
            self.solvers['submask'] = SubmaskSolver()
            
        if self.config.enable_ml_solver:
            self.solvers['ml'] = MLSolver()
            
        if self.config.enable_symmetry_solver:
            self.solvers['symmetry'] = SymmetrySolver()
            
        if self.config.enable_color_counter_solver:
            self.solvers['color_counter'] = ColorCounterSolver()
    
    def solve(self, task: Task) -> SolverResult:
        """
        Solve an ARC task using all available solvers.
        
        Args:
            task: The ARC task to solve
            
        Returns:
            SolverResult containing predictions and metadata
        """
        all_candidates = []
        solver_contributions = {}
        
        # Run each solver
        for solver_name, solver in self.solvers.items():
            try:
                candidates = solver.solve(task)
                if candidates:
                    all_candidates.extend(candidates)
                    solver_contributions[solver_name] = candidates
                    
            except Exception as e:
                print(f"Warning: Solver {solver_name} failed: {e}")
                continue
        
        # Select best candidates
        selected_candidates, scores = self.candidate_selector.select(
            all_candidates, 
            max_candidates=self.config.max_candidates
        )
        
        return SolverResult(
            task_id=task.task_id,
            predictions=selected_candidates,
            scores=scores,
            solver_contributions=solver_contributions,
            metadata={
                'num_solvers_used': len(solver_contributions),
                'total_candidates': len(all_candidates),
                'selected_candidates': len(selected_candidates)
            }
        )
    
    def solve_batch(self, tasks: List[Task]) -> List[SolverResult]:
        """
        Solve multiple ARC tasks.
        
        Args:
            tasks: List of ARC tasks to solve
            
        Returns:
            List of SolverResult objects
        """
        results = []
        for task in tasks:
            result = self.solve(task)
            results.append(result)
        return results
    
    def get_solver_info(self) -> Dict[str, Any]:
        """Get information about available solvers."""
        return {
            'available_solvers': list(self.solvers.keys()),
            'config': self.config.__dict__,
            'total_solvers': len(self.solvers)
        } 