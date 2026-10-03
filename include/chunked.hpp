#pragma once

#include "bigint.hpp"

/// Perform chunked multiplication with a given chunk_size (n = 3..10).
///
/// Splits operands into chunks of `chunk_size` decimal digits, each chunk
/// treated as a single "digit" in base 10^chunk_size, then performs
/// schoolbook-style multiplication at the chunk level.
///
/// Complexity: approximately O((D/n)²) chunk-level multiplications.
///
/// Instruments: num_chunks_a, num_chunks_b, chunk_multiplications,
///              additions, carry_operations, normalizations.
BigInt chunked_multiply(const BigInt& a, const BigInt& b, int chunk_size, OperationStats& stats);
