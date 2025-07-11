"""
Main flow control for ARC solver, aligned with main notebook logic.

This module implements the main inference loop and candidate management
functions from the main notebook, adapted for the arc_solver framework.
"""

import numpy as np
import pandas as pd
from typing import List, Dict, Any, Optional, Tuple
from collections import defaultdict

from ..data.task import Task, TaskExample
from ..core.patterns import (
    check_repeating, predict_repeating, predict_repeating_mask,
    check_chess, predict_chess, check_tiles_shape, predict_tiles_shape,
    check_grid, check_sub_grid_2x, check_grid_transforms,
    predict_grid_transforms, predict_transforms_grid_2x,
    check_subitem, check_sub_mask
)
from ..utils.core_functions import grid_filter
from ..utils.ml_functions import format_features, make_features, tree1
from ..solvers.symmetry import SymmetrySolver
from ..solvers.color_counter import ColorCounterSolver


class MainFlowController:
    """
    Main flow controller that orchestrates the solving process.
    
    This class implements the main inference loop from the notebook,
    managing candidate generation, selection, and final output formatting.
    """
    
    def __init__(self):
        """Initialize the main flow controller."""
        self.symmetry_solver = SymmetrySolver()
        self.color_counter_solver = ColorCounterSolver()
        
    def ganswer_answer(self, ganswer: List[np.ndarray]) -> List[List[List[int]]]:
        """
        Convert numpy array predictions to list format and remove duplicates.
        
        Args:
            ganswer: List of numpy array predictions
            
        Returns:
            List of unique predictions in list format
        """
        answer = []
        for j in range(len(ganswer)):
            ganswer_j = ganswer[j].tolist()
            
            if ganswer_j not in answer:
                answer.append(ganswer_j)
                
        return answer
    
    def ganswer_answer_1(self, ganswer: List) -> List:
        """
        Convert predictions to list format (for non-numpy inputs).
        
        Args:
            ganswer: List of predictions
            
        Returns:
            List of unique predictions
        """
        answer = []
        for j in range(len(ganswer)):
            ganswer_j = ganswer[j]
            
            if ganswer_j not in answer:
                answer.append(ganswer_j)
                
        return answer
    
    def prn_plus(self, prn: List, answer: List) -> List:
        """
        Add predictions to the candidate list with duplication for first prediction.
        
        Args:
            prn: Current candidate list
            answer: New predictions to add
            
        Returns:
            Updated candidate list
        """
        for j in range(len(answer)):
            # 判断answer[j]是否已在prn中
            found = False
            for el in prn:
                if isinstance(el, np.ndarray) and isinstance(answer[j], np.ndarray):
                    if np.array_equal(el, answer[j]):
                        found = True
                        break
                else:
                    if el == answer[j]:
                        found = True
                        break
            if not found:
                prn = prn + [answer[j]]
            if j == 0:
                prn = prn + [answer[j]]
        return prn
    
    def prn_select_2(self, prn: List) -> List:
        """
        Select top 2 candidates based on frequency.
        
        Args:
            prn: List of all candidates
            
        Returns:
            Top 2 most frequent candidates
        """
        if len(prn) > 2:
            value_list = []
            string_list = []
            for el in prn:
                value = 0
                for i in range(len(prn)):
                    if isinstance(el, np.ndarray) and isinstance(prn[i], np.ndarray):
                        if np.array_equal(el, prn[i]):
                            value += 1
                    else:
                        if el == prn[i]:
                            value += 1
                string_list.append(str(el))
                value_list.append(value)
            prn_df = pd.DataFrame({'prn': prn, 'value': value_list, 'string': string_list})
            prn_df1 = prn_df.drop_duplicates(subset=['string'])
            prn_df2 = prn_df1.sort_values(by='value', ascending=False)
            prn = prn_df2['prn'].values.tolist()[:2]
        return prn
    
    def run_single_task(self, task: Task, test_input: np.ndarray) -> Tuple[List, List]:
        """
        Run all solvers on a single task and test input.
        
        Args:
            task: The ARC task to solve
            test_input: The test input to predict for
            
        Returns:
            Tuple of (attempt_1, attempt_2) predictions
        """
        prn = []
        
        # Convert Task to dict format for compatibility with pattern functions
        task_dict = {
            'train': [{'input': ex.input.tolist(), 'output': ex.output.tolist()} for ex in task.train]
        }
        
        # ............................................................................... 1 - Different Solvers
        # Repeating patterns
        if check_repeating(task_dict, True):
            ganswer = predict_repeating(test_input)
            if ganswer:
                answer = self.ganswer_answer(ganswer)
                prn = self.prn_plus(prn, answer)
        
        # Grid 2x transforms
        if check_grid(task_dict) and check_sub_grid_2x(task_dict):
            ganswer = predict_transforms_grid_2x(task_dict, test_input)
            if ganswer:
                answer = self.ganswer_answer(ganswer)
                prn = self.prn_plus(prn, answer)
        
        # Chess patterns
        if check_grid(task_dict) and check_chess(task_dict, False, True):
            ganswer = predict_chess(grid_filter(test_input))
            if ganswer:
                answer = self.ganswer_answer(ganswer)
                prn = self.prn_plus(prn, answer)
        
        # Tiling patterns
        if check_tiles_shape(task_dict, True):
            ganswer = predict_tiles_shape(task_dict, test_input)
            if ganswer:
                answer = self.ganswer_answer(ganswer)
                prn = self.prn_plus(prn, answer)
        
        # Grid transforms
        if check_grid(task_dict) and check_grid_transforms(task_dict):
            ganswer = predict_grid_transforms(task_dict, test_input)
            if ganswer:
                answer = self.ganswer_answer(ganswer)
                prn = self.prn_plus(prn, answer)
        
        # Sub-mask patterns
        if check_sub_mask(task_dict):
            ganswer = predict_repeating_mask(test_input)
            if ganswer:
                answer = self.ganswer_answer(ganswer)
                prn = self.prn_plus(prn, answer)
        
        # ............................................................................... 2 - ML Solver (Sklearn tree)
        if check_subitem(task_dict):
            try:
                train_t = format_features(task_dict)
                test_t = make_features(test_input)
                ganswer = tree1(train_t, test_t, test_input)
                
                if ganswer:
                    answer = self.ganswer_answer(ganswer)
                    prn = self.prn_plus(prn, answer)
            except Exception as e:
                print(f"ML solver failed: {e}")
        
        # ............................................................................... 3 - Symmetry Solver
        try:
            ganswer = self.symmetry_solver.solve(task)
            if ganswer:
                answer = self.ganswer_answer_1(ganswer)
                prn = self.prn_plus(prn, answer)
        except Exception as e:
            print(f"Symmetry solver failed: {e}")
        
        # ............................................................................... 4 - Color Counter Solver
        try:
            answer = self.color_counter_solver.solve(task)
            if answer:
                answer = [answer[0].tolist()]  # Convert to list format
                prn = self.prn_plus(prn, answer)
        except Exception as e:
            print(f"Color counter solver failed: {e}")
        
        # ............................................................................... Conclusion
        if prn:
            prn = self.prn_select_2(prn)
            
            attempt_1 = prn[0] if len(prn) > 0 else [[0, 0], [0, 0]]
            attempt_2 = prn[1] if len(prn) > 1 else [[0, 0], [0, 0]]
            
            return attempt_1, attempt_2
        else:
            return [[0, 0], [0, 0]], [[0, 0], [0, 0]]
    
    def run_main_solvers(self, tasks: Dict[str, Any], 
                        sample_submission: Dict[str, Any],
                        icecube_submission: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        """
        Run main solvers on all tasks, aligned with notebook logic.
        
        Args:
            tasks: Dictionary of tasks with task_id as keys
            sample_submission: Sample submission template
            icecube_submission: Optional ICECube submission for fallback
            
        Returns:
            Dictionary with predictions for all tasks
        """
        sub_solver = sample_submission.copy()
        
        for task_id, task_data in tasks.items():
            # Convert task data to Task object
            task = Task(
                task_id=task_id,
                train=[TaskExample(input=np.array(ex['input']), output=np.array(ex['output'])) 
                       for ex in task_data['train']],
                test=[np.array(ex['input']) for ex in task_data['test']]
            )
            
            # Initialize task in submission
            if task_id not in sub_solver:
                sub_solver[task_id] = []
            
            # Process each test case
            for i, test_input in enumerate(task.test):
                # Ensure we have enough slots in sub_solver
                while len(sub_solver[task_id]) <= i:
                    sub_solver[task_id].append({'attempt_1': [[0, 0], [0, 0]], 'attempt_2': [[0, 0], [0, 0]]})
                
                # Run solvers
                attempt_1, attempt_2 = self.run_single_task(task, test_input)
                
                sub_solver[task_id][i]['attempt_1'] = attempt_1
                sub_solver[task_id][i]['attempt_2'] = attempt_2
                
                # ............................................................................... 5 - ICECube Integration
                if icecube_submission and task_id in icecube_submission and i < len(icecube_submission[task_id]):
                    icecube_attempt_1 = icecube_submission[task_id][i].get('attempt_1', [[0, 0], [0, 0]])
                    icecube_attempt_2 = icecube_submission[task_id][i].get('attempt_2', [[0, 0], [0, 0]])
                    
                    # If our solver found something but ICECube also found something different
                    if attempt_1 != [[0, 0], [0, 0]] and icecube_attempt_1 != [[0, 0], [0, 0]]:
                        if attempt_1 != icecube_attempt_1:
                            sub_solver[task_id][i]['attempt_2'] = icecube_attempt_1
                    
                    # If our solver found nothing, use ICECube results
                    if attempt_1 == [[0, 0], [0, 0]] and attempt_2 == [[0, 0], [0, 0]]:
                        if icecube_attempt_1 != [[0, 0], [0, 0]]:
                            sub_solver[task_id][i]['attempt_1'] = icecube_attempt_1
                        if icecube_attempt_2 != [[0, 0], [0, 0]]:
                            sub_solver[task_id][i]['attempt_2'] = icecube_attempt_2
        
        return sub_solver
    
    def solve_batch(self, tasks: Dict[str, Any], 
                   config: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        """
        Solve a batch of tasks using the main flow logic.
        
        Args:
            tasks: Dictionary of tasks
            config: Optional configuration dictionary
            
        Returns:
            Dictionary with predictions for all tasks
        """
        # Create sample submission template
        sample_submission = {}
        for task_id, task_data in tasks.items():
            sample_submission[task_id] = []
            for _ in task_data['test']:
                sample_submission[task_id].append({
                    'attempt_1': [[0, 0], [0, 0]],
                    'attempt_2': [[0, 0], [0, 0]]
                })
        
        # Run main solvers
        return self.run_main_solvers(tasks, sample_submission)


# Create global main flow controller instance
main_flow_controller = MainFlowController()

# Export main flow functions
ganswer_answer = main_flow_controller.ganswer_answer
ganswer_answer_1 = main_flow_controller.ganswer_answer_1
prn_plus = main_flow_controller.prn_plus
prn_select_2 = main_flow_controller.prn_select_2
run_main_solvers = main_flow_controller.run_main_solvers
solve_batch = main_flow_controller.solve_batch

# All main flow functions
__all__ = [
    'MainFlowController', 'main_flow_controller',
    'ganswer_answer', 'ganswer_answer_1', 'prn_plus', 'prn_select_2',
    'run_main_solvers', 'solve_batch'
] 