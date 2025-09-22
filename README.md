# ARC Solver - Multi-Strategy Abstract Reasoning Solution

A Python framework for solving ARC (Abstract Reasoning Corpus) tasks using multiple specialized algorithms and C++ optimizations

# Competition Result
- **ARC Prize 2024 Kaggle Competition**
- Final Rank: **25 / 1427 teams** (Top 2%)
 ![Official Leaderboard Screenshot](images/ARC_result.png)

## Quick Start

```bash
# Clone and install
git clone https://github.com/jiaxuan030331/ARC2024-solution.git
cd ARC2024-solution
pip install -r requirements.txt
pip install -e .

# Run demo
python demo.py
```

## Engineering Architecture & Design

### System Overview

Our ARC solver employs a **multi-strategy architecture** with intelligent solver selection and C++ optimizations for performance-critical algorithms.

```
┌─────────────────────────────────────────────────────────────┐
│                    ARC Solver Framework                     │
├─────────────────────────────────────────────────────────────┤
│  Main Controller (Python)                                   │
│  ├── Task Analysis & Pattern Recognition                    │
│  ├── Solver Selection Logic                                 │
│  └── Result Aggregation & Ranking                           │
├─────────────────────────────────────────────────────────────┤
│  Specialized Solvers (9)                                    │
│  ├── DAGSolver (C++) ──── Multi-step transformations        │
│  ├── TilingSolver (C++) ── Pattern repetition               │
│  ├── SymmetrySolver (C++) ─ Symmetry operations             │
│  ├── ChessSolver (C++) ──── Periodic patterns               │
│  ├── MLSolver (C++) ────── Feature-based classification     │
│  └── GridSolver (Python) ─ Grid transformations             │
├─────────────────────────────────────────────────────────────┤
│  Core Transform Engine                                      │
│  ├── 50+ Transform Functions (C++)                          │
│  ├── DAG Search Algorithm                                   │
│  ├── Pattern Extraction & Analysis                          │
│  └── Candidate Generation & Scoring                         │
└─────────────────────────────────────────────────────────────┘
```

### Core Project Structure

```
arc_solver/
├── core/                    # Main orchestration logic
│   ├── solver.py            # ArcSolver main class & pipeline
│   ├── config.py            # Configuration management
│   └── patterns.py          # Pattern recognition algorithms
├── solvers/                 # Specialized solving algorithms (40+)
│   ├── dag.py              # Multi-step transformation solver
│   ├── tiling.py           # Pattern tiling & repetition
│   ├── symmetry.py         # Symmetry detection & repair
│   └── [6 other solvers] # Chess, ML, Grid, Color, etc.
├── cpp/                    # C++ optimized implementations
│   ├── include/            # Headers for all C++ solvers
│   ├── src/               # Optimized algorithm implementations
│   └── bindings/          # Python-C++ integration layer
├── cpp_wrappers/          # Python interfaces for C++ solvers
└── utils/                 # Support functions & utilities
```

##  **[ Complete Examples & Technical Details →](EXAMPLES.md)**

See comprehensive examples of:
- **Detailed Solving Workflows** (Object Removal, Pattern Tiling)
- **Engineering Architecture & Design**
- **Solver Selection Strategies**
- **Performance Benchmarks**

## Performance

| Metric | Value |
|--------|-------|
| Solver Types | 40+ specialized algorithms |
| C++ Speedup | 4x-46x over pure Python |
| Success Rate | Variable by task complexity |
| Processing Time | ~1s average per task |

---

**Note**: This solver framework combines heuristic algorithms, pattern recognition, and performance optimization to tackle ARC reasoning tasks systematically.


