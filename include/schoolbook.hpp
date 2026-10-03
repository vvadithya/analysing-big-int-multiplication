#pragma once

#include "bigint.hpp"

/// Perform conventional (schoolbook) digit-by-digit multiplication.
/// This is the O(D²) baseline where D is the number of decimal digits.
///
/// Instruments: digit multiplications, additions, carry operations.
BigInt schoolbook_multiply(const BigInt& a, const BigInt& b, OperationStats& stats);
