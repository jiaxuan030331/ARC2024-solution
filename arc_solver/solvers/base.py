"""
Base solver class for ARC solvers.
"""

from abc import ABC, abstractmethod
from typing import List
import numpy as np

from ..data.task import Task


class BaseSolver(ABC):
    """Base class for all ARC solvers."""
    
    @abstractmethod
    def can_solve(self, task: Task) -> bool:
        """Check if this solver can handle the given task."""
        pass
    
    @abstractmethod
    def solve(self, task: Task) -> List[np.ndarray]:
        """Solve the task and return candidate outputs."""
        pass 