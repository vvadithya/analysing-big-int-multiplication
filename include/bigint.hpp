#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <random>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <cassert>
#include <cmath>

// ============================================================================
// Operation Statistics — used by all algorithms for instrumentation
// ============================================================================
struct OperationStats {
    uint64_t multiplications = 0;
    uint64_t additions = 0;
    uint64_t carry_operations = 0;
    uint64_t subtractions = 0;
    uint64_t recursive_calls = 0;
    uint64_t normalizations = 0;
    uint64_t reduction_operations = 0;
    uint64_t base_case_multiplications = 0;
    uint64_t local_accumulations = 0;
    int max_recursion_depth = 0;
    int thread_count = 0;
    uint64_t work_per_thread = 0;
    uint64_t num_chunks_a = 0;
    uint64_t num_chunks_b = 0;
    uint64_t chunk_multiplications = 0;

    void reset() {
        multiplications = 0;
        additions = 0;
        carry_operations = 0;
        subtractions = 0;
        recursive_calls = 0;
        normalizations = 0;
        reduction_operations = 0;
        base_case_multiplications = 0;
        local_accumulations = 0;
        max_recursion_depth = 0;
        thread_count = 0;
        work_per_thread = 0;
        num_chunks_a = 0;
        num_chunks_b = 0;
        chunk_multiplications = 0;
    }

    std::string summary() const {
        std::ostringstream oss;
        oss << "Operations: "
            << "muls=" << multiplications
            << " adds=" << additions
            << " carries=" << carry_operations
            << " subs=" << subtractions
            << " recursive=" << recursive_calls
            << " base_muls=" << base_case_multiplications
            << " norms=" << normalizations
            << " reductions=" << reduction_operations
            << " chunks_a=" << num_chunks_a
            << " chunks_b=" << num_chunks_b
            << " chunk_muls=" << chunk_multiplications
            << " max_depth=" << max_recursion_depth
            << " threads=" << thread_count;
        return oss.str();
    }

    // CSV header for operation stats
    static std::string csv_header() {
        return "multiplications,additions,carry_operations,subtractions,"
               "recursive_calls,base_case_multiplications,normalizations,"
               "reduction_operations,local_accumulations,"
               "num_chunks_a,num_chunks_b,chunk_multiplications,"
               "max_recursion_depth,thread_count,work_per_thread";
    }

    std::string to_csv() const {
        std::ostringstream oss;
        oss << multiplications << "," << additions << "," << carry_operations << ","
            << subtractions << "," << recursive_calls << "," << base_case_multiplications << ","
            << normalizations << "," << reduction_operations << "," << local_accumulations << ","
            << num_chunks_a << "," << num_chunks_b << "," << chunk_multiplications << ","
            << max_recursion_depth << "," << thread_count << "," << work_per_thread;
        return oss.str();
    }
};

// ============================================================================
// BigInt — Arbitrary-precision positive integer using decimal digit storage
// ============================================================================
class BigInt {
public:
    // Digits stored in least-significant-first order, each element is 0–9.
    std::vector<uint8_t> digits;

    // ---- Constructors ----
    BigInt();
    explicit BigInt(const std::string& decimal_str);
    explicit BigInt(uint64_t value);

    // ---- Conversions ----
    /// Convert to decimal string
    std::string to_string() const;

    // ---- Queries ----
    bool is_zero() const;
    size_t num_digits() const;

    // ---- Normalization ----
    /// Remove trailing zeros in internal (LSB-first) representation,
    /// ensuring at least one digit remains.
    void normalize();

    // ---- Chunk conversion ----
    /// Convert BigInt to chunked representation (LSB-first chunks).
    /// Each chunk is in range [0, 10^chunk_size).
    static std::vector<uint64_t> to_chunks(const BigInt& num, int chunk_size);

    /// Reconstruct BigInt from chunked representation.
    static BigInt from_chunks(const std::vector<uint64_t>& chunks, int chunk_size);

    // ---- Random generation ----
    /// Generate a random BigInt with exactly num_digits decimal digits.
    static BigInt random(size_t num_digits, std::mt19937_64& rng);

    /// Generate a random decimal string with exactly num_digits digits.
    static std::string random_decimal_string(size_t num_digits, std::mt19937_64& rng);

    // ---- Arithmetic operators (needed for Karatsuba) ----
    BigInt operator+(const BigInt& other) const;
    BigInt operator-(const BigInt& other) const;

    // ---- Comparison operators ----
    bool operator==(const BigInt& other) const;
    bool operator!=(const BigInt& other) const;
    bool operator<(const BigInt& other) const;
    bool operator>(const BigInt& other) const;
    bool operator<=(const BigInt& other) const;
    bool operator>=(const BigInt& other) const;

    // ---- Shift (multiply by 10^k) ----
    /// Shift left by k decimal digit positions (equivalent to *10^k).
    BigInt shift_left(size_t k) const;

    // ---- Slicing ----
    /// Get lower k digits (mod 10^k).
    BigInt low(size_t k) const;
    /// Get upper digits (div 10^k).
    BigInt high(size_t k) const;
};

// Utility: compute 10^n as uint64_t (for chunk bases)
inline uint64_t power_of_10(int n) {
    uint64_t result = 1;
    for (int i = 0; i < n; ++i) {
        result *= 10;
    }
    return result;
}
