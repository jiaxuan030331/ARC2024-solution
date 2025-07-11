"""
Data loading and processing utilities for ARC tasks, aligned with main notebook.

This module provides functions for loading, processing, and manipulating
ARC task data, including defensive copying and task creation utilities.
"""

import numpy as np
import json
import os
from typing import List, Dict, Any, Tuple, Optional, Union
from pathlib import Path

from ..data.task import Task, TaskExample


def defensive_copy(A: Union[List[List[int]], np.ndarray]) -> List[List[int]]:
    """
    Create a defensive copy of a 2D array.
    
    Args:
        A: Input 2D array or list
        
    Returns:
        Defensive copy as list of lists
    """
    A = np.array(A)
    n, k = A.shape
    L = np.zeros((n, k), dtype=int)
    for i in range(n):
        for j in range(k):
            L[i, j] = 0 + A[i, j]
    return L.tolist()


def create_task(task: Dict[str, Any], task_id: int = 0) -> Tuple[List[List[List[int]]], List[List[List[int]]]]:
    """
    Create task format for processing (aligned with notebook Create function).
    
    Args:
        task: Task dictionary
        task_id: Test example index
        
    Returns:
        Tuple of (Input, Output) lists
    """
    n = len(task['train'])
    Input = [defensive_copy(task['train'][i]['input']) for i in range(n)]
    Output = [defensive_copy(task['train'][i]['output']) for i in range(n)]
    Input.append(defensive_copy(task['test'][task_id]['input']))
    return Input, Output


def flattener(pred: Union[List[List[int]], np.ndarray]) -> str:
    """
    Flatten prediction to string format.
    
    Args:
        pred: Prediction array
        
    Returns:
        Flattened string representation
    """
    str_pred = str([row for row in pred])
    str_pred = str_pred.replace(', ', '')
    str_pred = str_pred.replace('[[', '|')
    str_pred = str_pred.replace('][', '|')
    str_pred = str_pred.replace(']]', '|')
    return str_pred


class DataProcessor:
    """Data processing utilities for ARC tasks."""
    
    @staticmethod
    def load_json_file(file_path: Union[str, Path]) -> Dict[str, Any]:
        """
        Load JSON file.
        
        Args:
            file_path: Path to JSON file
            
        Returns:
            Loaded JSON data
        """
        with open(file_path, 'r') as f:
            return json.load(f)
    
    @staticmethod
    def save_json_file(data: Dict[str, Any], file_path: Union[str, Path]) -> None:
        """
        Save data to JSON file.
        
        Args:
            data: Data to save
            file_path: Output file path
        """
        with open(file_path, 'w') as f:
            json.dump(data, f, indent=4)
    
    @staticmethod
    def load_tasks_from_directory(directory: Union[str, Path]) -> Dict[str, Dict[str, Any]]:
        """
        Load all JSON tasks from a directory.
        
        Args:
            directory: Directory containing task JSON files
            
        Returns:
            Dictionary mapping task IDs to task data
        """
        directory = Path(directory)
        tasks = {}
        
        for json_file in directory.glob("*.json"):
            task_id = json_file.stem
            task_data = DataProcessor.load_json_file(json_file)
            tasks[task_id] = task_data
        
        return tasks
    
    @staticmethod
    def convert_arc_prize_format(input_file: Union[str, Path], 
                                output_dir: Union[str, Path]) -> None:
        """
        Convert ARC Prize 2024 format to individual JSON files.
        
        Args:
            input_file: Path to ARC Prize JSON file
            output_dir: Output directory for individual files
        """
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Load the JSON content
        data = DataProcessor.load_json_file(input_file)
        
        # Split into individual files
        for task_id, task_data in data.items():
            output_file_path = output_dir / f'{task_id}.json'
            DataProcessor.save_json_file(task_data, output_file_path)
    
    @staticmethod
    def create_sample_submission(tasks: Dict[str, Dict[str, Any]]) -> Dict[str, List[Dict[str, Any]]]:
        """
        Create a sample submission format.
        
        Args:
            tasks: Dictionary of tasks
            
        Returns:
            Sample submission format
        """
        submission = {}
        
        for task_id, task_data in tasks.items():
            num_test = len(task_data.get('test', []))
            submission[task_id] = []
            
            for i in range(num_test):
                submission[task_id].append({
                    "attempt_1": [[0, 0], [0, 0]],
                    "attempt_2": [[0, 0], [0, 0]]
                })
        
        return submission
    
    @staticmethod
    def translate_submission_format(old_csv_path: Union[str, Path], 
                                  new_json_path: Union[str, Path]) -> None:
        """
        Translate from old CSV submission format to new JSON format.
        
        Args:
            old_csv_path: Path to old CSV submission file
            new_json_path: Path to new JSON submission file
        """
        submission_dict = {}
        
        with open(old_csv_path, 'r') as file:
            lines = file.readlines()
        
        for line in lines[1:]:  # Skip header
            output_id, output = line.strip().split(',')
            task_id, output_idx = output_id.split('_')
            predictions = output.split(' ')
            
            # Take only first two predictions
            if len(predictions) > 2:
                predictions = [predictions[0], predictions[2]]
            
            processed_predictions = []
            for pred in predictions:
                if pred:
                    pred_lines = pred.split('|')[1:-1]
                    pred_matrix = [list(map(int, line)) for line in pred_lines]
                    processed_predictions.append(pred_matrix)
            
            attempt_1 = processed_predictions[0] if len(processed_predictions) > 0 else []
            attempt_2 = processed_predictions[1] if len(processed_predictions) > 1 else []
            
            if task_id not in submission_dict:
                submission_dict[task_id] = []
            
            attempt_dict = {
                "attempt_1": attempt_1,
                "attempt_2": attempt_2
            }
            
            if output_idx == '0':
                submission_dict[task_id].insert(0, attempt_dict)
            else:
                submission_dict[task_id].append(attempt_dict)
        
        DataProcessor.save_json_file(submission_dict, new_json_path)


class TaskValidator:
    """Utilities for validating ARC tasks."""
    
    @staticmethod
    def validate_task_format(task: Dict[str, Any]) -> bool:
        """
        Validate task format.
        
        Args:
            task: Task dictionary
            
        Returns:
            True if valid, False otherwise
        """
        required_keys = ['train', 'test']
        
        if not all(key in task for key in required_keys):
            return False
        
        if not isinstance(task['train'], list) or not isinstance(task['test'], list):
            return False
        
        # Validate training examples
        for train_ex in task['train']:
            if not isinstance(train_ex, dict):
                return False
            if 'input' not in train_ex or 'output' not in train_ex:
                return False
            if not isinstance(train_ex['input'], list) or not isinstance(train_ex['output'], list):
                return False
        
        # Validate test examples
        for test_ex in task['test']:
            if not isinstance(test_ex, dict):
                return False
            if 'input' not in test_ex:
                return False
            if not isinstance(test_ex['input'], list):
                return False
        
        return True
    
    @staticmethod
    def get_task_statistics(tasks: Dict[str, Dict[str, Any]]) -> Dict[str, Any]:
        """
        Get statistics about a collection of tasks.
        
        Args:
            tasks: Dictionary of tasks
            
        Returns:
            Statistics dictionary
        """
        stats = {
            'total_tasks': len(tasks),
            'total_train_examples': 0,
            'total_test_examples': 0,
            'input_shapes': [],
            'output_shapes': [],
            'color_counts': []
        }
        
        for task_id, task_data in tasks.items():
            stats['total_train_examples'] += len(task_data['train'])
            stats['total_test_examples'] += len(task_data['test'])
            
            for train_ex in task_data['train']:
                input_shape = (len(train_ex['input']), len(train_ex['input'][0]))
                output_shape = (len(train_ex['output']), len(train_ex['output'][0]))
                stats['input_shapes'].append(input_shape)
                stats['output_shapes'].append(output_shape)
                
                # Count unique colors
                input_colors = len(set(color for row in train_ex['input'] for color in row))
                output_colors = len(set(color for row in train_ex['output'] for color in row))
                stats['color_counts'].append((input_colors, output_colors))
        
        return stats


# Convenience functions (aligned with notebook)
def Defensive_Copy(A: Union[List[List[int]], np.ndarray]) -> List[List[int]]:
    """Defensive copy (convenience function)."""
    return defensive_copy(A)


def Create(task: Dict[str, Any], task_id: int = 0) -> Tuple[List[List[List[int]]], List[List[List[int]]]]:
    """Create task format (convenience function)."""
    return create_task(task, task_id) 