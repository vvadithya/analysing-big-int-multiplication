#pragma once

#include "bigint.hpp"

/// Perform parallel chunked multiplication.
///
/// Each thread accumulates into a private result buffer, followed by
/// a reduction into the final result. Uses range partitioning of the
/// outer loop (chunks of operand A).
///
/// Thread pool is reused across calls for efficiency.
///
/// Instruments: thread_count, work_per_thread, chunk_multiplications,
///              additions, carry_operations, reduction_operations,
///              local_accumulations.
BigInt parallel_chunked_multiply(const BigInt& a, const BigInt& b,
                                 int chunk_size, int num_threads,
                                 OperationStats& stats);
