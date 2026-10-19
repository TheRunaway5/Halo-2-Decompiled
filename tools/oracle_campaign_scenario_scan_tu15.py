"""Unchanged OG TU1.5 D7AA0 with explicitly declared external test callees.

Tests only the automatic placement scan, its real type descriptors and call
chronology. Test predicates/factories are not live World owners or activation.
Run under tools/run_test.py; output is a required small differential fixture.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
ENTRY, END = 0xd7aa0, 0xd7b7f
SCENARIO, PLACEMENTS, STACK, SP, RETURN = 0x2100000, 0x2110000, 0x2400000, 0x2400800, 0x300f000
TARGETS = {0xd7f30: (0xd7b11, 0xd7b16, 0), 0xc1790: (0xd7b25, 0xd7b2a, 0),
           0xc1120: (0xd7b48, 0xd7b4d, 20), 0xc13b0: (0xd7b4d, 0xd7b52, 0)}
CASES = (
    # name, counts, suppress, predicate EAX, existing EAX, unavailable callee.
    ('empty', {}, 0, 1, 0xffffffff, None),
    ('negative-counts', {0: -1, 1: -0x80000000, 6: -7}, 1, 1, 0xffffffff, None),
    ('all-authored-types', 'all', 0, 1, 0xffffffff, None),
    ('multiple-placements', {0: 3, 1: 2, 6: 2, 11: 3, 12: 2}, 0, 1, 0xffffffff, None),
    ('predicate-false', {0: 1, 6: 1}, 0, 0, 0xffffffff, None),
    ('predicate-low-byte-zero', {1: 1}, 0, 0xffffff00, 0xffffffff, None),
    ('predicate-low-byte-one', {11: 1}, 0, 0xaabbcc01, 0xffffffff, None),
    ('suppress-absent', {0: 1, 11: 1}, 1, 1, 0xffffffff, None),
    ('suppress-existing', {0: 1, 11: 1}, 1, 1, 0xabcd0001, None),
    ('false-suppress-absent', {6: 1}, 1, 0, 0xffffffff, None),
    ('false-suppress-existing', {6: 1}, 1, 0, 0xabcd0001, None),
    ('argument-high-bytes-only', {12: 1}, 0xffffff00, 1, 0xabcd0001, None),
    ('argument-low-byte-80', {12: 1}, 0x12340080, 1, 0xabcd0001, None),
    ('missing-predicate', {0: 1}, 0, 1, 0xffffffff, 0xd7f30),
    ('missing-existing', {0: 1}, 1, 1, 0xffffffff, 0xc1790),
    ('missing-factory', {11: 1}, 0, 1, 0xffffffff, 0xc1120),
    ('missing-post-create', {11: 1}, 0, 1, 0xffffffff, 0xc13b0),
)

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def word(v):
    return struct.pack('<I', v & 0xffffffff)

def dump(p, value):
    with p.open('x') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--helpers', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--execute-original-after-grant', action='store_true')
    args = parser.parse_args()
    assert args.execute_original_after_grant and os.environ.get('HALO2_TEST_SCRATCH')
    assert sys.dont_write_bytecode
    if not args.xbe.is_file():
        return 77
    assert sha(args.xbe) == PIN and not args.output.exists()
    sys.path.insert(0, str(args.helpers))
    from xbe import Xbe
    from functions import _Code, _trace
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
    import unicorn.x86_const as reg
    image = Xbe(args.xbe)
    code = _Code(image, '.text')
    pcs = set(_trace(code, ENTRY, {ENTRY}).code)
    raw = image.read(ENTRY, END-ENTRY)
    assert len(pcs) == 72 and code.at(max(pcs)).mnemonic == 'ret' and code.at(max(pcs)).operands[0].imm == 4
    assert {code.at(pc).operands[0].imm for pc in pcs if code.at(pc).mnemonic == 'call'} == set(TARGETS)
    assert all(raw[pc-ENTRY:pc-ENTRY+code.at(pc).size] == bytes(code.at(pc).bytes) for pc in pcs)
    descriptors = {}
    for kind in range(13):
        pointer = struct.unpack('<I', image.read(0x474b90+kind*4, 4))[0]
        descriptors[kind] = (pointer, image.read(pointer, 16))
    args.output.mkdir(parents=True)
    report = dict(status='RUNNING_ORIGINAL_SCENARIO_SCAN', suite='campaign-scenario-scan', entry=ENTRY,
                  xbe_sha256=PIN, oracle_sha256=sha(__file__), original_body_hex=raw.hex(),
                  static_original_PCs=sorted(pcs), tested_owner_instruction_patches=[],
                  external_fixture_scope='MOV EAX,declared result; RET/RET20. Predicates and factory outcomes are test inputs, not original owners.',
                  full_startup_proven=False, cases=[])
    for name, counts, suppress, predicate, existing, missing in CASES:
        cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        cpu.mem_map(0x10000, 0x600000)
        cpu.mem_map(SCENARIO, 0x20000)
        cpu.mem_map(STACK, 0x1000)
        cpu.mem_map(RETURN, 0x1000)
        cpu.mem_write(ENTRY, raw)
        stack = bytearray([0xcd])*0x1000
        struct.pack_into('<II', stack, SP-STACK, RETURN, suppress)
        banks = {0x4ecc20: word(SCENARIO), 0x474b90: image.read(0x474b90, 13*4),
                 SCENARIO: bytes(0x400), STACK: bytes(stack)}
        scnr = bytearray(banks[SCENARIO])
        active_counts = {}
        for kind, (at, descriptor) in descriptors.items():
            banks[at] = descriptor
            placement, palette, stride = struct.unpack_from('<hhh', descriptor, 10)
            if placement == -1 or palette == -1:
                continue
            assert 0 <= placement <= 0x3f8 and 0 <= palette <= 0x3f8 and 0 < stride <= 0x100
            count = 1 if counts == 'all' else counts.get(kind, 0)
            active_counts[kind] = count
            struct.pack_into('<II', scnr, placement, count & 0xffffffff, PLACEMENTS+kind*0x1000)
            struct.pack_into('<II', scnr, palette, 2, 0x2120000+kind*0x100)
            if count > 0:
                assert count <= 3
                banks[PLACEMENTS+kind*0x1000] = bytes((kind*17+n)&255 for n in range(count*stride))
        banks[SCENARIO] = bytes(scnr)
        for at, data in banks.items():
            cpu.mem_write(at, data)
        results = {0xd7f30: predicate, 0xc1790: existing, 0xc1120: 0xcafe0011, 0xc13b0: 0xbabe0022}
        fixtures = {}
        for target, (_, _, cleanup) in TARGETS.items():
            stub = b'\xb8'+word(results[target])+(b'\xc2'+struct.pack('<H', cleanup) if cleanup else b'\xc3')
            fixtures[target] = dict(result=results[target], cleanup=cleanup, code_hex=stub.hex())
            cpu.mem_write(target, stub)
        registers = [getattr(reg, 'UC_X86_REG_'+n) for n in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
        gp = [0x10203040, 0x20304050, 0x30405060, 0x40506070, SP, 0x50607080, 0x66778899, 0x708090a0]
        xmm = [[0x3f000000+16*n+k for k in range(4)] for n in range(8)]
        for target, value in zip(registers, gp):
            cpu.reg_write(target, value)
        for n, values in enumerate(xmm):
            cpu.reg_write(getattr(reg, 'UC_X86_REG_XMM'+str(n)), sum(v << (32*k) for k, v in enumerate(values)))
        for target, value in ((reg.UC_X86_REG_EFLAGS, 0x202), (reg.UC_X86_REG_FPCW, 0x27f),
                              (reg.UC_X86_REG_FPSW, 0), (reg.UC_X86_REG_FPTAG, 0xffff), (reg.UC_X86_REG_MXCSR, 0x1f80)):
            cpu.reg_write(target, value)
        trace, stores, calls = [], [], []
        current = [None]
        stopped = [False]
        def state():
            return dict(gp=[cpu.reg_read(r) for r in registers], xmm=[[
                (cpu.reg_read(getattr(reg, 'UC_X86_REG_XMM'+str(n))) >> (32*k)) & 0xffffffff for k in range(4)] for n in range(8)],
                eflags=cpu.reg_read(reg.UC_X86_REG_EFLAGS))
        def extent(at, size):
            assert any(base <= at and at+size <= base+len(data) for base, data in banks.items()), ('Undeclared input', hex(at), size)
        def instruction(machine, pc, size, unused):
            if pc == RETURN:
                machine.emu_stop()
                return
            if pc in TARGETS:
                boundary = state()
                at = boundary['gp'][4]
                _, ret, cleanup = TARGETS[pc]
                assert struct.unpack('<I', bytes(machine.mem_read(at, 4)))[0] == ret
                boundary['stack'] = bytes(machine.mem_read(at, 4+cleanup)).hex()
                calls.append(dict(target=pc, abi=boundary, unavailable=pc == missing))
                if pc == missing:
                    stopped[0] = True
                    machine.emu_stop()
                return
            if any(pc == target+5 for target in TARGETS):
                return
            assert pc in pcs, ('Unexpected owner', hex(pc))
            current[0] = pc
            trace.append(pc)
            assert len(trace) < 10000
        def read(machine, access, address, size, value, unused):
            extent(address, size)
        def write(machine, access, address, size, value, unused):
            extent(address, size)
            assert STACK <= address and address+size <= STACK+0x1000
            stores.append(dict(pc=current[0], address=address, before=bytes(machine.mem_read(address, size)).hex(),
                               written=(value & ((1 << (8*size))-1)).to_bytes(size, 'little').hex()))
        cpu.hook_add(UC_HOOK_CODE, instruction)
        cpu.hook_add(UC_HOOK_MEM_READ, read)
        cpu.hook_add(UC_HOOK_MEM_WRITE, write)
        cpu.emu_start(ENTRY, RETURN+1, timeout=2000000, count=20000)
        final_pc = cpu.reg_read(reg.UC_X86_REG_EIP)
        assert final_pc == (missing if stopped[0] else RETURN)
        item = dict(name=name, entry=ENTRY, gp=gp, xmm=xmm, eflags=0x202,
                    suppress=suppress, active_counts=active_counts, unavailable=missing, fixtures=fixtures,
                    before={f'{p:08x}': b.hex() for p, b in banks.items()},
                    after={f'{p:08x}': bytes(cpu.mem_read(p, len(b))).hex() for p, b in banks.items()},
                    final=state(), final_pc=final_pc, trace=trace, stores=stores, calls=calls)
        dump(args.output / (name+'.json'), item)
        report['cases'].append(dict(name=name, file=name+'.json', calls=len(calls), stores=len(stores), stopped=stopped[0]))
    report['status'] = 'PASS_UNCHANGED_ORIGINAL_SCENARIO_SCAN_WITH_DECLARED_EXTERNAL_FIXTURES'
    dump(args.output / 'report.json', report)
    print(json.dumps(dict(status=report['status'], cases=len(CASES), output=str(args.output))))

if __name__ == '__main__':
    sys.exit(main())
