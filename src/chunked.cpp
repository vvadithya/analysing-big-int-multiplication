// ============================================================================
// chunked.cpp — Chunked (schoolbook at chunk level) multiplication
//
// Splits two BigInt operands into chunks of `chunk_size` decimal digits,
// treating each chunk as a single digit in base B = 10^chunk_size.
// Performs schoolbook O(n*m) multiplication on the chunk arrays, then
// normalizes carries and converts back to a BigInt.
//
// Uses unsigned __int128 for all intermediate accumulations so that
// chunk_size values up to 10 (and large fan-in accumulations) are safe.
// ============================================================================

#include "chunked.hpp"

#include <vector>
#include <cstdint>

BigInt chunked_multiply(const BigInt& a, const BigInt& b, int chunk_size, OperationStats& stats) {
    // ------------------------------------------------------------------
    // 0. Trivial cases
    // ------------------------------------------------------------------
    if (a.is_zero() || b.is_zero()) {
        return BigInt(static_cast<uint64_t>(0));
    }

    // ------------------------------------------------------------------
    // 1. Compute the chunk base  B = 10^chunk_size
    // ------------------------------------------------------------------
    const uint64_t BASE = power_of_10(chunk_size);
    // We keep a __int128 copy for carry normalisation arithmetic.
    const unsigned __int128 BASE128 = static_cast<unsigned __int128>(BASE);

    // ------------------------------------------------------------------
    // 2. Convert operands to chunks (LSB-first, each < BASE)
    // ------------------------------------------------------------------
    std::vector<uint64_t> chunks_a = BigInt::to_chunks(a, chunk_size);
    std::vector<uint64_t> chunks_b = BigInt::to_chunks(b, chunk_size);

    const size_t na = chunks_a.size();
    const size_t nb = chunks_b.size();

    // Record chunk counts for statistics
    stats.num_chunks_a = na;
    stats.num_chunks_b = nb;

    // ------------------------------------------------------------------
    // 3. Allocate result buffer (unsigned __int128 to handle overflow)
    //    Maximum result length = na + nb positions.
    // ------------------------------------------------------------------
    const size_t result_len = na + nb;
    std::vector<unsigned __int128> result(result_len, 0);

    // ------------------------------------------------------------------
    // 4. Schoolbook multiplication on chunks
    //    result[i + j] += chunks_a[i] * chunks_b[j]
    // ------------------------------------------------------------------
    for (size_t i = 0; i < na; ++i) {
        for (size_t j = 0; j < nb; ++j) {
            unsigned __int128 product =
                static_cast<unsigned __int128>(chunks_a[i]) *
                static_cast<unsigned __int128>(chunks_b[j]);
            result[i + j] += product;

            stats.chunk_multiplications++;
            stats.additions++;
        }
    }

    // ------------------------------------------------------------------
    // 5. Carry normalisation — propagate through the entire buffer
    //    After this pass every result[k] < BASE.
    // ------------------------------------------------------------------
    for (size_t k = 0; k < result_len; ++k) {
        unsigned __int128 carry = result[k] / BASE128;
        result[k] %= BASE128;

        if (carry > 0) {
            // If we're not at the very last slot, propagate upward.
            // (The buffer is already sized na+nb, which is one larger
            //  than the maximum significant index, so k+1 < result_len
            //  whenever a non-zero carry can appear.)
            if (k + 1 < result_len) {
                result[k + 1] += carry;
            }
            stats.carry_operations++;
        }
        stats.normalizations++;
    }

    // ------------------------------------------------------------------
    // 6. Convert the __int128 buffer down to vector<uint64_t>
    //    (each slot is now < BASE, so it fits comfortably in uint64_t)
    // ------------------------------------------------------------------
    std::vector<uint64_t> result_u64(result_len);
    for (size_t k = 0; k < result_len; ++k) {
        result_u64[k] = static_cast<uint64_t>(result[k]);
    }

    // ------------------------------------------------------------------
    // 7. Reconstruct the BigInt and normalise away leading zeros
    // ------------------------------------------------------------------
    BigInt out = BigInt::from_chunks(result_u64, chunk_size);
    out.normalize();
    return out;
}
