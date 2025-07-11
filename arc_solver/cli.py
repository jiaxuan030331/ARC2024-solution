"""
Command line interface for ARC Solver.
"""

import argparse
import json
import sys
from pathlib import Path

from . import ArcSolver, TaskLoader, SolverConfig


def main():
    """Main CLI function."""
    parser = argparse.ArgumentParser(
        description="ARC Solver - Multi-Strategy Abstract Reasoning Corpus Solver"
    )
    
    parser.add_argument(
        "input_file",
        help="Path to the input JSON file containing ARC task(s)"
    )
    
    parser.add_argument(
        "-o", "--output",
        help="Output file path (default: stdout)"
    )
    
    parser.add_argument(
        "--config",
        help="Path to configuration JSON file"
    )
    
    parser.add_argument(
        "--max-candidates",
        type=int,
        default=5,
        help="Maximum number of candidates to generate (default: 5)"
    )
    
    parser.add_argument(
        "--solvers",
        nargs="+",
        choices=["repeating", "grid", "chess", "tiling", "ml", "symmetry", "color_counter"],
        help="Specific solvers to use (default: all enabled)"
    )
    
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Enable verbose output"
    )
    
    args = parser.parse_args()
    
    # Load configuration
    config = SolverConfig(max_candidates=args.max_candidates)
    
    if args.config:
        with open(args.config, 'r') as f:
            config_data = json.load(f)
            for key, value in config_data.items():
                if hasattr(config, key):
                    setattr(config, key, value)
    
    # Override solvers if specified
    if args.solvers:
        config.enable_repeating_solver = "repeating" in args.solvers
        config.enable_grid_solver = "grid" in args.solvers
        config.enable_chess_solver = "chess" in args.solvers
        config.enable_tiling_solver = "tiling" in args.solvers
        config.enable_ml_solver = "ml" in args.solvers
        config.enable_symmetry_solver = "symmetry" in args.solvers
        config.enable_color_counter_solver = "color_counter" in args.solvers
    
    # Initialize solver
    solver = ArcSolver(config=config)
    
    if args.verbose:
        info = solver.get_solver_info()
        print(f"🔧 Available solvers: {info['available_solvers']}")
        print(f"⚙️ Configuration: {info['config']}")
    
    # Load input file
    input_path = Path(args.input_file)
    if not input_path.exists():
        print(f"❌ Error: Input file '{args.input_file}' not found", file=sys.stderr)
        sys.exit(1)
    
    try:
        with open(input_path, 'r') as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        print(f"❌ Error: Invalid JSON in input file: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Process tasks
    results = []
    
    if isinstance(data, dict) and 'train' in data:
        # Single task
        task = TaskLoader.from_json(data)
        result = solver.solve(task)
        results.append(result)
    elif isinstance(data, dict):
        # Multiple tasks
        for task_id, task_data in data.items():
            task_data['task_id'] = task_id
            task = TaskLoader.from_json(task_data)
            result = solver.solve(task)
            results.append(result)
    else:
        print("❌ Error: Invalid input format", file=sys.stderr)
        sys.exit(1)
    
    # Prepare output
    output_data = {}
    for result in results:
        output_data[result.task_id] = {
            'predictions': [pred.tolist() for pred in result.predictions],
            'scores': result.scores,
            'metadata': result.metadata
        }
    
    # Write output
    if args.output:
        with open(args.output, 'w') as f:
            json.dump(output_data, f, indent=2)
        print(f"✅ Results saved to '{args.output}'")
    else:
        json.dump(output_data, sys.stdout, indent=2)
    
    if args.verbose:
        total_predictions = sum(len(r.predictions) for r in results)
        print(f"\n📊 Summary: Generated {total_predictions} predictions for {len(results)} tasks")


if __name__ == "__main__":
    main() 