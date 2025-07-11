"""
Machine learning functions for ARC solver.
"""

import numpy as np
import pandas as pd
from typing import List, Dict, Any, Optional
from sklearn.tree import DecisionTreeClassifier
from sklearn.ensemble import BaggingClassifier
from skimage.measure import label, regionprops


def make_features(x: np.ndarray) -> pd.DataFrame:
    """
    Generate features for a single image.
    
    Args:
        x: Input image array
        
    Returns:
        DataFrame with features
    """
    features = []
    
    # Basic features
    features.append({
        'height': x.shape[0],
        'width': x.shape[1],
        'area': x.shape[0] * x.shape[1],
        'unique_colors': len(np.unique(x)),
        'mode_color': get_mode_color(x),
        'color_0_count': np.sum(x == 0),
        'color_1_count': np.sum(x == 1),
        'color_2_count': np.sum(x == 2),
        'color_3_count': np.sum(x == 3),
        'color_4_count': np.sum(x == 4),
        'color_5_count': np.sum(x == 5),
        'color_6_count': np.sum(x == 6),
        'color_7_count': np.sum(x == 7),
        'color_8_count': np.sum(x == 8),
        'color_9_count': np.sum(x == 9),
    })
    
    # Position-based features
    for i in range(x.shape[0]):
        for j in range(x.shape[1]):
            features.append({
                'x': i,
                'y': j,
                'color': x[i, j],
                'xmin': i,
                'ymin': j,
                'xmax': i,
                'ymax': j,
                'height': x.shape[0],
                'width': x.shape[1],
                'area': x.shape[0] * x.shape[1],
                'unique_colors': len(np.unique(x)),
                'mode_color': get_mode_color(x),
                'color_0_count': np.sum(x == 0),
                'color_1_count': np.sum(x == 1),
                'color_2_count': np.sum(x == 2),
                'color_3_count': np.sum(x == 3),
                'color_4_count': np.sum(x == 4),
                'color_5_count': np.sum(x == 5),
                'color_6_count': np.sum(x == 6),
                'color_7_count': np.sum(x == 7),
                'color_8_count': np.sum(x == 8),
                'color_9_count': np.sum(x == 9),
            })
    
    return pd.DataFrame(features)


def format_features(task: Dict[str, Any]) -> pd.DataFrame:
    """
    Format features for training data.
    
    Args:
        task: Task dictionary
        
    Returns:
        DataFrame with features and labels
    """
    all_features = []
    
    for train_example in task['train']:
        x = np.array(train_example['input'])
        y = np.array(train_example['output'])
        
        # Generate features for input
        input_features = make_features(x)
        
        # Generate features for output
        output_features = make_features(y)
        
        # Add labels
        input_features['label'] = 0  # Input
        output_features['label'] = 1  # Output
        
        all_features.extend(input_features.to_dict('records'))
        all_features.extend(output_features.to_dict('records'))
    
    return pd.DataFrame(all_features)


def tree1(train_df: pd.DataFrame, test_df: pd.DataFrame, test_input: np.ndarray) -> List[np.ndarray]:
    """
    Train decision tree and predict using BaggingClassifier.
    
    Args:
        train_df: Training features DataFrame
        test_df: Test features DataFrame
        test_input: Original test input array
        
    Returns:
        List of predicted outputs
    """
    if len(train_df) == 0 or len(test_df) == 0:
        return []
    
    # Prepare training data
    y = train_df.pop('label')
    X = train_df.drop(['xmin', 'ymin', 'xmax', 'ymax'], axis=1)
    
    # Train model
    base_estimator = DecisionTreeClassifier(random_state=42, max_depth=10)
    model = BaggingClassifier(
        base_estimator=base_estimator,
        n_estimators=10,
        random_state=42
    )
    
    try:
        model.fit(X, y)
    except Exception as e:
        print(f"Model training failed: {e}")
        return []
    
    # Prepare test data
    test_X = test_df.drop(['xmin', 'ymin', 'xmax', 'ymax'], axis=1)
    
    # Predict
    try:
        predictions = model.predict_proba(test_X)
        output_probs = predictions[:, 1]  # Probability of being output
        
        # Find regions with high output probability
        threshold = 0.5
        output_mask = output_probs > threshold
        
        if np.sum(output_mask) > 0:
            # Extract bounding box
            output_positions = test_df[output_mask]
            if len(output_positions) > 0:
                xmin = output_positions['xmin'].min()
                ymin = output_positions['ymin'].min()
                xmax = output_positions['xmax'].max()
                ymax = output_positions['ymax'].max()
                
                # Extract submatrix
                result = test_input[xmin:xmax+1, ymin:ymax+1]
                return [result]
    
    except Exception as e:
        print(f"Prediction failed: {e}")
        return []
    
    return []


def get_mode_color(ar: np.ndarray) -> int:
    """
    Get the most frequent color in array.
    
    Args:
        ar: Input array
        
    Returns:
        Most frequent color value
    """
    colors, counts = np.unique(ar, return_counts=True)
    return colors[np.argmax(counts)] 