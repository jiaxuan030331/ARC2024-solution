"""
Python wrapper for C++ DAG Solver.

This module provides a Python interface that matches the BaseSolver
API while using the optimized C++ DAG implementation under the hood.
"""

import warnings
import numpy as np
from typing import List, Optional, Any, Dict, Tuple
import time

# Try to import the C++ optimized version first
try:
    import arc_dag_solver_cpp
    CPP_AVAILABLE = True
except ImportError:
    CPP_AVAILABLE = False
    warnings.warn("C++ optimized DAG Solver not available")

# Import required components
try:
    from arc_solver.data.task import Task
    from arc_solver.solvers.base import BaseSolver
except ImportError:
    # Alternative import paths
    try:
        import sys
        import os
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        sys.path.insert(0, parent_dir)
        
        from data.task import Task  
        from solvers.base import BaseSolver
    except ImportError:
        Task = None
        BaseSolver = None
        warnings.warn("Could not import required components")


class DAGSolverWrapper(BaseSolver):
    """
    Wrapper class that provides a unified interface for the C++ DAG Solver
    implementation, compatible with the arc_solver framework.
    """
    
    def __init__(self, 
                 use_cpp: bool = True,
                 max_depth: int = 20,
                 max_side: int = 100,
                 max_area: int = 1600,
                 max_pixels: int = 8000,
                 complexity_penalty: float = 0.01,
                 max_answers: int = 3,
                 enable_logging: bool = False):
        """
        Initialize the DAG solver wrapper.
        
        Args:
            use_cpp: Whether to use C++ implementation (if available)
            max_depth: Maximum search depth in DAG
            max_side: Maximum image side length
            max_area: Maximum image area
            max_pixels: Maximum total pixels to process
            complexity_penalty: Penalty factor for complex solutions
            max_answers: Maximum number of answers to return
            enable_logging: Whether to enable detailed logging
        """
        super().__init__()
        
        self.use_cpp = use_cpp and CPP_AVAILABLE
        self.max_depth = max_depth
        self.max_side = max_side
        self.max_area = max_area
        self.max_pixels = max_pixels
        self.complexity_penalty = complexity_penalty
        self.max_answers = max_answers
        self.enable_logging = enable_logging
        
        # Statistics tracking
        self.solve_count = 0
        self.success_count = 0
        self.total_solve_time = 0.0
        self.last_solve_info = {}
        
        if self.use_cpp:
            try:
                self.cpp_solver = arc_dag_solver_cpp.DAGSolver(
                    maxDepth=max_depth,
                    maxSide=max_side,
                    maxArea=max_area,
                    maxPixels=max_pixels,
                    complexityPenalty=complexity_penalty,
                    maxAnswers=max_answers
                )
                if self.enable_logging:
                    print(f"✅ DAG Solver initialized with C++ backend")
                    config = self.cpp_solver.get_config()
                    print(f"   Config: {config}")
                    functions = self.cpp_solver.get_available_functions()
                    print(f"   Available functions: {len(functions)}")
            except Exception as e:
                self.use_cpp = False
                warnings.warn(f"Failed to initialize C++ DAG Solver: {e}")
        
        if not self.use_cpp:
            warnings.warn("DAG Solver fallback not implemented - C++ version required")
    
    def can_solve(self, task: Task) -> bool:
        """
        Check if this solver can handle the given task.
        
        Args:
            task: The ARC task to check
            
        Returns:
            True if the solver can handle this task
        """
        if not self.use_cpp:
            return False
        
        try:
            # Convert task to C++ format
            cpp_task = self._convert_task_to_cpp(task)
            
            # Use C++ solver's can_solve method
            can_solve = self.cpp_solver.can_solve(cpp_task)
            
            if self.enable_logging:
                print(f"🔍 DAG Solver can_solve: {can_solve} for task {getattr(task, 'name', 'unknown')}")
            
            return can_solve
        
        except Exception as e:
            if self.enable_logging:
                print(f"❌ Error in can_solve: {e}")
            return False
    
    def solve(self, task: Task) -> List[np.ndarray]:
        """
        Solve the task and return candidate outputs.
        
        Args:
            task: The ARC task to solve
            
        Returns:
            List of candidate output grids as numpy arrays
        """
        start_time = time.time()
        self.solve_count += 1
        
        if not self.use_cpp:
            if self.enable_logging:
                print("❌ C++ DAG Solver not available")
            return []
        
        try:
            # Convert task to C++ format
            cpp_task = self._convert_task_to_cpp(task)
            
            if self.enable_logging:
                print(f"🚀 DAG Solver solving task {getattr(task, 'name', 'unknown')}")
                print(f"   Test input shape: {task.test[0]['input'].shape}")
                print(f"   Training examples: {len(task.train)}")
            
            # Solve using C++ solver
            result = self.cpp_solver.solve(cpp_task)
            
            # Convert results back to numpy arrays
            outputs = []
            for answer in result.answers:
                outputs.append(np.array(answer, dtype=np.uint8))
            
            # Update statistics
            solve_time = time.time() - start_time
            self.total_solve_time += solve_time
            
            if result.success:
                self.success_count += 1
            
            # Store solve information
            self.last_solve_info = {
                'success': result.success,
                'solve_time': result.solvingTime,
                'total_pieces': result.totalPieces,
                'total_candidates': result.totalCandidates,
                'best_score': result.bestScore,
                'verdict': result.verdict,
                'num_answers': len(outputs)
            }
            
            if self.enable_logging:
                verdict_names = ['Nothing', 'Dimensions', 'Candidate', 'Correct']
                verdict_name = verdict_names[result.verdict] if result.verdict < len(verdict_names) else 'Unknown'
                
                print(f"✅ DAG Solver completed:")
                print(f"   Success: {result.success}")
                print(f"   Verdict: {verdict_name}")
                print(f"   Solve time: {result.solvingTime:.3f}s")
                print(f"   Pieces generated: {result.totalPieces}")
                print(f"   Candidates: {result.totalCandidates}")
                print(f"   Best score: {result.bestScore:.3f}")
                print(f"   Answers returned: {len(outputs)}")
            
            return outputs
        
        except Exception as e:
            solve_time = time.time() - start_time
            self.total_solve_time += solve_time
            
            if self.enable_logging:
                print(f"❌ DAG Solver error: {e}")
            
            return []
    
    def _convert_task_to_cpp(self, task: Task) -> 'arc_dag_solver_cpp.ARCTask':
        """
        Convert a Python Task to C++ ARCTask format.
        
        Args:
            task: Python Task object
            
        Returns:
            C++ ARCTask object
        """
        cpp_task = arc_dag_solver_cpp.ARCTask()
        
        # Set task ID
        cpp_task.taskId = getattr(task, 'name', f'task_{id(task)}')
        
        # Convert training examples
        cpp_task.training = []
        for example in task.train:
            input_array = np.array(example['input'], dtype=np.uint8)
            output_array = np.array(example['output'], dtype=np.uint8)
            cpp_task.training.append((input_array, output_array))
        
        # Convert test input
        cpp_task.testInput = np.array(task.test[0]['input'], dtype=np.uint8)
        
        return cpp_task
    
    def test_transform(self, transform_name: str, input_grid: np.ndarray) -> Optional[np.ndarray]:
        """
        Test a specific transform function on an input grid.
        
        Args:
            transform_name: Name of the transform function
            input_grid: Input grid as numpy array
            
        Returns:
            Transformed grid or None if transform failed
        """
        if not self.use_cpp:
            return None
        
        try:
            input_array = np.array(input_grid, dtype=np.uint8)
            result = self.cpp_solver.test_transform(transform_name, input_array)
            return np.array(result, dtype=np.uint8)
        except Exception as e:
            if self.enable_logging:
                print(f"❌ Transform {transform_name} failed: {e}")
            return None
    
    def get_available_functions(self) -> List[str]:
        """
        Get list of available transform functions.
        
        Returns:
            List of function names
        """
        if not self.use_cpp:
            return []
        
        try:
            return self.cpp_solver.get_available_functions()
        except Exception:
            return []
    
    def get_config(self) -> Dict[str, Any]:
        """
        Get current solver configuration.
        
        Returns:
            Configuration dictionary
        """
        if self.use_cpp:
            try:
                return dict(self.cpp_solver.get_config())
            except Exception:
                pass
        
        return {
            'max_depth': self.max_depth,
            'max_side': self.max_side,
            'max_area': self.max_area,
            'max_pixels': self.max_pixels,
            'complexity_penalty': self.complexity_penalty,
            'max_answers': self.max_answers,
            'use_cpp': self.use_cpp
        }
    
    def get_statistics(self) -> Dict[str, Any]:
        """
        Get solver statistics.
        
        Returns:
            Statistics dictionary
        """
        stats = {
            'solve_count': self.solve_count,
            'success_count': self.success_count,
            'success_rate': self.success_count / max(self.solve_count, 1),
            'total_solve_time': self.total_solve_time,
            'average_solve_time': self.total_solve_time / max(self.solve_count, 1),
            'last_solve_info': self.last_solve_info.copy(),
            'cpp_available': CPP_AVAILABLE,
            'using_cpp': self.use_cpp
        }
        
        if self.use_cpp:
            try:
                cpp_stats = self.cpp_solver.get_statistics()
                stats.update(cpp_stats)
            except Exception:
                pass
        
        return stats
    
    def reset_statistics(self):
        """Reset solver statistics."""
        self.solve_count = 0
        self.success_count = 0
        self.total_solve_time = 0.0
        self.last_solve_info = {}
    
    def __str__(self) -> str:
        """String representation of the solver."""
        backend = "C++" if self.use_cpp else "Fallback"
        return f"DAGSolverWrapper(backend={backend}, depth={self.max_depth})"
    
    def __repr__(self) -> str:
        """Detailed string representation."""
        return (f"DAGSolverWrapper(use_cpp={self.use_cpp}, max_depth={self.max_depth}, "
                f"max_side={self.max_side}, solved={self.solve_count})")


# Factory functions for common configurations
def create_fast_dag_solver(**kwargs) -> DAGSolverWrapper:
    """Create a DAG solver optimized for speed."""
    config = {
        'max_depth': 10,
        'max_side': 50,
        'max_area': 1000,
        'complexity_penalty': 0.02,
        **kwargs
    }
    return DAGSolverWrapper(**config)


def create_accurate_dag_solver(**kwargs) -> DAGSolverWrapper:
    """Create a DAG solver optimized for accuracy."""
    config = {
        'max_depth': 25,
        'max_side': 150,
        'max_area': 2500,
        'complexity_penalty': 0.005,
        **kwargs
    }
    return DAGSolverWrapper(**config)


def create_balanced_dag_solver(**kwargs) -> DAGSolverWrapper:
    """Create a balanced DAG solver."""
    config = {
        'max_depth': 20,
        'max_side': 100,
        'max_area': 1600,
        'complexity_penalty': 0.01,
        **kwargs
    }
    return DAGSolverWrapper(**config) 