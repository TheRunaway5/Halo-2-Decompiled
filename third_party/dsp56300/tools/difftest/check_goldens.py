#!/usr/bin/env python3
"""Offline regression: JIT emulator vs committed real-hardware goldens.

Runs the `difftest` binary over the corpus committed under goldens/ and
compares the result against goldens/results_hw.txt, which was captured from
real MCPX silicon via the xbtest hardware oracle (see goldens/PROVENANCE.md
for capture details). No console needed; runnable in CI.

The goldens are only meaningful for the exact corpus that produced them, so
this script refuses to run if goldens/MANIFEST.sha256 does not match the
committed corpus files. It also warns (non-fatal) when regenerating the
corpus from the current corpus.py would produce a different image - that
means case coverage changed and the goldens need recapture on hardware.

Usage: check_goldens.py [--difftest path/to/difftest]
"""

import argparse
import hashlib
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
GOLD = os.path.join(HERE, "goldens")

BOUND_FILES = ["corpus.lod", "corpus.meta", "results_hw.txt"]


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        h.update(f.read())
    return h.hexdigest()


def check_manifest(gold_dir):
    manifest = os.path.join(gold_dir, "MANIFEST.sha256")
    want = {}
    with open(manifest) as f:
        for line in f:
            p = line.split()
            if len(p) == 2:
                want[p[1]] = p[0]
    bad = []
    for name in BOUND_FILES:
        got = sha256(os.path.join(gold_dir, name))
        if want.get(name) != got:
            bad.append("%s: manifest %s != actual %s" % (name, want.get(name), got))
    if bad:
        print("%s/MANIFEST.sha256 does not match the committed corpus:"
              % gold_dir)
        for b in bad:
            print("  " + b)
        print("goldens and corpus must be updated together (recapture on "
              "hardware via xbtest run_corpus_hw.py, then refresh the manifest).")
        sys.exit(2)


def bank_dirs():
    """Golden banks: every goldens/<bank>/ directory with a manifest."""
    dirs = []
    for name in sorted(os.listdir(GOLD)):
        sub = os.path.join(GOLD, name)
        if os.path.isdir(sub) and os.path.exists(
                os.path.join(sub, "MANIFEST.sha256")):
            dirs.append(sub)
    return dirs


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--mode", default=None,
                    help="difftest execution mode (step|block); default step")
    ap.add_argument("--difftest", default=None,
                    help="prebuilt difftest binary (default: cargo run)")
    args = ap.parse_args()

    # Drift check: sealed fingerprints in corpus_pkg must match the
    # committed goldens (case-source drift is caught by the generator's
    # own sealed-bank check at regeneration time).
    sys.path.insert(0, HERE)
    from corpus_pkg.gen import goldens_fingerprint, load_seals
    seals = load_seals()

    worst = 0
    for gold_dir in bank_dirs():
        bank = os.path.relpath(gold_dir, GOLD)
        check_manifest(gold_dir)
        if bank in seals:
            got = goldens_fingerprint(bank)
            if got != seals[bank]:
                print("bank %s: goldens do not match the sealed fingerprint "
                      "(%s... != %s...)" % (bank, got[:12], seals[bank][:12]))
                sys.exit(2)
        else:
            print("bank %s: UNSEALED (authored, awaiting hardware capture "
                  "or `corpus.py --seal %s`)" % (bank, bank))

        lod = os.path.join(gold_dir, "corpus.lod")
        meta = os.path.join(gold_dir, "corpus.meta")
        if args.difftest:
            cmd = [args.difftest]
        else:
            cmd = ["cargo", "run", "-q", "--release", "-p", "dsp56300-emu",
                   "--bin", "difftest", "--"]
        cmd += ["corpus", lod, meta, "--boot=hw"]
        if args.mode:
            cmd.append("--mode=%s" % args.mode)
        # Goldens captured with --dump-mem / --dump-stack carry sparse
        # xm/ym/pm memory keys / sh/sl stack-slot keys; produce a matching
        # emulator dump so diff.py compares the same state. Goldens
        # captured without either flag keep working as-is.
        with open(os.path.join(gold_dir, "results_hw.txt")) as f:
            text = f.readlines()
        if any(line[:2] in ("xm", "ym", "pm") and line[2:6].strip()
               for line in text):
            cmd.append("--dump-mem")
        if any(line[:2] in ("sh", "sl") and line[2:4].strip()
               and line[4:5] == " " for line in text):
            cmd.append("--dump-stack")
        r = subprocess.run(cmd, cwd=REPO, capture_output=True, text=True)
        if r.returncode != 0:
            print("difftest run failed:\n" + r.stderr)
            sys.exit(2)

        out_emu = os.path.join(
            HERE, "out", "results_emu_vs_goldens_%s.txt" % bank.replace(os.sep, "_"))
        os.makedirs(os.path.dirname(out_emu), exist_ok=True)
        with open(out_emu, "w") as f:
            f.write(r.stdout)

        print("bank %s:" % bank)
        rc = subprocess.call([sys.executable, os.path.join(HERE, "diff.py"),
                              out_emu, os.path.join(gold_dir, "results_hw.txt")])
        worst = max(worst, rc)
    sys.exit(worst)


if __name__ == "__main__":
    main()
