"""
Candidate selection utilities for ARC solvers.
"""

import numpy as np
from typing import List, Tuple, Optional
import pandas as pd


class CandidateSelector:
    """Selects the best candidates from multiple solver outputs."""
    
    def __init__(self, max_candidates: int = 5):
        """
        Initialize the candidate selector.
        
        Args:
            max_candidates: Maximum number of candidates to return
        """
        self.max_candidates = max_candidates
    
    def select(self, candidates: List[np.ndarray], max_candidates: Optional[int] = None) -> Tuple[List[np.ndarray], List[float]]:
        """
        Select the best candidates from a list.
        
        Args:
            candidates: List of candidate arrays
            max_candidates: Override the default max_candidates
            
        Returns:
            Tuple of (selected_candidates, scores)
        """
        if not candidates:
            return [], []
        
        max_candidates = max_candidates if max_candidates is not None else self.max_candidates
        
        # Remove duplicates
        unique_candidates = self._remove_duplicates(candidates)
        
        # Score candidates
        scores = [self._score_candidate(cand) for cand in unique_candidates]
        
        # Sort by score and select top candidates
        sorted_pairs = sorted(zip(unique_candidates, scores), key=lambda x: x[1], reverse=True)
        
        selected_candidates = [cand for cand, _ in sorted_pairs[:max_candidates]]
        selected_scores = [score for _, score in sorted_pairs[:max_candidates]]
        
        return selected_candidates, selected_scores
    
    def _remove_duplicates(self, candidates: List[np.ndarray]) -> List[np.ndarray]:
        """Remove duplicate candidates."""
        unique_candidates = []
        seen_hashes = set()
        
        for candidate in candidates:
            candidate_hash = self._hash_array(candidate)
            if candidate_hash not in seen_hashes:
                unique_candidates.append(candidate)
                seen_hashes.add(candidate_hash)
        
        return unique_candidates
    
    def _hash_array(self, arr: np.ndarray) -> str:
        """Create a hash string for an array."""
        return str(arr.tobytes())
    
    def _score_candidate(self, candidate: np.ndarray) -> float:
        """
        Score a candidate based on various heuristics.
        
        Args:
            candidate: The candidate array to score
            
        Returns:
            Score (higher is better)
        """
        # Simple scoring based on non-zero elements and structure
        score = 0.0
        
        # Prefer candidates with some non-zero elements
        non_zero_ratio = np.count_nonzero(candidate) / candidate.size
        score += non_zero_ratio * 10
        
        # Prefer candidates with structure (not all same value)
        unique_ratio = len(np.unique(candidate)) / candidate.size
        score += unique_ratio * 5
        
        # Prefer candidates with reasonable size
        size_score = min(candidate.shape[0] * candidate.shape[1] / 100, 1.0)
        score += size_score * 2
        
        return score 