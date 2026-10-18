"""Corpus generator: assembles each bank into one bootable image.

See registry.py for authoring rules and banks.py for the packing
manifest / sealing model.
"""

import hashlib
import json
import os
import re
import subprocess
import sys

from .banks import BANKS
from .registry import REGISTRY
from . import cases_sweeps

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
HERE = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SEALS = os.path.join(os.path.dirname(__file__), "sealed.json")
# Slot size 0x18 keeps bank 0 in its historical layout; every bank must sit
# below the hardware runner's snapshot code at P:$0720 so it fits the one
# 0x800-word bootstrap DMA window (see run_corpus_hw.py in the xbtest repo).
SLOT_BASE = 0x100
SLOT_SIZE = 0x18
SNAP_ADDR = 0x0720
PROLOGUE = ["andi #$00,ccr"]

def asm(src_path, out_path):
    r = subprocess.run(
        ["cargo", "run", "-q", "-p", "dsp56300-asm", "--", "-f", "lod",
         "-o", out_path, src_path],
        cwd=REPO, capture_output=True, text=True)
    if r.returncode != 0:
        return r.stderr.strip() or r.stdout.strip()
    return None


def parse_lod(path):
    """simplified lod -> {space: {addr: word}}"""
    mem = {"P": {}, "X": {}, "Y": {}}
    with open(path) as f:
        for line in f:
            parts = line.split()
            if len(parts) == 3 and parts[0] in mem:
                mem[parts[0]][int(parts[1], 16)] = int(parts[2], 16)
    return mem


def case_source_lines(start, body):
    """Body lines may reference absolute case-relative addresses as @{N},
    replaced with the literal address start+N. Literal (non-label) targets
    are how the assembler's short branch/jump forms get selected."""
    lines = ["        org     p:$%04x" % start]
    for ins in PROLOGUE + list(body):
        ins = re.sub(r"@\{(-?\d+)\}",
                     lambda m: "$%04x" % (start + int(m.group(1))), ins)
        if ":" in ins.split()[0] and not ins.startswith(" "):
            lines.append(ins)  # label line
        else:
            lines.append("        " + ins)
    return lines


def emit_bank(outdir, prefix, cases, fixed_slots, fill=None, aux=None):
    """Assemble one corpus bank; returns the meta list.

    fixed_slots=True: historical bank-0 layout (0x18-word slots).
    fixed_slots=False: contiguous packing - each case starts at the previous
    case's end PC. Assembled sequentially so every case's org is exact.
    fill: Bank.fill dirty-state constants, emitted as the meta's first
    line (`fill kx cx ky cy sshb sslb`) so both engines and the
    hardware runner configure the same init from the meta alone.
    aux: Bank.aux verbatim asm lines (with their own org directives)
    appended to the combined image after the cases - vector pages /
    shared handlers for fault banks. Their words are covered by the
    sealed fingerprint through the combined lod.
    """
    meta = []
    combined = ["        org     p:$%04x" % SLOT_BASE]
    errors = []
    next_start = SLOT_BASE

    for i, (name, body, max_steps) in enumerate(cases):
        start = SLOT_BASE + i * SLOT_SIZE if fixed_slots else next_start
        lines = case_source_lines(start, body)
        src = "\n".join(lines) + "\n        end\n"
        src_path = os.path.join(outdir, "case_%s.asm" % name)
        lod_path = os.path.join(outdir, "case_%s.lod" % name)
        with open(src_path, "w") as f:
            f.write(src)
        err = asm(src_path, lod_path)
        if err:
            errors.append("%s: %s" % (name, err))
            continue
        words = parse_lod(lod_path)["P"]
        if not words:
            errors.append("%s: produced no P words" % name)
            continue
        end = max(words) + 1
        if fixed_slots and end > start + SLOT_SIZE:
            errors.append("%s: overflows slot (%d words)" % (name, end - start))
            continue
        if end > SNAP_ADDR:
            errors.append("%s: bank overflows P:$%04x (hw snapshot code)"
                          % (name, SNAP_ADDR))
            continue
        meta.append((name, start, end, max_steps))
        combined.extend(lines[1:])
        if fixed_slots:
            combined.append("        org     p:$%04x" % (start + SLOT_SIZE))
        next_start = end

    if aux:
        combined.extend(aux)

    if errors:
        print("assembly errors:")
        for e in errors:
            print("  " + e)
        sys.exit(1)

    comb_src = os.path.join(outdir, "%s.asm" % prefix)
    comb_lod = os.path.join(outdir, "%s.lod" % prefix)
    with open(comb_src, "w") as f:
        f.write("\n".join(combined) + "\n        end\n")
    err = asm(comb_src, comb_lod)
    if err:
        print("combined assembly failed:", err)
        sys.exit(1)

    # Verify combined == per-case words (catch cross-case interference)
    comb = parse_lod(comb_lod)
    for name, start, end, _ in meta:
        case = parse_lod(os.path.join(outdir, "case_%s.lod" % name))["P"]
        for a, w in case.items():
            assert comb["P"].get(a) == w, "combined mismatch at %s P:%04x" % (name, a)

    # Motorola _DATA format for sim56300's `load`
    with open(os.path.join(outdir, "%s_sim.lod" % prefix), "w") as f:
        f.write("_START CORPUS 0000 0000 0000\n")
        for space in ("P", "X", "Y"):
            words = comb[space]
            for a in sorted(words):
                # contiguous runs
                pass
            run = []
            run_start = None
            prev = None
            for a in sorted(words) + [None]:
                if a is not None and (prev is None or a == prev + 1):
                    if run_start is None:
                        run_start = a
                    run.append(words[a])
                    prev = a
                    continue
                if run:
                    f.write("_DATA %s %04X\n" % (space, run_start))
                    for j in range(0, len(run), 8):
                        f.write(" ".join("%06X" % w for w in run[j:j + 8]) + "\n")
                run = [words[a]] if a is not None else []
                run_start = a
                prev = a
        f.write("_END %04X\n" % SLOT_BASE)

    with open(os.path.join(outdir, "%s.meta" % prefix), "w") as f:
        if fill:
            f.write("fill %06x %06x %06x %06x %06x %06x\n" % tuple(fill))
        for name, start, end, max_steps in meta:
            f.write("case %s %04x %04x %d\n" % (name, start, end, max_steps))

    last_end = meta[-1][2] if meta else SLOT_BASE
    print("%s: %d cases, P image ends $%04x -> %s"
          % (prefix, len(meta), last_end, outdir))
    return meta




def load_seals():
    if os.path.exists(SEALS):
        with open(SEALS) as f:
            return json.load(f)
    return {}


def canonical_fingerprint(mem, meta_text):
    """sha256 over what both engines consume: sorted (space, addr, word)
    triples plus the meta lines. Deliberately not raw bytes - assembler
    symbol-record order is nondeterministic."""
    h = hashlib.sha256()
    for space in ("P", "X", "Y"):
        for a in sorted(mem[space]):
            h.update(b"%s %04X %06X\n" % (space.encode(), a, mem[space][a]))
    h.update(meta_text.encode())
    return h.hexdigest()


def bank_cases(name, bank):
    """Ordered (name, body, max_steps) for a bank; sweep bodies are
    generated lazily so non-generating commands need no difftest build."""
    sweeps = None
    out = []
    for case in bank.cases:
        if case in REGISTRY:
            body, steps, _theme = REGISTRY[case]
        else:
            if sweeps is None:
                sweeps = {n: (b, s) for n, b, s in cases_sweeps.sweep_cases()}
            body, steps = sweeps[case]
        out.append((case, body, steps))
    return out


def out_fingerprint(outdir, prefix):
    mem = parse_lod(os.path.join(outdir, "%s.lod" % prefix))
    with open(os.path.join(outdir, "%s.meta" % prefix)) as f:
        return canonical_fingerprint(mem, f.read())


def goldens_fingerprint(bank_name):
    gold = os.path.join(HERE, "goldens", bank_name)
    mem = parse_lod(os.path.join(gold, "corpus.lod"))
    with open(os.path.join(gold, "corpus.meta")) as f:
        return canonical_fingerprint(mem, f.read())


STATEFUL_MNEMONICS = {
    "add", "sub", "adc", "sbc", "inc", "dec", "addl", "subl", "addr",
    "subr", "neg", "not", "abs", "eor", "and", "or", "ori", "andi",
    "mpy", "mpyr", "mpyi", "mpyri", "mac", "macr", "maci", "macri",
    "macsu", "macuu", "mpysu", "mpyuu", "dmacss", "dmacsu", "dmacuu",
    "asl", "asr", "lsl", "lsr", "rol", "ror", "div", "norm", "normf",
    "bset", "bclr", "bchg", "jsr", "bsr", "enddo", "do", "dor", "rep",
}


def lint_loop_bodies(name, body):
    """Warn when a DO/DOR/REP body looks idempotent.

    An off-by-one loop count whose extra/missing iteration is a fixed
    point (e.g. `move x0,a` repeated) converges to the same final state
    and cannot be caught by the end-of-case snapshot. Heuristic: the
    body should contain at least one accumulating/shifting op, an
    EA-updating access, or a stack-popping read. Warning only - some
    probe cases legitimately test annulled/degenerate loops.
    """
    import re as _re

    def stateful(line):
        first = line.split()
        mnem = first[1] if ":" in first[0] and len(first) > 1 else first[0]
        return (mnem in STATEFUL_MNEMONICS
                or _re.search(r"\)\+|\)-|-\(", line)
                or "ssh" in line)

    i = 0
    while i < len(body):
        parts = body[i].split()
        mnem = parts[0]
        if mnem in ("do", "dor") and "forever" not in body[i]:
            label = body[i].rsplit(",", 1)[-1].strip()
            end = next((j for j in range(i + 1, len(body))
                        if body[j].split()[0].rstrip(":") == label), None)
            if end is not None and not any(
                    stateful(body[j]) for j in range(i + 1, end)):
                print("lint: %s: DO body at %r looks idempotent (an "
                      "off-by-one loop count would be invisible)"
                      % (name, body[i]), file=sys.stderr)
        elif mnem == "rep" and i + 1 < len(body):
            if not stateful(body[i + 1]):
                print("lint: %s: REP target %r looks idempotent"
                      % (name, body[i + 1]), file=sys.stderr)
        i += 1


def generate(outdir):
    os.makedirs(outdir, exist_ok=True)
    seals = load_seals()
    for name, bank in BANKS.items():
        if not bank.cases:
            continue
        prefix = "corpus_%s" % name
        cases = bank_cases(name, bank)
        for case_name, body, _steps in cases:
            lint_loop_bodies(case_name, body)
        emit_bank(outdir, prefix, cases, bank.fixed_slots, bank.fill, bank.aux)
        if name in seals:
            got = out_fingerprint(outdir, prefix)
            if got != seals[name]:
                print("sealed bank %s image changed (fingerprint %s... != "
                      "sealed %s...)" % (name, got[:12], seals[name][:12]))
                print("a case edit altered a golden-bound image: revert it, "
                      "or add a NEW case to the open bank instead; changing "
                      "%s requires hardware recapture + `corpus.py --seal %s`"
                      % (name, name))
                sys.exit(1)


def seal(bank_names):
    seals = load_seals()
    for name in bank_names:
        seals[name] = goldens_fingerprint(name)
        print("sealed %s from goldens: %s" % (name, seals[name]))
    with open(SEALS, "w") as f:
        json.dump(seals, f, indent=2, sort_keys=True)
        f.write("\n")


def status():
    seals = load_seals()
    for name, bank in BANKS.items():
        state = ("sealed" if name in seals else
                 "UNSEALED (awaiting hardware capture)" if bank.cases else
                 "open (empty)")
        print("%-4s %3d cases  %s" % (name, len(bank.cases), state))


def main():
    args = sys.argv[1:]
    if args and args[0] == "--status":
        return status()
    if args and args[0] == "--seal":
        return seal(args[1:])
    outdir = args[0] if args else os.path.join(HERE, "out")
    generate(outdir)
