"""Generated exhaustive parallel-ALU byte sweep (bank b5).

Lazy: shells out to target/release/difftest only when the bank is
actually packed, so status/fingerprint checks work without a build.
"""

import os
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))

THEME = "sweeps"

SWEEP_NAMES = ["alusweep_%02d" % i for i in range(21)]


def sweep_cases():
    difftest = os.path.join(REPO, "target", "release", "difftest")
    r = subprocess.run([difftest, "alu-dump"], capture_output=True, text=True)
    if r.returncode != 0:
        print("alu-dump failed (build target/release/difftest first)")
        sys.exit(1)
    ops = []
    for line in r.stdout.splitlines():
        b, text = line.split("\t")
        if text != "move":
            ops.append(text)
    init = [
        "move #>$654321,x0", "move #>$123456,x1",
        "move #>$789abc,y0", "move #>$def012,y1",
        "clr a", "move #>$345678,x0", "move x0,a1", "move #>$654321,x0",
        "clr b", "move #>$9abcde,x1", "move x1,b1", "move #>$123456,x1",
    ]
    cases = []
    chunk = 12
    for i in range(0, len(ops), chunk):
        cases.append((
            "alusweep_%02d" % (i // chunk),
            init + ops[i:i + chunk],
            100,
        ))
    return cases

