#pragma once

#include "bigint.hpp"

/// Perform Karatsuba recursive multiplication.
///
/// Uses the divide-and-conquer identity:
///   z0 = x0 * y0
///   z2 = x1 * y1
///   z1 = (x0+x1)(y0+y1) - z0 - z2
///   result = z2 * B^(2m) + z1 * B^m + z0
///
/// Falls back to schoolbook multiplication when both operands have
/// fewer than `threshold` digits.
///
/// Complexity: O(D^log2(3)) ≈ O(D^1.585)
///
/// Instruments: recursive_calls, base_case_multiplications, additions,
///              subtractions, max_recursion_depth.
BigInt karatsuba_multiply(const BigInt& a, const BigInt& b,
                          OperationStats& stats, int threshold = 64);
