"""
Data structures for ARC tasks.
"""

import numpy as np
from typing import List, Dict, Any
from dataclasses import dataclass


@dataclass
class TaskExample:
    """A single input-output example in an ARC task."""
    input: np.ndarray
    output: np.ndarray
    
    def __post_init__(self):
        """Convert to numpy arrays if needed."""
        if not isinstance(self.input, np.ndarray):
            self.input = np.array(self.input)
        if not isinstance(self.output, np.ndarray):
            self.output = np.array(self.output)


@dataclass
class Task:
    """An ARC task containing training examples and test inputs."""
    task_id: str
    train: List[TaskExample]
    test: List[np.ndarray]
    
    def __post_init__(self):
        """Convert test inputs to numpy arrays."""
        self.test = [np.array(t) if not isinstance(t, np.ndarray) else t for t in self.test]
    
    @property
    def num_train_examples(self) -> int:
        """Number of training examples."""
        return len(self.train)
    
    @property
    def num_test_examples(self) -> int:
        """Number of test examples."""
        return len(self.test)
    
    def get_train_inputs(self) -> List[np.ndarray]:
        """Get all training inputs."""
        return [ex.input for ex in self.train]
    
    def get_train_outputs(self) -> List[np.ndarray]:
        """Get all training outputs."""
        return [ex.output for ex in self.train]


class TaskLoader:
    """Utility class for loading ARC tasks from various formats."""
    
    @staticmethod
    def from_json(data: Dict[str, Any]) -> Task:
        """Load task from JSON format."""
        task_id = data.get('task_id', 'unknown')
        
        # Load training examples
        train = []
        for train_ex in data.get('train', []):
            train.append(TaskExample(
                input=train_ex['input'],
                output=train_ex['output']
            ))
        
        # Load test inputs
        test = data.get('test', [])
        
        return Task(task_id=task_id, train=train, test=test)
    
    @staticmethod
    def from_file(file_path: str) -> Task:
        """Load task from JSON file."""
        import json
        with open(file_path, 'r') as f:
            data = json.load(f)
        return TaskLoader.from_json(data)
    
    @staticmethod
    def from_dict(task_dict: Dict[str, Any]) -> Task:
        """Load task from dictionary format."""
        return TaskLoader.from_json(task_dict) 