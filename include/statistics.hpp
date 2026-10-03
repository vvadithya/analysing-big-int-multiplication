#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>

// ============================================================================
// Statistical summary structure
// ============================================================================
struct Statistics {
    double mean = 0.0;
    double median = 0.0;
    double min_val = 0.0;
    double max_val = 0.0;
    double std_dev = 0.0;
    double coeff_variation = 0.0;  // std_dev / mean
    size_t count = 0;
};

// ============================================================================
// Compute statistics from a vector of timing measurements
// ============================================================================
inline Statistics compute_statistics(const std::vector<double>& values) {
    Statistics stats;
    if (values.empty()) return stats;

    stats.count = values.size();

    // Mean
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    stats.mean = sum / static_cast<double>(stats.count);

    // Min/Max
    auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());
    stats.min_val = *min_it;
    stats.max_val = *max_it;

    // Median
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    if (stats.count % 2 == 0) {
        stats.median = (sorted[stats.count / 2 - 1] + sorted[stats.count / 2]) / 2.0;
    } else {
        stats.median = sorted[stats.count / 2];
    }

    // Standard deviation (population)
    double sq_sum = 0.0;
    for (double v : values) {
        double diff = v - stats.mean;
        sq_sum += diff * diff;
    }
    stats.std_dev = std::sqrt(sq_sum / static_cast<double>(stats.count));

    // Coefficient of variation
    if (stats.mean > 0.0) {
        stats.coeff_variation = stats.std_dev / stats.mean;
    }

    return stats;
}

// ============================================================================
// Format statistics for display
// ============================================================================
inline std::string format_statistics(const Statistics& stats) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2)
        << "n=" << stats.count
        << " mean=" << stats.mean << " ns"
        << " median=" << stats.median << " ns"
        << " min=" << stats.min_val << " ns"
        << " max=" << stats.max_val << " ns"
        << " stddev=" << stats.std_dev << " ns"
        << " cv=" << (stats.coeff_variation * 100.0) << "%";
    return oss.str();
}

// CSV header for statistics
inline std::string statistics_csv_header() {
    return "mean_ns,median_ns,min_ns,max_ns,stddev_ns,cv,count";
}

inline std::string statistics_to_csv(const Statistics& stats) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2)
        << stats.mean << "," << stats.median << ","
        << stats.min_val << "," << stats.max_val << ","
        << stats.std_dev << "," << stats.coeff_variation << ","
        << stats.count;
    return oss.str();
}
