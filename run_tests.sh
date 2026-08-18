#!/usr/bin/env bash
# run_tests.sh - Full test suite for codexion.
#
# Place this script (and check_log.py) at the root of your codexion repo,
# next to your Makefile, then run:
#
#   chmod +x run_tests.sh
#   ./run_tests.sh
#
# Requires: make, your codexion binary, python3, and (optionally) valgrind.
# Install valgrind if missing:  sudo apt-get install valgrind
#
set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHECK_LOG="$SCRIPT_DIR/check_log.py"
BIN=./codexion
LOGDIR=./test_logs
PASS=0
FAIL=0

mkdir -p "$LOGDIR"

if [ ! -f "$CHECK_LOG" ]; then
    echo "[FATAL] check_log.py not found at $CHECK_LOG"
    echo "        Make sure check_log.py sits in the same folder as run_tests.sh."
    exit 1
fi

section() { echo; echo "==================== $1 ===================="; }

# ---------------------------------------------------------------
# 0. Build
# ---------------------------------------------------------------
section "BUILD"
make re > "$LOGDIR/build.log" 2>&1
if [ $? -ne 0 ] || [ ! -x "$BIN" ]; then
    echo "[FAIL] build failed, see $LOGDIR/build.log"
    cat "$LOGDIR/build.log"
    exit 1
fi
echo "[ OK ] build succeeded"

# ---------------------------------------------------------------
# 1. Argument validation - all of these MUST be rejected
#    (non-zero exit or a clear error, and must NOT crash / hang)
# ---------------------------------------------------------------
section "ARGUMENT VALIDATION (must all be REJECTED)"

check_rejected() {
    local desc="$1"; shift
    timeout 2 "$BIN" "$@" > /dev/null 2>&1
    local rc=$?
    if [ $rc -eq 124 ]; then
        echo "[FAIL] $desc -> hung / timed out instead of rejecting"
        FAIL=$((FAIL+1))
    elif [ $rc -eq 139 ] || [ $rc -eq 134 ]; then
        echo "[FAIL] $desc -> CRASHED (segfault/abort) instead of rejecting"
        FAIL=$((FAIL+1))
    elif [ $rc -eq 0 ]; then
        echo "[FAIL] $desc -> accepted invalid input (exit 0)"
        FAIL=$((FAIL+1))
    else
        echo "[ OK ] $desc -> rejected (exit $rc)"
        PASS=$((PASS+1))
    fi
}

check_rejected "no arguments" 
check_rejected "too few arguments" 5 3000 200
check_rejected "too many arguments" 5 3000 200 200 200 10 800 fifo extra
check_rejected "zero coders" 0 3000 200 200 200 10 800 fifo
check_rejected "negative coders" -1 3000 200 200 200 10 800 fifo
check_rejected "negative burnout" 5 -3000 200 200 200 10 800 fifo
check_rejected "negative compile time" 5 3000 -200 200 200 10 800 fifo
check_rejected "negative debug time" 5 3000 200 -200 200 10 800 fifo
check_rejected "negative refactor time" 5 3000 200 200 -200 10 800 fifo
check_rejected "negative compiles_required" 5 3000 200 200 200 -10 800 fifo
check_rejected "negative cooldown" 5 3000 200 200 200 10 -800 fifo
check_rejected "non-integer coders" abc 3000 200 200 200 10 800 fifo
check_rejected "non-integer burnout" 5 abc 200 200 200 10 800 fifo
check_rejected "float argument" 5 3000.5 200 200 200 10 800 fifo
check_rejected "invalid scheduler" 5 3000 200 200 200 10 800 round_robin
check_rejected "empty scheduler" 5 3000 200 200 200 10 800 ""
check_rejected "scheduler case-sensitivity" 5 3000 200 200 200 10 800 FIFO

# ---------------------------------------------------------------
# 2. Functional scenarios - all of these MUST run and produce
#    a well-formed log validated by check_log.py
# ---------------------------------------------------------------
section "FUNCTIONAL SCENARIOS"

run_scenario() {
    # args: name n burnout compile debug refactor required cooldown sched extra_timeout
    local name="$1" n="$2" burnout="$3" compile="$4" debug="$5"
    local refactor="$6" required="$7" cooldown="$8" sched="$9"
    local timeout_s="${10:-15}"
    local log="$LOGDIR/${name}.log"

    echo
    echo "--- Scenario: $name ---"
    echo "    $BIN $n $burnout $compile $debug $refactor $required $cooldown $sched"
    timeout "$timeout_s" "$BIN" "$n" "$burnout" "$compile" "$debug" "$refactor" \
        "$required" "$cooldown" "$sched" > "$log" 2>&1
    local rc=$?
    if [ $rc -eq 124 ]; then
        echo "[FAIL] $name -> timed out / never stopped after ${timeout_s}s"
        FAIL=$((FAIL+1))
        return
    fi
    if [ $rc -ne 0 ] && [ $rc -ne 1 ]; then
        echo "[FAIL] $name -> abnormal exit code $rc (crash?)"
        FAIL=$((FAIL+1))
        return
    fi

    python3 "$CHECK_LOG" "$log" "$n" "$burnout" "$compile" "$debug" "$refactor" \
        "$required" "$cooldown" 60
    if [ $? -eq 0 ]; then
        echo "[ OK ] $name -> log validated"
        PASS=$((PASS+1))
    else
        echo "[FAIL] $name -> log validation failed (see above)"
        FAIL=$((FAIL+1))
    fi
}

# single coder edge case (only one dongle on the table)
run_scenario "single_coder_fifo" 1 5000 200 200 200 5 200 fifo

# small group, generous timings, should finish via compiles_required
run_scenario "small_group_fifo_completes" 4 4000 200 200 200 5 200 fifo
run_scenario "small_group_edf_completes" 4 4000 200 200 200 5 200 edf

# larger group, higher contention
run_scenario "large_group_fifo" 10 3000 150 100 100 4 200 fifo
run_scenario "large_group_edf" 10 3000 150 100 100 4 200 edf

# odd number of coders (tests the asymmetric lock-order fix on the wrap-around pair)
run_scenario "odd_group_fifo" 7 3000 150 150 150 4 150 fifo
run_scenario "odd_group_edf" 7 3000 150 150 150 4 150 edf

# tight burnout window relative to compile+debug+refactor -> should burn out
run_scenario "tight_burnout_should_burn" 5 400 200 200 200 20 400 fifo 10

# high cooldown relative to cycle time -> stresses cooldown correctness
run_scenario "high_cooldown" 4 6000 100 100 100 5 1500 fifo

# scheduler stress: many coders, minimal timings
run_scenario "stress_many_coders" 20 5000 100 50 50 3 100 edf 20

# ---------------------------------------------------------------
# 3. Valgrind - memory leaks / invalid access
# ---------------------------------------------------------------
section "VALGRIND MEMCHECK (leaks + invalid access)"
if command -v valgrind > /dev/null 2>&1; then
    VG_LOG="$LOGDIR/valgrind_memcheck.log"
    timeout 30 valgrind --leak-check=full --show-leak-kinds=all \
        --track-origins=yes --error-exitcode=42 \
        "$BIN" 5 4000 200 200 200 5 200 fifo > /dev/null 2> "$VG_LOG"
    rc=$?
    if grep -q "definitely lost: 0 bytes" "$VG_LOG" && \
       grep -q "indirectly lost: 0 bytes" "$VG_LOG" && \
       [ "$rc" -ne 42 ]; then
        echo "[ OK ] no leaks / no memory errors detected"
        PASS=$((PASS+1))
    else
        echo "[FAIL] valgrind reported leaks or errors, see $VG_LOG"
        tail -n 30 "$VG_LOG"
        FAIL=$((FAIL+1))
    fi
else
    echo "[SKIP] valgrind not installed (sudo apt-get install valgrind)"
fi

# ---------------------------------------------------------------
# 4. Helgrind - data races / lock-order problems
# ---------------------------------------------------------------
section "VALGRIND HELGRIND (data races)"
if command -v valgrind > /dev/null 2>&1; then
    HG_LOG="$LOGDIR/valgrind_helgrind.log"
    timeout 30 valgrind --tool=helgrind \
        "$BIN" 5 4000 200 200 200 5 200 fifo > /dev/null 2> "$HG_LOG"
    if grep -qi "ERROR SUMMARY: 0 errors" "$HG_LOG"; then
        echo "[ OK ] no data races detected by helgrind"
        PASS=$((PASS+1))
    else
        echo "[FAIL] helgrind reported possible data races, see $HG_LOG"
        grep -A 3 "Possible data race" "$HG_LOG" | head -n 40
        FAIL=$((FAIL+1))
    fi
else
    echo "[SKIP] valgrind not installed"
fi

# ---------------------------------------------------------------
# Summary
# ---------------------------------------------------------------
section "SUMMARY"
echo "Passed: $PASS"
echo "Failed: $FAIL"
echo "Logs saved in: $LOGDIR/"
[ "$FAIL" -eq 0 ]
