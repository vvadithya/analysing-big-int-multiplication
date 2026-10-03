#include "benchmark.hpp"
#include "chunked.hpp"
#include "karatsuba.hpp"
#include "parallel_chunked.hpp"
#include "schoolbook.hpp"
#include <boost/multiprecision/cpp_int.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <filesystem>
#include <regex>

using boost::multiprecision::cpp_int;

std::string BenchmarkResult::csv_header() {
    return "algorithm,chunk_size,digits,threads,run,time_ns," + OperationStats::csv_header();
}

std::string BenchmarkResult::to_csv() const {
    std::ostringstream oss;
    oss << algorithm << ","
        << chunk_size << ","
        << num_digits << ","
        << threads << ","
        << run << ","
        << std::fixed << time_ns << ","
        << stats.to_csv();
    return oss.str();
}

// Simple JSON parser for the specific format
BenchmarkConfig load_config(const std::string& config_path) {
    BenchmarkConfig config;
    std::ifstream file(config_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + config_path);
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    auto extract_ints = [&](const std::string& key) {
        std::vector<int> res;
        std::regex r("\"" + key + "\"\\s*:\\s*\\[(.*?)\\]");
        std::smatch match;
        if (std::regex_search(content, match, r)) {
            std::string arr = match[1];
            std::regex r_int("\\d+");
            auto begin = std::sregex_iterator(arr.begin(), arr.end(), r_int);
            auto end = std::sregex_iterator();
            for (auto it = begin; it != end; ++it) {
                res.push_back(std::stoi(it->str()));
            }
        }
        return res;
    };
    
    auto extract_size_ts = [&](const std::string& key) {
        std::vector<size_t> res;
        std::regex r("\"" + key + "\"\\s*:\\s*\\[(.*?)\\]");
        std::smatch match;
        if (std::regex_search(content, match, r)) {
            std::string arr = match[1];
            std::regex r_int("\\d+");
            auto begin = std::sregex_iterator(arr.begin(), arr.end(), r_int);
            auto end = std::sregex_iterator();
            for (auto it = begin; it != end; ++it) {
                res.push_back(std::stoull(it->str()));
            }
        }
        return res;
    };

    auto extract_int_val = [&](const std::string& key, auto& val) {
        std::regex r("\"" + key + "\"\\s*:\\s*(\\d+)");
        std::smatch match;
        if (std::regex_search(content, match, r)) {
            val = std::stoull(match[1]);
        }
    };

    auto extract_string_val = [&](const std::string& key, std::string& val) {
        std::regex r("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch match;
        if (std::regex_search(content, match, r)) {
            val = match[1];
        }
    };

    config.digit_sizes = extract_size_ts("digit_sizes");
    config.chunk_sizes = extract_ints("chunk_sizes");
    config.thread_counts = extract_ints("thread_counts");
    extract_int_val("num_iterations", config.num_iterations);
    extract_int_val("seed", config.seed);
    extract_string_val("output_dir", config.output_dir);
    extract_int_val("max_schoolbook_digits", config.max_schoolbook_digits);

    return config;
}

std::string get_system_info() {
    std::ostringstream info;
    info << "System Information:\n";
    info << "Logical CPUs: " << std::thread::hardware_concurrency() << "\n";
#ifdef __linux__
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") == 0) {
            info << "CPU Model: " << line.substr(line.find(':') + 2) << "\n";
            break;
        }
    }
    
    std::ifstream os_release("/etc/os-release");
    while (std::getline(os_release, line)) {
        if (line.find("PRETTY_NAME=") == 0) {
            info << "OS: " << line.substr(13, line.length() - 14) << "\n";
            break;
        }
    }
#endif
    return info.str();
}

void save_system_info(const std::string& filepath) {
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << get_system_info();
    }
}

void save_results_csv(const std::vector<BenchmarkResult>& results, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return;

    file << BenchmarkResult::csv_header() << "\n";
    for (const auto& res : results) {
        file << res.to_csv() << "\n";
    }
}

void run_all_benchmarks(const BenchmarkConfig& config) {
    std::filesystem::create_directories(config.output_dir + "/raw");
    save_system_info(config.output_dir + "/system_info.txt");

    std::mt19937_64 rng(config.seed);
    std::vector<BenchmarkResult> all_results;

    auto time_it = [](auto func, double& time_ns) {
        auto start = std::chrono::steady_clock::now();
        func();
        auto end = std::chrono::steady_clock::now();
        time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    };

    for (size_t digits : config.digit_sizes) {
        std::cout << "Benchmarking " << digits << " digits...\n";
        
        for (int iter = 1; iter <= config.num_iterations; ++iter) {
            BigInt a = BigInt::random(digits, rng);
            BigInt b = BigInt::random(digits, rng);

            // 1. Schoolbook
            if (digits <= config.max_schoolbook_digits) {
                OperationStats stats;
                double time_ns = 0;
                time_it([&]() { schoolbook_multiply(a, b, stats); }, time_ns);
                all_results.push_back({"schoolbook", 0, digits, 1, iter, time_ns, stats});
            }

            // 2. Karatsuba
            {
                OperationStats stats;
                double time_ns = 0;
                time_it([&]() { karatsuba_multiply(a, b, stats); }, time_ns);
                all_results.push_back({"karatsuba", 0, digits, 1, iter, time_ns, stats});
            }

            // 3. Chunked
            for (int chunk_size : config.chunk_sizes) {
                OperationStats stats;
                double time_ns = 0;
                time_it([&]() { chunked_multiply(a, b, chunk_size, stats); }, time_ns);
                all_results.push_back({"chunked", chunk_size, digits, 1, iter, time_ns, stats});
            }

            // 4. Parallel Chunked
            for (int chunk_size : config.chunk_sizes) {
                for (int threads : config.thread_counts) {
                    OperationStats stats;
                    double time_ns = 0;
                    time_it([&]() { parallel_chunked_multiply(a, b, chunk_size, threads, stats); }, time_ns);
                    all_results.push_back({"parallel_chunked", chunk_size, digits, threads, iter, time_ns, stats});
                }
            }

            // 5. Boost cpp_int
            {
                cpp_int ba(a.to_string());
                cpp_int bb(b.to_string());
                double time_ns = 0;
                cpp_int result;
                time_it([&]() { result = ba * bb; }, time_ns);
                OperationStats stats; // empty stats
                all_results.push_back({"boost_cpp_int", 0, digits, 1, iter, time_ns, stats});
            }
        }
    }

    save_results_csv(all_results, config.output_dir + "/raw/benchmark_results.csv");
    std::cout << "Benchmarking complete. Results saved to " << config.output_dir << "\n";
}
