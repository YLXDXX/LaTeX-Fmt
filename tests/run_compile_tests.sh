#!/bin/bash
# T4 Compile regression test
# Verifies that formatting never breaks LaTeX compilation.
# Requires xelatex. Run from project root or any subdirectory.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
SNAPSHOT_DIR="$PROJECT_DIR/tests/snapshots"
TMPDIR=$(mktemp -d /tmp/latex-fmt-compile-test.XXXXXX)
PASS=0
FAIL=0
SKIP=0

cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

if ! command -v xelatex &>/dev/null; then
    echo "SKIP: xelatex not found. Install TeX Live or set PATH."
    echo "T4 Compile Regression: 0 passed, 0 failed, 0 skipped (xelatex missing)"
    exit 0
fi

for tex_file in "$SNAPSHOT_DIR"/*.tex; do
    [[ -f "$tex_file" ]] || continue

    if [[ "$tex_file" == *.expected.tex ]]; then
        continue
    fi

    base_name=$(basename "$tex_file" .tex)
    expected_file="$SNAPSHOT_DIR/${base_name}.expected.tex"

    if [[ ! -f "$expected_file" ]]; then
        echo "SKIP: missing expected file for $base_name"
        SKIP=$((SKIP + 1))
        continue
    fi

    # Compile original source file
    xelatex \
        -interaction=nonstopmode \
        -halt-on-error \
        -output-directory="$TMPDIR" \
        "$tex_file" > /dev/null 2>&1
    orig_ok=$?

    # Compile formatted (expected) file
    xelatex \
        -interaction=nonstopmode \
        -halt-on-error \
        -output-directory="$TMPDIR" \
        "$expected_file" > /dev/null 2>&1
    fmt_ok=$?

    if [[ $orig_ok -eq 0 ]] && [[ $fmt_ok -ne 0 ]]; then
        echo "P0 BUG: Formatted file failed to compile but original succeeded: $base_name"
        FAIL=$((FAIL + 1))
    else
        PASS=$((PASS + 1))
    fi

    # Clean aux files between iterations to avoid cross-contamination
    rm -f "$TMPDIR"/*.aux "$TMPDIR"/*.log "$TMPDIR"/*.out
done

echo "T4 Compile Regression: $PASS passed, $FAIL failed, $SKIP skipped"
exit $FAIL
