#include "benchmark.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::string config_path = "benchmarks/benchmark_config.json";
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "--config" && argc > 2) {
            config_path = argv[2];
        }
    }
    
    try {
        BenchmarkConfig config = load_config(config_path);
        run_all_benchmarks(config);
    } catch (const std::exception& e) {
        std::cerr << "Error running benchmarks: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
