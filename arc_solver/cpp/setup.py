from setuptools import setup, Extension
import pybind11
from pybind11.setup_helpers import Pybind11Extension, build_ext

__version__ = "0.0.1"

# Define the extension module
ext_modules = [
    Pybind11Extension(
        "arc_solver_cpp",
        [
            "src/symmetry_solver.cpp",
            "src/chess_solver.cpp",
            "src/tiling_solver.cpp",
            "src/ml_solver.cpp",
            "bindings/bindings.cpp",
        ],
        include_dirs=[
            "include",
            pybind11.get_include(),
        ],
        language="c++",
        cxx_std=17,
        extra_compile_args=[
            "-O3",
            "-DVERSION_INFO=\"" + __version__ + "\"",
        ],
    ),
]

setup(
    name="arc_solver_cpp",
    version=__version__,
    author="ARC Solver Team",
    author_email="",
    url="",
    description="C++ optimized modules for ARC Solver",
    long_description="",
    ext_modules=ext_modules,
    extras_require={"test": "pytest"},
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.8",
    install_requires=[
        "numpy>=1.21.0",
    ],
) 