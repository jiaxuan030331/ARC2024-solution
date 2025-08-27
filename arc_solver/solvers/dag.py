"""
DAG Solver for ARC tasks.

This solver uses a Directed Acyclic Graph (DAG) approach to explore
the space of image transformations, similar to the icecuber solution.
"""

import warnings
import numpy as np
from typing import List, Optional, Any, Dict
import time

from .base import BaseSolver
from ..data.task import Task

# Try to import the C++ optimized version from arc_solver
try:
    import sys
    import os
    # Add parent directory to path to import arc_solver_cpp
    sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    import arc_solver_cpp
    CPP_AVAILABLE = hasattr(arc_solver_cpp, 'DAGSolverCpp')
except ImportError:
    CPP_AVAILABLE = False
    warnings.warn("C++ optimized DAG Solver not available")


class DAGSolver(BaseSolver):
    """
    DAG-based solver for ARC tasks using C++ backend.
    
    This solver implements a DAG (Directed Acyclic Graph) approach to
    systematically explore the space of image transformations.
    """
    
    def __init__(self, 
                 max_depth: int = 20,
                 max_side: int = 100,
                 max_area: int = 1600,
                 max_pixels: int = 8000,
                 complexity_penalty: float = 0.01,
                 max_answers: int = 3,
                 enable_logging: bool = False):
        """
        Initialize the DAG solver.
        
        Args:
            max_depth: Maximum search depth in DAG
            max_side: Maximum image side length
            max_area: Maximum image area
            max_pixels: Maximum total pixels to process
            complexity_penalty: Penalty factor for complex solutions
            max_answers: Maximum number of answers to return
            enable_logging: Whether to enable detailed logging
        """
        super().__init__()
        
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
        
        # Initialize C++ solver if available
        self.cpp_solver = None
        self.use_cpp = CPP_AVAILABLE
        
        if self.use_cpp:
            try:
                self.cpp_solver = arc_solver_cpp.DAGSolverCpp()
                if self.enable_logging:
                    print(f"✅ DAG Solver initialized with C++ backend")
                    functions = self.cpp_solver.get_available_functions()
                    print(f"   Available functions: {len(functions)}")
            except Exception as e:
                self.use_cpp = False
                self.cpp_solver = None
                warnings.warn(f"Failed to initialize C++ DAG Solver: {e}")
        
        if not self.use_cpp:
            warnings.warn("DAG Solver requires C++ backend - falling back to basic mode")
    
    def can_solve(self, task: Task) -> bool:
        """
        Check if this solver can handle the given task.
        
        Args:
            task: The ARC task to check
            
        Returns:
            True if the solver can handle this task
        """
        if not self.use_cpp or self.cpp_solver is None:
            # Basic checks without C++ backend
            return self._basic_can_solve_check(task)
        
        try:
            # Convert task to C++ format
            train_inputs, train_outputs = self._convert_task_to_cpp_format(task)
            
            # Use C++ solver's can_solve method
            can_solve = self.cpp_solver.can_solve(train_inputs, train_outputs)
            
            if self.enable_logging:
                print(f"🔍 DAG Solver can_solve: {can_solve} for task")
            
            return can_solve
        
        except Exception as e:
            if self.enable_logging:
                print(f"❌ Error in can_solve: {e}")
            return self._basic_can_solve_check(task)
    
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
        
        if not self.use_cpp or self.cpp_solver is None:
            if self.enable_logging:
                print("❌ C++ DAG Solver not available")
            return []
        
        try:
            # Convert task to C++ format
            train_inputs, train_outputs, test_inputs = self._convert_task_for_solve(task)
            
            if self.enable_logging:
                print(f"🚀 DAG Solver solving task")
                print(f"   Test input shape: {np.array(test_inputs[0]).shape}")
                print(f"   Training examples: {len(train_inputs)}")
            
            # Solve using C++ solver
            results = self.cpp_solver.solve(train_inputs, train_outputs, test_inputs)
            
            # Convert results back to numpy arrays
            outputs = []
            for result in results:
                outputs.append(np.array(result, dtype=np.uint8))
            
            # Update statistics
            solve_time = time.time() - start_time
            self.total_solve_time += solve_time
            
            if outputs:
                self.success_count += 1
            
            # Store solve information
            self.last_solve_info = {
                'success': len(outputs) > 0,
                'solve_time': solve_time,
                'num_answers': len(outputs)
            }
            
            if self.enable_logging:
                print(f"✅ DAG Solver completed:")
                print(f"   Success: {len(outputs) > 0}")
                print(f"   Solve time: {solve_time:.3f}s")
                print(f"   Answers returned: {len(outputs)}")
            
            return outputs
        
        except Exception as e:
            solve_time = time.time() - start_time
            self.total_solve_time += solve_time
            
            if self.enable_logging:
                print(f"❌ DAG Solver error: {e}")
            
            return []
    
    def _basic_can_solve_check(self, task: Task) -> bool:
        """
        Basic check without C++ backend.
        
        Args:
            task: The ARC task to check
            
        Returns:
            True if basic checks pass
        """
        try:
            # Check test input size
            test_input = task.test[0]['input']
            if (test_input.shape[0] > self.max_side or 
                test_input.shape[1] > self.max_side):
                return False
            
            if test_input.shape[0] * test_input.shape[1] > self.max_area:
                return False
            
            # Check training examples
            for example in task.train:
                input_shape = example['input'].shape
                output_shape = example['output'].shape
                
                if (input_shape[0] > self.max_side or input_shape[1] > self.max_side or
                    output_shape[0] > self.max_side or output_shape[1] > self.max_side):
                    return False
            
            return True
        
        except Exception:
            return False
    
    def _convert_task_to_cpp_format(self, task: Task):
        """Convert task for can_solve check."""
        train_inputs = []
        train_outputs = []
        
        for example in task.train:
            train_inputs.append(example['input'].tolist())
            train_outputs.append(example['output'].tolist())
        
        return train_inputs, train_outputs
    
    def _convert_task_for_solve(self, task: Task):
        """Convert task for solve method."""
        train_inputs = []
        train_outputs = []
        
        for example in task.train:
            train_inputs.append(example['input'].tolist())
            train_outputs.append(example['output'].tolist())
        
        test_inputs = [task.test[0]['input'].tolist()]
        
        return train_inputs, train_outputs, test_inputs
    
    def get_available_functions(self) -> List[str]:
        """
        Get list of available transform functions.
        
        Returns:
            List of function names
        """
        if not self.use_cpp or self.cpp_solver is None:
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
        
        return stats
    
    def reset_statistics(self):
        """Reset solver statistics."""
        self.solve_count = 0
        self.success_count = 0
        self.total_solve_time = 0.0
        self.last_solve_info = {}
    
    def __str__(self) -> str:
        """String representation of the solver."""
        backend = "C++" if self.use_cpp else "Python"
        return f"DAGSolver(backend={backend}, depth={self.max_depth})"
    
    def __repr__(self) -> str:
        """Detailed string representation."""
        return (f"DAGSolver(use_cpp={self.use_cpp}, max_depth={self.max_depth}, "
                f"max_side={self.max_side}, solved={self.solve_count})")


# Factory functions for common configurations
def create_fast_dag_solver(**kwargs) -> DAGSolver:
    """Create a DAG solver optimized for speed."""
    config = {
        'max_depth': 10,
        'max_side': 50,
        'max_area': 1000,
        'complexity_penalty': 0.02,
        **kwargs
    }
    return DAGSolver(**config)


def create_accurate_dag_solver(**kwargs) -> DAGSolver:
    """Create a DAG solver optimized for accuracy."""
    config = {
        'max_depth': 25,
        'max_side': 150,
        'max_area': 2500,
        'complexity_penalty': 0.005,
        **kwargs
    }
    return DAGSolver(**config)


def create_balanced_dag_solver(**kwargs) -> DAGSolver:
    """Create a balanced DAG solver."""
    config = {
        'max_depth': 20,
        'max_side': 100,
        'max_area': 1600,
        'complexity_penalty': 0.01,
        **kwargs
    }
    return DAGSolver(**config) 