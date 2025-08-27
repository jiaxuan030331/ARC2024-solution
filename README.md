# ARC Solver - Multi-Strategy Abstract Reasoning Solution

A Python framework for solving ARC (Abstract Reasoning Corpus) tasks using multiple specialized algorithms and C++ optimizations, winning SILVER Prize in Kaggle 2024 ARC Competition.

## 🏗️ Architecture

```
arc_solver/
├── solvers/          # 40+ specialized pattern solvers
├── cpp/             # C++ optimized implementations  
├── core/            # Main orchestration logic
└── utils/           # Support functions and transforms
```

## 📁 Project Structure

```
arc_solver/
├── solvers/        # 40+ specialized pattern solvers
├── cpp/           # C++ optimized implementations  
├── core/          # Main orchestration logic
├── utils/         # Support functions and transforms
└── data/          # Task handling structures
```

### Core Components
- **Solvers**: DAG, Tiling, Symmetry, Chess, ML, Grid, Color Counter
- **C++ Extensions**: pybind11 bindings for performance-critical algorithms
- **Utilities**: Pattern detection, transforms, visualization, ML functions
- **Data**: Task structures and processing pipelines

## 🚀 Quick Start

```python
from arc_solver import ArcSolver

# Initialize solver
solver = ArcSolver()

# Load and solve task
task = solver.load_task("path/to/task.json")
result = solver.solve(task)

# Get predictions
predictions = result.get_predictions()
```

## 🛠️ Installation

```bash
# Basic installation
pip install -r requirements.txt

# Development installation with C++ optimizations
pip install -e .
cd arc_solver/cpp && python setup.py build_ext --inplace
```

### Prerequisites
- Python 3.8+
- C++ compiler (GCC 7+ or Clang 5+) for optimizations
- CMake 3.15+ for C++ builds

## 📋 Solving Process

### 1. Task Analysis
```python
# Automatic pattern detection
solver.analyze_task(task)  # → identifies task type
```

### 2. Solver Selection
```python
# Multi-strategy approach
if dag_solver.can_solve(task):
    return dag_solver.solve(task)
elif tiling_solver.can_solve(task):
    return tiling_solver.solve(task)
# ... fallback chain
```

### 3. Transform Pipeline
```python
# Example: Object removal task
transforms = [
    "filterCol_0",      # Remove black pixels
    "extractPattern",   # Learn background pattern  
    "fillHoles",        # Fill empty regions
    "replicate"         # Reconstruct background
]
```

## 🔍 Example: Tiling Task

**Problem**: Expand 2×2 pattern to 6×6 grid

**Input**: `[[blue, magenta], [magenta, yellow]]`  
**Output**: 6×6 grid with 2×2 pattern repeated 3×3 times

**Solution**:
```python
# TilingSolver detects 3x scale factor
output = np.tile(input_pattern, (3, 3))
```

## 📊 Performance

| Metric | Value |
|--------|-------|
| Solver Types | 40+ specialized algorithms |
| C++ Speedup | 4x-46x over pure Python |
| Success Rate | Variable by task complexity |
| Processing Time | ~1s average per task |

## 🔧 Configuration

```python
# Custom solver configuration
config = SolverConfig(
    max_depth=20,
    enable_cpp=True,
    timeout=30,
    max_candidates=5
)
solver = ArcSolver(config=config)
```

## 🚀 Deployment

### Local Development
```bash
git clone <repository>
cd arc-solver
pip install -e .
python -c "import arc_solver; print('Ready!')"
```

### Production
```bash
# Install from PyPI
pip install arc-solver

# Or build from source
python -m build
pip install dist/arc_solver-*.whl
```

### Docker
```dockerfile
FROM python:3.9-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install -r requirements.txt
COPY . .
RUN pip install -e .
ENTRYPOINT ["python", "-m", "arc_solver.cli"]
```

## 🧪 Testing

```bash
# Run test suite
python -m pytest tests/

# Test specific solver
python -m pytest tests/test_tiling_solver.py
```

## 📖 Technical Details

### Transform Functions
- **Geometric**: rotation, reflection, scaling
- **Color**: filtering, mapping, inversion  
- **Pattern**: extraction, replication, filling
- **Composition**: layering, masking, alignment

### DAG Search Strategy
1. **State Representation**: Grid + metadata
2. **Transform Application**: Function chaining
3. **Candidate Generation**: Multiple solution paths
4. **Scoring**: Confidence-based ranking

---

**Note**: This solver framework combines heuristic algorithms, pattern recognition, and performance optimization to tackle ARC reasoning tasks systematically.


