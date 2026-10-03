// ============================================================================
// bigint.cpp — Implementation of BigInt arbitrary-precision positive integer
// ============================================================================
// Digits are stored in least-significant-first (LSB-first) order.
// Each element of the digits vector is a single decimal digit (0–9).
// ============================================================================

#include "bigint.hpp"

// ============================================================================
// Constructors
// ============================================================================

/// Default constructor: creates zero (single digit 0).
BigInt::BigInt() : digits{0} {}

/// Construct from a decimal string.
/// Leading zeros are stripped. An empty or all-zero string yields zero.
BigInt::BigInt(const std::string& decimal_str) {
    if (decimal_str.empty()) {
        digits = {0};
        return;
    }

    // Find the first non-zero digit to skip leading zeros
    size_t start = 0;
    while (start < decimal_str.size() && decimal_str[start] == '0') {
        ++start;
    }

    if (start == decimal_str.size()) {
        // Entire string was zeros (or empty after stripping)
        digits = {0};
        return;
    }

    // Store digits in LSB-first order: iterate the string from the end
    digits.resize(decimal_str.size() - start);
    for (size_t i = 0; i < digits.size(); ++i) {
        char c = decimal_str[decimal_str.size() - 1 - i];
        if (c < '0' || c > '9') {
            throw std::invalid_argument("BigInt: non-digit character in input string");
        }
        digits[i] = static_cast<uint8_t>(c - '0');
    }
}

/// Construct from a 64-bit unsigned integer.
BigInt::BigInt(uint64_t value) {
    if (value == 0) {
        digits = {0};
        return;
    }
    digits.clear();
    while (value > 0) {
        digits.push_back(static_cast<uint8_t>(value % 10));
        value /= 10;
    }
}

// ============================================================================
// Conversions
// ============================================================================

/// Convert to a human-readable decimal string (MSB-first).
std::string BigInt::to_string() const {
    if (is_zero()) {
        return "0";
    }
    std::string result;
    result.reserve(digits.size());
    // Digits are stored LSB-first, so iterate in reverse for MSB-first output
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        result.push_back(static_cast<char>('0' + *it));
    }
    return result;
}

// ============================================================================
// Queries
// ============================================================================

/// Returns true if the value is zero (all digits are 0, or single digit 0).
bool BigInt::is_zero() const {
    return digits.size() == 1 && digits[0] == 0;
}

/// Returns the number of significant decimal digits.
/// After normalization this equals digits.size().
size_t BigInt::num_digits() const {
    return digits.size();
}

// ============================================================================
// Normalization
// ============================================================================

/// Remove trailing zeros from the internal LSB-first representation.
/// Trailing zeros in LSB-first correspond to leading zeros in the number.
/// Always keeps at least one digit so that zero is represented as {0}.
void BigInt::normalize() {
    while (digits.size() > 1 && digits.back() == 0) {
        digits.pop_back();
    }
}

// ============================================================================
// Chunk Conversion
// ============================================================================

/// Convert a BigInt to a chunked representation.
/// Each chunk packs `chunk_size` decimal digits into a single uint64_t.
/// Chunks are returned in LSB-first order.
///
/// Example: num = 123456 with chunk_size = 3
///   digits (LSB-first): [6, 5, 4, 3, 2, 1]
///   chunk 0 = 456  (digits at positions 0,1,2 → 6*1 + 5*10 + 4*100 = 456)
///   chunk 1 = 123  (digits at positions 3,4,5 → 3*1 + 2*10 + 1*100 = 123)
std::vector<uint64_t> BigInt::to_chunks(const BigInt& num, int chunk_size) {
    std::vector<uint64_t> chunks;
    const auto& d = num.digits;
    size_t n = d.size();

    for (size_t pos = 0; pos < n; pos += static_cast<size_t>(chunk_size)) {
        uint64_t chunk_val = 0;
        uint64_t place = 1; // 10^(position within chunk)
        // Process up to chunk_size digits starting at 'pos'
        size_t end = std::min(pos + static_cast<size_t>(chunk_size), n);
        for (size_t i = pos; i < end; ++i) {
            chunk_val += static_cast<uint64_t>(d[i]) * place;
            place *= 10;
        }
        chunks.push_back(chunk_val);
    }

    return chunks;
}

/// Reconstruct a BigInt from a chunked representation.
/// Each chunk is expanded back into `chunk_size` decimal digits (LSB-first).
BigInt BigInt::from_chunks(const std::vector<uint64_t>& chunks, int chunk_size) {
    if (chunks.empty()) {
        return BigInt(); // zero
    }

    BigInt result;
    result.digits.clear();

    for (size_t c = 0; c < chunks.size(); ++c) {
        uint64_t val = chunks[c];
        // Extract chunk_size digits from this chunk
        for (int d = 0; d < chunk_size; ++d) {
            result.digits.push_back(static_cast<uint8_t>(val % 10));
            val /= 10;
        }
    }

    result.normalize();
    return result;
}

// ============================================================================
// Random Generation
// ============================================================================

/// Generate a random BigInt with exactly num_digits decimal digits.
/// The most significant digit is in [1, 9], the rest in [0, 9].
/// If num_digits == 0, returns zero.
BigInt BigInt::random(size_t num_digits, std::mt19937_64& rng) {
    if (num_digits == 0) {
        return BigInt();
    }

    std::uniform_int_distribution<int> first_digit(1, 9);
    std::uniform_int_distribution<int> other_digit(0, 9);

    BigInt result;
    result.digits.resize(num_digits);

    // The most significant digit is stored at the END (LSB-first layout)
    result.digits[num_digits - 1] = static_cast<uint8_t>(first_digit(rng));

    // Fill remaining digits (positions 0 to num_digits-2)
    for (size_t i = 0; i < num_digits - 1; ++i) {
        result.digits[i] = static_cast<uint8_t>(other_digit(rng));
    }

    return result;
}

/// Generate a random decimal string with exactly num_digits digits.
/// First digit [1,9], rest [0,9]. If num_digits == 0, returns "0".
std::string BigInt::random_decimal_string(size_t num_digits, std::mt19937_64& rng) {
    if (num_digits == 0) {
        return "0";
    }

    std::uniform_int_distribution<int> first_digit(1, 9);
    std::uniform_int_distribution<int> other_digit(0, 9);

    std::string result;
    result.reserve(num_digits);

    result.push_back(static_cast<char>('0' + first_digit(rng)));
    for (size_t i = 1; i < num_digits; ++i) {
        result.push_back(static_cast<char>('0' + other_digit(rng)));
    }

    return result;
}

// ============================================================================
// Arithmetic Operators
// ============================================================================

/// Addition: standard grade-school addition with carry propagation.
/// Both operands are assumed non-negative.
BigInt BigInt::operator+(const BigInt& other) const {
    const auto& a = this->digits;
    const auto& b = other.digits;
    size_t max_len = std::max(a.size(), b.size());

    BigInt result;
    result.digits.resize(max_len + 1); // +1 for possible final carry

    uint8_t carry = 0;
    for (size_t i = 0; i < max_len; ++i) {
        uint8_t da = (i < a.size()) ? a[i] : 0;
        uint8_t db = (i < b.size()) ? b[i] : 0;
        uint16_t sum = static_cast<uint16_t>(da) + db + carry;
        result.digits[i] = static_cast<uint8_t>(sum % 10);
        carry = static_cast<uint8_t>(sum / 10);
    }
    result.digits[max_len] = carry;

    result.normalize();
    return result;
}

/// Subtraction: assumes *this >= other (result is non-negative).
/// Uses standard borrow-based subtraction.
BigInt BigInt::operator-(const BigInt& other) const {
    // If both are equal, short-circuit to zero
    if (*this == other) {
        return BigInt();
    }

    // Defensive check: if *this < other, return zero
    // (this project only deals with positive numbers)
    if (*this < other) {
        return BigInt();
    }

    const auto& a = this->digits;
    const auto& b = other.digits;

    BigInt result;
    result.digits.resize(a.size());

    int8_t borrow = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        int16_t da = static_cast<int16_t>(a[i]);
        int16_t db = (i < b.size()) ? static_cast<int16_t>(b[i]) : 0;
        int16_t diff = da - db - borrow;
        if (diff < 0) {
            diff += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        result.digits[i] = static_cast<uint8_t>(diff);
    }

    result.normalize();
    return result;
}

// ============================================================================
// Comparison Operators
// ============================================================================

bool BigInt::operator==(const BigInt& other) const {
    return digits == other.digits;
}

bool BigInt::operator!=(const BigInt& other) const {
    return !(*this == other);
}

/// Less-than: compare by number of digits first, then digit-by-digit from MSB.
bool BigInt::operator<(const BigInt& other) const {
    if (digits.size() != other.digits.size()) {
        return digits.size() < other.digits.size();
    }
    // Same number of digits — compare from MSB (end of vector) to LSB
    for (size_t i = digits.size(); i > 0; --i) {
        if (digits[i - 1] != other.digits[i - 1]) {
            return digits[i - 1] < other.digits[i - 1];
        }
    }
    return false; // equal
}

bool BigInt::operator>(const BigInt& other) const {
    return other < *this;
}

bool BigInt::operator<=(const BigInt& other) const {
    return !(other < *this);
}

bool BigInt::operator>=(const BigInt& other) const {
    return !(*this < other);
}

// ============================================================================
// Shift (multiply by 10^k)
// ============================================================================

/// Shift left by k decimal positions.
/// In LSB-first representation, this inserts k zeros at the BEGINNING.
/// Equivalent to multiplying by 10^k.
BigInt BigInt::shift_left(size_t k) const {
    if (is_zero() || k == 0) {
        return *this;
    }

    BigInt result;
    result.digits.resize(digits.size() + k);

    // First k positions are zero
    std::fill(result.digits.begin(), result.digits.begin() + static_cast<ptrdiff_t>(k), 0);

    // Copy original digits after the zeros
    std::copy(digits.begin(), digits.end(), result.digits.begin() + static_cast<ptrdiff_t>(k));

    return result;
}

// ============================================================================
// Slicing
// ============================================================================

/// Return the lower k digits (positions 0..k-1 in LSB-first order).
/// Equivalent to num mod 10^k.
BigInt BigInt::low(size_t k) const {
    if (k == 0) {
        return BigInt();
    }

    BigInt result;
    size_t count = std::min(k, digits.size());
    result.digits.assign(digits.begin(), digits.begin() + static_cast<ptrdiff_t>(count));

    result.normalize();
    return result;
}

/// Return the upper digits from position k onwards (positions k..end).
/// Equivalent to num div 10^k.
BigInt BigInt::high(size_t k) const {
    if (k >= digits.size()) {
        return BigInt(); // zero
    }

    BigInt result;
    result.digits.assign(digits.begin() + static_cast<ptrdiff_t>(k), digits.end());

    result.normalize();
    return result;
}
