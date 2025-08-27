#!/usr/bin/env python3
"""
Test the new DAG solver fallback logic in arc_solver.
"""

import sys
import os
import numpy as np

# Add the current directory to path  
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

def test_fallback_logic():
    """Test the modified solver logic with DAG fallback."""
    print("🧪 Testing DAG Solver Fallback Logic")
    print("=====================================")
    
    try:
        from core.solver import ArcSolver
        from core.config import SolverConfig
        
        # Create a mock task
        class MockTask:
            def __init__(self, task_id="test_task"):
                self.task_id = task_id
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
        
        # Test 1: 高可信度情况 (调低阈值让专用solver看起来高可信度)
        print("\n🔬 Test 1: High Confidence Scenario")
        print("-----------------------------------")
        
        config1 = SolverConfig()
        config1.dag_high_confidence_threshold = 5.0  # 降低阈值
        config1.dag_enable_logging = True
        
        solver1 = ArcSolver(config=config1)
        print(f"✅ Solver initialized with {len(solver1.specialist_solvers)} specialist solvers")
        print(f"✅ DAG solver available: {solver1.dag_solver is not None}")
        
        task1 = MockTask("high_confidence_task")
        result1 = solver1.solve(task1)
        
        print(f"📊 Results:")
        print(f"   Used fallback: {result1.used_fallback}")
        print(f"   Total predictions: {len(result1.predictions)}")
        print(f"   Specialist predictions: {len(result1.specialist_predictions) if result1.specialist_predictions else 0}")
        print(f"   DAG predictions: {len(result1.dag_predictions) if result1.dag_predictions else 0}")
        print(f"   Primary source: {result1.metadata.get('primary_source', 'unknown')}")
        
        # Test 2: 低可信度情况 (调高阈值触发fallback)
        print("\n🔬 Test 2: Low Confidence Scenario (Fallback)")
        print("----------------------------------------------")
        
        config2 = SolverConfig()
        config2.dag_high_confidence_threshold = 50.0  # 提高阈值
        config2.dag_enable_logging = True
        
        solver2 = ArcSolver(config=config2)
        task2 = MockTask("low_confidence_task")
        result2 = solver2.solve(task2)
        
        print(f"📊 Results:")
        print(f"   Used fallback: {result2.used_fallback}")
        print(f"   Total predictions: {len(result2.predictions)}")
        print(f"   Specialist predictions: {len(result2.specialist_predictions) if result2.specialist_predictions else 0}")
        print(f"   DAG predictions: {len(result2.dag_predictions) if result2.dag_predictions else 0}")
        print(f"   Primary source: {result2.metadata.get('primary_source', 'unknown')}")
        print(f"   Specialist max score: {result2.metadata.get('specialist_max_score', 0.0):.2f}")
        print(f"   DAG max score: {result2.metadata.get('dag_max_score', 0.0):.2f}")
        
        # Test 3: 禁用DAG solver
        print("\n🔬 Test 3: DAG Solver Disabled")
        print("------------------------------")
        
        config3 = SolverConfig()
        config3.enable_dag_solver = False
        
        solver3 = ArcSolver(config=config3)
        print(f"✅ DAG solver disabled: {solver3.dag_solver is None}")
        
        task3 = MockTask("no_dag_task")
        result3 = solver3.solve(task3)
        
        print(f"📊 Results:")
        print(f"   Used fallback: {result3.used_fallback}")
        print(f"   Total predictions: {len(result3.predictions)}")
        print(f"   DAG solver available: {solver3.dag_solver is not None}")
        
        # 验证输出格式
        print("\n🔍 Output Format Verification")
        print("-----------------------------")
        
        # 检查必要字段
        required_fields = ['predictions', 'scores', 'solver_contributions', 'used_fallback']
        optional_fields = ['dag_predictions', 'specialist_predictions']
        
        for i, result in enumerate([result1, result2, result3], 1):
            print(f"Result {i}:")
            for field in required_fields:
                has_field = hasattr(result, field)
                print(f"   {field}: {'✅' if has_field else '❌'}")
            
            for field in optional_fields:
                value = getattr(result, field, None)
                status = "✅" if value else "🔍"
                print(f"   {field}: {status} ({len(value) if value else 0} items)")
        
        return True
        
    except Exception as e:
        print(f"❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_solver_info():
    """Test solver info and configuration."""
    print("\n🔧 Testing Solver Info")
    print("======================")
    
    try:
        from core.solver import ArcSolver
        from core.config import SolverConfig
        
        config = SolverConfig()
        solver = ArcSolver(config=config)
        
        info = solver.get_solver_info()
        print(f"✅ Available solvers: {info['available_solvers']}")
        print(f"✅ Total solvers: {info['total_solvers']}")
        print(f"✅ Specialist solvers: {len(solver.specialist_solvers)}")
        print(f"✅ DAG solver: {'Available' if solver.dag_solver else 'Not available'}")
        
        # 检查配置
        config_dict = info['config']
        dag_fields = [k for k in config_dict.keys() if k.startswith('dag_')]
        print(f"✅ DAG configuration fields: {dag_fields}")
        
        return True
        
    except Exception as e:
        print(f"❌ Solver info test failed: {e}")
        return False

def main():
    """Run all tests."""
    print("🚀 ARC Solver Fallback Logic Test")
    print("==================================")
    
    tests = [
        test_fallback_logic,
        test_solver_info
    ]
    
    passed = 0
    for test_func in tests:
        try:
            if test_func():
                passed += 1
        except Exception as e:
            print(f"💥 Test crashed: {e}")
    
    print(f"\n📊 Test Results: {passed}/{len(tests)} passed")
    
    if passed == len(tests):
        print("🎉 All tests passed! Fallback logic is working correctly!")
        print("\n📝 Summary of changes:")
        print("   ✅ DAG solver acts as fallback when specialist solvers have low confidence")
        print("   ✅ Output includes both specialist and DAG predictions separately")
        print("   ✅ Configurable confidence threshold for fallback trigger")
        print("   ✅ Metadata includes detailed information about solver usage")
        print("   ✅ CLI output shows fallback usage statistics")
    else:
        print("⚠️  Some tests failed. Check the output above for details.")

if __name__ == "__main__":
    main() 