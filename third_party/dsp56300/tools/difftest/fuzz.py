#!/usr/bin/env python3
"""Differential fuzzer: dsp56300 JIT step-mode vs block-mode codegen.

Generates random legal DSP56300 programs, assembles them, runs the
difftest harness in --mode=step and --mode=block, and diffs the
architectural state. Any divergence is a real JIT bug (same JIT, two
codegen paths). Findings are shrunk to minimal reproducers.
"""

import os
import random
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ASM = ROOT + "/target/release/dsp56300-asm"
DIFFTEST = ROOT + "/target/release/difftest"
WORK = os.path.join(ROOT, "tools", "difftest", "out")
FINDINGS_DIR = os.path.join(WORK, "findings")
os.makedirs(FINDINGS_DIR, exist_ok=True)

START_PC = 0x100
MAX_STEPS = 5000
INFORMATIONAL = {"cyc", "ictr", "steps"}

BOUNDARY = [
    0x000000, 0x000001, 0x000002, 0x7FFFFF, 0x800000, 0x800001,
    0xFFFFFF, 0xFFFFFE, 0xC00001, 0x400000, 0x3FFFFF, 0x000080,
    0x123456, 0xAAAAAA, 0x555555, 0x808080, 0x000100, 0x010000,
]
DATA_REGS = ["x0", "x1", "y0", "y1"]
QQQ_PAIRS = ["x0,x0", "y0,y0", "x1,x0", "y1,y0",
             "x0,y1", "y0,x0", "x1,y0", "y1,x1"]
QQQQ_PAIRS = QQQ_PAIRS + ["x1,x1", "y1,y1", "x0,x1", "y0,y1",
                          "y1,x0", "x0,y0", "y0,x1", "x1,y1"]
CONDS = ["eq", "ne", "cc", "cs", "ge", "gt", "le", "lt",
         "mi", "pl", "ls", "ec", "es", "lc", "nr", "nn"]
SR_MASKS = [0xC00300, 0xC00700, 0xC00B00, 0xD00300, 0xE00300]

# X scratch $0100-$03BF, Y scratch $0100-$03BF (within legal windows)
def xaddr(rng):
    return rng.randrange(0x0100, 0x03C0)

def yaddr(rng):
    return rng.randrange(0x0100, 0x03C0)


def pick_value(rng):
    if rng.random() < 0.7:
        return rng.choice(BOUNDARY)
    return rng.randrange(1 << 24)


class Gen(object):
    def __init__(self, rng, weights, nbody):
        self.rng = rng
        self.weights = weights
        self.nbody = nbody
        self.nlabel = 0
        self.do_depth = 0

    def label(self):
        self.nlabel += 1
        return "L%d" % self.nlabel

    # ---------------- units ----------------

    def u_alu(self):
        rng = self.rng
        acc = rng.choice("ab")
        other = "b" if acc == "a" else "a"
        k = rng.randrange(14)
        if k == 0:
            return ["%s %s,%s" % (rng.choice(
                ["add", "sub", "cmp", "cmpm", "cmpu", "and", "or", "eor"]),
                rng.choice(DATA_REGS), acc)]
        if k == 1:
            return ["%s %s,%s" % (rng.choice(["adc", "sbc"]),
                                  rng.choice(["x", "y"]), acc)]
        if k == 2:
            return ["%s %s,%s" % (rng.choice(
                ["addl", "subl", "addr", "subr"]), other, acc)]
        if k == 3:
            return ["%s %s" % (rng.choice(
                ["abs", "neg", "rnd", "tst", "not", "inc", "dec",
                 "lsl", "lsr", "asl", "asr", "rol", "ror"]), acc)]
        if k == 4:
            return ["%s #%d,%s" % (rng.choice(["lsl", "lsr"]),
                                   rng.randrange(24), acc)]
        if k == 5:
            return ["%s #%d,%s,%s" % (rng.choice(["asl", "asr"]),
                                      rng.randrange(9), acc, acc)]
        if k == 6:
            return [rng.choice(["max a,b", "maxm a,b",
                                "clb %s,%s" % (acc, other),
                                "tfr %s,%s" % (rng.choice(DATA_REGS), acc)])]
        if k == 7:
            sign = rng.choice(["", "-"])
            return ["%s %s%s,%s" % (rng.choice(["mpy", "mpyr", "mac", "macr"]),
                                    sign, rng.choice(QQQ_PAIRS), acc)]
        if k == 8:
            return ["%s %s,%s" % (rng.choice(
                ["mpysu", "mpyuu", "macsu", "macuu"]),
                rng.choice(QQQQ_PAIRS), acc)]
        if k == 9:
            return ["%s %s,%s" % (rng.choice(["dmacss", "dmacsu", "dmacuu"]),
                                  rng.choice(QQQQ_PAIRS), acc)]
        if k == 10:
            return ["mpy %s,#%d,%s" % (rng.choice(DATA_REGS),
                                       rng.randrange(4), acc)]
        if k == 11:
            # immediate forms
            if rng.random() < 0.5:
                return ["%s #>$%06x,%s" % (rng.choice(
                    ["add", "sub", "cmp", "and", "or", "eor"]),
                    pick_value(rng), acc)]
            return ["%s #$%02x,%s" % (rng.choice(
                ["add", "sub", "cmp", "and", "or", "eor"]),
                rng.randrange(0x40), acc)]
        if k == 12:
            return ["%s #$%02x,ccr" % (rng.choice(["ori", "andi"]),
                                       rng.randrange(0x100))]
        return ["div %s,%s" % (rng.choice(DATA_REGS), acc)]

    def u_sr_stash(self):
        return ["movec sr,%s" % self.rng.choice(
            ["r4", "r5", "r6", "r7", "n4", "n5", "x0", "y1"])]

    def u_sr_write(self):
        return ["movec #>$%06x,sr" % self.rng.choice(SR_MASKS)]

    def u_pmove(self):
        rng = self.rng
        k = rng.randrange(6)
        acc = rng.choice("ab")
        ops1 = ["add x0,%s" % acc, "sub y0,%s" % acc, "cmp x1,%s" % acc,
                "and y1,%s" % acc, "or x0,%s" % acc, "eor y0,%s" % acc,
                "asl %s" % acc, "asr %s" % acc, "abs %s" % acc,
                "neg %s" % acc, "rnd %s" % acc, "tst %s" % acc,
                "clr %s" % acc, "not %s" % acc, "tfr x0,%s" % acc,
                "mac x0,y0,%s" % acc, "mpy x1,y1,%s" % acc,
                "macr y0,x0,%s" % acc, "addl %s,%s" % (
                    "b" if acc == "a" else "a", acc)]
        op = rng.choice(ops1)
        r = "r%d" % rng.randrange(8)
        upd = rng.choice(["+", "-", "", "+n%s" % r[1], "-n%s" % r[1]])
        ea = "(%s)%s" % (r, upd) if upd else "(%s)" % r
        sp = rng.choice(["x", "y"])
        if k == 0:  # read ea -> reg
            dst = rng.choice(DATA_REGS + ["a", "b"])
            return ["%s %s:%s,%s" % (op, sp, ea, dst)]
        if k == 1:  # write reg -> ea
            src = rng.choice(DATA_REGS + ["a", "b"])
            return ["%s %s,%s:%s" % (op, src, sp, ea)]
        if k == 2:  # XY dual read
            ra = "r%d" % rng.randrange(4)
            rb = "r%d" % rng.randrange(4, 8)
            d1 = rng.choice(["x0", "x1", "a", "b"])
            d2c = [d for d in ["y0", "y1", "a", "b"] if d != d1]
            d2 = rng.choice(d2c)
            return ["%s x:(%s)+,%s y:(%s)+,%s" % (op, ra, d1, rb, d2)]
        if k == 3:  # XY dual write / mixed
            ra = "r%d" % rng.randrange(4)
            rb = "r%d" % rng.randrange(4, 8)
            s1 = rng.choice(["x0", "x1", "a", "b"])
            s2 = rng.choice(["y0", "y1", "a", "b"])
            if rng.random() < 0.5:
                return ["%s %s,x:(%s)+ %s,y:(%s)+" % (op, s1, ra, s2, rb)]
            return ["%s %s,x:(%s)+ y:(%s)+,%s" % (
                op, s1, ra, rb, rng.choice(["y0", "y1"]))]
        if k == 4:  # reg-to-reg parallel
            s = rng.choice(["a", "b"])
            d = rng.choice(["x0", "x1", "y0", "y1"])
            return ["%s %s,%s" % (op, s, d)]
        # immediate + reg move
        return ["move #>$%06x,%s %s,%s" % (
            pick_value(rng), rng.choice(["x0", "x1"]),
            rng.choice(["a", "b"]), rng.choice(["y0", "y1"]))]

    def u_move(self):
        rng = self.rng
        k = rng.randrange(8)
        if k == 0:
            return ["move #>$%06x,%s" % (pick_value(rng), rng.choice(
                DATA_REGS + ["a", "b", "a0", "a1", "b0", "b1", "a2", "b2",
                             "r%d" % rng.randrange(8),
                             "n%d" % rng.randrange(8)]))]
        if k == 1:
            src = rng.choice(DATA_REGS + ["a", "b"])
            return ["move %s,%s:$%04x" % (src, rng.choice("xy"), xaddr(rng))]
        if k == 2:
            dst = rng.choice(DATA_REGS + ["a", "b", "a0", "b1"])
            return ["move %s:$%04x,%s" % (rng.choice("xy"), xaddr(rng), dst)]
        if k == 3:  # L-move write+read
            a = xaddr(rng)
            lw = rng.choice(["a10", "b10", "x", "y", "ab", "ba"])
            lr = rng.choice(["a10", "b10", "x", "y", "ab", "ba"])
            return ["move %s,l:$%04x" % (lw, a),
                    "move l:$%04x,%s" % (a, lr)]
        if k == 4:  # movem p scratch
            a = 0x700 + rng.randrange(0xC0)
            s = rng.choice(DATA_REGS)
            d = rng.choice([d for d in DATA_REGS if d != s])
            return ["movem %s,p:>$%04x" % (s, a),
                    "movem p:>$%04x,%s" % (a, d)]
        if k == 5:  # move reg,reg
            s = rng.choice(DATA_REGS + ["a", "b", "r%d" % rng.randrange(8)])
            d = rng.choice(["r%d" % rng.randrange(8),
                            "n%d" % rng.randrange(8), "a1", "b1", "x0", "y0"])
            return ["move %s,%s" % (s, d)]
        if k == 6:  # ea move with update
            r = "r%d" % rng.randrange(8)
            upd = rng.choice(["+", "-", "+n%s" % r[1]])
            if rng.random() < 0.5:
                return ["move %s,%s:(%s)%s" % (
                    rng.choice(DATA_REGS + ["a", "b"]),
                    rng.choice("xy"), r, upd)]
            return ["move %s:(%s)%s,%s" % (
                rng.choice("xy"), r, upd,
                rng.choice(DATA_REGS + ["a", "b"]))]
        # pre-decrement
        r = "r%d" % rng.randrange(8)
        return ["move %s:-(%s),%s" % (rng.choice("xy"), r,
                                      rng.choice(DATA_REGS))]

    def u_agu(self):
        rng = self.rng
        k = rng.randrange(5)
        if k == 0:
            i = rng.randrange(8)
            r, n = "r%d" % i, "n%d" % i  # nN must match rN in EA
            d = rng.choice(["r%d" % rng.randrange(8),
                            "n%d" % rng.randrange(8), "x0", "y1", "a", "b"])
            return ["lua (%s)%s,%s" % (r, rng.choice(
                ["+", "-", "+%s" % n, "-%s" % n]), d)]
        if k == 1:
            r = "r%d" % rng.randrange(8)
            disp = rng.randrange(-32, 32)
            d = rng.choice(["r%d" % rng.randrange(8),
                            "n%d" % rng.randrange(8)])
            if disp < 0:
                return ["lua (%s-%d),%s" % (r, -disp, d)]
            return ["lua (%s+%d),%s" % (r, disp, d)]
        if k == 2:
            return ["lra $%04x,%s" % (0x100 + rng.randrange(0x100),
                                      rng.choice(["r%d" % rng.randrange(8),
                                                  "n%d" % rng.randrange(8)]))]
        if k == 3:  # modulo unit: set m, do a few updates, restore
            i = rng.randrange(8)
            m = rng.choice([1, 3, 4, 5, 7, 15])
            r = "r%d" % i
            base = 0x100 + rng.randrange(0x2A0)
            body = ["movec #$%x,m%d" % (m, i),
                    "move #>$%04x,%s" % (base, r)]
            for _ in range(rng.randrange(1, 4)):
                body.append("move x:(%s)%s,%s" % (
                    r, rng.choice(["+", "-", "+n%d" % i]),
                    rng.choice(DATA_REGS)))
            body.append("move %s,n%d" % (r, (i + 1) % 8))
            body.append("movec #>$ffffff,m%d" % i)
            return body
        # bit-reverse unit
        i = rng.randrange(8)
        r = "r%d" % i
        body = ["movec #$0,m%d" % i,
                "move #>$%04x,%s" % (0x100 + 8 * rng.randrange(0x40), r),
                "move #$%x,n%d" % (1 << rng.randrange(4), i),
                "move x:(%s)+n%d,%s" % (r, i, rng.choice(DATA_REGS)),
                "move %s,n%d" % (r, (i + 1) % 8),
                "movec #>$ffffff,m%d" % i]
        return body

    def u_bits(self):
        rng = self.rng
        k = rng.randrange(4)
        bit = rng.randrange(24)
        if k == 0:
            a = xaddr(rng)
            return ["%s #$%x,%s:$%04x" % (rng.choice(
                ["bset", "bclr", "bchg", "btst"]), bit,
                rng.choice("xy"), a)]
        if k == 1:
            reg = rng.choice(DATA_REGS + ["a", "b", "a1", "b1",
                                          "r%d" % rng.randrange(8),
                                          "n%d" % rng.randrange(8)])
            return ["%s #$%x,%s" % (rng.choice(
                ["bset", "bclr", "bchg", "btst"]), bit, reg)]
        if k == 2:
            r = "r%d" % rng.randrange(8)
            return ["%s #$%x,%s:(%s)" % (rng.choice(
                ["bset", "bclr", "bchg", "btst"]), bit,
                rng.choice("xy"), r)]
        a = rng.randrange(0x40)  # aa short form
        return ["%s #$%x,%s:$%02x" % (rng.choice(
            ["bset", "bclr", "bchg", "btst"]), bit, rng.choice("xy"), a)]

    def u_branch(self):
        rng = self.rng
        lbl = self.label()
        nfill = rng.randrange(1, 3)
        fill = []
        for _ in range(nfill):
            fill += rng.choice([self.u_alu, self.u_move, self.u_bits])()
        k = rng.randrange(6)
        if k == 0:
            head = ["b%s %s" % (rng.choice(CONDS), lbl)]
        elif k == 1:
            head = ["j%s %s" % (rng.choice(CONDS), lbl)]
        elif k == 2:
            head = [rng.choice(["bra", "jmp"]) + " " + lbl]
        elif k == 3:
            bit = rng.randrange(24)
            src = rng.choice([
                "x:$%02x" % rng.randrange(0x40),
                "y:$%02x" % rng.randrange(0x40),
                rng.choice(DATA_REGS),
                "n%d" % rng.randrange(8),
                "x:(r%d)" % rng.randrange(8),
            ])
            head = ["%s #$%x,%s,%s" % (rng.choice(["jclr", "jset"]),
                                       bit, src, lbl)]
        elif k == 4:
            bit = rng.randrange(24)
            src = rng.choice([
                "x:$%02x" % rng.randrange(0x40),
                "y:$%02x" % rng.randrange(0x40),
                rng.choice(DATA_REGS),
                "y:(r%d)-" % rng.randrange(8),
            ])
            head = ["%s #$%x,%s,%s" % (rng.choice(["brclr", "brset"]),
                                       bit, src, lbl)]
        else:
            # bsclr/bsset would push; skip. plain bcc again
            head = ["b%s %s" % (rng.choice(CONDS), lbl)]
        return head + fill + ["%s: nop" % lbl]

    def u_tcc(self):
        rng = self.rng
        k = rng.randrange(3)
        cc = rng.choice(CONDS)
        if k == 0:
            return ["t%s %s,%s" % (cc, rng.choice(DATA_REGS),
                                   rng.choice("ab"))]
        if k == 1:
            return ["t%s %s,%s r%d,r%d" % (cc, rng.choice(DATA_REGS),
                                           rng.choice("ab"),
                                           rng.randrange(8),
                                           rng.randrange(8))]
        # ifcc
        acc = rng.choice("ab")
        op = rng.choice(["add x0,%s" % acc, "sub y0,%s" % acc,
                         "asl %s" % acc, "neg %s" % acc,
                         "add y1,%s" % acc, "and x1,%s" % acc])
        suf = rng.choice(["", ".u"])
        return ["%s if%s%s" % (op, cc, suf)]

    def u_do(self):
        rng = self.rng
        if self.do_depth >= 2:
            return self.u_alu()
        lbl = self.label()
        lc = rng.randrange(0, 6)
        k = rng.randrange(5)
        head = []
        if k == 0:
            # immediate #0 means 65536 iterations (documented); avoid
            head = ["do #%d,%s" % (max(lc, 1), lbl)]
        elif k == 1:
            n = "n%d" % rng.randrange(8)
            head = ["move #$%x,%s" % (lc, n), "do %s,%s" % (n, lbl)]
        elif k == 2:
            a = rng.randrange(0x20, 0x40)  # do aa form: $00-$3f only
            sp = rng.choice("xy")
            head = ["move #>$%06x,x0" % lc, "move x0,%s:$%02x" % (sp, a),
                    "do %s:$%02x,%s" % (sp, a, lbl)]
        elif k == 3:
            head = ["dor #%d,%s" % (max(lc, 1), lbl)]
        else:
            a = xaddr(rng)
            r = "r%d" % rng.randrange(8)
            head = ["move #>$%06x,x1" % lc, "move x1,x:$%04x" % a,
                    "move #>$%04x,%s" % (a, r),
                    "do x:(%s)+,%s" % (r, lbl)]
        self.do_depth += 1
        body = []
        nb = rng.randrange(1, 4)
        for _ in range(nb):
            body += rng.choice([self.u_alu, self.u_move, self.u_pmove,
                                self.u_bits, self.u_tcc, self.u_do,
                                self.u_rep])()
        self.do_depth -= 1
        return head + body + ["%s: nop" % lbl]

    def u_rep(self):
        rng = self.rng
        # single-word repeatable target
        acc = rng.choice("ab")
        tgt = rng.choice(["add x0,%s" % acc, "asr %s" % acc,
                          "asl %s" % acc, "add y1,%s" % acc,
                          "ror %s" % acc, "mac x0,y0,%s" % acc,
                          "move x:(r%d)+,x1" % rng.randrange(4),
                          "sub x1,%s" % acc])
        k = rng.randrange(3)
        lc = rng.randrange(0, 6)
        if k == 0:
            # immediate #0 means 65536 iterations (documented); avoid
            return ["rep #%d" % max(lc, 1), tgt]
        if k == 1:
            n = "n%d" % rng.randrange(8)
            return ["move #$%x,%s" % (lc, n), "rep %s" % n, tgt]
        a = rng.randrange(0x20, 0x40)  # rep aa form: $00-$3f only
        return ["move #>$%06x,y0" % lc, "move y0,y:$%02x" % a,
                "rep y:$%02x" % a, tgt]

    def u_sub(self):
        rng = self.rng
        sub = self.label()
        over = self.label()
        body = []
        for _ in range(rng.randrange(1, 3)):
            body += rng.choice([self.u_alu, self.u_move])()
        call = rng.choice(["bsr %s" % sub, "jsr %s" % sub])
        return ([call, "nop", "bra %s" % over, "%s:" % sub] + body +
                ["rts", "%s: nop" % over])

    def loop_head(self, mnem, count, label):
        """A DO/DOR header taking its count immediately or from a register.

        Returns the lines needed, so a register-sourced count carries its
        own setup.
        """
        rng = self.rng
        if rng.random() < 0.5:
            return ["%s #%d,%s" % (mnem, count, label)]
        n = "n%d" % rng.randrange(8)
        return ["move #$%x,%s" % (count, n), "%s %s,%s" % (mnem, n, label)]

    def u_call_in_loop(self):
        """A jsr from inside a DO body into a subroutine that runs its own
        DO loop and returns from the loop's exit label.

        u_do and u_sub never combine on their own - u_do's body never calls
        and u_sub's body never loops - so nothing else in this generator
        produces hardware loops with calls woven through them, which is what
        the MCPX EP encoder overlays do everywhere.
        """
        rng = self.rng
        lend, sub, over, send = (self.label(), self.label(),
                                 self.label(), self.label())
        outer = rng.choice(["do", "dor"])
        body = self.u_alu()
        body += [rng.choice(["jsr %s" % sub, "bsr %s" % sub])]
        # Program flow may not be redirected from the last three words of a
        # loop body (DO/DOR restriction on LA-2..LA), so keep the call clear
        # of the end - and likewise the rts inside the subroutine's loop.
        body += ["nop", "nop", "nop"]
        loop = self.loop_head(outer, rng.randrange(1, 5), lend)
        loop += body + ["%s: nop" % lend]

        # The inner loop is long enough that the run loop's cycle budget
        # lands inside it, exercising the inline loop's backedge bail and
        # the resume that follows - the EP's loops run 64 taps.
        subbody = self.u_alu()
        subbody += self.loop_head(rng.choice(["do", "dor"]),
                                  rng.randrange(20, 64), send)
        subbody += self.u_alu() + self.u_pmove() + ["nop", "nop", "nop"]
        # The EP's shape: the routine's rts sits at LA + 1, immediately
        # after the inner loop's last instruction.
        subbody += ["%s: rts" % send]
        return (loop + ["bra %s" % over, "%s:" % sub] + subbody +
                ["%s: nop" % over])

    # Single-word instructions. Swapping one for another in place keeps the
    # word count fixed, so everything after the patched word stays put.
    ONE_WORD = ["nop", "inc a", "dec a", "add x0,a", "sub x0,a", "and x1,b",
                "or y0,a", "eor y1,b", "asl a", "asr b", "neg a", "abs b",
                "clr b", "tst a", "move x0,y1", "move a,b", "not a",
                "rol b", "ror a", "add x0,b", "sub y0,b", "lsl a", "lsr b",
                "clr a", "tst b", "asl b", "asr a", "neg b", "abs a",
                "inc b", "dec b", "move y0,x1", "move b,a"]

    def u_selfmod(self):
        """Rewrite P words that cached blocks already cover, then re-enter
        the rewritten code from more than one PC.

        The MCPX EP DMAs code overlays over P:$0300+ while a dozen entry
        points into that region already sit in the block cache, so a rewrite
        has to be seen by every block spanning it, not just the first one
        entered afterwards. Nothing else here writes P at all, which is how
        a suite green in both modes still ran stale code.

        The replacement words live in the program itself and are copied in
        with movem, so the generator never has to know an encoding.
        """
        rng = self.rng
        sub, mid, tgt, over = (self.label() for _ in range(4))
        alt = [self.label() for _ in range(3)]
        orig = [self.label() for _ in range(3)]
        stub = [rng.choice(self.ONE_WORD) for _ in range(3)]
        repl = [rng.choice(self.ONE_WORD) for _ in range(3)]
        anchors = [sub, mid, tgt]

        # Where the rewrite lands and how wide it is. A patch at MID or TGT
        # falls inside both the outer (SUB) and the inner (MID) block, which
        # is the overlapping shape; a patch at SUB covers the simple case.
        first = rng.choice([0, 1, 1, 2, 2])
        width = min(rng.choice([1, 1, 1, 2, 3]), 3 - first)
        reg = rng.choice(["a1", "b1", "x0", "x1", "y0", "y1"])

        lines = ["move #%s,r0" % anchors[first],
                 "move #%s,r1" % alt[first],
                 "jsr %s" % sub, "jsr %s" % mid]
        for _ in range(width):
            lines += ["movem p:(r1)+,%s" % reg, "movem %s,p:(r0)+" % reg]
        # Re-enter from one or both entry points, in either order: whichever
        # block recompiles first must not launder the other one clean.
        lines += ["jsr %s" % e for e in rng.choice(
            [[sub], [mid], [sub, mid], [mid, sub], [sub, mid, sub]])]
        # Half the time, swap the original words back and run again. An
        # overlay that cycles is what the EP does every frame, and it is the
        # case a translation cache keyed by content has to get right: the
        # words returning are the ones already translated.
        if rng.random() < 0.5:
            lines += ["move #%s,r2" % anchors[first],
                      "move #%s,r3" % orig[first]]
            for _ in range(width):
                lines += ["movem p:(r3)+,%s" % reg,
                          "movem %s,p:(r2)+" % reg]
            lines += ["jsr %s" % e for e in rng.choice(
                [[sub], [mid], [sub, mid], [mid, sub]])]
        lines += ["bra %s" % over,
                  "%s: %s" % (sub, stub[0]),
                  "%s: %s" % (mid, stub[1]),
                  "%s: %s" % (tgt, stub[2]),
                  "rts"]
        # Unreachable: the replacement words, and a spare copy of the
        # originals, read as data by the movem pairs above.
        lines += ["%s: %s" % (alt[i], repl[i]) for i in range(3)]
        lines += ["%s: %s" % (orig[i], stub[i]) for i in range(3)]
        lines += ["%s: nop" % over]
        return lines

    def u_stack(self):
        rng = self.rng
        k = rng.randrange(3)
        if k == 0:
            mid = []
            for _ in range(rng.randrange(0, 2)):
                mid += self.u_alu()
            return (["movec #>$%06x,ssh" % pick_value(rng)] + mid +
                    ["movec ssh,%s" % rng.choice(
                        ["r4", "r5", "n6", "x1", "y0"])])
        if k == 1:
            return ["movec #>$%06x,%s" % (pick_value(rng),
                                          rng.choice(["lc", "la"])),
                    "movec %s,%s" % (rng.choice(["lc", "la"]),
                                     rng.choice(["r6", "r7", "y1"]))]
        return ["movec %s,%s" % (rng.choice(["sp", "sc", "lc", "la", "ssl"]),
                                 rng.choice(["x0", "y1", "r5", "n7"]))]

    def u_forever(self):
        rng = self.rng
        if self.do_depth >= 1:
            return self.u_alu()
        lbl = self.label()
        n = rng.randrange(2, 5)
        return ["clr a", "move #>$000001,x0",
                "%s forever,%s" % (rng.choice(["do", "dor"]), lbl),
                "add x0,a", "cmp #$%x,a" % n, "nop",
                rng.choice(["brkge", "brkeq"]),
                "nop", "nop", "nop", "%s: nop" % lbl,
                "movec #>$ffffff,la", "movec #>$0,lc"]

    def u_enddo(self):
        rng = self.rng
        if self.do_depth >= 2:
            return self.u_alu()
        lbl = self.label()
        return ["clr b", "move #>$000001,x1",
                "do #%d,%s" % (rng.randrange(2, 6), lbl),
                "add x1,b", "enddo", "add x1,b", "nop", "nop",
                "%s: nop" % lbl, "movec #>$0,lc"]

    def u_vsl(self):
        rng = self.rng
        r = "r%d" % rng.randrange(8)
        return ["vsl %s,%d,(%s)+" % (rng.choice("ab"), rng.randrange(2), r)]

    # ---------------- program ----------------

    UNITS = {
        "alu": "u_alu", "sr_stash": "u_sr_stash", "sr_write": "u_sr_write",
        "pmove": "u_pmove", "move": "u_move", "agu": "u_agu",
        "bits": "u_bits", "branch": "u_branch", "tcc": "u_tcc",
        "do": "u_do", "rep": "u_rep", "sub": "u_sub", "stack": "u_stack",
        "forever": "u_forever", "enddo": "u_enddo", "vsl": "u_vsl",
        "call_in_loop": "u_call_in_loop", "selfmod": "u_selfmod",
    }

    def prologue(self):
        rng = self.rng
        lines = []
        for reg in DATA_REGS:
            lines.append("move #>$%06x,%s" % (pick_value(rng), reg))
        lines += ["clr a", "move %s,a1" % rng.choice(["x0", "x1"]),
                  "clr b", "move %s,b1" % rng.choice(["y0", "y1"])]
        if rng.random() < 0.5:
            lines.append("move %s,a0" % rng.choice(DATA_REGS))
        if rng.random() < 0.4:
            lines.append("move %s,b2" % rng.choice(DATA_REGS))
        for i in range(8):
            lines.append("move #>$%04x,r%d" % (0x100 + rng.randrange(0x2A0), i))
            lines.append("move #$%x,n%d" % (rng.randrange(8), i))
        # seed a few memory cells
        for _ in range(rng.randrange(2, 5)):
            lines.append("move #>$%06x,x0" % pick_value(rng))
            lines.append("move x0,%s:$%04x" % (rng.choice("xy"), xaddr(rng)))
        lines.append("move #>$%06x,x0" % pick_value(rng))
        return lines

    def generate(self):
        rng = self.rng
        lines = self.prologue()
        names = list(self.weights.keys())
        wts = [self.weights[n] for n in names]
        count = 0
        while count < self.nbody:
            name = rng.choices(names, weights=wts)[0]
            unit = getattr(self, self.UNITS[name])()
            lines += unit
            count += len(unit)
        lines.append("nop")
        return lines


# ---------------- pools ----------------

POOLS = {
    "mixed": {"alu": 20, "sr_stash": 4, "sr_write": 2, "pmove": 12,
              "move": 12, "agu": 8, "bits": 8, "branch": 8, "tcc": 6,
              "do": 6, "rep": 5, "sub": 3, "stack": 4, "forever": 1,
              "enddo": 1, "vsl": 1, "call_in_loop": 4,
              "selfmod": 4},
    "flow": {"alu": 8, "sr_stash": 2, "sr_write": 1, "pmove": 4,
             "move": 6, "agu": 2, "bits": 4, "branch": 20, "tcc": 10,
             "do": 14, "rep": 8, "sub": 8, "stack": 4, "forever": 4,
             "enddo": 4, "vsl": 0, "call_in_loop": 14,
             "selfmod": 8},
    "flags": {"alu": 30, "sr_stash": 10, "sr_write": 4, "pmove": 8,
              "move": 4, "agu": 0, "bits": 6, "branch": 4, "tcc": 10,
              "do": 2, "rep": 3, "sub": 0, "stack": 2, "forever": 0,
              "enddo": 0, "vsl": 0, "call_in_loop": 0,
              "selfmod": 0},
    "agu": {"alu": 6, "sr_stash": 2, "sr_write": 0, "pmove": 25,
            "move": 15, "agu": 20, "bits": 4, "branch": 2, "tcc": 2,
            "do": 4, "rep": 4, "sub": 0, "stack": 0, "forever": 0,
            "enddo": 0, "vsl": 3, "call_in_loop": 0, "selfmod": 0},
    "stack": {"alu": 8, "sr_stash": 4, "sr_write": 2, "pmove": 4,
              "move": 6, "agu": 2, "bits": 2, "branch": 6, "tcc": 2,
              "do": 12, "rep": 4, "sub": 14, "stack": 14, "forever": 3,
              "enddo": 3, "vsl": 0, "call_in_loop": 14,
              "selfmod": 6},
    # Overlay loads under every kind of surrounding control flow: the EP
    # rewrites P while calls, hardware loops and branches are in flight.
    "selfmod": {"alu": 8, "sr_stash": 1, "sr_write": 0, "pmove": 4,
                "move": 4, "agu": 2, "bits": 2, "branch": 6, "tcc": 2,
                "do": 6, "rep": 2, "sub": 6, "stack": 2, "forever": 1,
                "enddo": 1, "vsl": 0, "call_in_loop": 6, "selfmod": 40},
}


# ---------------- harness ----------------

def write_asm(lines, path):
    out = ["        org     p:$0100"]
    for ln in lines:
        if ":" in ln.split()[0]:
            out.append(ln)
        else:
            out.append("        " + ln)
    out.append("        end")
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")


def assemble(asm_path, lod_path):
    p = subprocess.run([ASM, "-f", "lod", "-o", lod_path, asm_path],
                       capture_output=True, text=True, timeout=30)
    return p.returncode == 0, p.stderr


def lod_end_pc(lod_path):
    end = 0
    with open(lod_path) as f:
        for line in f:
            parts = line.split()
            if len(parts) == 3 and parts[0] == "P":
                end = max(end, int(parts[1], 16) + 1)
    return end


def run_mode(lod_path, meta_path, mode):
    try:
        p = subprocess.run(
            [DIFFTEST, "corpus", lod_path, meta_path,
             "--boot=hw", "--mode=" + mode],
            capture_output=True, text=True, timeout=30)
    except subprocess.TimeoutExpired:
        return None, "TIMEOUT-HOST"
    if p.returncode != 0:
        return None, "rc=%d %s" % (p.returncode, p.stderr.strip()[-300:])
    regs = {}
    for line in p.stdout.splitlines():
        parts = line.split()
        if len(parts) == 2 and parts[0] not in ("case", "end"):
            regs[parts[0]] = parts[1]
    return regs, None


def classify(lines, tag):
    """Run one program both ways. Returns (status, detail).

    status: 'ok', 'skip' (both bad the same way), 'diverge'
    """
    asm_path = os.path.join(WORK, "cur_%s.asm" % tag)
    lod_path = os.path.join(WORK, "cur_%s.lod" % tag)
    meta_path = os.path.join(WORK, "cur_%s.meta" % tag)
    write_asm(lines, asm_path)
    ok, err = assemble(asm_path, lod_path)
    if not ok:
        return "asmfail", err
    end = lod_end_pc(lod_path)
    with open(meta_path, "w") as f:
        f.write("case p %x %x %d\n" % (START_PC, end, MAX_STEPS))
    s, serr = run_mode(lod_path, meta_path, "step")
    b, berr = run_mode(lod_path, meta_path, "block")
    if s is None and b is None:
        return "skip", "both failed: step[%s] block[%s]" % (serr, berr)
    if s is None or b is None:
        return "diverge", {"kind": "crash", "step_err": serr,
                           "block_err": berr, "step": s, "block": b}
    end_hex = "%06x" % end
    s_done = s.get("pc") == end_hex
    b_done = b.get("pc") == end_hex
    if not s_done and not b_done:
        # step accounting differs between modes (block mode retires whole
        # blocks), so timeout states legitimately differ: always a skip.
        return "skip", "both did not reach end"
    if s_done != b_done:
        return "diverge", {"kind": "one-mode-timeout",
                           "step_done": s_done, "block_done": b_done,
                           "step": s, "block": b}
    diffs = {k for k in set(s) | set(b)
             if k not in INFORMATIONAL and s.get(k) != b.get(k)}
    if diffs:
        return "diverge", {"kind": "state", "regs": sorted(diffs),
                           "step": s, "block": b}
    return "ok", None


# ---------------- validity for shrinking ----------------

def valid_structure(lines):
    """Conservative spelling rules that must survive shrinking."""
    ops = []
    for ln in lines:
        w = ln.split()
        if not w:
            return False
        if ":" in w[0]:
            ops.append(("label", w[0].rstrip(":"), ln))
        else:
            ops.append((w[0], None, ln))
    label_idx = {lab: i for i, (op, lab, _) in enumerate(ops) if op == "label"}
    open_loops = []  # label names of active do loops
    for i, (op, lab, ln) in enumerate(ops):
        if op in ("do", "dor"):
            tgt = ln.split(",")[-1].strip()
            open_loops.append(tgt)
        elif op == "label":
            if open_loops and open_loops[-1] == lab:
                open_loops.pop()
        elif op.startswith("brk"):
            if not open_loops:
                return False
            if i == 0 or ops[i - 1][0] != "nop":
                return False
            # need >= 3 following non-label lines before the loop label
            cnt = 0
            for j in range(i + 1, len(ops)):
                if ops[j][0] == "label":
                    break
                cnt += 1
            if cnt < 3:
                return False
        elif op == "enddo":
            if not open_loops:
                return False
            cnt = 0
            for j in range(i + 1, len(ops)):
                if ops[j][0] == "label":
                    break
                cnt += 1
            if cnt < 2:
                return False
        elif op == "movem":
            # A P-space access must keep the setup that aims it, or shrinking
            # would point it at arbitrary code and manufacture divergence.
            for areg in re.findall(r"p:\((r[0-7])\)", ln):
                setup = [l for l in lines[:i] if l.startswith("move #")
                         and l.split(",")[-1].strip() == areg]
                if not setup:
                    return False
                if ",p:(" not in ln:
                    continue
                # A rewrite must stay out of the writer's own straight-line
                # flow: writing into the extent of the block that is running
                # is not the overlay shape and is not well defined.
                tgt = label_idx.get(setup[-1].split("#")[1].split(",")[0])
                if tgt is None:
                    return False
                lo, hi = min(i, tgt), max(i, tgt)
                if not any(ops[j][0] in ("bra", "jmp")
                           for j in range(lo + 1, hi)):
                    return False
        elif op == "rts":
            pass  # reachable only via bsr/jsr; leave to runtime
    return True


def shrink(lines, tag):
    cur = list(lines)
    st, detail = classify(cur, tag)
    assert st == "diverge"
    changed = True
    while changed:
        changed = False
        for size in (16, 8, 4, 2, 1):
            i = 0
            while i < len(cur):
                cand = cur[:i] + cur[i + size:]
                if len(cand) < 1 or not valid_structure(cand):
                    i += 1
                    continue
                st, d = classify(cand, tag)
                if st == "diverge":
                    cur = cand
                    detail = d
                    changed = True
                else:
                    i += 1
    return cur, detail


# ---------------- main ----------------

def summarize(detail):
    if detail["kind"] == "crash":
        return "crash: step_err=%s block_err=%s" % (
            detail.get("step_err"), detail.get("block_err"))
    if detail["kind"] == "one-mode-timeout":
        return "one-mode-timeout: step_done=%s block_done=%s" % (
            detail["step_done"], detail["block_done"])
    lines = ["differing regs: %s" % ",".join(detail["regs"])]
    for r in detail["regs"]:
        lines.append("  %-4s step=%s block=%s" % (
            r, (detail["step"] or {}).get(r),
            (detail["block"] or {}).get(r)))
    return "\n".join(lines)


def run_round(pool_name, nprogs, seed_base, nbody_range=(10, 60)):
    stats = {"ok": 0, "asmfail": 0, "skip": 0, "diverge": 0}
    findings = []
    for i in range(nprogs):
        seed = seed_base + i
        rng = random.Random(seed)
        nbody = rng.randrange(*nbody_range)
        g = Gen(rng, POOLS[pool_name], nbody)
        lines = g.generate()
        st, detail = classify(lines, pool_name)
        stats[st] += 1
        if st == "diverge":
            name = "%s_seed%d" % (pool_name, seed)
            print("[FINDING] %s: %s" % (name, detail["kind"]))
            mini, mdetail = shrink(lines, pool_name + "_s")
            path = os.path.join(FINDINGS_DIR, name + ".asm")
            write_asm(mini, path)
            with open(os.path.join(FINDINGS_DIR, name + ".txt"), "w") as f:
                f.write("pool=%s seed=%d\n" % (pool_name, seed))
                f.write(summarize(mdetail) + "\n\nminimal:\n")
                f.write("\n".join(mini) + "\n")
            findings.append((name, mini, mdetail))
            print(summarize(mdetail))
            print("minimal (%d lines) -> %s" % (len(mini), path))
        if (i + 1) % 50 == 0:
            print("  [%s] %d/%d  %s" % (pool_name, i + 1, nprogs, stats))
    print("round %s done: %s" % (pool_name, stats))
    return stats, findings


if __name__ == "__main__":
    pool = sys.argv[1]
    n = int(sys.argv[2])
    seed_base = int(sys.argv[3])
    run_round(pool, n, seed_base)
