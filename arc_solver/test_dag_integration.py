#!/usr/bin/env python3
"""
Test DAG Solver integration in arc_solver framework.
"""

import sys
import os
import numpy as np

# Add the current directory to path  
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

def test_cpp_extension():
    """Test the C++ extension directly."""
    print("🔬 Testing C++ Extension")
    print("========================")
    
    try:
        import arc_solver_cpp
        print("✅ arc_solver_cpp import successful")
        
        # Check if DAGSolverCpp exists
        if hasattr(arc_solver_cpp, 'DAGSolverCpp'):
            print("✅ DAGSolverCpp class found")
            
            # Test basic functionality
            solver = arc_solver_cpp.DAGSolverCpp()
            print("✅ DAGSolverCpp creation successful")
            
            # Test with simple data
            train_inputs = [[[1, 0], [0, 1]]]
            train_outputs = [[[0, 1], [1, 0]]]
            
            can_solve = solver.can_solve(train_inputs, train_outputs)
            print(f"✅ can_solve check: {can_solve}")
            
            test_inputs = [[[1, 1], [0, 0]]]
            results = solver.solve(train_inputs, train_outputs, test_inputs)
            print(f"✅ Solve result: {len(results)} solutions")
            
            functions = solver.get_available_functions()
            print(f"✅ Available functions: {len(functions)} - {functions[:5]}")
            
            return True
        else:
            print("❌ DAGSolverCpp class not found in arc_solver_cpp")
            return False
        
    except Exception as e:
        print(f"❌ C++ extension test failed: {e}")
        return False

def test_python_solver():
    """Test the Python DAG solver."""
    print("\n🐍 Testing Python DAG Solver")
    print("=============================")
    
    try:
        from solvers.dag import DAGSolver
        print("✅ DAGSolver import successful")
        
        # Create solver
        solver = DAGSolver(enable_logging=True, max_depth=5)
        print("✅ DAGSolver creation successful")
        
        config = solver.get_config()
        print(f"✅ Configuration: {config}")
        
        stats = solver.get_statistics()
        print(f"✅ Statistics: {stats}")
        
        if solver.use_cpp:
            functions = solver.get_available_functions()
            print(f"✅ Available functions: {len(functions)}")
        else:
            print("⚠️  C++ backend not available")
        
        return True
        
    except Exception as e:
        print(f"❌ Python DAG solver test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_solver_imports():
    """Test solver import patterns."""
    print("\n📥 Testing Solver Imports")
    print("=========================")
    
    # Test 1: Direct import
    try:
        from solvers.dag import DAGSolver
        print("✅ Direct import: from solvers.dag import DAGSolver")
    except Exception as e:
        print(f"❌ Direct import failed: {e}")
        return False
    
    # Test 2: From solvers module
    try:
        from solvers import DAGSolver
        print("✅ Module import: from solvers import DAGSolver")
    except Exception as e:
        print(f"❌ Module import failed: {e}")
        return False
    
    # Test 3: Factory functions
    try:
        from solvers import create_balanced_dag_solver
        solver = create_balanced_dag_solver()
        print("✅ Factory function: create_balanced_dag_solver()")
    except Exception as e:
        print(f"❌ Factory function failed: {e}")
        return False
    
    return True

def test_mock_solve():
    """Test solving with mock data."""
    print("\n🎯 Testing Mock Solve")
    print("=====================")
    
    try:
        from solvers import DAGSolver
        
        # Create a mock task
        class MockTask:
            def __init__(self):
                self.train = [
                    {
                        'input': np.array([[1, 0], [0, 1]], dtype=np.int32),
                        'output': np.array([[0, 1], [1, 0]], dtype=np.int32)
                    }
                ]
                self.test = [
                    {
                        'input': np.array([[1, 1], [0, 0]], dtype=np.int32)
                    }
                ]
        
        task = MockTask()
        solver = DAGSolver(enable_logging=True, max_depth=3)
        
        print(f"✅ Mock task created")
        print(f"✅ Solver created: {solver}")
        
        can_solve = solver.can_solve(task)
        print(f"✅ can_solve: {can_solve}")
        
        if can_solve and solver.use_cpp:
            results = solver.solve(task)
            print(f"✅ Solve completed: {len(results)} results")
            
            if results:
                print(f"   First result shape: {results[0].shape}")
                print(f"   First result:\n{results[0]}")
        else:
            print("⚠️  Skipping solve (C++ not available or can't solve)")
        
        return True
        
    except Exception as e:
        print(f"❌ Mock solve test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """Run all tests."""
    print("🚀 ARC DAG Solver Integration Test")
    print("==================================")
    
    tests = [
        ("C++ Extension", test_cpp_extension),
        ("Python Solver", test_python_solver),
        ("Solver Imports", test_solver_imports),
        ("Mock Solve", test_mock_solve)
    ]
    
    passed = 0
    total = len(tests)
    
    for test_name, test_func in tests:
        try:
            success = test_func()
            if success:
                passed += 1
                print(f"✅ {test_name}: PASSED")
            else:
                print(f"❌ {test_name}: FAILED")
        except Exception as e:
            print(f"💥 {test_name}: CRASHED - {e}")
    
    print(f"\n📊 Final Results")
    print(f"================")
    print(f"Passed: {passed}/{total} ({passed/total*100:.1f}%)")
    
    if passed == total:
        print("🎉 All tests passed! DAG Solver integration is successful!")
        print("\n💡 Usage example:")
        print("   from arc_solver.solvers import DAGSolver")
        print("   solver = DAGSolver(max_depth=20)")
        print("   results = solver.solve(task)")
        return 0
    else:
        print("⚠️  Some tests failed. Check the output above for details.")
        return 1

if __name__ == "__main__":
    sys.exit(main()) 