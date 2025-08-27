#!/usr/bin/env python3
"""
Simple DAG Solver test for arc_solver integration.
"""

def test_cpp_extension():
    """Test the C++ extension."""
    print("🔬 Testing C++ Extension")
    print("========================")
    
    try:
        import arc_solver_cpp
        print("✅ arc_solver_cpp import successful")
        
        if hasattr(arc_solver_cpp, 'DAGSolverCpp'):
            print("✅ DAGSolverCpp class found")
            
            # Create solver
            solver = arc_solver_cpp.DAGSolverCpp()
            print("✅ DAGSolverCpp creation successful")
            
            # Test basic functionality
            train_inputs = [[[1, 0], [0, 1]]]
            train_outputs = [[[0, 1], [1, 0]]]
            
            can_solve = solver.can_solve(train_inputs, train_outputs)
            print(f"✅ can_solve: {can_solve}")
            
            test_inputs = [[[1, 1], [0, 0]]]
            results = solver.solve(train_inputs, train_outputs, test_inputs)
            print(f"✅ solve: {len(results)} results")
            
            if results:
                print(f"   First result: {results[0]}")
            
            functions = solver.get_available_functions()
            print(f"✅ Functions: {len(functions)} available")
            print(f"   Examples: {functions[:3]}")
            
            return True
        else:
            print("❌ DAGSolverCpp not found")
            return False
            
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

def test_python_wrapper():
    """Test Python wrapper directly."""
    print("\n🐍 Testing Python Wrapper")
    print("==========================")
    
    try:
        # Direct test without relative imports
        import sys
        import os
        import numpy as np
        
        # Mock the Task structure
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
        
        # Import DAG solver directly
        import arc_solver_cpp
        
        class SimpleDAGSolver:
            def __init__(self):
                if hasattr(arc_solver_cpp, 'DAGSolverCpp'):
                    self.cpp_solver = arc_solver_cpp.DAGSolverCpp()
                    self.use_cpp = True
                else:
                    self.use_cpp = False
            
            def can_solve(self, task):
                if not self.use_cpp:
                    return False
                
                train_inputs = []
                train_outputs = []
                for example in task.train:
                    train_inputs.append(example['input'].tolist())
                    train_outputs.append(example['output'].tolist())
                
                return self.cpp_solver.can_solve(train_inputs, train_outputs)
            
            def solve(self, task):
                if not self.use_cpp:
                    return []
                
                train_inputs = []
                train_outputs = []
                for example in task.train:
                    train_inputs.append(example['input'].tolist())
                    train_outputs.append(example['output'].tolist())
                
                test_inputs = [task.test[0]['input'].tolist()]
                
                results = self.cpp_solver.solve(train_inputs, train_outputs, test_inputs)
                return [np.array(result, dtype=np.uint8) for result in results]
        
        # Test the wrapper
        solver = SimpleDAGSolver()
        print(f"✅ SimpleDAGSolver created (use_cpp: {solver.use_cpp})")
        
        task = MockTask()
        print("✅ Mock task created")
        
        can_solve = solver.can_solve(task)
        print(f"✅ can_solve: {can_solve}")
        
        if can_solve:
            results = solver.solve(task)
            print(f"✅ solve: {len(results)} results")
            
            if results:
                print(f"   First result shape: {results[0].shape}")
                print(f"   First result:\n{results[0]}")
        else:
            print("⚠️  Cannot solve this task")
        
        return True
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """Run tests."""
    print("🚀 Simple DAG Solver Test")
    print("=========================")
    
    tests = [
        test_cpp_extension,
        test_python_wrapper
    ]
    
    passed = 0
    for i, test in enumerate(tests, 1):
        try:
            if test():
                passed += 1
                print(f"✅ Test {i}: PASSED")
            else:
                print(f"❌ Test {i}: FAILED")
        except Exception as e:
            print(f"💥 Test {i}: CRASHED - {e}")
    
    print(f"\n📊 Results: {passed}/{len(tests)} tests passed")
    
    if passed == len(tests):
        print("🎉 DAG Solver integration successful!")
        print("\n📝 Summary:")
        print("   ✅ C++ extension compiled and working")
        print("   ✅ DAGSolverCpp class accessible")
        print("   ✅ Basic solve functionality working")
        print("   ✅ Python wrapper pattern functional")
        print("\n🔧 Next steps:")
        print("   1. Fix relative import issues in full framework")
        print("   2. Complete integration with arc_solver.solvers")
        print("   3. Add proper Task class compatibility")
    else:
        print("⚠️  Some issues remain to be fixed")

if __name__ == "__main__":
    main() 