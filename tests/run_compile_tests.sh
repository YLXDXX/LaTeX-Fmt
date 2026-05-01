#!/bin/bash
# T4 Compile regression test
# Requires xelatex to be installed

SNAPSHOT_DIR="tests/snapshots"
PASS=0
FAIL=0

for tex_file in "$SNAPSHOT_DIR"/*.tex; do
    if [[ "$tex_file" == *".expected.tex" ]]; then
        continue
    fi

    base_name=$(basename "$tex_file" .tex)
    expected_file="$SNAPSHOT_DIR/${base_name}.expected.tex"

    if [ ! -f "$expected_file" ]; then
        continue
    fi

    # Compile original
    xelatex -interaction=nonstopmode -halt-on-error -output-directory=/tmp "$tex_file" > /dev/null 2>&1
    orig_ok=$?

    # Compile formatted
    xelatex -interaction=nonstopmode -halt-on-error -output-directory=/tmp "$expected_file" > /dev/null 2>&1
    fmt_ok=$?

    if [ $orig_ok -eq 0 ] && [ $fmt_ok -ne 0 ]; then
        echo "P0 BUG: Formatted file failed to compile but original succeeded: $base_name"
        FAIL=$((FAIL+1))
    else
        PASS=$((PASS+1))
    fi
done

echo "T4 Compile Regression: $PASS passed, $FAIL failed"
exit $FAIL
