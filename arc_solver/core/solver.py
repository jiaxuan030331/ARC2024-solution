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
    ColorCounterSolver,
    DAGSolver
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
    dag_predictions: Optional[List[np.ndarray]] = None  # DAG solver结果
    specialist_predictions: Optional[List[np.ndarray]] = None  # 专用solver结果
    used_fallback: bool = False  # 是否使用了fallback


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
        # 专用solvers (非DAG)
        self.specialist_solvers = {}
        
        if self.config.enable_repeating_solver:
            self.specialist_solvers['repeating'] = RepeatingSolver()
            
        if self.config.enable_grid_solver:
            self.specialist_solvers['grid'] = GridSolver()
            
        if self.config.enable_chess_solver:
            self.specialist_solvers['chess'] = ChessSolver()
            
        if self.config.enable_tiling_solver:
            self.specialist_solvers['tiling'] = TilingSolver()
            
        if self.config.enable_submatrix_solver:
            self.specialist_solvers['submatrix'] = SubmatrixSolver()
            
        if self.config.enable_submask_solver:
            self.specialist_solvers['submask'] = SubmaskSolver()
            
        if self.config.enable_ml_solver:
            self.specialist_solvers['ml'] = MLSolver()
            
        if self.config.enable_symmetry_solver:
            self.specialist_solvers['symmetry'] = SymmetrySolver()
            
        if self.config.enable_color_counter_solver:
            self.specialist_solvers['color_counter'] = ColorCounterSolver()
        
        # DAG solver (fallback)
        self.dag_solver = None
        if getattr(self.config, 'enable_dag_solver', True):
            try:
                self.dag_solver = DAGSolver(
                    max_depth=getattr(self.config, 'dag_max_depth', 20),
                    enable_logging=getattr(self.config, 'dag_enable_logging', False)
                )
            except Exception as e:
                print(f"Warning: DAG Solver initialization failed: {e}")
        
        # 保持兼容性：所有solvers的组合视图
        self.solvers = self.specialist_solvers.copy()
        if self.dag_solver:
            self.solvers['dag'] = self.dag_solver
    
    def solve(self, task: Task) -> SolverResult:
        """
        Solve an ARC task using specialist solvers first, then DAG solver as fallback.
        
        Args:
            task: The ARC task to solve
            
        Returns:
            SolverResult containing predictions and metadata
        """
        # Step 1: 运行专用solvers
        specialist_candidates = []
        specialist_contributions = {}
        
        for solver_name, solver in self.specialist_solvers.items():
            try:
                candidates = solver.solve(task)
                if candidates:
                    specialist_candidates.extend(candidates)
                    specialist_contributions[solver_name] = candidates
                    
            except Exception as e:
                print(f"Warning: Specialist solver {solver_name} failed: {e}")
                continue
        
        # Step 2: 评估专用solvers的结果可信度
        specialist_selected, specialist_scores = self.candidate_selector.select(
            specialist_candidates, 
            max_candidates=self.config.max_candidates
        )
        
        # 定义高可信度阈值 (可配置)
        high_confidence_threshold = getattr(self.config, 'dag_high_confidence_threshold', 15.0)
        has_high_confidence = any(score >= high_confidence_threshold for score in specialist_scores)
        
        # Step 3: 运行DAG solver (如果需要)
        dag_candidates = []
        dag_contributions = {}
        used_fallback = False
        
        if not has_high_confidence and self.dag_solver:
            try:
                if self.dag_solver.can_solve(task):
                    dag_results = self.dag_solver.solve(task)
                    if dag_results:
                        dag_candidates.extend(dag_results)
                        dag_contributions['dag'] = dag_results
                        used_fallback = True
                        
            except Exception as e:
                print(f"Warning: DAG solver failed: {e}")
        
        # Step 4: 处理最终结果
        dag_selected, dag_scores = self.candidate_selector.select(
            dag_candidates, 
            max_candidates=self.config.max_candidates
        ) if dag_candidates else ([], [])
        
        # Step 5: 确定最终输出
        if has_high_confidence:
            # 高可信度：使用专用solver结果
            final_predictions = specialist_selected
            final_scores = specialist_scores
            all_contributions = specialist_contributions
            primary_source = "specialist_solvers"
        else:
            # 低可信度：综合两者，优先DAG结果
            all_candidates = dag_candidates + specialist_candidates
            all_contributions = {**specialist_contributions, **dag_contributions}
            
            if dag_selected:
                # DAG solver有结果，优先使用
                final_predictions = dag_selected + specialist_selected[:max(0, self.config.max_candidates - len(dag_selected))]
                final_scores = dag_scores + specialist_scores[:max(0, self.config.max_candidates - len(dag_scores))]
                primary_source = "dag_solver"
            else:
                # DAG solver无结果，使用专用solver
                final_predictions = specialist_selected
                final_scores = specialist_scores
                primary_source = "specialist_solvers"
        
        # 限制最终结果数量
        final_predictions = final_predictions[:self.config.max_candidates]
        final_scores = final_scores[:self.config.max_candidates]
        
        return SolverResult(
            task_id=task.task_id,
            predictions=final_predictions,
            scores=final_scores,
            solver_contributions=all_contributions,
            dag_predictions=dag_selected,
            specialist_predictions=specialist_selected,
            used_fallback=used_fallback,
            metadata={
                'num_specialist_solvers_used': len(specialist_contributions),
                'num_dag_candidates': len(dag_candidates),
                'num_specialist_candidates': len(specialist_candidates),
                'has_high_confidence': has_high_confidence,
                'used_fallback': used_fallback,
                'primary_source': primary_source,
                'specialist_max_score': max(specialist_scores) if specialist_scores else 0.0,
                'dag_max_score': max(dag_scores) if dag_scores else 0.0,
                'high_confidence_threshold': high_confidence_threshold,
                'total_candidates': len(dag_candidates) + len(specialist_candidates),
                'selected_candidates': len(final_predictions)
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