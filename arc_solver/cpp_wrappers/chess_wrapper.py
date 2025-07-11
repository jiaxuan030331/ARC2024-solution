"""
Python wrapper for C++ ChessSolver implementation.
"""

import numpy as np
from typing import List, Optional
from ..data.task import Task

try:
    import arc_solver.arc_solver_cpp as arc_solver_cpp
    ChessSolverCpp = arc_solver_cpp.ChessSolverCpp
    CPP_AVAILABLE = True
except (ImportError, AttributeError):
    CPP_AVAILABLE = False
    ChessSolverCpp = None


class ChessSolverWrapper:
    """
    Python wrapper for C++ ChessSolver with fallback to Python implementation.
    """
    
    def __init__(self, use_cpp: bool = True):
        """
        Initialize the Chess Solver wrapper.
        
        Args:
            use_cpp: Whether to use C++ implementation (if available)
        """
        self.use_cpp = use_cpp and CPP_AVAILABLE
        
        if self.use_cpp and ChessSolverCpp is not None:
            self.cpp_solver = ChessSolverCpp()
            print("ChessSolver: Using C++ implementation")
        else:
            # Import Python fallback
            from ..solvers.chess import ChessSolver
            self.python_solver = ChessSolver()
            if not CPP_AVAILABLE:
                print("ChessSolver: C++ not available, using Python fallback")
            else:
                print("ChessSolver: Using Python implementation by choice")
    
    def can_solve(self, task: Task) -> bool:
        """
        Check if task involves chess patterns.
        
        Args:
            task: The ARC task to analyze
            
        Returns:
            True if the task appears to involve chess patterns
        """
        if self.use_cpp:
            return self._can_solve_cpp(task)
        else:
            return self.python_solver.can_solve(task)
    
    def solve(self, task: Task) -> List[np.ndarray]:
        """
        Generate chess pattern predictions for test inputs.
        
        Args:
            task: The ARC task to solve
            
        Returns:
            List of candidate output arrays
        """
        if self.use_cpp:
            return self._solve_cpp(task)
        else:
            return self.python_solver.solve(task)
    
    def _can_solve_cpp(self, task: Task) -> bool:
        """C++ implementation of can_solve."""
        try:
            train_inputs = [ex.input for ex in task.train]
            train_outputs = [ex.output for ex in task.train]
            
            return self.cpp_solver.can_solve(train_inputs, train_outputs)
        except Exception as e:
            print(f"C++ ChessSolver can_solve failed: {e}")
            # Fallback to Python
            from ..solvers.chess import ChessSolver
            python_solver = ChessSolver()
            return python_solver.can_solve(task)
    
    def _solve_cpp(self, task: Task) -> List[np.ndarray]:
        """C++ implementation of solve."""
        try:
            train_inputs = [ex.input for ex in task.train]
            train_outputs = [ex.output for ex in task.train]
            test_inputs = [test_input for test_input in task.test]
            
            return self.cpp_solver.solve(train_inputs, train_outputs, test_inputs)
        except Exception as e:
            print(f"C++ ChessSolver solve failed: {e}")
            # Fallback to Python
            from ..solvers.chess import ChessSolver
            python_solver = ChessSolver()
            return python_solver.solve(task)
    
    @property
    def is_using_cpp(self) -> bool:
        """Return True if using C++ implementation."""
        return self.use_cpp
    
    @property
    def implementation_info(self) -> str:
        """Return information about current implementation."""
        if self.use_cpp:
            return "C++ (Optimized)"
        else:
            return "Python (Fallback)"


# Convenience function for backward compatibility
def create_chess_solver(use_cpp: bool = True) -> ChessSolverWrapper:
    """
    Create a ChessSolver instance.
    
    Args:
        use_cpp: Whether to use C++ implementation
        
    Returns:
        ChessSolverWrapper instance
    """
    return ChessSolverWrapper(use_cpp=use_cpp) 