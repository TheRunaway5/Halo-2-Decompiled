#!/usr/bin/env python3
"""Physical original 1E2800/D8BB0/122B60 versus native breakable contact damage.

Oracle: the unchanged English TU1.5 instructions of 1E2800 (with its two real
callees D8BB0 and 122B60) single-stepped on the host CPU through
OriginalAudioCpu (Linux i386 compatibility mode, ptrace, no opcode patch, no
supplied callee result).  Fixtures are declared data only; every expected value
(ordered stores, record bytes, EAX/ECX/EDX, XMM0-2, x87 SW, MXCSR, EFLAGS) comes
from that execution.

Subject: native_tu15_build_breakable_contact_damage() exactly as found under
--source-root, compiled with GCC and Clang in an owned mini project through
tools/run_tests.py.  Mutated copies (never production) must be rejected.

Run through tools/run_test.py; all generated outputs stay in HALO2_TEST_SCRATCH.
Only the bounded report directory given by --output is written elsewhere.
"""
import argparse, ctypes, hashlib, json, os, random, re, shutil, struct, subprocess, sys, time
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
M = 0xffffffff
BANK, BANK_WIN = 0x1000000, 0x10000
HIGH, HIGH_WIN = 0x7FFFF000, 0x2000
SENT = 0x3000000
IMAGE_BASE, IMAGE_SIZE = 0x10000, 0x800000
REC_SIZE, GUARD = 0x94, 16
BODIES = {0x1e2800: (0x179, '3d079a2e906bd9d933252c2fb6c3ef79554c6ed5cc3f5c550583aa7ed6d0721f'),
          0xd8bb0: (0x68, '8477c62d4f6d73f50e11ae3750a2f0e5acd2fd5290831f2e34a5aac12f53079a'),
          0x122b60: (0x7d, 'e263726c998e7972b229ba1c45d1893a1fac962834cc8b4bd890ab1124c3c3f5')}
NATIVE_FILES = ['include/halo2/native_tu15_breakable_contact_damage.hpp',
                'src/physics/native_tu15_breakable_contact_damage.cpp',
                'include/halo2/native_tu15_vector_normalize.hpp',
                'src/physics/native_tu15_vector_normalize.cpp',
                'include/halo2/native_tu15_physics_frame.hpp',
                'include/halo2/physics_contact_record.hpp']
KERNEL, NORMALIZE = NATIVE_FILES[1], NATIVE_FILES[3]
ROUTER = 'src/physics/physics_live_world_tu15.cpp'
TOOL_FILES = ['tools/original_audio_cpu.py', 'tools/run_test.py', 'tools/run_tests.py',
              'tools/generate_physics_live_support_tu15.py', 'tools/research_mp_sandbox_update_functions.py',
              'tools/reference/halo2-decompiled/tools/functions.py', 'tools/reference/halo2-decompiled/tools/xbe.py']
# Fixture offsets inside the bank. Odd low bytes keep masked/unmasked values distinct.
O = dict(obj_hdr=0x0010, obj_slot=0x0124, obj_data=0x0238, comp_hdr=0x0440, comp_slot=0x0514,
         row=0x0734, struct_hdr=0x0850, surf=0x0964, sbsp_hdr=0x0A70, inst=0x0C14, defn=0x0D28,
         isurf=0x0E3C, matg_tag=0x0F44, matg_block=0x1058, fallback=0x116C)
FAULT_MESSAGE = 'unsupported numerical-fault owner'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def f32(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def add_flags(a, b):
    r = (a + b) & M
    cf = (a + b) >> 32 & 1
    pf = 1 if bin(r & 0xff).count('1') % 2 == 0 else 0
    af = ((a ^ b ^ r) >> 4) & 1
    zf = 1 if r == 0 else 0
    sf = r >> 31
    of = (((~(a ^ b)) & (a ^ r)) >> 31) & 1
    return cf | pf << 2 | af << 4 | zf << 6 | sf << 7 | of << 11


# --------------------------------------------------------------------------- original side
def load_original_modules(tools_root):
    sys.path[:0] = [str(tools_root / 'tools'), str(tools_root / 'tools/reference/halo2-decompiled/tools')]
    from original_audio_cpu import OriginalAudioCpu, Registers
    from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
    from xbe import Xbe
    from functions import _Code
    import generate_physics_live_support_tu15 as generator
    from capstone import CS_AC_READ, CS_AC_WRITE
    from capstone.x86 import X86_OP_MEM
    return dict(OriginalAudioCpu=OriginalAudioCpu, Registers=Registers, NEW_DEFAULT=NEW_DEFAULT,
                NEW_PIN=NEW_PIN, Xbe=Xbe, _Code=_Code, complete_body=generator.complete_body,
                CS_AC_READ=CS_AC_READ, CS_AC_WRITE=CS_AC_WRITE, X86_OP_MEM=X86_OP_MEM)


def make_cpu_class(mods):
    Base, Registers = mods['OriginalAudioCpu'], mods['Registers']
    READ, WRITE, MEM = mods['CS_AC_READ'], mods['CS_AC_WRITE'], mods['X86_OP_MEM']
    aliases = {'eax': 'rax', 'ebx': 'rbx', 'ecx': 'rcx', 'edx': 'rdx', 'esi': 'rsi', 'edi': 'rdi',
               'ebp': 'rbp', 'esp': 'rsp'}

    class Cpu(Base):
        """OriginalAudioCpu with a complete declared FXSAVE input and a store/read ledger."""

        def prepare_full(self, entry, stack, gpr, eflags, fp):
            regs = Registers.from_buffer_copy(bytes(self.initial))
            regs.rip = entry; regs.rsp = stack; regs.eflags = eflags
            for name, value in gpr.items():
                setattr(regs, aliases[name], value & M)
            self.trace(13, 0, ctypes.addressof(regs))
            context = bytearray(self.fp.raw)
            depth = fp['depth']; top = (8 - depth) & 7
            tag = 0
            for n in range(depth):
                tag |= 1 << ((top + n) & 7)
            struct.pack_into('<HHBBH', context, 0, fp['cw'], fp['sw'], tag, 0, 0)
            struct.pack_into('<QQ', context, 8, 0, 0)
            struct.pack_into('<I', context, 24, fp['mxcsr'])
            for n in range(8):
                context[32 + 16 * n:48 + 16 * n] = (fp['st'][n] + bytes(6)) if n < len(fp['st']) else bytes(16)
            for n in range(8):
                struct.pack_into('<4I', context, 160 + 16 * n, *fp['xmm'][n])
            native_fp = ctypes.create_string_buffer(bytes(context), 512)
            self.trace(15, 0, ctypes.addressof(native_fp))
            self.trace(6, self.debug_offset + 8 * 6, 0)
            check = self.fpstate()
            assert (check['cw'], check['sw'], check['tag'], check['mxcsr']) == (fp['cw'], fp['sw'], tag, fp['mxcsr']), (
                'FXRSTOR did not admit declared FP input', check, fp)
            assert check['xmm'] == [list(v) for v in fp['xmm']]

        def fpstate(self):
            buffer = ctypes.create_string_buffer(512); self.trace(14, 0, ctypes.addressof(buffer)); raw = buffer.raw
            cw, sw, tag, fop = struct.unpack_from('<HHHH', raw, 0)
            fip, fdp = struct.unpack_from('<QQ', raw, 8)
            return dict(cw=cw, sw=sw, tag=tag & 0xff, fop=fop, fip=fip, fdp=fdp,
                        mxcsr=struct.unpack_from('<I', raw, 24)[0],
                        st=[raw[32 + 16 * n:42 + 16 * n] for n in range(8)],
                        xmm=[list(struct.unpack_from('<4I', raw, 160 + 16 * n)) for n in range(8)])

        def trace_case(self, code, cache, window_lo, window_size, limit=4000):
            regs = self.registers(); stores = []; reads = []; executed = []
            def value(name):
                return 0 if not name else getattr(regs, aliases[name]) & M
            for _ in range(limit):
                pc = regs.rip
                if pc == SENT:
                    return regs, stores, reads, executed
                instruction = cache.get(pc)
                if instruction is None:
                    instruction = code.at(pc); assert instruction, hex(pc); cache[pc] = instruction
                assert self.read(pc, len(instruction.bytes)) == instruction.bytes
                executed.append(pc); pending = []
                x87_store = instruction.mnemonic in ('fst', 'fstp', 'fist', 'fistp', 'fisttp')
                for operand in instruction.operands:
                    if operand.type != MEM:
                        continue
                    mem = operand.mem
                    target = (value(instruction.reg_name(mem.base)) +
                              value(instruction.reg_name(mem.index)) * mem.scale + mem.disp) & M
                    if operand.access & READ and not x87_store:
                        reads.append((target, operand.size))
                    if operand.access & WRITE or x87_store:
                        if target < window_lo + window_size and target + operand.size > window_lo:
                            assert window_lo <= target and target + operand.size <= window_lo + window_size, (
                                'store straddles the declared window', hex(pc), hex(target))
                            pending.append((target, operand.size, self.read(target, operand.size)))
                before = self.read(window_lo, window_size)
                self.trace(9); regs = self.wait_stop()
                after = self.read(window_lo, window_size)
                covered = set()
                for target, size, old in pending:
                    stores.append((pc, target, size, old.hex(), self.read(target, size).hex()))
                    covered.update(range(target - window_lo, target - window_lo + size))
                changed = [n for n in range(window_size) if before[n] != after[n]]
                assert set(changed) <= covered, ('unlisted original store', hex(pc), instruction.mnemonic,
                                                 instruction.op_str, [hex(window_lo + n) for n in changed[:8]])
            raise RuntimeError('original instruction limit reached')

    return Cpu


# --------------------------------------------------------------------------- fixtures
DEFAULT = dict(
    eax=0xE2E60077, ecx=1, slot=0, sp=0x1006000, comp_datum=0xE1F2000A,
    kind=1, low=0x1342, part=0x0123, def_index=3, struct_breakable=0x5A, inst_breakable=0xA5,
    position=(f32(1.5), f32(-2.25), f32(3.0)), normal=(0, 0, f32(1.0)), impulse=f32(1.0), row12=0x1234,
    obj28=0xAABBCCDD, obj2c=0x11223344, def_tag=0xE1A20034, globals=(),
    cw=0x027F, sw=0x0000, mxcsr=0x1F80, depth=0, eflags=0x202,
    ebx=0x0BADB0B1, ebp=0x0BADBEEF, edi=0xFFFFFFFF, edx=0x10097500,
    row=None, objdata=None, record=None, refusal=False)
ST_LIVE = [struct.pack('<Q', 0xC000000000000000) + struct.pack('<H', 0x3FFE),   # 0.75
           struct.pack('<Q', 0x8000000000000000) + struct.pack('<H', 0xBFFE),   # -0.5
           struct.pack('<Q', 0xA000000000000000) + struct.pack('<H', 0x3FFF),   # 1.25
           struct.pack('<Q', 0xC90FDAA22168C235) + struct.pack('<H', 0x4000)]   # pi


def xmm_poison(cid):
    rows = []
    for n in range(8):
        rows.append([(0x9E3779B1 * (cid * 8 + n + 1) + lane * 0x7F4A7C15) & M for lane in range(4)])
    rows[1][0] = 0x7FC0DEAD; rows[2][0] = 0xFFC0BEEF   # distinct NaN poison in tested lanes
    return rows


class Fixture:
    def __init__(self, cid):
        rng = random.Random(0x5EED0000 + cid)
        self.bank = bytearray(rng.randbytes(BANK_WIN)); self.high = bytearray(rng.randbytes(HIGH_WIN))
        self.patches = {}

    def w(self, address, data):
        address &= M
        if BANK <= address and address + len(data) <= BANK + BANK_WIN:
            self.bank[address - BANK:address - BANK + len(data)] = data
        elif HIGH <= address and address + len(data) <= HIGH + HIGH_WIN:
            self.high[address - HIGH:address - HIGH + len(data)] = data
        elif IMAGE_BASE <= address and address + len(data) <= IMAGE_BASE + IMAGE_SIZE:
            for n, b in enumerate(data):
                self.patches[address + n] = b
        else:
            raise AssertionError(f'fixture write outside declared memory {address:#x}')

    def w32(self, a, v): self.w(a, struct.pack('<I', v & M))
    def w16(self, a, v): self.w(a, struct.pack('<H', v & 0xffff))
    def w8(self, a, v): self.w(a, bytes([v & 0xff]))

    def patch_list(self):
        out = []; run = None
        for address in sorted(self.patches):
            if run and run[0] + len(run[1]) == address:
                run[1].append(self.patches[address])
            else:
                run = [address, bytearray([self.patches[address]])]; out.append(run)
        return [(a, bytes(b)) for a, b in out]


def build(spec, cid):
    s = dict(DEFAULT); s.update(spec)
    f = Fixture(cid); B = BANK
    sp = s['sp']; rec = s['record'] if s['record'] is not None else (sp + 4 + 0x32C + s['slot'] * 0x94) & M
    f.w(rec - GUARD, b'\xCD' * (REC_SIZE + 2 * GUARD))
    f.w32(sp, SENT)
    low_obj = s['eax'] & 0xffff
    objdata = s['objdata'] if s['objdata'] is not None else B + O['obj_data']
    f.w32(0x4ecbd0, B + O['obj_hdr']); f.w32(B + O['obj_hdr'] + 0x44, B + O['obj_slot'] - low_obj * 12)
    f.w32(B + O['obj_slot'] + 8, objdata)
    cidx = s['comp_datum'] & 0xffff
    f.w32(0x52be5c, B + O['comp_hdr']); f.w32(B + O['comp_hdr'] + 0x44, B + O['comp_slot'] - cidx * 0xa0)
    row = s['row'] if s['row'] is not None else B + O['row']
    contacts = (row - (((s['ecx'] * 9) & M) * 8)) & M
    f.w32(B + O['comp_slot'] + 0x88, contacts)
    key = (s['kind'] << 29) | (s['part'] << 16) | s['low']
    low, part = s['low'], s['part']
    f.w32(0x4ecc10, B + O['struct_hdr']); f.w32(B + O['struct_hdr'] + 0x2c, B + O['surf'] - low * 8)
    f.w8(B + O['surf'] + 5, s['struct_breakable'])
    d = s['def_index']; signed = d - 0x10000 if d & 0x8000 else d
    f.w32(0x4ecc18, B + O['sbsp_hdr']); f.w32(B + O['sbsp_hdr'] + 0x144, B + O['inst'] - low * 0x58)
    f.w16(B + O['inst'] + 0x34, d)
    f.w32(B + O['sbsp_hdr'] + 0x13c, B + O['defn'] - signed * 0xc8)
    f.w32(B + O['defn'] + 0x9c, B + O['isurf'] - part * 8); f.w8(B + O['isurf'] + 5, s['inst_breakable'])
    f.w32(0x4ecc1c, B + O['matg_tag']); f.w32(B + O['matg_tag'] + 0xbc, B + O['matg_block'])
    f.w32(B + O['matg_block'] + 4, s['def_tag'])
    # Object/row last: alias cases intentionally overlay the record pre-state.
    f.w32(objdata + 0x28, s['obj28']); f.w32(objdata + 0x2c, s['obj2c']); f.w32(objdata + 0xb4, s['comp_datum'])
    f.w32(row + 4, key); f.w16(row + 0x12, s['row12'])
    for n in range(3):
        f.w32(row + 0x1c + 4 * n, s['position'][n]); f.w32(row + 0x28 + 4 * n, s['normal'][n])
    f.w32(row + 0x38, s['impulse'])
    for address, data in s['globals']:
        f.w(address, data)
    depth = s['depth']; top = (8 - depth) & 7
    fp = dict(cw=s['cw'], sw=(s['sw'] & ~0x3800) | (top << 11), mxcsr=s['mxcsr'], depth=depth,
              st=ST_LIVE[:depth], xmm=xmm_poison(cid))
    gpr = dict(eax=s['eax'], ecx=s['ecx'], esi=rec, ebx=s['ebx'], ebp=s['ebp'], edi=s['edi'], edx=s['edx'])
    return s, f, fp, gpr, rec, key


def case_specs():
    """Declared cases. Each label states the domain it exercises."""
    out = []
    def add(label, **kw):
        out.append(dict(label=label, **kw))
    thr = 0x3ECCCCCD
    impulses = [('eq_threshold', thr), ('next_up', thr + 1), ('next_down', thr - 1), ('one', f32(1.0)),
                ('neg_one', f32(-1.0)), ('pos_inf', 0x7F800000), ('neg_inf', 0xFF800000),
                ('qnan', 0x7FC00000), ('qnan_payload', 0x7FC12345), ('neg_qnan', 0xFFC00000),
                ('snan', 0x7F800001), ('snan_payload', 0x7FA00000), ('denorm', 0x00000001),
                ('neg_denorm', 0x80000001), ('pos_zero', 0), ('neg_zero', 0x80000000), ('max', 0x7F7FFFFF)]
    for mx in (0x1F80, 0x1FC0, 0x9F80, 0x1FBF):
        for name, bits in impulses:
            add(f'gate/{name}/mxcsr{mx:04x}', impulse=bits, mxcsr=mx)
    for kind in range(8):
        add(f'kind/{kind}', kind=kind)
    for kind, low, part in ((1, 0, 0), (1, 0xFFFF, 0x1FFF), (5, 0, 0), (5, 0xFFFF, 0x1FFF), (0, 0x0001, 0x1FFF),
                            (7, 0xFFFF, 0x1FFF), (2, 0x8000, 0x1000), (3, 0x00FF, 0x00FF)):
        add(f'key_edges/kind{kind}/low{low:04x}/part{part:04x}', kind=kind, low=low, part=part)
    for kind in (1, 5):
        for value in (0, 0x7F, 0xFF):
            add(f'breakable/kind{kind}/{value:02x}', kind=kind, struct_breakable=value, inst_breakable=value)
    for d in (0, 1, 0x7FFF, 0xFFFF, 0x8000, 0x1234):
        add(f'definition/{d:04x}', kind=5, def_index=d)
        add(f'definition/kind6/{d:04x}', kind=6, def_index=d)
    eps = 0x38D1B717
    vectors = [('unit_z', (0, 0, f32(1.0))), ('three_four', (f32(3.0), f32(4.0), 0)),
               ('zero', (0, 0, 0)), ('neg_zero', (0x80000000, 0x80000000, 0x80000000)),
               ('eps', (eps, 0, 0)), ('eps_down', (eps - 1, 0, 0)), ('eps_up', (eps + 1, 0, 0)),
               ('eps_pair', (f32(6e-5), f32(8e-5), 0)), ('eps_pair_down', (f32(6e-5) - 1, f32(8e-5) - 1, 0)),
               ('eps_pair_up', (f32(6e-5) + 1, f32(8e-5) + 1, 0)),
               ('denorm', (0x00000001, 0x007FFFFF, 0x80800000)), ('min_normal', (0x00800000, 0x00800000, 0x00800000)),
               ('qnan', (0x7FC12345, f32(1.0), f32(-1.0))), ('qnan_pair', (0x7FC00001, 0x7FC00002, 0)),
               ('qnan_pair_rev', (0x7FC00002, 0x7FC00001, 0)), ('qnan_triple', (0xFFC00005, 0x7FC00003, 0x7FC00004)),
               ('snan', (0x7F812345, f32(1.0), f32(-1.0))), ('inf', (0x7F800000, f32(1.0), f32(-1.0))),
               ('inf_pair', (0x7F800000, 0xFF800000, 0)), ('max', (0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF)),
               ('mixed_large', (f32(1e20), f32(-3e19), f32(7e18))), ('tiny_tail', (f32(1.0), f32(1e-30), f32(1e-30))),
               ('third', (f32(1 / 3), f32(-2 / 3), f32(2 / 3))), ('odd', (f32(0.1), f32(0.2), f32(0.7)))]
    for name, normal in vectors:
        for cw in (0x027F, 0x007F, 0x037F, 0x067F, 0x0A7F, 0x0E7F):
            add(f'normal/{name}/cw{cw:04x}', normal=normal, cw=cw)
        for mx in (0x1FC0, 0x9F80, 0x7F80, 0x1FBF):
            add(f'normal/{name}/mxcsr{mx:04x}', normal=normal, mxcsr=mx)
    for depth in range(4):
        for sw in (0x0000, 0x0020, 0x4700, 0x027F):
            for name in ('unit_z', 'zero', 'three_four', 'qnan_pair'):
                add(f'x87/depth{depth}/sw{sw:04x}/{name}', depth=depth, sw=sw, normal=dict(vectors)[name])
    for eax in (0x00000000, 0xFFFFFFFF, 0x0001FFFF, 0x12340000):
        add(f'index/object/{eax:08x}', eax=eax)
    for comp in (0x00000000, 0xFFFFFFFF, 0x7FFF8001):
        add(f'index/component/{comp:08x}', comp_datum=comp)
    for ecx in (0, 15, 0x20000001, 0xFFFFFFFF, 0x80000000, 0x1C71C71D):
        add(f'index/contact/{ecx:08x}', ecx=ecx)
    for slot in (1, 7, 15):
        add(f'index/slot/{slot}', slot=slot)
    add('globals/48a0b0', globals=((0x48a0b0, struct.pack('<I', 0xBEEF1234)),))
    add('globals/474c24', globals=((0x474c24, struct.pack('<I', 0x7ACE0003)),))
    add('globals/474c24_kind5', kind=5, globals=((0x474c24, struct.pack('<I', 0x00008002)),))
    fallback = BANK + O['fallback']
    custom = ((fallback, struct.pack('<3I', 0x3E800000, 0x7FC0BEEF, 0xC0400000)), (0x474d08, struct.pack('<I', fallback)))
    add('globals/474d08_fallback', normal=(0, 0, 0), globals=custom)
    add('globals/474d08_normalized', normal=(f32(3.0), 0, f32(4.0)), globals=custom)
    add('globals/46a240_below', impulse=f32(1.5), globals=((0x46a240, struct.pack('<I', f32(2.0))),))
    add('globals/46a240_above', impulse=f32(2.5), globals=((0x46a240, struct.pack('<I', f32(2.0))),))
    add('globals/46a0d0_two', normal=(f32(3.0), f32(4.0), 0), globals=((0x46a0d0, struct.pack('<I', f32(2.0))),))
    add('globals/46a0ec_one', normal=(f32(0.5), 0, 0), globals=((0x46a0ec, struct.pack('<I', f32(1.0))),))
    add('globals/46a0ec_zero', normal=(0, 0, 0), globals=((0x46a0ec, struct.pack('<I', 0)),))
    add('globals/46a0ec_nan', normal=(0, 0, 0), globals=((0x46a0ec, struct.pack('<I', 0x7FC00000)),))
    for sp in (0x1006004, 0x1006008, 0x100600C, 0x10060F0, 0x10060FC, 0x100607C, 0x1006FFC):
        add(f'stack/{sp:08x}', sp=sp)
        add(f'stack/{sp:08x}/flags_in', sp=sp, eflags=0xAD7)
    for sp in (0x80000004, 0x80000010, 0x7FFFF800):
        add(f'stack_high/{sp:08x}', sp=sp)
        add(f'stack_high/{sp:08x}/flags_in', sp=sp, eflags=0xAD7, impulse=0x7FC00000)
    rec0 = 0x1006000 + 4 + 0x32C
    add('alias/row_in_record', row=rec0 + 0x38)
    add('alias/row_before_record', row=rec0 - 0x30)
    add('alias/objdata_in_record', objdata=rec0 - 0x28 + 0x40, normal=(f32(3.0), f32(4.0), f32(12.0)))
    add('alias/fallback_in_record', normal=(0, 0, 0), globals=((0x474d08, struct.pack('<I', rec0 + 0x40)),))
    for kind in (1, 5):
        add(f'crash_replica/kind{kind}', kind=kind, eax=0xE2E60077, ecx=1, ebx=1, edi=0xFFFFFFFF, edx=0x10097500)
    # Native-only declared refusals: unsupported numerical-fault domains.
    for label, kw in (('depth4', dict(depth=4)), ('depth4_false_path', dict(depth=4, impulse=f32(0.1))),
                      ('cw_im_unmasked', dict(cw=0x027E)), ('cw_partial_masks', dict(cw=0x0262)),
                      ('cw_false_path', dict(cw=0x027E, impulse=f32(0.1))),
                      ('sw_es', dict(sw=0x0080)), ('sw_busy', dict(sw=0x8000)),
                      ('mxcsr_im_unmasked', dict(mxcsr=0x1F00)), ('mxcsr_zm_unmasked', dict(mxcsr=0x1D80)),
                      ('mxcsr_reserved_high', dict(mxcsr=0x11F80))):
        add(f'refusal/{label}', refusal=True, **kw)
    return out


# --------------------------------------------------------------------------- native project
MUTATIONS = [
    ('gate_ge', KERNEL, '"comiss %2,%1; seta %0"', '"comiss %2,%1; setae %0"'),
    ('kind5_only_instance', KERNEL, 'if((key>>29)==1u) {', 'if((key>>29)!=5u) {'),
    ('missing_or_0x100', KERNEL, 'read(m,p+4u)|0x100u', 'read(m,p+4u)'),
    ('swap_copy_order', KERNEL, None, None),
    ('fallback_dropped', KERNEL, 'result.xmm0={length,0,0,0};', 'result.xmm0={length,0,0,0};result.normalized=true;'),
    ('hardcoded_48a0b0', KERNEL, 'read(m,0x48a0b0u,2)', '0xffffu'),
    ('definition_zero_extended', KERNEL,
     'std::uint32_t(std::int32_t(std::int16_t(read(m,read(m,bsp+0x144u)+low*0x58u+0x34u,2))))',
     'read(m,read(m,bsp+0x144u)+low*0x58u+0x34u,2)'),
    ('gate_mxcsr_dropped', KERNEL, 'fp.mxcsr=_mm_getcsr();_mm_setcsr(host);return above;', '_mm_setcsr(host);return above;'),
    ('xmm1_xmm2_swapped', KERNEL, 'result.xmm1[0]=read(m,p+0x44u);result.xmm2[0]=read(m,p+0x40u);',
     'result.xmm1[0]=read(m,p+0x40u);result.xmm2[0]=read(m,p+0x44u);'),
    ('object_salt_unmasked', KERNEL, '(object_datum&0xffffu)*12u', 'object_datum*12u'),
    ('false_eax_unmasked', KERNEL, 'result.eax=component&0xffffff00u;', 'result.eax=component;'),
    ('structure_breakable_offset4', KERNEL, '+low*8u+5u,1)', '+low*8u+4u,1)'),
    ('hardcoded_46a0d0', KERNEL, 'const auto one=read(m,0x46a0d0u);', 'const auto one=0x3f800000u;'),
    ('hardcoded_474d08', KERNEL, 'read(m,0x474d08u)', '0x44ac28u'),
    ('normalizer_nan_operand_order', NORMALIZE, 'bits(mul(first,reciprocal))', 'bits(mul(reciprocal,first))'),
    ('normalizer_status_dropped', NORMALIZE, 'asm volatile("fnstsw %0":"=a"(fp_.status));', ''),
]
SWAP_A = 'for(unsigned n=0;n<3;++n)store(m,o,position_pcs[n],p+0x30u+n*4u,read(m,row+0x1cu+n*4u));'
SWAP_B = 'for(unsigned n=0;n<3;++n)store(m,o,origin_pcs[n],p+0x24u+n*4u,read(m,row+0x1cu+n*4u));'


def mutate(text, name, needle, replacement):
    if name == 'swap_copy_order':
        if text.count(SWAP_A) != 1 or text.count(SWAP_B) != 1:
            raise RuntimeError('mutation seam changed: swap_copy_order')
        return text.replace(SWAP_A, '\0').replace(SWAP_B, SWAP_A).replace('\0', SWAP_B)
    if text.count(needle) != 1:
        raise RuntimeError(f'mutation seam changed: {name} ({text.count(needle)} occurrences)')
    return text.replace(needle, replacement)


def router_snippet(text):
    match = re.search(r'const auto left=sp-0xcu,sum=left\+0xcu;\s*const auto flags=[^;]*;', text)
    return match.group(0) if match else None


DRIVER = r'''// Native breakable contact damage versus physical original expectations.
#include "halo2/native_tu15_breakable_contact_damage.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <xmmintrin.h>
using J=nlohmann::json;using namespace halo2;
namespace {
std::string hexbytes(std::span<const std::byte>s){static const char*d="0123456789abcdef";std::string r;
 for(auto b:s){auto v=std::to_integer<unsigned>(b);r+=d[v>>4];r+=d[v&15];}return r;}
std::vector<std::byte> unhex(const std::string&s){std::vector<std::byte>r(s.size()/2);
 for(std::size_t n=0;n<r.size();++n)r[n]=std::byte(std::stoul(s.substr(n*2,2),nullptr,16));return r;}
std::vector<std::byte> slurp(const char*p){std::ifstream f(p,std::ios::binary|std::ios::ate);
 if(!f)throw std::runtime_error(std::string("cannot read ")+p);std::vector<std::byte>r(std::size_t(f.tellg()));
 f.seekg(0);f.read(reinterpret_cast<char*>(r.data()),std::streamsize(r.size()));return r;}
std::string hex(std::uint64_t v){char b[24];std::snprintf(b,sizeof b,"%#llx",static_cast<unsigned long long>(v));return b;}
struct Region{std::uint32_t base{};std::vector<std::byte>bytes;};
struct Bank final:SourcePhysicsContactMemory{
 std::vector<Region>regions;mutable std::vector<std::pair<std::uint32_t,std::uint32_t>>reads;
 std::vector<std::pair<std::uint32_t,std::uint32_t>>writes;
 std::byte*at(std::uint32_t a){for(auto&r:regions)if(std::uint32_t(a-r.base)<r.bytes.size())return&r.bytes[a-r.base];return nullptr;}
 const std::byte*at(std::uint32_t a)const{for(auto&r:regions)if(std::uint32_t(a-r.base)<r.bytes.size())return&r.bytes[a-r.base];return nullptr;}
 void read(std::uint32_t a,std::span<std::byte>out)const override{reads.emplace_back(a,std::uint32_t(out.size()));
  for(std::size_t n=0;n<out.size();++n){auto p=at(a+std::uint32_t(n));if(!p)throw std::runtime_error("native read of undeclared memory "+hex(a+n));out[n]=*p;}}
 void write(std::uint32_t a,std::span<const std::byte>in)override{writes.emplace_back(a,std::uint32_t(in.size()));
  for(std::size_t n=0;n<in.size();++n){auto p=at(a+std::uint32_t(n));if(!p)throw std::runtime_error("native write of undeclared memory "+hex(a+n));*p=in[n];}}
};
std::uint32_t router_flags([[maybe_unused]]std::uint32_t sp){
/*ROUTER_FLAGS*/
 return flags;}
std::uint32_t hardware_add_flags(std::uint32_t left,std::uint32_t right){std::uint64_t flags;
 asm volatile("addl %2,%1\n\tleaq -128(%%rsp),%%rsp\n\tpushfq\n\tpopq %0\n\tleaq 128(%%rsp),%%rsp"
  :"=r"(flags),"+r"(left):"r"(right):"cc","memory");return std::uint32_t(flags)&0x8d5u;}
J lanes(const std::array<std::uint32_t,4>&v){return J::array({v[0],v[1],v[2],v[3]});}
constexpr std::uint32_t foreign_mxcsr[4]{0x1f80u,0x9fc0u,0x7f80u,0x1fbfu};
constexpr std::uint16_t foreign_cw[4]{0x037fu,0x027fu,0x0f7fu,0x007fu};
}
int main(int argc,char**argv){J out{{"passed",false}};int status=1;
 try{if(argc!=6)throw std::runtime_error("usage: cases image banks report expect-pass|expect-mismatch");
  std::ifstream cf(argv[1]);const J doc=J::parse(cf);const auto image=slurp(argv[2]),banks=slurp(argv[3]);
  const std::string mode=argv[5];if(mode!="expect-pass"&&mode!="expect-mismatch")throw std::runtime_error("mode");
  if(image.size()!=doc.at("image_size").get<std::size_t>())throw std::runtime_error("image size differs");
  Bank m;m.regions.push_back({doc.at("image_base").get<std::uint32_t>(),image});
  for(const auto&w:doc.at("windows"))m.regions.push_back({w.at("base").get<std::uint32_t>(),std::vector<std::byte>(w.at("size").get<std::size_t>())});
  J router{{"available",ROUTER_AVAILABLE}};
  if(ROUTER_AVAILABLE){std::uint64_t checked{},bad{};J first=nullptr;
   auto probe=[&](std::uint32_t sp){++checked;const auto h=hardware_add_flags(sp-0xcu,0xcu),r=router_flags(sp)&0x8d5u;
    if(h!=r){++bad;if(first.is_null())first={{"sp",sp},{"hardware",h},{"router",r}};}};
   for(std::uint32_t sp=0;sp<0x20000u;++sp)probe(sp);for(std::uint32_t sp=0xfffe0000u;sp!=0;++sp)probe(sp);
   for(std::uint32_t sp=0x7fff0000u;sp<0x80010000u;++sp)probe(sp);
   std::uint32_t x=0x2545f491u;for(int n=0;n<1000000;++n){x^=x<<13;x^=x>>17;x^=x<<5;probe(x);}
   router["hardware_add_comparisons"]=checked;router["hardware_add_mismatches"]=bad;router["first_mismatch"]=first;}
  std::uint64_t compared{},refusals{},parity_failed{},other_failed{};J failures=J::array();
  for(const auto&c:doc.at("cases")){
   std::size_t off=c.at("blob");for(std::size_t i=1;i<m.regions.size();++i){auto&r=m.regions[i];
    if(off+r.bytes.size()>banks.size())throw std::runtime_error("bank blob short");std::memcpy(r.bytes.data(),banks.data()+off,r.bytes.size());off+=r.bytes.size();}
   std::vector<std::pair<std::uint32_t,std::vector<std::byte>>>undo;
   for(const auto&p:c.at("patches")){const auto a=p.at(0).get<std::uint32_t>();const auto bytes=unhex(p.at(1));std::vector<std::byte>old(bytes.size());
    for(std::size_t n=0;n<bytes.size();++n){auto q=m.at(a+std::uint32_t(n));if(!q)throw std::runtime_error("patch outside image");old[n]=*q;*q=bytes[n];}
    undo.emplace_back(a,std::move(old));}
   const std::uint32_t id=c.at("id"),rec=c.at("esi");const auto&f=c.at("fp");
   NativeTu15PhysicsFrameFp fp{std::uint16_t(f.at("cw").get<unsigned>()),std::uint16_t(f.at("sw").get<unsigned>()),f.at("mxcsr").get<std::uint32_t>()};
   const unsigned depth=f.at("depth");m.reads.clear();m.writes.clear();
   std::vector<SourcePhysicsContactRecordStore>events;
   SourcePhysicsContactRecordObserver observer=[&](const SourcePhysicsContactRecordStore&e){events.push_back(e);};
   NativeTu15BreakableContactDamageStats stats;std::optional<NativeTu15BreakableContactDamageResult>result;std::string error;
   const auto host_mx=_mm_getcsr();std::uint16_t host_cw,host_sw,after_cw,after_sw;
   asm volatile("fnstcw %0":"=m"(host_cw));asm volatile("fnstsw %0":"=a"(host_sw));
   const auto fmx=foreign_mxcsr[id%4];const auto fcw=foreign_cw[id%4];
   _mm_setcsr(fmx);asm volatile("fldcw %0"::"m"(fcw));
   try{result=native_tu15_build_breakable_contact_damage(m,c.at("eax").get<std::uint32_t>(),c.at("ecx").get<std::uint32_t>(),{rec},fp,depth,observer,&stats);}
   catch(const std::exception&e){error=e.what();}
   const auto after_mx=_mm_getcsr();asm volatile("fnstcw %0":"=m"(after_cw));asm volatile("fnstsw %0":"=a"(after_sw));
   _mm_setcsr(host_mx);asm volatile("fldcw %0"::"m"(host_cw));
   J diffs=J::array();bool parity_bad=false;
   auto diff=[&](const std::string&aspect,bool parity,J native,J original){if(parity)parity_bad=true;
    if(diffs.size()<8)diffs.push_back({{"aspect",aspect},{"parity",parity},{"native",native},{"original",original}});else if(parity)parity_bad=true;};
   if(after_mx!=fmx||after_cw!=fcw||after_sw!=host_sw)diff("host_fp_leak",false,J::array({after_mx,after_cw,after_sw}),J::array({fmx,fcw,host_sw}));
   if(c.at("refusal").get<bool>()){++refusals;
    if(error.find(c.at("refusal_message").get<std::string>())==std::string::npos)diff("refusal_message",true,error,c.at("refusal_message"));
    if(!m.writes.empty()||!events.empty())diff("refusal_wrote",true,m.writes.size(),0);
   }else{++compared;const auto&e=c.at("expected");
    if(!error.empty())diff("native_exception",true,error,nullptr);
    else{const auto&r=*result;
     if(r.built!=(e.at("al").get<unsigned>()==1u))diff("al_built",true,r.built,e.at("al"));
     if(r.eax!=e.at("eax"))diff("eax",true,r.eax,e.at("eax"));
     if(r.ecx!=e.at("ecx"))diff("ecx",true,r.ecx,e.at("ecx"));
     if(r.edx!=e.at("edx"))diff("edx",true,r.edx,e.at("edx"));
     if(lanes(r.xmm0)!=e.at("xmm").at(0))diff("xmm0",true,lanes(r.xmm0),e.at("xmm").at(0));
     const auto&xin=c.at("xmm_in");
     const J x1=r.normalized?lanes(r.xmm1):xin.at(1),x2=r.normalized?lanes(r.xmm2):xin.at(2);
     if(x1!=e.at("xmm").at(1))diff("xmm1_after_router",true,x1,e.at("xmm").at(1));
     if(x2!=e.at("xmm").at(2))diff("xmm2_after_router",true,x2,e.at("xmm").at(2));
     if(!e.at("normalized_path").is_null()&&r.normalized!=e.at("normalized_path").get<bool>())diff("normalized_path",true,r.normalized,e.at("normalized_path"));
     if(e.at("normalized_path").is_null()&&r.normalized)diff("normalized_path",true,true,nullptr);
     if(fp.control_word!=e.at("cw"))diff("x87_cw",true,fp.control_word,e.at("cw"));
     if(fp.status!=e.at("sw"))diff("x87_sw",true,fp.status,e.at("sw"));
     if(fp.mxcsr!=e.at("mxcsr"))diff("mxcsr",true,fp.mxcsr,e.at("mxcsr"));
     J ledger=J::array();for(const auto&s:events)ledger.push_back(J::array({s.instruction,s.address,s.written.size(),hexbytes(s.before),hexbytes(s.written)}));
     if(ledger!=e.at("stores")){std::size_t n=0;const auto&o=e.at("stores");while(n<ledger.size()&&n<o.size()&&ledger[n]==o[n])++n;
      diff("ordered_stores",true,J{{"count",ledger.size()},{"index",n},{"entry",n<ledger.size()?ledger[n]:J(nullptr)}},
       J{{"count",o.size()},{"index",n},{"entry",n<o.size()?o[n]:J(nullptr)}});}
     const auto&wnd=e.at("window");const std::uint32_t wbase=wnd.at(0);const auto want=unhex(wnd.at(1));std::vector<std::byte>got(want.size());
     for(std::size_t n=0;n<got.size();++n){auto q=m.at(wbase+std::uint32_t(n));got[n]=q?*q:std::byte{0xee};}
     if(got!=want){std::size_t n=0;while(got[n]==want[n])++n;diff("record_and_guard_bytes",true,J{{"offset",n},{"byte",std::to_integer<unsigned>(got[n])}},J{{"offset",n},{"byte",std::to_integer<unsigned>(want[n])}});}
     for(const auto&[a,n]:m.writes)if(std::uint32_t(a-rec)>=0x94u||std::uint32_t(a-rec)+n>0x94u){diff("write_outside_record",true,J::array({a,n}),J::array({rec,0x94}));break;}
     std::set<std::uint32_t>allowed;for(const auto&rd:e.at("reads"))for(std::uint32_t n=0;n<rd.at(1).get<std::uint32_t>();++n)allowed.insert(rd.at(0).get<std::uint32_t>()+n);
     for(const auto&[a,n]:m.reads){bool ok=true;for(std::uint32_t k=0;k<n;++k){const auto b=a+k;if(!allowed.count(b)&&std::uint32_t(b-rec)>=0x94u)ok=false;}
      if(!ok){diff("read_not_read_by_original",true,J::array({a,n}),nullptr);break;}}
     if(ROUTER_AVAILABLE&&(router_flags(c.at("sp").get<std::uint32_t>())&0x8d5u)!=e.at("eflags_arith"))
      diff("router_eflags",false,router_flags(c.at("sp").get<std::uint32_t>())&0x8d5u,e.at("eflags_arith"));
     const unsigned kind=c.at("kind");const bool built=r.built,norm=r.normalized;
     const bool stats_ok=stats.calls==1&&stats.below_threshold==(built?0u:1u)&&stats.structure_keys==(built&&kind==1?1u:0u)&&
      stats.instance_keys==(built&&kind!=1?1u:0u)&&stats.other_kind_keys==(built&&kind!=1&&kind!=5?1u:0u)&&
      stats.normalize.calls==(built?1u:0u)&&stats.normalize.normalized==(built&&norm?1u:0u)&&stats.normalize.fallbacks==(built&&!norm?1u:0u);
     if(!stats_ok)diff("stats",false,J::array({stats.calls,stats.below_threshold,stats.structure_keys,stats.instance_keys,stats.other_kind_keys,
      stats.normalize.calls,stats.normalize.normalized,stats.normalize.fallbacks}),nullptr);
    }}
   for(auto it=undo.rbegin();it!=undo.rend();++it)for(std::size_t n=0;n<it->second.size();++n)*m.at(it->first+std::uint32_t(n))=it->second[n];
   if(!diffs.empty()){if(parity_bad)++parity_failed;else ++other_failed;
    if(failures.size()<24)failures.push_back({{"id",id},{"label",c.at("label")},{"differences",diffs}});}
  }
  const bool clean=parity_failed==0&&other_failed==0&&(!ROUTER_AVAILABLE||router.at("hardware_add_mismatches")==0);
  const bool detected=parity_failed>0;
  out={{"passed",mode=="expect-pass"?clean:detected},{"mode",mode},{"compared_cases",compared},{"refusal_cases",refusals},
   {"parity_failed_cases",parity_failed},{"non_parity_failed_cases",other_failed},{"router",router},{"first_failures",failures}};
  status=out.at("passed").get<bool>()?0:1;
 }catch(const std::exception&e){out["error"]=e.what();std::cerr<<e.what()<<'\n';status=2;}
 if(argc>=5){std::ofstream o(argv[4]);o<<out.dump(1)<<'\n';}
 std::cout<<J{{"passed",out.value("passed",false)},{"parity_failed",out.value("parity_failed_cases",0)}}.dump()<<'\n';
 return status;}
'''


# --------------------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--source-root', type=Path, default=REPO, help='checkout or frozen snapshot holding the native files')
    ap.add_argument('--tools-root', type=Path, default=REPO, help='checkout holding OriginalAudioCpu and run_tests.py')
    ap.add_argument('--output', type=Path, required=True, help='bounded report directory (outside scratch)')
    ap.add_argument('--pin', action='append', default=[], help='relative/path=sha256 required native source pin')
    ap.add_argument('--compiler', action='append', default=None, help='C++ compiler (default g++ and clang++)')
    ap.add_argument('--jobs', type=int, default=2)
    a = ap.parse_args()
    started = time.time()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source_root, tools_root = a.source_root.resolve(strict=True), a.tools_root.resolve(strict=True)
    a.output = a.output.resolve(); a.output.mkdir(parents=True, exist_ok=False)
    assert not a.output.is_relative_to(scratch)
    compilers = a.compiler or ['g++', 'clang++']
    report = dict(passed=False, verdict='incomplete', scope=__doc__.strip().splitlines()[0], scratch=str(scratch),
                  source_root=str(source_root), tools_root=str(tools_root), retained_large_artifacts=[])
    try:
        # ---- pins
        native_pins = {n: sha(source_root / n) for n in NATIVE_FILES}
        router_text = (source_root / ROUTER).read_text() if (source_root / ROUTER).exists() else ''
        router_pin = sha(source_root / ROUTER) if router_text else None
        tool_pins = {n: sha(tools_root / n) for n in TOOL_FILES}
        harness_pin = sha(Path(__file__))
        report.update(native_source_sha256=native_pins, router_source_sha256=router_pin,
                      tool_sha256=tool_pins, harness_sha256=harness_pin, harness_path=str(Path(__file__).resolve()))
        for entry in a.pin:
            name, digest = entry.split('=', 1)
            assert native_pins.get(name, router_pin if name == ROUTER else None) == digest, ('pin mismatch', name)
        for name in NATIVE_FILES:   # header closure guard: the six files are the entire halo2 dependency set
            for inc in re.findall(r'#include "(halo2/[^"]+)"', (source_root / name).read_text()):
                assert 'include/' + inc in NATIVE_FILES, ('native header closure grew', name, inc)
        mods = load_original_modules(tools_root)
        xbe_path = Path(mods['NEW_DEFAULT'])
        assert sha(xbe_path) == mods['NEW_PIN'] == '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
        image = mods['Xbe'](xbe_path); code = mods['_Code'](image, '.text')
        bodies = {}; body_pcs = {}
        for entry, (size, digest) in BODIES.items():
            raw = image.read(entry, size); assert hashlib.sha256(raw).hexdigest() == digest, hex(entry)
            fn = mods['complete_body'](code, entry)
            assert min(fn.code) == entry and max(p + n for p, n in fn.code.items()) == entry + size, hex(entry)
            assert not fn.tail_jumps
            calls = sorted((s[0], s[1]) for s in fn.sites if s[2] == 'call')
            bodies[hex(entry)] = dict(size=size, sha256=digest, instructions=len(fn.code),
                                      calls=[[hex(x), hex(y)] for x, y in calls])
            body_pcs[entry] = set(fn.code)
        assert bodies['0x1e2800']['calls'] == [['0x1e28dd', '0xd8bb0'], ['0x1e292b', '0x122b60']]
        assert bodies['0xd8bb0']['calls'] == [] and bodies['0x122b60']['calls'] == []
        all_pcs = set().union(*body_pcs.values())
        report.update(xbe=str(xbe_path), xbe_sha256=mods['NEW_PIN'], original_bodies=bodies)

        # ---- physical original
        Cpu = make_cpu_class(mods)
        specs = case_specs(); cases = []; blob = bytearray(); digests = []; executed_union = set()
        fp_changes = dict(x87_sw_bits={}, mxcsr_bits={}, fip_changed=0, empty_register_contents_changed=0)
        stack_low = 0
        workdir = scratch / 'work'; workdir.mkdir()
        host = Cpu(image, workdir / 'glass-original32.elf', 0x27f, 0x1f80,
                   extra_declared_regions=((HIGH, HIGH_WIN),), source_bank_bytes=BANK_WIN)
        try:
            pristine = bytearray(host.read(IMAGE_BASE, IMAGE_SIZE))
            touched = set()
            cache = {}
            for cid, spec in enumerate(specs):
                s, f, fp, gpr, rec, key = build(spec, cid)
                patches = f.patch_list()
                case = dict(id=cid, label=s['label'], refusal=bool(s['refusal']), blob=len(blob),
                            patches=[[addr, data.hex()] for addr, data in patches], eax=gpr['eax'], ecx=gpr['ecx'],
                            esi=rec, sp=s['sp'], kind=key >> 29,
                            fp=dict(cw=fp['cw'], sw=fp['sw'], mxcsr=fp['mxcsr'], depth=fp['depth']), xmm_in=fp['xmm'])
                blob += f.bank + f.high
                if s['refusal']:
                    case['refusal_message'] = FAULT_MESSAGE; cases.append(case); continue
                # restore every previously patched image byte, then apply this case
                for addr in touched:
                    host.write(addr, bytes([pristine[addr - IMAGE_BASE]]))
                expected_image = bytearray(pristine)
                for addr, data in patches:
                    host.write(addr, data); expected_image[addr - IMAGE_BASE:addr - IMAGE_BASE + len(data)] = data
                    touched.update(range(addr, addr + len(data)))
                host.write(BANK, bytes(f.bank)); host.write(HIGH, bytes(f.high))
                window_lo = rec - GUARD; window_size = REC_SIZE + 2 * GUARD
                window_before = host.read(window_lo, window_size)
                host.prepare_full(0x1e2800, s['sp'], gpr, s['eflags'], fp)
                before_fp = host.fpstate()
                regs, stores, reads, executed = host.trace_case(code, cache, window_lo, window_size)
                after_fp = host.fpstate()
                r = {n: getattr(regs, n) & M for n in ('rax', 'rbx', 'rcx', 'rdx', 'rsi', 'rdi', 'rbp', 'rsp')}
                eflags = regs.eflags & M
                problems = []
                def need(ok, what):
                    if not ok:
                        problems.append(what)
                need(r['rsp'] == (s['sp'] + 4) & M, 'esp')
                need((r['rbx'], r['rbp'], r['rsi'], r['rdi']) == (gpr['ebx'], gpr['ebp'], rec, gpr['edi']), 'callee-saved')
                need(eflags & 0x8d5 == add_flags((s['sp'] - 0xc) & M, 0xc), 'eflags arithmetic != ADD(ESP0-0xC,0xC)')
                need(eflags & ~(0x8d5 | 0x10000 | 0x100) & 0x3fffff == s['eflags'] & ~(0x8d5 | 0x100) & 0x3fffff, 'eflags control bits')
                need(set(executed) <= all_pcs, 'executed outside the three bodies')
                need(after_fp['xmm'][3:] == fp['xmm'][3:], 'xmm3-7')
                normalized = None
                if 0x122b92 in executed: normalized = True
                if 0x122bc8 in executed: normalized = False
                if normalized is not True:
                    need(after_fp['xmm'][1:3] == fp['xmm'][1:3], 'xmm1/xmm2 changed without normalization')
                need(after_fp['cw'] == fp['cw'], 'x87 cw')
                need(after_fp['tag'] == before_fp['tag'] and (after_fp['sw'] & 0x3800) == (fp['sw'] & 0x3800), 'x87 depth/tags')
                need(after_fp['st'][:fp['depth']] == fp['st'][:fp['depth']], 'live x87 values')
                need(after_fp['mxcsr'] & ~0x3f == fp['mxcsr'] & ~0x3f, 'mxcsr control bits')
                need(r['rax'] & 0xff in (0, 1), 'al')
                need(all(rec <= st[1] and st[1] + st[2] <= rec + REC_SIZE for st in stores), 'store outside record')
                # stray-write census: image, bank and high windows outside record and stack
                need(host.read(IMAGE_BASE, IMAGE_SIZE) == bytes(expected_image), 'image write')
                window_after = host.read(window_lo, window_size)
                stack_lo = s['sp'] - 0x40
                for base, size, initial in ((BANK, BANK_WIN, f.bank), (HIGH, HIGH_WIN, f.high)):
                    now = bytearray(host.read(base, size)); want = bytearray(initial)
                    for lo, hi in ((window_lo, window_lo + window_size), (stack_lo, s['sp'])):
                        a0, a1 = max(lo, base), min(hi, base + size)
                        if a0 < a1:
                            now[a0 - base:a1 - base] = bytes(a1 - a0); want[a0 - base:a1 - base] = bytes(a1 - a0)
                    need(now == want, f'stray write in {base:#x} window')
                    sa0, sa1 = max(stack_lo, base), min(s['sp'], base + size)
                    if sa0 < sa1:
                        stack_now, stack_init = host.read(sa0, sa1 - sa0), initial[sa0 - base:sa1 - base]
                        used = [sa0 + n for n in range(sa1 - sa0) if stack_now[n] != stack_init[n]]
                        if used:
                            stack_low = max(stack_low, s['sp'] - min(used))
                if problems:
                    raise AssertionError(f'physical assertion failed for {s["label"]}: {problems}')
                executed_union.update(executed)
                for n in range(16):
                    if (before_fp['sw'] ^ after_fp['sw']) >> n & 1:
                        fp_changes['x87_sw_bits'][hex(1 << n)] = fp_changes['x87_sw_bits'].get(hex(1 << n), 0) + 1
                    if (before_fp['mxcsr'] ^ after_fp['mxcsr']) >> n & 1:
                        fp_changes['mxcsr_bits'][hex(1 << n)] = fp_changes['mxcsr_bits'].get(hex(1 << n), 0) + 1
                fp_changes['fip_changed'] += int(after_fp['fip'] != before_fp['fip'])
                fp_changes['empty_register_contents_changed'] += int(after_fp['st'][fp['depth']:] != before_fp['st'][fp['depth']:])
                merged = []
                for address, size in reads:
                    if merged and merged[-1][0] + merged[-1][1] == address:
                        merged[-1][1] += size
                    else:
                        merged.append([address, size])
                case['expected'] = dict(al=r['rax'] & 0xff, eax=r['rax'], ecx=r['rcx'], edx=r['rdx'],
                                        xmm=after_fp['xmm'][:3], cw=after_fp['cw'], sw=after_fp['sw'],
                                        mxcsr=after_fp['mxcsr'], eflags_arith=eflags & 0x8d5,
                                        stores=[list(x) for x in stores], window=[window_lo, window_after.hex()],
                                        reads=merged, normalized_path=normalized)
                cases.append(case)
                digests.append([cid, s['label'], r['rax'] & 0xff, normalized, hex(after_fp['sw']), hex(after_fp['mxcsr']),
                                len(stores), hex(r['rax']), hex(r['rcx']), hex(r['rdx'])])
            for entry, size in ((e, BODIES[e][0]) for e in BODIES):
                assert host.read(entry, size) == image.read(entry, size)
        finally:
            host.close()
        coverage = {hex(e): dict(executed=len(pcs & executed_union), instructions=len(pcs),
                                 missing=[hex(p) for p in sorted(pcs - executed_union)]) for e, pcs in body_pcs.items()}
        compare = [c for c in cases if not c['refusal']]
        report.update(original=dict(
            backend='Unchanged English TU1.5 i386 instructions single-stepped by Linux ptrace (OriginalAudioCpu); '
                    'no opcode patch, no supplied callee result',
            compared_cases=len(compare), refusal_cases=len(cases) - len(compare),
            built_cases=sum(c['expected']['al'] == 1 for c in compare),
            rejected_gate_cases=sum(c['expected']['al'] == 0 for c in compare),
            normalized_cases=sum(c['expected']['normalized_path'] is True for c in compare),
            fallback_cases=sum(c['expected']['normalized_path'] is False for c in compare),
            ordered_store_events=sum(len(c['expected']['stores']) for c in compare),
            rep_stosd_events_per_built_case=sorted({sum(1 for x in c['expected']['stores'] if x[0] == 0xd8bc2)
                                                    for c in compare if c['expected']['al'] == 1}),
            coverage=coverage, deepest_stack_bytes_written=stack_low, fp_live_out_census=fp_changes,
            physical_assertions=['ESP=entry+4', 'EBX/EBP/ESI/EDI preserved', 'EFLAGS&0x8D5 = ADD(ESP0-0xC,0xC)',
                                 'non-arithmetic EFLAGS unchanged', 'XMM3-7 unchanged',
                                 'XMM1/XMM2 unchanged unless 122B92 path', 'x87 CW/TOP/tags/live ST unchanged',
                                 'MXCSR control bits unchanged', 'stores only inside the 0x94 record',
                                 'no write anywhere in image/bank/high window except record and stack',
                                 'executed PCs within the three pinned bodies', 'every in-window byte change listed']))
        for entry, cov in coverage.items():
            assert not cov['missing'], ('original body not fully covered', entry, cov['missing'])

        # ---- native project
        data = scratch / 'data'; data.mkdir()
        doc = dict(image_base=IMAGE_BASE, image_size=IMAGE_SIZE,
                   windows=[dict(base=BANK, size=BANK_WIN), dict(base=HIGH, size=HIGH_WIN)], cases=cases)
        cases_json = json.dumps(doc, separators=(',', ':'))
        (data / 'cases.json').write_text(cases_json); (data / 'image.bin').write_bytes(bytes(pristine))
        (data / 'banks.bin').write_bytes(bytes(blob))
        report['oracle_cases_sha256'] = hashlib.sha256(cases_json.encode()).hexdigest()
        report['case_digest'] = digests
        project = scratch / 'project'; (project / 'include/halo2').mkdir(parents=True)
        for name in NATIVE_FILES:
            if name.startswith('include/'):
                shutil.copyfile(source_root / name, project / name)
        kernel_text = (source_root / KERNEL).read_text(); normalize_text = (source_root / NORMALIZE).read_text()
        (project / 'kernel.cpp').write_text(kernel_text); (project / 'normalize.cpp').write_text(normalize_text)
        snippet = router_snippet(router_text) if router_text else None
        driver = DRIVER.replace('/*ROUTER_FLAGS*/', snippet if snippet else 'const std::uint32_t flags=0;')
        driver = driver.replace('ROUTER_AVAILABLE', 'true' if snippet else 'false')
        (project / 'driver.cpp').write_text(driver)
        report['router_flags_snippet'] = snippet
        results = scratch / 'results'
        cmake = ['cmake_minimum_required(VERSION 3.20)', 'project(glass LANGUAGES CXX)', 'include(CTest)',
                 'find_package(nlohmann_json REQUIRED)', 'add_custom_target(glass_all)',
                 'add_library(glass_driver OBJECT driver.cpp)', 'target_include_directories(glass_driver PRIVATE include)',
                 'target_compile_features(glass_driver PRIVATE cxx_std_20)',
                 'target_link_libraries(glass_driver PRIVATE nlohmann_json::nlohmann_json)',
                 'function(glass_exe name kernel normalize mode)',
                 '  add_executable(${name} ${kernel} ${normalize} $<TARGET_OBJECTS:glass_driver>)',
                 '  target_include_directories(${name} PRIVATE include)',
                 '  target_compile_features(${name} PRIVATE cxx_std_20)',
                 '  set_source_files_properties(${kernel} ${normalize} TARGET_DIRECTORY ${name} PROPERTIES COMPILE_OPTIONS '
                 '"-fno-fast-math;-ffp-contract=off;-frounding-math")',
                 '  target_link_libraries(${name} PRIVATE nlohmann_json::nlohmann_json)',
                 '  add_dependencies(glass_all ${name})',
                 f'  add_test(NAME ${{name}} COMMAND ${{name}} "{data}/cases.json" "{data}/image.bin" "{data}/banks.bin" '
                 f'"{results}/${{CMAKE_CXX_COMPILER_ID}}/${{name}}.json" ${{mode}})',
                 'endfunction()', 'glass_exe(glass_native kernel.cpp normalize.cpp expect-pass)']
        mutants = []
        for name, target, needle, replacement in MUTATIONS:
            if target == KERNEL:
                (project / f'mut_{name}.cpp').write_text(mutate(kernel_text, name, needle, replacement))
                cmake.append(f'glass_exe(glass_mut_{name} mut_{name}.cpp normalize.cpp expect-mismatch)')
            else:
                (project / f'mut_{name}.cpp').write_text(mutate(normalize_text, name, needle, replacement))
                cmake.append(f'glass_exe(glass_mut_{name} kernel.cpp mut_{name}.cpp expect-mismatch)')
            mutants.append(name)
        (project / 'CMakeLists.txt').write_text('\n'.join(cmake) + '\n')
        builds = scratch / 'builds'
        native = {}; commands = []
        for compiler in compilers:
            label = Path(compiler).name
            command = [sys.executable, str(tools_root / 'tools/run_tests.py'), '--source', str(project),
                       '--target', 'glass_all', '--filter', '^glass_', '--jobs', str(a.jobs), '--test-jobs', str(a.jobs),
                       '--scratch-root', str(builds), '--output', str(scratch / 'run-tests' / label),
                       '--cmake-arg=-DCMAKE_CXX_COMPILER=' + compiler]
            commands.append(command)
            for compiler_id in ('GNU', 'Clang'):
                (results / compiler_id).mkdir(parents=True, exist_ok=True)
            run = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            ids = sorted(p.name for p in results.iterdir())
            entry = dict(run_tests_exit=run.returncode, output_tail=run.stdout[-1500:])
            rt_report = scratch / 'run-tests' / label / 'report.json'
            if rt_report.exists():
                rt = json.loads(rt_report.read_text())
                entry['run_tests_report'] = {k: rt[k] for k in rt if k in ('passed', 'exit_code', 'returncode', 'phase', 'status')}
            leftovers = [str(p) for p in builds.iterdir()] if builds.exists() else []
            entry['private_build_leftovers'] = leftovers
            # driver reports live under results/<CMAKE_CXX_COMPILER_ID>
            produced = {}
            for cid_dir in ids:
                for path in sorted((results / cid_dir).glob('*.json')):
                    produced.setdefault(cid_dir, {})[path.stem] = json.loads(path.read_text())
            entry['driver_reports'] = produced
            native[label] = entry
            shutil.rmtree(results, ignore_errors=True)   # keep compilers separate
        # ---- verdicts
        verdict_rows = {}; all_ok = True; kernel_mismatch = []
        for label, entry in native.items():
            reports = next(iter(entry['driver_reports'].values()), {}) if entry['driver_reports'] else {}
            main_report = reports.get('glass_native')
            ok = bool(main_report and main_report.get('passed'))
            if main_report and main_report.get('parity_failed_cases'):
                kernel_mismatch.append(dict(compiler=label, failures=main_report.get('first_failures')))
            mut = {}
            for name in mutants:
                rep = reports.get(f'glass_mut_{name}')
                first = (rep or {}).get('first_failures') or [{}]
                mut[name] = dict(detected=bool(rep and rep.get('passed')),
                                 parity_failed_cases=(rep or {}).get('parity_failed_cases'),
                                 first_case=first[0].get('label'),
                                 first_aspects=[d['aspect'] for d in first[0].get('differences', []) if d.get('parity')][:4],
                                 error=(rep or {}).get('error'))
            verdict_rows[label] = dict(native_passed=ok, native=dict((k, main_report.get(k)) for k in (
                'compared_cases', 'refusal_cases', 'parity_failed_cases', 'non_parity_failed_cases', 'router', 'first_failures', 'error'))
                if main_report else None, mutants=mut, run_tests_exit=entry['run_tests_exit'],
                private_build_leftovers=entry['private_build_leftovers'],
                run_tests_report=entry.get('run_tests_report'), output_tail=None if ok else entry['output_tail'])
            all_ok &= ok and all(v['detected'] for v in mut.values()) and not entry['private_build_leftovers']
        report.update(native=verdict_rows, mutations=[dict(name=n, file=t) for n, t, *_ in MUTATIONS], commands=commands)
        # ---- source stability guard
        after = {n: sha(source_root / n) for n in NATIVE_FILES}
        copies = {n: sha(project / n) for n in NATIVE_FILES if n.startswith('include/')}
        copies[KERNEL] = sha(project / 'kernel.cpp'); copies[NORMALIZE] = sha(project / 'normalize.cpp')
        stable = after == native_pins and copies == native_pins and \
            (sha(source_root / ROUTER) if router_text else None) == router_pin and \
            {n: sha(tools_root / n) for n in TOOL_FILES} == tool_pins and sha(Path(__file__)) == harness_pin
        report['source_hashes_stable'] = stable
        all_ok &= stable
        report['kernel_mismatch'] = kernel_mismatch
        evidence = {}
        for row in kernel_mismatch:
            for failure in row['failures'] or []:
                c = cases[failure['id']]; e = c['expected']
                evidence[failure['id']] = dict(
                    label=c['label'], compilers=sorted({*evidence.get(failure['id'], {}).get('compilers', []), row['compiler']}),
                    overrides_authenticated_rdata=any(0x41b600 <= p[0] < 0x46d6e0 for p in c['patches']),
                    image_patches=c['patches'], fp_in=c['fp'], xmm_in_1_2=c['xmm_in'][1:3],
                    original=dict(al=e['al'], eax=e['eax'], ecx=e['ecx'], edx=e['edx'], xmm0_2=e['xmm'], sw=e['sw'],
                                  mxcsr=e['mxcsr'], normalized_path=e['normalized_path'],
                                  normalizer_stores=[x for x in e['stores'] if 0x122b60 <= x[0] < 0x122bdd]),
                    native_differences=failure['differences'])
        report['kernel_mismatch_evidence'] = list(evidence.values())
        report['kernel_mismatch_only_with_rdata_overrides'] = bool(evidence) and all(
            v['overrides_authenticated_rdata'] for v in evidence.values()) and all(
            (row['failures'] is not None and len(row['failures']) < 24) for row in kernel_mismatch)
        report['passed'] = bool(all_ok)
        report['verdict'] = ('KERNEL MISMATCH' if kernel_mismatch else 'qualified' if all_ok else 'harness or control failure')
    except BaseException as error:
        report['error'] = f'{type(error).__name__}: {error}'
        raise
    finally:
        report['elapsed_seconds'] = round(time.time() - started, 1)
        (a.output / 'report.json').write_text(json.dumps(report, indent=1) + '\n')
        print(json.dumps({k: report.get(k) for k in ('passed', 'verdict', 'error', 'source_hashes_stable')}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
