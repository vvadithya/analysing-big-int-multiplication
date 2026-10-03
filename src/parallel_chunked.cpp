// ============================================================================
// parallel_chunked.cpp — Parallel chunked multiplication implementation
//
// Strategy: Each thread computes partial products into a PRIVATE buffer,
// then a single sequential reduction phase sums them into the final result.
// No mutexes or atomics are needed during computation.
// ============================================================================

#include "parallel_chunked.hpp"

#include <thread>
#include <vector>
#include <algorithm>
#include <cstdint>

BigInt parallel_chunked_multiply(const BigInt& a, const BigInt& b,
                                 int chunk_size, int num_threads,
                                 OperationStats& stats) {
    stats.reset();

    // --- Edge case: either operand is zero ---
    if (a.is_zero() || b.is_zero()) {
        stats.thread_count = 0;
        return BigInt(uint64_t(0));
    }

    // --- Convert operands to chunked representation (LSB-first) ---
    std::vector<uint64_t> chunks_a = BigInt::to_chunks(a, chunk_size);
    std::vector<uint64_t> chunks_b = BigInt::to_chunks(b, chunk_size);

    const size_t num_chunks_a = chunks_a.size();
    const size_t num_chunks_b = chunks_b.size();
    const size_t result_size  = num_chunks_a + num_chunks_b;

    // Compute the base for this chunk size (10^chunk_size)
    const uint64_t BASE = power_of_10(chunk_size);

    // --- Record chunk stats ---
    stats.num_chunks_a        = num_chunks_a;
    stats.num_chunks_b        = num_chunks_b;
    stats.chunk_multiplications = num_chunks_a * num_chunks_b;

    // --- Determine actual thread count ---
    // Never use more threads than there are chunks in A (each thread needs work).
    int actual_threads = std::max(1, std::min(num_threads,
                                              static_cast<int>(num_chunks_a)));

    // --- Single-threaded fast path (avoid thread creation overhead) ---
    if (actual_threads <= 1) {
        actual_threads = 1;
        stats.thread_count    = 1;
        stats.work_per_thread = stats.chunk_multiplications;
        stats.local_accumulations = stats.chunk_multiplications;

        // Accumulate directly into the final buffer
        std::vector<unsigned __int128> result(result_size, 0);

        for (size_t i = 0; i < num_chunks_a; ++i) {
            for (size_t j = 0; j < num_chunks_b; ++j) {
                result[i + j] +=
                    static_cast<unsigned __int128>(chunks_a[i]) * chunks_b[j];
                ++stats.multiplications;
                ++stats.additions;
            }
        }

        // --- Normalize carries ---
        for (size_t k = 0; k < result_size; ++k) {
            if (result[k] >= BASE) {
                unsigned __int128 carry = result[k] / BASE;
                result[k] %= BASE;
                if (k + 1 < result_size) {
                    result[k + 1] += carry;
                }
                ++stats.carry_operations;
            }
        }

        // Convert result chunks (as uint64_t) back to BigInt
        std::vector<uint64_t> final_chunks(result_size);
        for (size_t k = 0; k < result_size; ++k) {
            final_chunks[k] = static_cast<uint64_t>(result[k]);
        }

        return BigInt::from_chunks(final_chunks, chunk_size);
    }

    // --- Multi-threaded path ---
    stats.thread_count    = actual_threads;
    stats.work_per_thread = stats.chunk_multiplications / actual_threads;
    stats.local_accumulations = stats.chunk_multiplications;

    const size_t chunk_per_thread = num_chunks_a / actual_threads;

    // Each thread writes to its own local buffer — no contention.
    std::vector<std::thread> threads;
    threads.reserve(actual_threads);
    std::vector<std::vector<unsigned __int128>> local_results(actual_threads);

    for (int t = 0; t < actual_threads; ++t) {
        size_t start = t * chunk_per_thread;
        size_t end   = (t == actual_threads - 1)
                           ? num_chunks_a
                           : (t + 1) * chunk_per_thread;

        threads.emplace_back([&, t, start, end]() {
            // Private accumulator for this thread
            local_results[t].resize(result_size, 0);

            for (size_t i = start; i < end; ++i) {
                for (size_t j = 0; j < num_chunks_b; ++j) {
                    local_results[t][i + j] +=
                        static_cast<unsigned __int128>(chunks_a[i]) * chunks_b[j];
                }
            }
        });
    }

    // Wait for all threads to finish their computation phase
    for (auto& th : threads) {
        th.join();
    }

    // --- Count multiplications (done after threads complete) ---
    stats.multiplications = stats.chunk_multiplications;
    stats.additions       = stats.chunk_multiplications;

    // --- Sequential reduction: sum all local buffers into the final result ---
    std::vector<unsigned __int128> result(result_size, 0);
    uint64_t reduction_ops = 0;

    for (int t = 0; t < actual_threads; ++t) {
        for (size_t k = 0; k < result_size; ++k) {
            result[k] += local_results[t][k];
            ++reduction_ops;
        }
    }
    stats.reduction_operations = reduction_ops;

    // --- Normalize carries across the result ---
    for (size_t k = 0; k < result_size; ++k) {
        if (result[k] >= static_cast<unsigned __int128>(BASE)) {
            unsigned __int128 carry = result[k] / BASE;
            result[k] %= BASE;
            if (k + 1 < result_size) {
                result[k + 1] += carry;
            }
            ++stats.carry_operations;
        }
    }

    // --- Convert to uint64_t chunks and reconstruct BigInt ---
    std::vector<uint64_t> final_chunks(result_size);
    for (size_t k = 0; k < result_size; ++k) {
        final_chunks[k] = static_cast<uint64_t>(result[k]);
    }

    return BigInt::from_chunks(final_chunks, chunk_size);
}
