"""
Python wrapper for C++ SymmetrySolver.

This module provides a Python interface that matches the original SymmetrySolver
API while using the optimized C++ implementation under the hood.
"""

import warnings
import numpy as np
from typing import List, Optional, Any

# Try to import the C++ optimized version first
try:
    import arc_solver_cpp
    CPP_AVAILABLE = True
except ImportError:
    CPP_AVAILABLE = False
    warnings.warn("C++ optimized SymmetrySolver not available, falling back to Python implementation")

# Import Python fallback - use absolute imports to avoid relative import issues
try:
    from arc_solver.data.task import Task
    from arc_solver.solvers.symmetry import SymmetrySolver as PythonSymmetrySolver
except ImportError:
    # Alternative import paths
    try:
        import sys
        import os
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        sys.path.insert(0, parent_dir)
        
        from data.task import Task  
        from solvers.symmetry import SymmetrySolver as PythonSymmetrySolver
    except ImportError:
        Task = None
        PythonSymmetrySolver = None
        warnings.warn("Could not import Python SymmetrySolver fallback")

class SymmetrySolverWrapper:
    """
    Wrapper class that provides a unified interface for both C++ optimized 
    and Python fallback implementations of SymmetrySolver.
    """
    
    def __init__(self, use_cpp: bool = True):
        """
        Initialize the solver wrapper.
        
        Args:
            use_cpp: Whether to prefer C++ implementation when available
        """
        self.use_cpp = use_cpp and CPP_AVAILABLE
        
        if self.use_cpp:
            self.cpp_solver = arc_solver_cpp.SymmetrySolverCpp()
            print("Using C++ optimized SymmetrySolver")
        else:
            if PythonSymmetrySolver is not None:
                self.python_solver = PythonSymmetrySolver()
                print("Using Python SymmetrySolver")
            else:
                raise ImportError("Neither C++ nor Python SymmetrySolver available")
    
    def can_solve(self, train_inputs: List[np.ndarray], train_outputs: List[np.ndarray]) -> bool:
        """Check if the task can be solved by symmetry patterns"""
        if self.use_cpp:
            return self.cpp_solver.can_solve(train_inputs, train_outputs)
        else:
            # For Python version, we need to create Task objects
            if Task is None:
                return False
            
            # Convert to the format expected by Python solver
            train_pairs = []
            for inp, out in zip(train_inputs, train_outputs):
                train_pairs.append({'input': inp, 'output': out})
            
            task = Task({'train': train_pairs, 'test': []})
            return self.python_solver.can_solve(task)
    
    def solve(self, train_inputs: List[np.ndarray], train_outputs: List[np.ndarray], 
              test_inputs: List[np.ndarray]) -> List[np.ndarray]:
        """Solve the task and return predictions"""
        if self.use_cpp:
            return self.cpp_solver.solve(train_inputs, train_outputs, test_inputs)
        else:
            if Task is None:
                return []
                
            # Convert to the format expected by Python solver
            train_pairs = []
            for inp, out in zip(train_inputs, train_outputs):
                train_pairs.append({'input': inp, 'output': out})
            
            test_pairs = []
            for inp in test_inputs:
                test_pairs.append({'input': inp})
                
            task = Task({'train': train_pairs, 'test': test_pairs})
            result = self.python_solver.solve(task)
            
            # Extract the predictions from the result
            if result and len(result) > 0:
                return [pred for pred in result if pred is not None]
            return []
    
    def get_implementation_info(self) -> dict:
        """Get information about which implementation is being used"""
        return {
            'using_cpp': self.use_cpp,
            'cpp_available': CPP_AVAILABLE,
            'python_available': PythonSymmetrySolver is not None
        } 