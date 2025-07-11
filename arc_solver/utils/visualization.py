"""
Visualization utilities for ARC tasks, aligned with main notebook.

This module provides visualization functions for displaying ARC tasks,
patterns, and results using matplotlib.
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as colors
from typing import List, Dict, Any, Optional, Union
from ..data.task import Task, TaskExample


# Color mapping from main notebook
CMAP = colors.ListedColormap([
    '#000000', '#0074D9', '#FF4136', '#2ECC40', '#FFDC00',
    '#AAAAAA', '#F012BE', '#FF851B', '#7FDBFF', '#870C25'
])
NORM = colors.Normalize(vmin=0, vmax=9)
COLOR_LIST = ["black", "blue", "red", "green", "yellow", 
              "gray", "magenta", "orange", "sky", "brown"]


class ARCVisualizer:
    """Visualization utilities for ARC tasks."""
    
    def __init__(self, cmap: Optional[colors.Colormap] = None, 
                 norm: Optional[colors.Normalize] = None):
        """
        Initialize visualizer.
        
        Args:
            cmap: Colormap for visualization
            norm: Normalization for color values
        """
        self.cmap = cmap or CMAP
        self.norm = norm or NORM
    
    def plot_pic(self, x: Union[np.ndarray, List[List[int]]], 
                 title: Optional[str] = None, 
                 figsize: Optional[tuple] = None) -> None:
        """
        Plot a single image.
        
        Args:
            x: Image array or list
            title: Plot title
            figsize: Figure size
        """
        x = np.array(x)
        if figsize:
            plt.figure(figsize=figsize)
        plt.imshow(x, cmap=self.cmap, norm=self.norm)
        if title:
            plt.title(title)
        plt.show()
    
    def plot_picture(self, x: Union[np.ndarray, List[List[int]]]) -> None:
        """
        Plot a single picture (alias for plot_pic).
        
        Args:
            x: Image array or list
        """
        self.plot_pic(x)
    
    def plot_task(self, task: Union[Task, Dict[str, Any]], 
                  figsize: Optional[tuple] = None) -> None:
        """
        Plot a complete task with all train and test examples.
        
        Args:
            task: Task object or dictionary
            figsize: Figure size
        """
        if isinstance(task, dict):
            # Convert dict to Task format
            n = len(task["train"]) + len(task["test"])
            fig, axs = plt.subplots(2, n, figsize=figsize or (4*n, 8), dpi=200)
            plt.subplots_adjust(wspace=0, hspace=0)
            fig_num = 0
            
            # Plot training examples
            for i, t in enumerate(task["train"]):
                t_in, t_out = np.array(t["input"]), np.array(t["output"])
                axs[0][fig_num].imshow(t_in, cmap=self.cmap, norm=self.norm)
                axs[0][fig_num].set_title(f'Train-{i} in')
                axs[0][fig_num].set_yticks(list(range(t_in.shape[0])))
                axs[0][fig_num].set_xticks(list(range(t_in.shape[1])))
                axs[1][fig_num].imshow(t_out, cmap=self.cmap, norm=self.norm)
                axs[1][fig_num].set_title(f'Train-{i} out')
                axs[1][fig_num].set_yticks(list(range(t_out.shape[0])))
                axs[1][fig_num].set_xticks(list(range(t_out.shape[1])))
                fig_num += 1
            
            # Plot test examples
            for i, t in enumerate(task["test"]):
                t_in = np.array(t["input"])
                axs[0][fig_num].imshow(t_in, cmap=self.cmap, norm=self.norm)
                axs[0][fig_num].set_title(f'Test-{i} in')
                axs[0][fig_num].set_yticks(list(range(t_in.shape[0])))
                axs[0][fig_num].set_xticks(list(range(t_in.shape[1])))
                fig_num += 1
        else:
            # Task object
            n = task.num_train_examples + task.num_test_examples
            fig, axs = plt.subplots(2, n, figsize=figsize or (4*n, 8), dpi=200)
            plt.subplots_adjust(wspace=0, hspace=0)
            fig_num = 0
            
            # Plot training examples
            for i, ex in enumerate(task.train):
                axs[0][fig_num].imshow(ex.input, cmap=self.cmap, norm=self.norm)
                axs[0][fig_num].set_title(f'Train-{i} in')
                axs[0][fig_num].set_yticks(list(range(ex.input.shape[0])))
                axs[0][fig_num].set_xticks(list(range(ex.input.shape[1])))
                axs[1][fig_num].imshow(ex.output, cmap=self.cmap, norm=self.norm)
                axs[1][fig_num].set_title(f'Train-{i} out')
                axs[1][fig_num].set_yticks(list(range(ex.output.shape[0])))
                axs[1][fig_num].set_xticks(list(range(ex.output.shape[1])))
                fig_num += 1
            
            # Plot test examples
            for i, test_input in enumerate(task.test):
                axs[0][fig_num].imshow(test_input, cmap=self.cmap, norm=self.norm)
                axs[0][fig_num].set_title(f'Test-{i} in')
                axs[0][fig_num].set_yticks(list(range(test_input.shape[0])))
                axs[0][fig_num].set_xticks(list(range(test_input.shape[1])))
                fig_num += 1
        
        plt.tight_layout()
        plt.show()
    
    def plot_task1(self, task: Union[Task, Dict[str, Any]], 
                   figsize: Optional[tuple] = None) -> None:
        """
        Plot task without test outputs (for prediction tasks).
        
        Args:
            task: Task object or dictionary
            figsize: Figure size
        """
        if isinstance(task, dict):
            n = len(task["train"]) + len(task["test"])
            fig, axs = plt.subplots(2, n, figsize=figsize or (4*n, 8), dpi=200)
            plt.subplots_adjust(wspace=0, hspace=0)
            fig_num = 0
            
            # Plot training examples
            for i, t in enumerate(task["train"]):
                t_in, t_out = np.array(t["input"]), np.array(t["output"])
                axs[0][fig_num].imshow(t_in, cmap=self.cmap, norm=self.norm)
                axs[0][fig_num].set_title(f'Train-{i} in')
                axs[0][fig_num].set_yticks(list(range(t_in.shape[0])))
                axs[0][fig_num].set_xticks(list(range(t_in.shape[1])))
                axs[1][fig_num].imshow(t_out, cmap=self.cmap, norm=self.norm)
                axs[1][fig_num].set_title(f'Train-{i} out')
                axs[1][fig_num].set_yticks(list(range(t_out.shape[0])))
                axs[1][fig_num].set_xticks(list(range(t_out.shape[1])))
                fig_num += 1
            
            # Plot test inputs only
            for i, t in enumerate(task["test"]):
                t_in = np.array(t["input"])
                axs[0][fig_num].imshow(t_in, cmap=self.cmap, norm=self.norm)
                axs[0][fig_num].set_title(f'Test-{i} in')
                axs[0][fig_num].set_yticks(list(range(t_in.shape[0])))
                axs[0][fig_num].set_xticks(list(range(t_in.shape[1])))
                fig_num += 1
        else:
            n = task.num_train_examples + task.num_test_examples
            fig, axs = plt.subplots(2, n, figsize=figsize or (4*n, 8), dpi=200)
            plt.subplots_adjust(wspace=0, hspace=0)
            fig_num = 0
            
            # Plot training examples
            for i, ex in enumerate(task.train):
                axs[0][fig_num].imshow(ex.input, cmap=self.cmap, norm=self.norm)
                axs[0][fig_num].set_title(f'Train-{i} in')
                axs[0][fig_num].set_yticks(list(range(ex.input.shape[0])))
                axs[0][fig_num].set_xticks(list(range(ex.input.shape[1])))
                axs[1][fig_num].imshow(ex.output, cmap=self.cmap, norm=self.norm)
                axs[1][fig_num].set_title(f'Train-{i} out')
                axs[1][fig_num].set_yticks(list(range(ex.output.shape[0])))
                axs[1][fig_num].set_xticks(list(range(ex.output.shape[1])))
                fig_num += 1
            
            # Plot test inputs only
            for i, test_input in enumerate(task.test):
                axs[0][fig_num].imshow(test_input, cmap=self.cmap, norm=self.norm)
                axs[0][fig_num].set_title(f'Test-{i} in')
                axs[0][fig_num].set_yticks(list(range(test_input.shape[0])))
                axs[0][fig_num].set_xticks(list(range(test_input.shape[1])))
                fig_num += 1
        
        plt.tight_layout()
        plt.show()
    
    def plot_objects(self, objects: List[np.ndarray], 
                    titles: Optional[List[str]] = None,
                    figsize: Optional[tuple] = None) -> None:
        """
        Plot multiple objects side by side.
        
        Args:
            objects: List of image arrays
            titles: List of titles for each object
            figsize: Figure size
        """
        if titles is None:
            titles = [''] * len(objects)
        
        fig, axs = plt.subplots(1, len(objects), 
                               figsize=figsize or (3*len(objects), 3), 
                               gridspec_kw={'wspace': 0.02, 'hspace': 0.02}, 
                               squeeze=False)
        
        for i, (obj, title) in enumerate(zip(objects, titles)):
            obj = np.array(obj)
            axs[0, i].grid(True, which='both', color='lightgrey', linewidth=0.5)
            shape = ' '.join(map(str, obj.shape))
            axs[0, i].set_title(f"{title} {shape}")
            axs[0, i].set_yticks([x-0.5 for x in range(1+len(obj))])
            axs[0, i].set_xticks([x-0.5 for x in range(1+len(obj[0]))])
            axs[0, i].set_yticklabels([])
            axs[0, i].set_xticklabels([])
            axs[0, i].imshow(obj, cmap=self.cmap, norm=self.norm)
        
        plt.show()
    
    def plot_color_legend(self, figsize: tuple = (5, 2)) -> None:
        """
        Plot color legend showing all ARC colors.
        
        Args:
            figsize: Figure size
        """
        plt.figure(figsize=figsize, dpi=200)
        plt.imshow([list(range(10))], cmap=self.cmap, norm=self.norm)
        plt.xticks(list(range(10)))
        plt.yticks([])
        plt.title("ARC Color Legend")
        plt.show()


# Convenience functions (aligned with notebook)
def plot_pic(x: Union[np.ndarray, List[List[int]]]) -> None:
    """Plot a single image (convenience function)."""
    visualizer = ARCVisualizer()
    visualizer.plot_pic(x)


def plot_picture(x: Union[np.ndarray, List[List[int]]]) -> None:
    """Plot a single picture (convenience function)."""
    visualizer = ARCVisualizer()
    visualizer.plot_picture(x)


def plot_task(task: Union[Task, Dict[str, Any]]) -> None:
    """Plot a complete task (convenience function)."""
    visualizer = ARCVisualizer()
    visualizer.plot_task(task)


def plot_task1(task: Union[Task, Dict[str, Any]]) -> None:
    """Plot task without test outputs (convenience function)."""
    visualizer = ARCVisualizer()
    visualizer.plot_task1(task)


def plot_objects(objects: List[np.ndarray], titles: Optional[List[str]] = None) -> None:
    """Plot multiple objects (convenience function)."""
    visualizer = ARCVisualizer()
    visualizer.plot_objects(objects, titles) 