// ============================================================================
// schoolbook.cpp — O(D²) schoolbook (grade-school) multiplication
// ============================================================================
// Each digit of 'a' is multiplied with each digit of 'b', accumulated in an
// intermediate result array, then carries are propagated in a single pass.
// ============================================================================

#include "schoolbook.hpp"

BigInt schoolbook_multiply(const BigInt& a, const BigInt& b, OperationStats& stats) {
    // Handle zero operands early
    if (a.is_zero() || b.is_zero()) {
        return BigInt();
    }

    const size_t na = a.num_digits();
    const size_t nb = b.num_digits();
    const size_t result_size = na + nb; // maximum possible digits in the product

    // Use uint32_t to safely accumulate partial products before carry normalization.
    // Maximum accumulated value at any position before normalization:
    //   At most min(na, nb) additions, each contributing at most 9*9 = 81,
    //   so max value ≈ min(na,nb)*81, well within uint32_t range.
    std::vector<uint32_t> result(result_size, 0);

    // ---- Multiply phase: O(na * nb) digit multiplications ----
    for (size_t i = 0; i < na; ++i) {
        for (size_t j = 0; j < nb; ++j) {
            uint32_t product = static_cast<uint32_t>(a.digits[i]) *
                               static_cast<uint32_t>(b.digits[j]);
            result[i + j] += product;

            // Instrumentation
            stats.multiplications++;
            stats.additions++;
        }
    }

    // ---- Carry normalization phase ----
    // Propagate carries so each position holds a single decimal digit (0–9).
    for (size_t pos = 0; pos < result_size; ++pos) {
        if (result[pos] >= 10) {
            uint32_t carry = result[pos] / 10;
            result[pos] %= 10;
            if (pos + 1 < result_size) {
                result[pos + 1] += carry;
            }
            stats.carry_operations++;
        }
    }

    // ---- Build the BigInt from the result array ----
    BigInt product;
    product.digits.resize(result_size);
    for (size_t i = 0; i < result_size; ++i) {
        product.digits[i] = static_cast<uint8_t>(result[i]);
    }

    product.normalize();
    return product;
}
