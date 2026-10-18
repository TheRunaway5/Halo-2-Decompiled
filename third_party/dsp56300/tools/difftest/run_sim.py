#!/usr/bin/env python3
"""Run the difftest corpus on Motorola sim56300 and emit canonical dumps.

Usage: run_sim.py <outdir>
Reads <outdir>/corpus_sim.lod, <outdir>/corpus.meta and
<outdir>/results_emu.txt (for per-case dynamic instruction counts), drives
the simulator (load once, then `change pc` + `step k` per case), and writes
<outdir>/results_sim.txt in the same format as the emulator runner:

  case <name>
  steps <k>
  pc <6-hex>
  ... registers ...
  cyc <n>
  end
"""

import os
import sys

from simdrv import SimSession, DUMP_REGS


def read_meta(path):
    cases = []
    with open(path) as f:
        for line in f:
            p = line.split()
            if len(p) == 5 and p[0] == "case":
                cases.append((p[1], int(p[2], 16), int(p[3], 16), int(p[4])))
    return cases


def read_emu_steps(path):
    steps = {}
    name = None
    with open(path) as f:
        for line in f:
            p = line.split()
            if not p:
                continue
            if p[0] == "case":
                name = p[1]
            elif p[0] == "steps" and name:
                steps[name] = int(p[1])
    return steps


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "out")
    cases = read_meta(os.path.join(outdir, "corpus.meta"))
    emu_steps = read_emu_steps(os.path.join(outdir, "results_emu.txt"))

    sim = SimSession(cwd=outdir)
    r = sim.cmd("load corpus_sim.lod")
    if "rror" in r:
        print("load failed:", r, file=sys.stderr)
        sys.exit(1)

    results = []
    for name, start, _end, max_steps in cases:
        k = emu_steps.get(name)
        if k is None or k == 0 or k >= max_steps:
            # Emulator side hit its cap (or is missing): still run the sim
            # up to the cap so the divergence is visible in the dumps.
            k = max_steps
        sim.cmd("change pc $%x" % start)
        text = sim.cmd("step %d" % k, max_t=60.0)
        regs = sim.parse_dump(text)
        missing = [n for n in ["pc", "sr", "a1"] if n not in regs]
        if missing:
            print("WARN %s: dump missing %s" % (name, missing), file=sys.stderr)
        results.append((name, k, regs))
        print("%-24s pc=%06x" % (name, regs.get("pc", -1)), file=sys.stderr)

    sim.close()

    with open(os.path.join(outdir, "results_sim.txt"), "w") as f:
        for name, k, regs in results:
            f.write("case %s\n" % name)
            f.write("steps %d\n" % k)
            for rn in ["pc"] + [r for r in DUMP_REGS if r != "pc"]:
                if rn in regs:
                    f.write("%s %06x\n" % (rn, regs[rn]))
            if "cyc" in regs:
                f.write("cyc %d\n" % regs["cyc"])
            f.write("end\n")
    with open(os.path.join(outdir, "sim_raw.log"), "wb") as f:
        f.write(sim.out)
    print("wrote %s/results_sim.txt" % outdir, file=sys.stderr)


if __name__ == "__main__":
    main()
