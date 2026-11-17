#!/usr/bin/env python3
"""Which callee-written registers can a TU1.5 caller consume after a call?

For every `call TARGET` in the original .text, walks every path from the
return address (following direct jumps and both branch arms, stopping at the
next call, return or indirect jump) and reports any read of a register the
callee leaves behind before the caller rewrites it:
  - EAX bytes 1-3 (AL is the callee's bool result), ECX, EDX (byte-granular,
    so `mov cx,..; mov [..],cx` is a definition, not a read);
  - EFLAGS;
  - XMM0-7, per low lane and upper lanes: scalar SSE ops read and write only
    lane 0 and keep the destination's upper lanes, register copies carry both
    lane states, packed ops and full-width stores read all lanes.
Dependency-breaking idioms (xor/sub r,r; sbb r,r; or r,-1; lea r,[r]) are
not reads. With --wrappers, every function that returns with the callee's
EAX bits still live is followed to its own callers for EAX bits 1-3.
A native binding that leaves these registers unchanged therefore matches
the original for every consumer listed as clean. Requires capstone.
"""
import argparse
import hashlib
from pathlib import Path
import struct
import sys

import capstone
from capstone import x86 as X

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
SCALAR_RW = {'addss', 'subss', 'mulss', 'divss', 'maxss', 'minss', 'addsd', 'subsd', 'mulsd', 'divsd'}
SCALAR_W = {'sqrtss', 'rcpss', 'rsqrtss', 'cvtsi2ss', 'cvtsd2ss', 'cvtss2sd', 'cvtsi2sd'}
SCALAR_R = {'comiss', 'ucomiss', 'comisd', 'ucomisd', 'cvttss2si', 'cvtss2si', 'cvttsd2si', 'cvtsd2si'}
BYTES = {'eax': (0, 15), 'ax': (0, 3), 'al': (0, 1), 'ah': (0, 2), 'ecx': (1, 15), 'cx': (1, 3), 'cl': (1, 1),
         'ch': (1, 2), 'edx': (2, 15), 'dx': (2, 3), 'dl': (2, 1), 'dh': (2, 2)}
PURE = {'mov', 'movzx', 'movsx', 'lea', 'pop'} | {'set' + c for c in ('b', 'e', 'ne', 'a', 'ae', 'be', 'l', 'g', 'le', 'ge', 's', 'ns', 'p', 'np')}


class Image:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        base = struct.unpack_from('<I', self.data, 0x104)[0]
        count = struct.unpack_from('<I', self.data, 0x11c)[0]
        header = struct.unpack_from('<I', self.data, 0x120)[0] - base
        self.sections = [struct.unpack_from('<IIIII', self.data, header + i * 56)[1:5] for i in range(count)]
        self.text = self.sections[0]
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = True
        self.cache = {}

    def read(self, va, n):
        for start, size, raw, raw_size in self.sections:
            if start <= va < start + max(size, raw_size):
                offset = va - start
                out = self.data[raw + offset:raw + min(offset + n, raw_size)]
                return out + b'\0' * (n - len(out))
        raise KeyError(hex(va))

    def insn(self, va):
        if va not in self.cache:
            self.cache[va] = next(self.md.disasm(self.read(va, 16), va))
        return self.cache[va]

    def call_sites(self, target):
        start, size = self.text[0], self.text[1]
        code = self.read(start, size)
        return [start + i for i in range(len(code) - 5)
                if code[i] == 0xe8 and start + i + 5 + struct.unpack_from('<i', code, i + 1)[0] == target]


def operand(i, k):
    o = i.operands[k]
    if o.type == X.X86_OP_REG:
        return i.reg_name(o.reg)
    return 'mem' if o.type == X.X86_OP_MEM else 'imm'


def step(i, state, issues):
    m, n = i.mnemonic, len(i.operands)
    state = dict(state)
    ops = [operand(i, k) for k in range(n)]
    same = n == 2 and ops[0] == ops[1]

    def undefined(r, part):
        return state.get((r, part), False)

    def read(r, part):
        if undefined(r, part):
            issues.add((i.address, f'{m} {i.op_str}', f'{r}.{part}'))

    def define(r, part, value):
        state[(r, part)] = value
    masks = list(state.get('gp', (0, 0, 0)))
    nop = m == 'lea' and n == 2 and i.op_str.replace(' ', '') == f'{ops[0]},[{ops[0]}]'
    for o in ([] if nop else i.operands):
        if o.type == X.X86_OP_MEM:
            for reg in (o.mem.base, o.mem.index):
                if reg and i.reg_name(reg) in BYTES and masks[BYTES[i.reg_name(reg)][0]]:
                    issues.add((i.address, f'{m} {i.op_str}', i.reg_name(reg) + '-address'))
    if any(x.startswith('xmm') for x in ops):
        d = ops[0]
        s = ops[1] if n > 1 else None
        if m in ('movss', 'movsd'):
            if d == 'mem':
                read(s, 'lo')
            elif s == 'mem':
                define(d, 'lo', False), define(d, 'hi', False)
            else:
                read(s, 'lo'), define(d, 'lo', False)
        elif m in ('movaps', 'movups', 'movapd', 'movdqa', 'movdqu'):
            if d == 'mem':
                read(s, 'lo'), read(s, 'hi')
            elif s == 'mem':
                define(d, 'lo', False), define(d, 'hi', False)
            else:
                define(d, 'lo', undefined(s, 'lo')), define(d, 'hi', undefined(s, 'hi'))
        elif m in ('xorps', 'xorpd', 'pxor') and same:
            define(d, 'lo', False), define(d, 'hi', False)
        elif m in SCALAR_RW:
            read(d, 'lo')
            if s and s.startswith('xmm'):
                read(s, 'lo')
            define(d, 'lo', False)
        elif m in SCALAR_W:
            if s and s.startswith('xmm'):
                read(s, 'lo')
            define(d, 'lo', False)
        elif m in SCALAR_R:
            for x in ops:
                if x.startswith('xmm'):
                    read(x, 'lo')
        elif m == 'movd':
            if d.startswith('xmm'):
                define(d, 'lo', False), define(d, 'hi', False)
            else:
                read(s, 'lo')
        else:
            for k, x in enumerate(ops):
                if x.startswith('xmm'):
                    read(x, 'lo'), read(x, 'hi')
            if d.startswith('xmm'):
                define(d, 'lo', False), define(d, 'hi', False)
    registers_read, registers_written = i.regs_access()
    R = {i.reg_name(r) for r in registers_read}
    W = {i.reg_name(r) for r in registers_written}
    idiom = (m in ('xor', 'sub', 'sbb') and same) or (m == 'or' and n == 2 and i.operands[1].type == X.X86_OP_IMM
             and (i.operands[1].imm & 0xffffffff) == 0xffffffff) or (m == 'lea' and i.op_str.replace(' ', '') in
             (f'{ops[0]},[{ops[0]}]',))
    for r in R:
        if r in BYTES and not idiom:
            g, b = BYTES[r]
            if m in PURE and r in W and ops and ops[0] == r:
                continue
            if masks[g] & b:
                issues.add((i.address, f'{m} {i.op_str}', ('eax', 'ecx', 'edx')[g] + f'.bytes{masks[g] & b:x}'))
        if r == 'eflags' and state.get(('flags', 'r'), False) and m != 'sbb':
            issues.add((i.address, f'{m} {i.op_str}', 'flags'))
    for r in W:
        if r in BYTES:
            g, b = BYTES[r]
            masks[g] &= ~b
        if r == 'eflags':
            state[('flags', 'r')] = False
    state['gp'] = tuple(masks)
    return state


def analyse(image, ret, initial, limit=600):
    issues, ends, work, seen = set(), [], [(ret, initial, 0)], set()
    while work:
        pc, state, steps = work.pop()
        while steps < limit:
            live = frozenset((k, v) for k, v in state.items() if v and v != (0, 0, 0))
            if not live or (pc, live) in seen:
                break
            seen.add((pc, live))
            steps += 1
            i = image.insn(pc)
            m = i.mnemonic
            if m == 'call':
                ends.append((pc, 'call', live))
                break
            if m.startswith('ret'):
                ends.append((pc, 'ret', live))
                break
            state = step(i, state, issues)
            if m == 'jmp':
                o = i.operands[0]
                if o.type == X.X86_OP_IMM:
                    pc = o.imm
                    continue
                ends.append((pc, 'indirect', live))
                break
            if m[0] == 'j' or m.startswith('loop'):
                work.append((i.operands[0].imm, state, steps))
            pc += i.size
    return issues, ends


def function_entry(image, pc):
    """The nearest preceding int3/ret-padded boundary (MSVC aligns entries)."""
    at = pc
    while at > image.text[0]:
        if image.read(at - 1, 1) == b'\xcc' and at % 16 == 0:
            return at
        at -= 1
    return None


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--target', required=True, help='callee address, hex')
    p.add_argument('--wrappers', action='store_true')
    p.add_argument('--require-clean', action='store_true',
                   help='fail unless no call site reads a leftover register and wrapper paths read only EAX')
    a = p.parse_args()
    if not a.xbe.is_file():
        return 77
    if hashlib.sha256(a.xbe.read_bytes()).hexdigest() != PIN:
        raise SystemExit('Wrong original executable')
    image = Image(a.xbe)
    target = int(a.target, 16)
    initial = {('flags', 'r'): True, 'gp': (14, 15, 15)}
    for k in range(8):
        initial[(f'xmm{k}', 'lo')] = initial[(f'xmm{k}', 'hi')] = True
    sites = image.call_sites(target)
    flagged = 0
    non_eax = 0
    returners = set()
    for site in sites:
        issues, ends = analyse(image, site + 5, initial)
        if any(kind == 'ret' and dict(live).get('gp', (0,))[0] for _, kind, live in ends):
            returners.add(site)
        flagged += bool(issues)
        print(f'{site:06x}', 'READ ' + ' '.join(sorted({f'{p:x}:{w}' for p, _, w in issues})) if issues else 'clean')
    print(f'{len(sites)} call sites of {target:x}; {flagged} with reads')
    if a.wrappers:
        done, pending, eax_reads = set(), sorted(returners), 0
        while pending:
            site = pending.pop()
            if site in done:
                continue
            done.add(site)
            issues, ends = analyse(image, site + 5, {'gp': (14, 0, 0)})
            for p_, _, w in sorted(issues):
                eax_reads += 1
                non_eax += not w.startswith('eax.')
                print(f'  wrapper path from {site:06x}: {w} read at {p_:x}')
            for pc, kind, live in ends:
                if kind == 'ret' and dict(live).get('gp', (0,))[0]:
                    entry = function_entry(image, pc)
                    if entry:
                        pending.extend(image.call_sites(entry))
        print(f'{len(done)} wrapper return paths followed; {eax_reads} EAX bit 8-31 reads')
    if a.require_clean and (flagged or non_eax):
        print('FAIL: a caller consumes a register the native binding leaves at its entry value')
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
