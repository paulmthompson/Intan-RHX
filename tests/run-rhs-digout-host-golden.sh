#!/usr/bin/env bash
# Host golden tests for RHS digital-output programming (no FPGA).
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

for candidate in \
    "${root}/build/rhsdigoutconfiguration_golden_test" \
    "${root}/build/tests/rhsdigoutconfiguration_golden_test" \
    "${root}/cmake-build-release/rhsdigoutconfiguration_golden_test"; do
    if [[ -x "${candidate}" ]]; then
        exec "${candidate}"
    fi
done

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

qt_flags="$(pkg-config --cflags --libs Qt5Core 2>/dev/null || true)"
if [[ -z "${qt_flags}" ]]; then
    echo "Build the CMake target rhsdigoutconfiguration_golden_test, or install Qt5Core (pkg-config)." >&2
    exit 1
fi

g++ -std=c++17 -Wall -Wextra \
    -ffunction-sections -fdata-sections \
    -I"${root}/Engine/Processing" \
    "${root}/tests/rhsdigoutconfiguration_golden_test.cpp" \
    "${root}/Engine/Processing/rhsdigoutconfiguration.cpp" \
    -o "${tmp}/rhsdigoutconfiguration_golden_test" \
    ${qt_flags} \
    -Wl,--gc-sections

"${tmp}/rhsdigoutconfiguration_golden_test"
