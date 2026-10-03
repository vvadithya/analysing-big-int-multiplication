#include "bigint.hpp"
#include "schoolbook.hpp"
#include "karatsuba.hpp"
#include "chunked.hpp"
#include "parallel_chunked.hpp"
#include <boost/multiprecision/cpp_int.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <random>

using boost::multiprecision::cpp_int;

bool test_multiplication(const std::string& a_str, const std::string& b_str, const std::string& test_name) {
    BigInt a(a_str);
    BigInt b(b_str);
    cpp_int ba(a_str);
    cpp_int bb(b_str);
    cpp_int expected = ba * bb;
    std::string expected_str = expected.str();

    std::cout << "Running test: " << test_name << "... ";

    OperationStats stats;
    BigInt res_schoolbook = schoolbook_multiply(a, b, stats);
    if (res_schoolbook.to_string() != expected_str) {
        std::cout << "FAIL (schoolbook)\n";
        return false;
    }

    BigInt res_karatsuba = karatsuba_multiply(a, b, stats);
    if (res_karatsuba.to_string() != expected_str) {
        std::cout << "FAIL (karatsuba)\n";
        return false;
    }

    for (int chunk_size = 3; chunk_size <= 10; ++chunk_size) {
        BigInt res_chunked = chunked_multiply(a, b, chunk_size, stats);
        if (res_chunked.to_string() != expected_str) {
            std::cout << "FAIL (chunked, size " << chunk_size << ")\n";
            return false;
        }

        for (int threads : {1, 2, 4}) {
            BigInt res_parallel = parallel_chunked_multiply(a, b, chunk_size, threads, stats);
            if (res_parallel.to_string() != expected_str) {
                std::cout << "FAIL (parallel chunked, size " << chunk_size << ", threads " << threads << ")\n";
                return false;
            }
        }
    }

    std::cout << "PASS\n";
    return true;
}

int main() {
    std::mt19937_64 rng(42);
    
    std::vector<std::pair<std::pair<std::string, std::string>, std::string>> tests = {
        {{"0", "12345"}, "Zero * anything"},
        {{"1", "12345"}, "One * anything"},
        {{"123", "456"}, "Small known products"},
        {{"1000", "10000"}, "Powers of 10"},
        {{"99999", "99999"}, "Repeated digits"},
        {{"12", "123456789"}, "Different operand lengths"}
    };
    
    for (const auto& t : tests) {
        if (!test_multiplication(t.first.first, t.first.second, t.second)) return 1;
    }

    auto run_random_test = [&](size_t size_a, size_t size_b, const std::string& name) {
        std::string sa = BigInt::random_decimal_string(size_a, rng);
        std::string sb = BigInt::random_decimal_string(size_b, rng);
        if (!test_multiplication(sa, sb, name)) exit(1);
    };

    run_random_test(10, 10, "Random small (10 digits)");
    run_random_test(100, 100, "Random medium (100 digits)");
    run_random_test(1000, 1000, "Random large (1000 digits)");
    run_random_test(10000, 10000, "Random very large (10000 digits)");
    run_random_test(18, 18, "Values around chunk boundaries (18)");
    run_random_test(19, 19, "Values around chunk boundaries (19)");
    run_random_test(3, 1000, "Highly unequal sizes (3 digits * 1000 digits)");

    std::cout << "All tests passed successfully.\n";
    return 0;
}
