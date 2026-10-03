#!/usr/bin/env bash
set -euo pipefail

# Configuration (overridable via environment variables)
BUILD_TYPE=${BUILD_TYPE:-Release}
JOBS=${JOBS:-$(nproc)}
SEED=${SEED:-42}
ITERATIONS=${ITERATIONS:-5}
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info()  { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok()    { echo -e "${GREEN}[OK]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }

# Stage tracking
STAGE=0
FAILED=0
START_TIME=$(date +%s)

run_stage() {
    STAGE=$((STAGE + 1))
    local name="$1"
    shift
    local stage_start=$(date +%s)
    echo ""
    echo "========================================"
    echo "  Stage $STAGE: $name"
    echo "========================================"
    if "$@"; then
        local stage_end=$(date +%s)
        local stage_dur=$((stage_end - stage_start))
        log_ok "Stage $STAGE ($name) completed successfully in ${stage_dur}s."
    else
        local stage_end=$(date +%s)
        local stage_dur=$((stage_end - stage_start))
        log_error "Stage $STAGE ($name) FAILED in ${stage_dur}s!"
        FAILED=$((FAILED + 1))
        return 1
    fi
}

# Stage functions
check_deps() {
    local missing=0
    for cmd in cmake python3; do
        if ! command -v $cmd &> /dev/null; then
            log_error "$cmd could not be found"
            missing=1
        fi
    done
    
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        log_error "Neither g++ nor clang++ could be found"
        missing=1
    fi

    if command -v python3 &> /dev/null; then
        python3 -m pip install pandas matplotlib 'reportlab==3.6.13' numpy seaborn --quiet || {
            log_warn "Failed to install some pip packages, scripts might fail"
        }
    fi
    
    if [ ! -f "/usr/include/boost/version.hpp" ] && [ -z "$(find /usr/local/include /opt/homebrew/include -name version.hpp -path "*/boost/*" 2>/dev/null)" ]; then
        log_warn "Boost headers might be missing. If build fails, install Boost."
    fi

    return $missing
}

cmake_configure() {
    cmake -B "$BUILD_DIR" -S "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
}

build_project() {
    cmake --build "$BUILD_DIR" -j "$JOBS"
}

correctness_tests() {
    if [ ! -f "$BUILD_DIR/test_correctness" ]; then
        log_warn "test_correctness not found. Skipping."
        return 0
    fi
    "$BUILD_DIR/test_correctness"
}

run_benchmarks() {
    if [ ! -f "$BUILD_DIR/bigint_benchmark" ]; then
        log_warn "bigint_benchmark not found. Skipping."
        return 0
    fi
    mkdir -p "$SCRIPT_DIR/benchmarks"
    if [ ! -f "$SCRIPT_DIR/benchmarks/benchmark_config.json" ]; then
        echo '{"iterations": '$ITERATIONS', "seed": '$SEED'}' > "$SCRIPT_DIR/benchmarks/benchmark_config.json"
    fi
    "$BUILD_DIR/bigint_benchmark" --benchmark --config "$SCRIPT_DIR/benchmarks/benchmark_config.json"
}

stat_analysis() {
    if [ ! -f "$SCRIPT_DIR/scripts/analyze_results.py" ]; then
        log_warn "analyze_results.py not found. Skipping."
        return 0
    fi
    python3 "$SCRIPT_DIR/scripts/analyze_results.py"
}

generate_graphs() {
    if [ ! -f "$SCRIPT_DIR/scripts/generate_graphs.py" ]; then
        log_warn "generate_graphs.py not found. Skipping."
        return 0
    fi
    python3 "$SCRIPT_DIR/scripts/generate_graphs.py"
}

generate_report() {
    if [ ! -f "$SCRIPT_DIR/scripts/generate_report.py" ]; then
        log_warn "generate_report.py not found. Skipping."
        return 0
    fi
    python3 "$SCRIPT_DIR/scripts/generate_report.py"
}

verify_output() {
    if [ -s "$SCRIPT_DIR/report/analysis.pdf" ]; then
        log_ok "PDF report exists and is not empty."
        return 0
    else
        log_error "PDF report is missing or empty."
        return 1
    fi
}

mkdir -p "$SCRIPT_DIR/scripts" "$SCRIPT_DIR/benchmarks" "$SCRIPT_DIR/report" "$SCRIPT_DIR/results"

run_stage "Check Dependencies" check_deps || log_warn "Continuing despite missing dependencies..."
run_stage "CMake Configure" cmake_configure || exit 1
run_stage "Build" build_project || exit 1
run_stage "Correctness Tests" correctness_tests
run_stage "Run Benchmarks" run_benchmarks
run_stage "Statistical Analysis" stat_analysis
run_stage "Generate Graphs" generate_graphs
run_stage "Generate PDF Report" generate_report
run_stage "Verify Output" verify_output

END_TIME=$(date +%s)
TOTAL_DUR=$((END_TIME - START_TIME))

echo ""
echo "========================================"
echo "  Final Summary"
echo "========================================"
if [ $FAILED -eq 0 ]; then
    log_ok "All stages completed successfully!"
else
    log_error "$FAILED stage(s) failed."
fi
log_info "Total execution time: ${TOTAL_DUR}s"

if [ $FAILED -gt 0 ]; then
    exit 1
fi
