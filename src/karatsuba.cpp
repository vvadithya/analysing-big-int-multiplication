// ============================================================================
// karatsuba.cpp — Karatsuba recursive multiplication for BigInt
//
// Implements the classic divide-and-conquer algorithm:
//   Given a = a1·10^m + a0 and b = b1·10^m + b0,
//     z0 = a0 * b0
//     z2 = a1 * b1
//     z1 = (a0 + a1)(b0 + b1) - z0 - z2
//     result = z2·10^(2m) + z1·10^m + z0
//
// Falls back to schoolbook multiplication for operands smaller than
// the configurable threshold (default 64 digits).
//
// Complexity: O(D^log₂3) ≈ O(D^1.585)
// ============================================================================

#include "karatsuba.hpp"
#include "schoolbook.hpp"

#include <algorithm>

// ----------------------------------------------------------------------------
// Internal recursive implementation with depth tracking
// ----------------------------------------------------------------------------
static BigInt karatsuba_impl(const BigInt& a, const BigInt& b,
                              OperationStats& stats, int threshold, int depth)
{
    // Track recursive call count and maximum depth reached
    stats.recursive_calls++;
    stats.max_recursion_depth = std::max(stats.max_recursion_depth, depth);

    // ----- Edge case: either operand is zero → result is zero -----
    if (a.is_zero() || b.is_zero()) {
        return BigInt(static_cast<uint64_t>(0));
    }

    // ----- Base case: fall back to schoolbook for small operands -----
    if (static_cast<int>(a.num_digits()) < threshold ||
        static_cast<int>(b.num_digits()) < threshold) {
        stats.base_case_multiplications++;
        return schoolbook_multiply(a, b, stats);
    }

    // ----- Divide step -----
    // Split point is half the maximum operand length
    size_t m = std::max(a.num_digits(), b.num_digits()) / 2;

    // a = a1 * 10^m + a0
    BigInt a0 = a.low(m);
    BigInt a1 = a.high(m);

    // b = b1 * 10^m + b0
    BigInt b0 = b.low(m);
    BigInt b1 = b.high(m);

    // ----- Three recursive multiplications (instead of four) -----

    // z0 = a0 * b0
    BigInt z0 = karatsuba_impl(a0, b0, stats, threshold, depth + 1);

    // z2 = a1 * b1
    BigInt z2 = karatsuba_impl(a1, b1, stats, threshold, depth + 1);

    // z1 = (a0 + a1)(b0 + b1) - z0 - z2
    BigInt sum_a = a0 + a1;
    stats.additions += std::max(a0.num_digits(), a1.num_digits());

    BigInt sum_b = b0 + b1;
    stats.additions += std::max(b0.num_digits(), b1.num_digits());

    BigInt z1_full = karatsuba_impl(sum_a, sum_b, stats, threshold, depth + 1);

    BigInt z1 = z1_full - z0;
    stats.subtractions += z1_full.num_digits();

    z1 = z1 - z2;
    stats.subtractions += z1.num_digits();

    // ----- Combine step -----
    // result = z0 + z1·10^m + z2·10^(2m)
    BigInt z1_shifted = z1.shift_left(m);
    BigInt z2_shifted = z2.shift_left(2 * m);

    BigInt result = z0 + z1_shifted;
    stats.additions += std::max(z0.num_digits(), z1_shifted.num_digits());

    result = result + z2_shifted;
    stats.additions += std::max(result.num_digits(), z2_shifted.num_digits());

    return result;
}

// ============================================================================
// Public API
// ============================================================================

BigInt karatsuba_multiply(const BigInt& a, const BigInt& b,
                           OperationStats& stats, int threshold)
{
    return karatsuba_impl(a, b, stats, threshold, /*depth=*/0);
}
