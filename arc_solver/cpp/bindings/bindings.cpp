#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "../include/symmetry_solver.hpp"
#include "../include/chess_solver.hpp"
#include "../include/tiling_solver.hpp"
#include "../include/ml_solver.hpp"
#include "../include/dag_solver.hpp"

namespace py = pybind11;

PYBIND11_MODULE(arc_solver_cpp, m) {
    m.doc() = "ARC Solver C++ optimized modules";

    py::class_<SymmetrySolverCpp>(m, "SymmetrySolverCpp")
        .def(py::init<>())
        .def("can_solve", &SymmetrySolverCpp::can_solve,
             "Check if the solver can solve the given task",
             py::arg("train_inputs"), py::arg("train_outputs"))
        .def("solve", &SymmetrySolverCpp::solve,
             "Solve the task and return predictions",
             py::arg("train_inputs"), py::arg("train_outputs"), py::arg("test_inputs"));

    py::class_<ChessSolverCpp>(m, "ChessSolverCpp")
        .def(py::init<>())
        .def("can_solve", &ChessSolverCpp::can_solve,
             "Check if the solver can solve chess pattern tasks",
             py::arg("train_inputs"), py::arg("train_outputs"))
        .def("solve", &ChessSolverCpp::solve,
             "Solve chess pattern tasks and return predictions",
             py::arg("train_inputs"), py::arg("train_outputs"), py::arg("test_inputs"));

    py::class_<TilingSolverCpp>(m, "TilingSolverCpp")
        .def(py::init<>())
        .def("can_solve", &TilingSolverCpp::can_solve,
             "Check if the solver can solve tiling pattern tasks",
             py::arg("train_inputs"), py::arg("train_outputs"))
        .def("solve", &TilingSolverCpp::solve,
             "Solve tiling pattern tasks and return predictions",
             py::arg("train_inputs"), py::arg("train_outputs"), py::arg("test_inputs"));

    py::class_<MLSolverCpp>(m, "MLSolverCpp")
        .def(py::init<>())
        .def("can_solve", &MLSolverCpp::can_solve,
             "Check if the solver can solve ML-based tasks",
             py::arg("train_inputs"), py::arg("train_outputs"))
        .def("solve", &MLSolverCpp::solve,
             "Solve ML-based tasks and return predictions",
             py::arg("train_inputs"), py::arg("train_outputs"), py::arg("test_inputs"));

    py::class_<arc_solver::DAGSolverCpp>(m, "DAGSolverCpp")
        .def(py::init<>())
        .def(py::init<const arc_solver::SolverConfig&>())
        .def("can_solve", &arc_solver::DAGSolverCpp::can_solve,
             "Check if the DAG solver can solve the given task",
             py::arg("train_inputs"), py::arg("train_outputs"))
        .def("solve", &arc_solver::DAGSolverCpp::solve,
             "Solve task using DAG-based search and return predictions",
             py::arg("train_inputs"), py::arg("train_outputs"), py::arg("test_inputs"))
        .def("get_available_functions", &arc_solver::DAGSolverCpp::getAvailableFunctions,
             "Get list of available transform functions");
} 