#pragma once

#include "bigint.hpp"
#include "statistics.hpp"
#include <chrono>
#include <functional>
#include <map>
#include <vector>
#include <string>
#include <fstream>

// ============================================================================
// Benchmark Configuration
// ============================================================================
struct BenchmarkConfig {
    std::vector<size_t> digit_sizes;     // e.g., {10, 50, 100, 500, 1000, ...}
    std::vector<int> chunk_sizes;         // 3..10
    std::vector<int> thread_counts;       // {1, 2, 4, 8, ...}
    int num_iterations = 5;               // repetitions per experiment
    uint64_t seed = 42;                   // for reproducibility
    std::string output_dir = "results";   // base output directory
    size_t max_schoolbook_digits = 50000; // skip schoolbook above this
};

// ============================================================================
// Benchmark Result (one measurement)
// ============================================================================
struct BenchmarkResult {
    std::string algorithm;       // "schoolbook", "chunked", "parallel_chunked", "karatsuba", "boost_cpp_int"
    int chunk_size;              // 0 for non-chunked algorithms
    size_t num_digits;           // input size in decimal digits
    int threads;                 // 1 for sequential algorithms
    int run;                     // iteration number (1-based)
    double time_ns;              // wall-clock time in nanoseconds
    OperationStats stats;        // operation counts

    // CSV serialization
    static std::string csv_header();
    std::string to_csv() const;
};

// ============================================================================
// Benchmark Runner
// ============================================================================

/// Load benchmark configuration from a JSON file.
BenchmarkConfig load_config(const std::string& config_path);

/// Run the complete benchmark matrix and write results to CSV.
void run_all_benchmarks(const BenchmarkConfig& config);

/// Save a vector of results to a CSV file.
void save_results_csv(const std::vector<BenchmarkResult>& results, const std::string& filepath);

/// Detect and return system/environment information as a formatted string.
std::string get_system_info();

/// Save system info to a file.
void save_system_info(const std::string& filepath);
