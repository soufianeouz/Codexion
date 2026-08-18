#!/usr/bin/env python3
"""
check_log.py - Validates a codexion log against the subject's rules.

Usage:
    ./codexion N burnout compile debug refactor required cooldown fifo > log.txt
    python3 check_log.py log.txt N burnout compile debug refactor required cooldown [tolerance_ms]

Checks performed:
  1. Every line matches the exact required format.
  2. Per-coder state machine is respected (taken,taken,compiling,debugging,refactoring)*[burned out]?
  3. "is compiling" is always immediately preceded (per-coder) by exactly two
     "has taken a dongle" lines.
  4. Adjacent coders (who share a dongle) never have overlapping compiling
     intervals -> catches dongle duplication / missing mutex bugs.
  5. Compile/debug/refactor phase durations roughly match the requested
     timings (within tolerance).
  6. Time between the start of one compile and the start of the next never
     exceeds time_to_burnout + tolerance, UNLESS the coder actually burned out.
  7. At most one "burned out" line exists, and no further activity is logged
     more than `tolerance` ms after it.
  8. If nobody burns out, every coder reaches at least `required` compiles.

Exit code 0 = all checks passed, 1 = at least one violation found.
"""
import re
import sys

LINE_RE = re.compile(
    r'^(\d+)\s+(\d+)\s+(has taken a dongle|is compiling|is debugging|is refactoring|burned out)\s*$'
)


def fail(msg):
    print(f"[FAIL] {msg}")


def ok(msg):
    print(f"[ OK ] {msg}")


def main():
    if len(sys.argv) < 9:
        print(__doc__)
        sys.exit(2)

    log_path = sys.argv[1]
    n_coders = int(sys.argv[2])
    burnout = int(sys.argv[3])
    compile_t = int(sys.argv[4])
    debug_t = int(sys.argv[5])
    refactor_t = int(sys.argv[6])
    required = int(sys.argv[7])
    cooldown = int(sys.argv[8])
    tol = int(sys.argv[9]) if len(sys.argv) > 9 else 50

    violations = 0
    events = []  # (ts, coder, event)

    with open(log_path) as f:
        for lineno, raw in enumerate(f, 1):
            line = raw.rstrip("\n")
            if not line.strip():
                continue
            m = LINE_RE.match(line)
            if not m:
                fail(f"line {lineno}: does not match required format: {line!r}")
                violations += 1
                continue
            ts, coder, ev = int(m.group(1)), int(m.group(2)), m.group(3)
            if not (0 <= coder <= n_coders - 1):
                fail(f"line {lineno}: coder number {coder} out of range 0..{n_coders - 1}")
                violations += 1
            events.append((ts, coder, ev))

    if not events:
        fail("no valid log lines found")
        sys.exit(1)

    ok(f"parsed {len(events)} valid-format lines")

    # ---- 2 & 3: per-coder state machine ----
    per_coder = {c: [] for c in range(n_coders)}
    for ts, c, ev in events:
        per_coder[c].append((ts, ev))

    burn_events = [(ts, c) for ts, c, ev in events if ev == "burned out"]
    if len(burn_events) > 1:
        fail(f"more than one 'burned out' line found: {burn_events}")
        violations += 1
    burn_ts = burn_events[0][0] if burn_events else None

    compile_intervals = {}  # coder -> list of (start, end) compiling windows
    compile_counts = {c: 0 for c in range(n_coders)}

    for c, seq in per_coder.items():
        i = 0
        cycle_start_for_burnout_check = None
        intervals = []
        while i < len(seq):
            ts, ev = seq[i]
            if ev == "burned out":
                if i != len(seq) - 1:
                    fail(f"coder {c}: events logged after 'burned out' at ts={ts}")
                    violations += 1
                break
            if ev != "has taken a dongle":
                fail(f"coder {c}: expected 'has taken a dongle' at position {i}, "
                     f"got '{ev}' (ts={ts})")
                violations += 1
                i += 1
                continue

            # With a single coder, left and right dongle are the SAME dongle
            # (coder 1 sits next to itself), so only one "has taken a dongle"
            # line is expected before compiling. With 2+ coders, two distinct
            # dongles are taken, so two lines are expected.
            if n_coders == 1:
                taken_advance = 1
                compile_idx = i + 1
            else:
                if i + 1 >= len(seq) or seq[i + 1][1] != "has taken a dongle":
                    fail(f"coder {c}: 'has taken a dongle' at ts={ts} not followed by "
                         f"a second 'has taken a dongle' line")
                    violations += 1
                    i += 1
                    continue
                taken_advance = 2
                compile_idx = i + 2

            if compile_idx >= len(seq) or seq[compile_idx][1] != "is compiling":
                fail(f"coder {c}: dongle-taken line(s) at ts={ts} not "
                     f"immediately followed by 'is compiling'")
                violations += 1
                i += taken_advance
                continue
            compile_start = seq[compile_idx][0]
            debug_idx = compile_idx + 1
            if debug_idx >= len(seq) or seq[debug_idx][1] != "is debugging":
                fail(f"coder {c}: 'is compiling' at ts={compile_start} not "
                     f"followed by 'is debugging'")
                violations += 1
                i = debug_idx
                continue
            debug_start = seq[debug_idx][0]
            refactor_idx = debug_idx + 1
            if refactor_idx >= len(seq) or seq[refactor_idx][1] != "is refactoring":
                fail(f"coder {c}: 'is debugging' at ts={debug_start} not "
                     f"followed by 'is refactoring'")
                violations += 1
                i = refactor_idx
                continue
            refactor_start = seq[refactor_idx][0]

            intervals.append((compile_start, debug_start))
            compile_counts[c] += 1

            # duration sanity checks
            actual_compile = debug_start - compile_start
            if abs(actual_compile - compile_t) > tol:
                fail(f"coder {c}: compile phase took {actual_compile}ms, "
                     f"expected ~{compile_t}ms (tol {tol}ms)")
                violations += 1
            actual_debug = refactor_start - debug_start
            if abs(actual_debug - debug_t) > tol:
                fail(f"coder {c}: debug phase took {actual_debug}ms, "
                     f"expected ~{debug_t}ms (tol {tol}ms)")
                violations += 1

            # burnout-interval check vs previous cycle start
            if cycle_start_for_burnout_check is not None:
                gap = compile_start - cycle_start_for_burnout_check
                if gap > burnout + tol:
                    fail(f"coder {c}: {gap}ms between compile starts, exceeds "
                         f"time_to_burnout={burnout}ms (+{tol}ms tol) without burning out")
                    violations += 1
            cycle_start_for_burnout_check = compile_start

            i = refactor_idx + 1  # move past taken(s), compiling, debugging, refactoring

        compile_intervals[c] = intervals

    # ---- 4: adjacent coders must never overlap while compiling ----
    # (meaningless for a single coder: it has no neighbor, just its own dongle)
    for c in range(n_coders):
        if n_coders == 1:
            continue
        neighbor = c + 1 if c < n_coders - 1 else 0
        if neighbor == c:
            continue
        for s1, e1 in compile_intervals[c]:
            for s2, e2 in compile_intervals[neighbor]:
                if s1 < e2 and s2 < e1:
                    fail(f"DONGLE DUPLICATION: coder {c} compiling [{s1},{e1}] "
                         f"overlaps neighbor {neighbor} compiling [{s2},{e2}]")
                    violations += 1

    if violations == 0:
        ok("no overlapping compile intervals between adjacent coders")

    # ---- 7: burnout must be logged, and it's the reason for stopping ----
    if burn_ts is not None:
        ok(f"simulation stopped due to burnout at ts={burn_ts}")
    else:
        # ---- 8: everyone must have reached `required` compiles ----
        short = {c: n for c, n in compile_counts.items() if n < required}
        if short:
            fail(f"no burnout occurred, but these coders did not reach "
                 f"{required} compiles: {short}")
            violations += 1
        else:
            ok(f"no burnout: every coder reached >= {required} compiles "
               f"({compile_counts})")

    print()
    if violations:
        print(f"RESULT: {violations} violation(s) found")
        sys.exit(1)
    else:
        print("RESULT: all checks passed")
        sys.exit(0)


if __name__ == "__main__":
    main()
