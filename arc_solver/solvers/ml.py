"""Machine learning solver for ARC tasks."""

import numpy as np
from typing import List
from .base import BaseSolver
from ..data.task import Task
import pandas as pd
from sklearn.tree import DecisionTreeClassifier
from sklearn.ensemble import BaggingClassifier
from skimage.measure import label, regionprops


class MLSolver(BaseSolver):
    """Solver using machine learning approaches (Bagging+DecisionTree)."""
    
    def can_solve(self, task: Task) -> bool:
        """Check if task can be solved with ML."""
        for ex in task.train:
            if not self._has_subitem(ex.input, ex.output):
                return False
        return True
    
    def solve(self, task: Task) -> List[np.ndarray]:
        """Generate ML-based predictions."""
        if not self.can_solve(task):
            return []
        train_df = self._format_features(task)
        y = train_df.pop('label')
        X = train_df.drop(['xmin','ymin','xmax','ymax'], axis=1)
        model = BaggingClassifier(estimator=DecisionTreeClassifier(), n_estimators=100, random_state=4372)
        model.fit(X, y)
        results = []
        for test_in in task.test:
            test_df = self._make_features(test_in)
            X_test = test_df.drop(['xmin','ymin','xmax','ymax'], axis=1)
            preds = model.predict_proba(X_test)[:,1]
            idx = np.argmax(preds)
            xmin, ymin, xmax, ymax = test_df.loc[idx, ['xmin','ymin','xmax','ymax']].astype(int)
            obj = test_in[xmin:xmax, ymin:ymax]
            results.append(obj)
        return results

    def _has_subitem(self, x: np.ndarray, y: np.ndarray) -> bool:
        """判断y是否为x的子块"""
        for i in range(x.shape[0]-y.shape[0]+1):
            for j in range(x.shape[1]-y.shape[1]+1):
                if np.array_equal(x[i:i+y.shape[0], j:j+y.shape[1]], y):
                    return True
        return False

    def _format_features(self, task: Task) -> pd.DataFrame:
        """为每个训练样本生成特征DataFrame，并标注label"""
        dfs = []
        for ex in task.train:
            x = ex.input
            y = ex.output
            df = self._make_features(x)
            df['label'] = False
            positions = self._find_sub(x, y)
            for xmin, ymin, xmax, ymax in positions:
                mask = (df['xmin']==xmin)&(df['ymin']==ymin)&(df['xmax']==xmax)&(df['ymax']==ymax)
                df.loc[mask, 'label'] = True
            dfs.append(df)
        return pd.concat(dfs).reset_index(drop=True)

    def _find_sub(self, matrix: np.ndarray, sub: np.ndarray):
        """返回所有sub在matrix中的位置"""
        positions = []
        for x in range(matrix.shape[0]-sub.shape[0]+1):
            for y in range(matrix.shape[1]-sub.shape[1]+1):
                if np.equal(matrix[x:x+sub.shape[0], y:y+sub.shape[1]], sub).all():
                    positions.append((x, y, x+sub.shape[0], y+sub.shape[1]))
        return positions

    def _make_features(self, x: np.ndarray) -> pd.DataFrame:
        """生成所有子块的特征"""
        n, m = x.shape
        records = []
        for xmin in range(n):
            for ymin in range(m):
                for xmax in range(xmin+1, n+1):
                    for ymax in range(ymin+1, m+1):
                        box = x[xmin:xmax, ymin:ymax]
                        if box.size == x.size:
                            continue  # 跳过全图
                        rec = {
                            'xmin': xmin, 'ymin': ymin, 'xmax': xmax, 'ymax': ymax,
                            'area': box.size,
                            'unique_colors': len(np.unique(box)),
                            'mode_color': self._get_mode_color(box),
                        }
                        # 区域属性
                        rec['has_frame'] = self._has_frame(box)
                        rec['rps4'] = self._has_region(box, conn=1)
                        rec['rps8'] = self._has_region(box, conn=2)
                        records.append(rec)
        return pd.DataFrame(records)

    def _get_mode_color(self, arr: np.ndarray) -> int:
        """获取数组中出现次数最多的颜色"""
        colors, counts = np.unique(arr, return_counts=True)
        return int(colors[np.argmax(counts)])

    def _has_frame(self, arr: np.ndarray) -> int:
        """判断边框是否统一"""
        if arr.shape[0] < 2 or arr.shape[1] < 2:
            return 0
        border = np.concatenate([
            arr[0,:], arr[-1,:], arr[:,0], arr[:,-1]
        ])
        return int(np.all(border == border[0]))

    def _has_region(self, arr: np.ndarray, conn=1) -> int:
        """区域连通性"""
        labels = label(arr, background=-1, connectivity=conn)
        if isinstance(labels, np.ndarray):
            return int(np.max(labels) > 0)
        return 0 