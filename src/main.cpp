#include "benchmark.hpp"
#include <iostream>
#include <string>
#include <cstdlib>

int main(int argc, char** argv) {
    std::string config_path = "benchmarks/benchmark_config.json";
    bool run_tests = false;
    bool run_bench = false;
    bool print_info = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        } else if (arg == "--test") {
            run_tests = true;
        } else if (arg == "--benchmark") {
            run_bench = true;
        } else if (arg == "--all") {
            run_tests = true;
            run_bench = true;
        } else if (arg == "--info") {
            print_info = true;
        }
    }
    
    if (!run_tests && !run_bench && !print_info) {
        run_tests = true;
        run_bench = true;
    }

    if (print_info || run_bench) {
        std::cout << get_system_info() << "\n";
        if (print_info && !run_bench && !run_tests) return 0;
    }

    if (run_tests) {
        std::cout << "Running tests...\n";
        int ret = std::system("./test_correctness");
        if (ret != 0) {
            std::cerr << "Tests failed.\n";
            return 1;
        }
    }

    if (run_bench) {
        std::cout << "Running benchmarks...\n";
        try {
            BenchmarkConfig config = load_config(config_path);
            run_all_benchmarks(config);
        } catch (const std::exception& e) {
            std::cerr << "Error running benchmarks: " << e.what() << "\n";
            return 1;
        }
    }
    
    return 0;
}
