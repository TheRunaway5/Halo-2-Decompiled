#!/usr/bin/env python3
"""Captured modern native text methods versus actual Rust/CXX callbacks.

The enclosing run_tests.py/run_test.py owner removes generated build products.
Only this bounded report is retained. Native IO observations are synthetic;
this does not certify historical locale/session producers or installed input.
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


def cases():
    rng = random.Random(0x2580b2)
    playlists = [dict(id="ranked", name="Ranked", description="ranked description", eligible=True, ranked=True, minimum=2, maximum=8, level=37),
                 dict(id="casual", name="Casual", description="casual description", eligible=True, minimum=4, maximum=16),
                 dict(id="clan", name="Clan", description="clan description", clan=True, minimum=4, maximum=8)]
    out = []
    for original, pane, index, matched, gathering in itertools.product((False, True), range(4), range(13), (None, 0, 8), (None, 2)):
        out.append(dict(original=original, pane=pane, query_pane=pane, index=index,
            matched=matched, gathering=gathering, minimum=rng.choice([0, 2, 4, 8, 17]),
            screen=rng.choice([14, 210, 219, 237, 0xffffffff]), row=rng.randrange(5), selected=rng.choice(["", "ranked", "clan"]),
            playlists=playlists, status=rng.choice([*range(10), 261, 65539, 0xffffffff]),
            ui=rng.choice([*range(9), -1, 0x7fffffff]), title=rng.choice([*range(9), -1, 0x7fffffff]),
            countdown=rng.choice([-0x80000000, -2, -1, 0, 1, 3, 10, 0x7fffffff]),
            pc=bool(rng.randrange(2)), queued=rng.choice([0, 1, 2, 0xffffffff]), party=bool(rng.randrange(2)), leader=bool(rng.randrange(2)),
            message=rng.choice(["", "queue message", [0xff, 0, 0x80]]), party_message=rng.choice(["", "party message", [0xfe, 0x80]]),
            variant=rng.choice(["Slayer", [0xff, 0x80, 0], "\ue40a"]), map=rng.choice(["map", "MAP", "missing", [0xff, 0x80, 0]]),
            party_count=rng.choice([0, 1, 4, 17, 0xffffffffffffffff]), variant_available=bool(rng.randrange(2))))
    for screen, status, ui, countdown in itertools.product((210, 219, 237, 14), range(10), range(9), (-1, 0, 10)):
        out.append(dict(screen=screen, status=status, ui=ui, title=ui, countdown=countdown, pane=3,
                        pc=True, matched=8, index=0, playlists=playlists, selected="ranked", row=2))
    # Actual std::string byte semantics and first matching duplicate ID.
    for screen, row, index in itertools.product((14, 219, 237), (0, 1, 2, 0xffffffff), range(4)):
        out.append(dict(screen=screen, row=row, index=index, selected=[0xff, 0], playlists=[
            dict(id=[0xff, 0], name=[0xff, 0, 0x80], description=[0xfe, 0, 0x81], eligible=False, clan=True),
            dict(id=[0xff, 0], name="second", description="second", eligible=True)]))
    return out


def run(native, corpus):
    result = subprocess.run([str(native)], input="".join(json.dumps(c) + "\n" for c in corpus), text=True, capture_output=True)
    lines = policy_lines(result.stdout)
    if result.returncode or len(lines) != len(corpus):
        raise RuntimeError(json.dumps(dict(exit_code=result.returncode, lines=len(lines), expected=len(corpus), stderr=result.stderr[-4000:], stdout=result.stdout[:1000])))
    return [json.loads(line) for line in lines]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native", type=Path)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    corpus = cases()
    answers = run(args.native, corpus)
    failures = []
    comparisons = calls = identity = 0
    for i, (case, value) in enumerate(zip(corpus, answers)):
        calls += len(value["before"]["calls"])
        comparisons += 1 + len(value["before"]["calls"])
        if value["before"] != value["rust"] and len(failures) < 8:
            failures.append(dict(index=i, case=case, actual=value))
    # Inject the same genuine native exception at every observed callback in
    # selected ordinary/override/112-getter paths, preserving type and object.
    fault_cases = []
    selected = [dict(original=True, pane=3, query_pane=3, matched=8, index=4),
                dict(original=True, pane=0, query_pane=0, index=0, title=3),
                dict(original=True, pane=1, query_pane=1, gathering=2, minimum=4, index=3),
                dict(screen=210, pane=3, ui=7, pc=True, matched=8, countdown=3, index=0),
                dict(original=True, pane=3, query_pane=3, matched=8, index=3)]
    for case, result in zip(selected, run(args.native, selected)):
        for index in range(len(result["before"]["calls"])):
            for nonstd in (False, True):
                fault_cases.append(case | dict(fail=index, nonstd=nonstd))
    for case, value in zip(fault_cases, run(args.native, fault_cases)):
        comparisons += 1 + len(value["before"]["calls"])
        identity += value["rust"].get("same_object", value["rust"].get("error") == "int")
        if value["before"] != value["rust"] and len(failures) < 8:
            failures.append(dict(case=case, actual=value))
    report = dict(passed=not failures, cases=len(corpus), native_io_observations=calls, comparisons=comparisons,
        fault_controls=len(fault_cases), preserved_native_exception_identity=identity, differences=failures,
        native_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest(), scope=__doc__)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "differences"}))
    if failures:
        print(json.dumps(failures[0])[:6000])
    raise SystemExit(0 if report["passed"] else 1)


if __name__ == "__main__":
    main()
