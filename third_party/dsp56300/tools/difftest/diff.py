#!/usr/bin/env python3
"""Compare canonical difftest dumps from two engines.

Usage: diff.py <results_a.txt> <results_b.txt> [--cycles]

Registers are compared exactly, per case. Cycle counts are informational
(reported with --cycles, never fatal): the emulator's cycle model and the
simulator's cyc counter are not expected to match yet.
"""

import sys

INFORMATIONAL = {"cyc", "ictr", "steps"}


def read_results(path):
    cases = {}
    order = []
    fill = None
    cur = None
    with open(path) as f:
        for line in f:
            p = line.split()
            if not p:
                continue
            if p[0] == "case":
                cur = {}
                cases[p[1]] = cur
                order.append(p[1])
            elif p[0] == "fill" and len(p) == 7:
                # Dirty-state header: this file's memory dumps are
                # deviations from (k*addr+c)&$FFFFFF, not from zero.
                fill = tuple(int(v, 16) for v in p[1:])
            elif p[0] == "end":
                cur = None
            elif cur is not None and len(p) == 2:
                cur[p[0]] = p[1]
    return cases, order, fill


def mem_default(fill, key):
    """Value of a missing xm/ym/pm (memory) or sh/sl (stack slot) key:
    the file's init at that address/slot (affine fill / seed ramp under
    a `fill` header, zero otherwise; P never fills)."""
    if fill is None or key[:2] == "pm":
        return "000000"
    if key[:2] in ("sh", "sl"):
        base = fill[4] if key[:2] == "sh" else fill[5]
        return "%06x" % ((base + int(key[2:], 16) - 1) & 0xFFFFFF)
    addr = int(key[2:], 16)
    k, c = (fill[0], fill[1]) if key[:2] == "xm" else (fill[2], fill[3])
    return "%06x" % ((k * addr + c) & 0xFFFFFF)


def main():
    a_path, b_path = sys.argv[1], sys.argv[2]
    show_cycles = "--cycles" in sys.argv
    a, order, fill_a = read_results(a_path)
    b, _, fill_b = read_results(b_path)
    if fill_a != fill_b:
        print("WARNING: fill headers differ (%s vs %s); memory defaults "
              "compare per-file" % (fill_a, fill_b))

    total = 0
    bad = 0
    for name in order:
        if name not in b:
            print("%-24s MISSING in %s" % (name, b_path))
            bad += 1
            continue
        total += 1
        diffs = []
        keys = sorted(set(a[name]) | set(b[name]))
        for k in keys:
            if k in INFORMATIONAL:
                continue
            va, vb = a[name].get(k), b[name].get(k)
            # Memory end-state keys (xm/ym/pm + hex address) are sparse:
            # only deviations from the file's init are dumped, so a key
            # missing on one side means that side holds its init value
            # (zero, or the affine fill under a `fill` header) there.
            if (k[:2] in ("xm", "ym", "pm") and len(k) == 6) or (
                    k[:2] in ("sh", "sl") and len(k) == 4):
                va = va or mem_default(fill_a, k)
                vb = vb or mem_default(fill_b, k)
            if va == "-" or vb == "-":
                continue  # explicitly unknown on one side (e.g. hw ssh at sp=0)
            if va != vb:
                diffs.append((k, va, vb))
        if diffs:
            bad += 1
            print("%-24s DIFF" % name)
            for k, va, vb in diffs:
                print("    %-4s %8s | %8s" % (k, va, vb))
        if show_cycles:
            ca, cb = a[name].get("cyc"), b[name].get("cyc")
            print("%-24s cyc %s | %s" % (name, ca, cb))

    print()
    print("%d/%d cases match (%d differ)" % (total - bad, total, bad))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
