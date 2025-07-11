from setuptools import setup, find_packages
import os
import sys

# Check if we're on a platform that supports C++ compilation
def get_cpp_requirements():
    """Get C++ related requirements if available."""
    cpp_reqs = []
    if os.name != 'nt':  # Not Windows
        cpp_reqs.extend([
            "pybind11>=2.10.0",
            "numpy>=1.21.0",  # Required for pybind11
        ])
    return cpp_reqs

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

with open("requirements.txt", "r", encoding="utf-8") as fh:
    requirements = [line.strip() for line in fh if line.strip() and not line.startswith("#")]

# Add C++ requirements
requirements.extend(get_cpp_requirements())

setup(
    name="arc-solver",
    version="1.0.0",
    author="ARC Solver Team",
    author_email="contact@arc-solver.com",
    description="2024 Kaggle ARC Competition Silver Medal Solution - High-performance multi-strategy solver with C++ optimizations",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/arc-solver/arc-solver",
    packages=find_packages(),
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "Topic :: Scientific/Engineering :: Image Processing",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],
    python_requires=">=3.8",
    install_requires=requirements,
    extras_require={
        "dev": [
            "pytest>=6.2.0",
            "pytest-cov>=2.12.0",
            "black>=21.0.0",
            "flake8>=3.9.0",
            "mypy>=0.910",
        ],
        "cpp": [
            "pybind11>=2.10.0",
            "cmake>=3.15.0",
        ],
    },
    entry_points={
        "console_scripts": [
            "arc-solver=arc_solver.cli:main",
        ],
    },
    include_package_data=True,
    package_data={
        "arc_solver": ["*.json", "*.txt", "*.md"],
    },
    keywords=[
        "arc", "abstraction", "reasoning", "artificial-intelligence", 
        "machine-learning", "pattern-recognition", "kaggle", "competition"
    ],
    project_urls={
        "Bug Reports": "https://github.com/arc-solver/arc-solver/issues",
        "Source": "https://github.com/arc-solver/arc-solver",
        "Documentation": "https://github.com/arc-solver/arc-solver#readme",
        "Kaggle Competition": "https://www.kaggle.com/competitions/abstraction-and-reasoning-challenge",
    },
) 