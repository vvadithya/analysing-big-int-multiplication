# BigInt Multiplication Benchmark

A C++ research project to benchmark different BigInt multiplication algorithms, including parallel implementations.

## Prerequisites

- **C++20 Compiler**: `g++` (version 10+) or `clang++` (version 11+)
- **CMake**: 3.16 or higher
- **Boost Libraries**: For `Boost.Multiprecision` (header-only, but may require installing Boost)
- **Python**: 3.8+
- **Python Packages**: `pandas`, `matplotlib`, `reportlab`, `numpy`

## Quick Start

The easiest way to build, test, and run the entire benchmark pipeline is using the provided automation script:

```bash
chmod +x run_all.sh
./run_all.sh
```

## Manual Build Instructions

If you prefer to build the project manually:

```bash
# 1. Configure the project
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. Build the executables
cmake --build build -j $(nproc)

# 3. Run correctness tests
./build/test_correctness

# 4. Run the benchmarks
./build/bigint_benchmark --benchmark --config benchmarks/benchmark_config.json
```

## Project Structure

- `src/` - C++ source files containing the BigInt multiplication implementations.
- `include/` - C++ header files.
- `benchmarks/` - Benchmark configuration files (JSON).
- `scripts/` - Python scripts for statistical analysis, graphing, and report generation.
- `build/` - CMake build directory (generated).
- `results/` - Raw benchmark output data (generated).
- `report/` - Generated PDF reports and graphs (generated).

## Configuration Options

You can override certain build and run parameters in `run_all.sh` using environment variables:

- `BUILD_TYPE` (default: `Release`) - CMake build type (`Release`, `Debug`, etc.)
- `JOBS` (default: `$(nproc)`) - Number of parallel jobs for building.
- `SEED` (default: `42`) - Random seed for benchmarking.
- `ITERATIONS` (default: `5`) - Number of benchmark iterations.

Example:
```bash
BUILD_TYPE=Debug JOBS=4 ./run_all.sh
```

## Output Files

After a successful run, the following main outputs will be generated:
- `results/` - Raw CSV/JSON data from the C++ benchmarks.
- `report/` - Visualizations of the results (PNG) and the final `analysis.pdf` containing a comprehensive summary.

## Algorithms

- **Standard Multiplication**: Base $O(N^2)$ algorithm.
- **Karatsuba**: $O(N^{1.58})$ algorithm.
- **Parallel Implementations**: Multi-threaded variants utilizing parallel processing.


