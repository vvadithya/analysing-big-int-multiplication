cmake_minimum_required(VERSION 3.16)
project(bigint-multiplication LANGUAGES CXX)

# ── C++20 standard ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Default to Release build ────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -fsanitize=address,undefined")

# ── Threading ───────────────────────────────────────────────────────────────
find_package(Threads REQUIRED)

# ── Boost (header-only — we only need the include path) ─────────────────────
# Try the standard CMake finder first.  If that fails (common on minimal
# installs), fall back to pkg-config, then to well-known system paths.
find_package(Boost QUIET)

if(NOT Boost_FOUND)
    message(STATUS "find_package(Boost) failed — trying pkg-config …")
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(BOOST QUIET boost)
        if(BOOST_FOUND)
            set(Boost_INCLUDE_DIRS ${BOOST_INCLUDE_DIRS})
            set(Boost_FOUND TRUE)
            message(STATUS "Boost found via pkg-config: ${Boost_INCLUDE_DIRS}")
        endif()
    endif()
endif()

if(NOT Boost_FOUND)
    # Last resort: probe common system paths
    foreach(_dir /usr/include /usr/local/include /opt/homebrew/include)
        if(EXISTS "${_dir}/boost/multiprecision/cpp_int.hpp")
            set(Boost_INCLUDE_DIRS "${_dir}")
            set(Boost_FOUND TRUE)
            message(STATUS "Boost headers discovered at ${_dir}")
            break()
        endif()
    endforeach()
endif()

if(NOT Boost_FOUND)
    message(FATAL_ERROR
        "Could not locate Boost headers.\n"
        "Install libboost-dev (Ubuntu/Debian) or boost (Arch/Fedora),\n"
        "or set -DBOOST_ROOT=/path/to/boost on the CMake command line."
    )
endif()

# ── Include paths ───────────────────────────────────────────────────────────
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})

# ── Static library: all algorithm + utility sources ─────────────────────────
add_library(bigint_lib STATIC
    src/bigint.cpp
    src/schoolbook.cpp
    src/chunked.cpp
    src/karatsuba.cpp
    src/parallel_chunked.cpp
    src/benchmark.cpp
)
target_link_libraries(bigint_lib PUBLIC Threads::Threads)

# ── Main benchmark executable ──────────────────────────────────────────────
add_executable(bigint_benchmark src/main.cpp)
target_link_libraries(bigint_benchmark PRIVATE bigint_lib Threads::Threads)

# ── Correctness-test executable ────────────────────────────────────────────
add_executable(test_correctness tests/test_correctness.cpp)
target_link_libraries(test_correctness PRIVATE bigint_lib Threads::Threads)

# ── Dedicated benchmark runner ─────────────────────────────────────────────
add_executable(run_benchmarks benchmarks/run_benchmarks.cpp)
target_link_libraries(run_benchmarks PRIVATE bigint_lib Threads::Threads)

# ── Install (optional) ─────────────────────────────────────────────────────
install(TARGETS bigint_benchmark test_correctness run_benchmarks
        RUNTIME DESTINATION bin)
