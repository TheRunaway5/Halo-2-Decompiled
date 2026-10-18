"""Seeded value sweeps: randomized-but-deterministic ALU op chains.

Each case initializes x0/x1/y0/y1 and both accumulators from a
boundary-heavy value pool, then runs a chain of silicon-safe
single-word ALU/multiply/shift ops with SR stashes between them
(`movec sr,rN`), so intermediate CCR state is observable in the final
snapshot. Ops draw only from forms with existing silicon goldens:
no stack, loops, AGU updates, peripherals, mode-SR writes, or
out-of-range shift counts. The generator is pure python with a fixed
seed - regeneration is bit-stable.
"""

import random

THEME = "valsweeps"

_BOUNDARY = [
    0x000000, 0x000001, 0x000002, 0x7FFFFF, 0x800000, 0x800001,
    0xFFFFFF, 0xFFFFFE, 0xC00001, 0x400000, 0x3FFFFF, 0x000080,
    0x123456, 0xAAAAAA, 0x555555, 0x808080,
]

_QQQ_PAIRS = ["x0,x0", "y0,y0", "x1,x0", "y1,y0",
              "x0,y1", "y0,x0", "x1,y0", "y1,x1"]
_QQQQ_PAIRS = _QQQ_PAIRS + ["x1,x1", "y1,y1", "x0,x1", "y0,y1",
                            "y1,x0", "x0,y0", "y0,x1", "x1,y1"]
_MULSHIFT_REGS = ["y1", "x0", "y0", "x1"]
_DATA_REGS = ["x0", "x1", "y0", "y1"]

NUM_CASES = 72
_OPS_PER_CASE = 5


def _pick_value(rng):
    if rng.random() < 0.75:
        return rng.choice(_BOUNDARY)
    return rng.randrange(1 << 24)


def _pick_op(rng, acc):
    other = "b" if acc == "a" else "a"
    kind = rng.randrange(12)
    if kind == 0:
        return "%s %s,%s" % (rng.choice(
            ["add", "sub", "cmp", "cmpm", "cmpu", "and", "or", "eor"]),
            rng.choice(_DATA_REGS), acc)
    if kind == 1:
        return "%s %s,%s" % (rng.choice(["adc", "sbc"]),
                             rng.choice(["x", "y"]), acc)
    if kind == 2:
        return "%s %s,%s" % (rng.choice(
            ["addl", "subl", "addr", "subr"]), other, acc)
    if kind == 3:
        return "%s %s" % (rng.choice(
            ["abs", "neg", "rnd", "tst", "not", "inc", "dec",
             "lsl", "lsr", "asl", "asr", "rol", "ror"]), acc)
    if kind == 4:
        return "%s #%d,%s" % (rng.choice(["lsl", "lsr"]),
                              rng.randrange(24), acc)
    if kind == 5:
        return "%s #%d,%s,%s" % (rng.choice(["asl", "asr"]),
                                 rng.randrange(9), acc, acc)
    if kind == 6:
        return rng.choice(["max a,b", "maxm a,b",
                           "clb %s,%s" % (acc, other),
                           "tfr %s,%s" % (rng.choice(_DATA_REGS), acc)])
    if kind == 7:
        sign = rng.choice(["", "-"])
        return "%s %s%s,%s" % (rng.choice(["mpy", "mpyr", "mac", "macr"]),
                               sign, rng.choice(_QQQ_PAIRS), acc)
    if kind == 8:
        return "%s %s,%s" % (rng.choice(
            ["mpysu", "mpyuu", "macsu", "macuu"]),
            rng.choice(_QQQQ_PAIRS), acc)
    if kind == 9:
        return "%s %s,%s" % (rng.choice(["dmacss", "dmacsu", "dmacuu"]),
                             rng.choice(_QQQQ_PAIRS), acc)
    if kind == 10:
        return "mpy %s,#%d,%s" % (rng.choice(_MULSHIFT_REGS),
                                  rng.randrange(4), acc)
    return "%s %s,%s" % (rng.choice(["mac", "macr"]),
                         rng.choice(_QQQ_PAIRS), acc)


def _gen_case(rng):
    body = []
    for reg in _DATA_REGS:
        body.append("move #>$%06x,%s" % (_pick_value(rng), reg))
    body += ["clr a", "move %s,a1" % rng.choice(["x0", "x1"]),
             "clr b", "move %s,b1" % rng.choice(["y0", "y1"])]
    if rng.random() < 0.5:
        body.append("move %s,a0" % rng.choice(_DATA_REGS))
    if rng.random() < 0.5:
        body.append("move %s,b2" % rng.choice(_DATA_REGS))
    stash = ["r4", "r5"]
    for i in range(_OPS_PER_CASE):
        body.append(_pick_op(rng, rng.choice("ab")))
        if i in (2, 4):
            body.append("movec sr,%s" % stash.pop(0))
    return body


def _generate():
    rng = random.Random(0x56D5B3 ^ 10)  # bank number 10
    cases = []
    for i in range(NUM_CASES):
        cases.append(("valsweep_%02d" % i, _gen_case(rng), 100))
    return cases


CASES = _generate()
VALSWEEP_NAMES = [name for name, _, _ in CASES]
