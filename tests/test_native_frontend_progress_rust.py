#!/usr/bin/env python3
"""Compare actual Rust/native wrappers with captured production method bytes.

Run under CMake/run_tests.py. Original sort tie permutations are an intentional
change, checked separately against unchanged original caller observations.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import subprocess


def policy_lines(stdout):
    lines = stdout.splitlines()
    if lines[:1] == ["Shutdown handler: initialize."] and lines[-1:] == ["Shutdown handler: cleanup."]:
        return lines[1:-1]
    return lines


def corpus():
    rng = random.Random(0x25780b)
    cases = []
    for ui, seconds, pane in itertools.product(range(-1, 10), (-1, 0, 1, 3, 10), range(5)):
        cases.append(dict(op="primitives", ui=ui, seconds=seconds, pane=pane,
            elapsed=rng.choice([0, 999, 1000, 10000, 59999, 60000, 0x7fffffff, 0x80000000, 0xffffffff]),
            ranked=bool(rng.randrange(2)), teams=bool(rng.randrange(2)),
            variant_type=rng.choice([-0x80000000, -1, *range(11), 0x7fffffff]),
            seed=rng.randrange(1 << 32), count=rng.choice([0, 1, 2, 31, 32, 33, 0xffffffff])))
    for pane, ranked, status, party, game in itertools.product(range(4), (False, True), range(10), (False, True), range(3)):
        def members(n):
            out = []
            for i in range(n):
                level = rng.choice([None, 0, 1, 24, 25, 49, 50, 51, 0xffffffff])
                out.append(dict(team=rng.choice([-1, 0, 1, 2, 7]), value=level if level is not None else -1,
                    level=level, self=i < 2, controller=-1 if i % 3 == 0 else i % 3 - 2,
                    handle="self" if i < 2 else "peer"))
            return out
        f = dict(pane=pane, ranked=ranked, status=status, teams=bool(rng.randrange(2)),
            party_present=party, match_present=game == 1, gathering_present=game == 2,
            ui=rng.choice([0, 3, 4, 5, 6, 7, 8]), maximum=rng.choice([0, 4, 8, 16]),
            viewer_level=rng.choice([None, 0, 1, 25, 50, 51, 0xffffffff]),
            party=members(rng.randrange(1, 5)) if party else [], local=members(rng.randrange(1, 5)),
            matched=members(rng.randrange(1, 17)) if game == 1 else [],
            gathering=members(rng.randrange(1, 17)) if game == 2 else [],
            party_voice=bool(rng.randrange(2)), voice_joined=bool(rng.randrange(2)),
            muted=bool(rng.randrange(2)), transmitting=bool(rng.randrange(2)),
            talking=rng.choice([[], ["self"], ["peer"], ["self", "peer"]]),
            screen=rng.choice([14, 210, 210, 210]), entries=[rng.choice([-3, -2, -1, 0, 1]) for _ in range(18)])
        # Native production's int service-level conversion is a 32-bit cast.
        for m in f["matched"]:
            if m["value"] > 0x7fffffff:
                m["value"] -= 1 << 32
        cases.append(f)
    # Empty refresh with an installed frame retains only the old viewer-rank
    # fallback. Invalid screens/placeholders still block every identity query.
    for pane, ranked, status, value in itertools.product(range(4), (False, True), range(10), (-3, -2, -1)):
        cases.append(dict(pane=pane, ranked=ranked, status=status, viewer_level=37,
            entries=[value], local=[], party=[], matched=[], gathering=[]))
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native", type=Path)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    cases = corpus()
    run = subprocess.run([str(args.native)], input="".join(json.dumps(c) + "\n" for c in cases),
                         text=True, capture_output=True)
    mismatches = []
    comparisons = permutations = signed_timer_changes = 0
    lines = policy_lines(run.stdout)
    if run.returncode or len(lines) != len(cases):
        mismatches.append(dict(error="Native fixture failed", exit_code=run.returncode,
                               stderr=run.stderr[-4000:], lines=len(lines), expected=len(cases)))
    for index, (case, line) in enumerate(zip(cases, lines)):
        value = json.loads(line)
        if case.get("op") == "primitives":
            comparisons += len(value["before"])
            expected = value["before"][:]
            # Separately replayed2584AE..2584C9/258515 uses signed JL.
            if case["pane"] >= 2 and case["elapsed"] >= 0x80000000:
                assert expected[2] is True
                expected[2] = False
                signed_timer_changes += 1
            if expected != value["rust"]:
                mismatches.append(dict(index=index, case=case, actual=value))
            continue
        before_rows = value["before_rows"]
        expected_rows = before_rows
        expected_queries = value["before_queries"]
        if case.get("pane") == 3 and case.get("match_present"):
            old_order, new_order = value["before_order"], value["rust_order"]
            assert sorted(old_order) == sorted(new_order)
            expected_rows = [before_rows[old_order.index(i)] for i in new_order]
            expected_queries = value["before_reordered_queries"]
            permutations += old_order != new_order
        comparisons += len(before_rows) + 3 * len(expected_queries)
        if expected_rows != value["rust_rows"] or expected_queries != value["rust_queries"]:
            if len(mismatches) < 8:
                mismatches.append(dict(index=index, case=case, expected_rows=expected_rows,
                    expected_queries=expected_queries, actual=value))
    fixture = Path(__file__).resolve().parents[1] / "tests/fixtures/matchmaking_progress_sort_tu15_original.json"
    original = json.loads(fixture.read_text())["cases"]
    data = "".join(" ".join(map(str, [len(c["players"]), int(c["ranked"]), int(c["teams"]),
                    *(v for p in c["players"] for v in p)])) + "\n" for c in original)
    sort = subprocess.run([str(args.native), "--sort"], input=data, text=True, capture_output=True)
    actual = [list(map(int, line.split())) for line in policy_lines(sort.stdout)]
    sort_passed = sort.returncode == 0 and actual == [c["original"] for c in original]
    report = dict(passed=not mismatches and sort_passed, cases=len(cases), comparisons=comparisons,
        modern_differences=mismatches, intended_original_sort_permutations=permutations,
        intended_original_signed_timer_changes=signed_timer_changes,
        original_sort_cases=len(original), original_sort_passed=sort_passed,
        original_sort_fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        native_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest(),
        scope="Captured modern roster/rank/appearance/voice methods, native Rust wrappers and original sort fixture; "
              "synthetic typed observations, no network/session producer or delivered runtime claim.")
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "modern_differences"}))
    if mismatches:
        print(json.dumps(mismatches[0])[:6000])
    raise SystemExit(0 if report["passed"] else 1)


if __name__ == "__main__":
    main()
