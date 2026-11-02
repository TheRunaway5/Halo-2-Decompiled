#!/usr/bin/env python3
"""Physical unchanged TU1.5 17C140 with explicit lower-owner ABI controls.

Only the outer advance routine executes here. Particle/update, next-event,
selection, retirement and random-range callees have declared control results;
this is not proof of those callees or complete rendered/audio output. No
instruction is patched. All stores, traces and caller/FP states are physical.
"""
import argparse
import ctypes
import gzip
import hashlib
import json
import os
from pathlib import Path
import random
import signal
import struct
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_dispatch_tu15 import STACK, STOP, NONE, pack
from oracle_weapon_effect_state_tu15 import (
    ALIASES, REGS, DEBUG_FLAGS, CWS, CSRS, EXTENDED, POOL, BODY, TAGS,
    DEFINITION, EVENTS, BAD,
)
from weapon_reference_versions import images

ENTRY = 0x17c140
SERVICES = {0x17beb0: 12, 0x17ce40: 0, 0x17c020: 4, 0x17b130: 0,
            0x17d210: 4, 0x17dc60: 0, 0x17a4a0: 0, 0x17aab0: 0, 0x259b0: 8}
FLOATS = [0, 0x80000000, 0x3e800000, 0x3f800000, 0xbe800000,
          0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345, 1, 0x80000001]


def getfp(host, depth):
    buffer = ctypes.create_string_buffer(512)
    host.trace(14, 0, ctypes.addressof(buffer))
    raw = buffer.raw
    return dict(cw=struct.unpack_from('<H', raw, 0)[0],
                status=struct.unpack_from('<H', raw, 2)[0],
                mxcsr=struct.unpack_from('<I', raw, 24)[0],
                stack=[raw[32+i*16:42+i*16].hex() for i in range(depth)],
                xmm=[raw[160+i*16:176+i*16].hex() for i in range(8)])


def putfp(host, fp):
    buffer = ctypes.create_string_buffer(512)
    host.trace(14, 0, ctypes.addressof(buffer))
    raw = bytearray(buffer.raw)
    top = (fp['status'] >> 11) & 7
    struct.pack_into('<HHH', raw, 0, fp['cw'], fp['status'],
                     sum(1 << ((top+i) & 7) for i in range(len(fp['stack']))))
    struct.pack_into('<I', raw, 24, fp['mxcsr'])
    for i, value in enumerate(fp['stack']):
        raw[32+i*16:42+i*16] = bytes.fromhex(value)
    for i, value in enumerate(fp['xmm']):
        raw[160+i*16:176+i*16] = bytes.fromhex(value)
    buffer = ctypes.create_string_buffer(bytes(raw), 512)
    host.trace(15, 0, ctypes.addressof(buffer))


def state(host, regs, depth):
    return dict(gp=[getattr(regs, n) & NONE for n in ALIASES],
                eflags=regs.eflags & ~DEBUG_FLAGS, fp=getfp(host, depth),
                stack=host.read(STACK-64, 128).hex())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _, image, pin = next(x for x in images() if x[0] == 'TU1.5')
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    rng = random.Random(ENTRY)
    rows, instructions, steps = [], {}, 0
    recipes = [('base', n) for n in range(128)]
    recipes += [(name, n) for n, name in enumerate([
        'fault-pool', 'fault-body', 'fault-tags', 'fault-definition-begin',
        'fault-definition-next', 'fault-events', 'fault-event-maximum',
        'alias-counter-elapsed', 'alias-counter-flags',
        'retire-success', 'retire-hold', 'eight-iteration-cap',
        'begin-equal-range', 'begin-random-range',
    ])]
    with tempfile.TemporaryDirectory(prefix='native-effect-advance-', dir=scratch) as temporary, \
         OriginalAudioCpu(image, Path(temporary)/'original.elf',
                          0x27f, 0x1f80, source_bank_bytes=0x1100000) as host:
        host.trace(6, host.debug_offset+7*8, 0)
        for recipe, n in recipes:
            body = BODY
            if recipe == 'alias-counter-elapsed':
                body = STACK-0x6c
                # PUSH EBP aliases this record's selected event. Keep that
                # actual saved input in the three-row authored event bank.
                gp_ebp = 0
            elif recipe == 'alias-counter-flags':
                body = STACK-0xe
            gp = [rng.getrandbits(32) for _ in ALIASES]
            gp[4] = STACK
            if recipe == 'alias-counter-elapsed':
                gp[5] = gp_ebp
            handle = 0xa1230002
            flags = [0, 1, 4, 8, 9, 0x40, 0x41, 0x80, 0x101, 0x201, 0x209][n % 11]
            seconds, elapsed, duration = (FLOATS[n % 11], FLOATS[(n+2) % 11],
                                          FLOATS[(n+3) % 11])
            if recipe != 'base':
                flags, seconds, elapsed, duration = 0, 0x3f800000, 0, 0x3e800000
            if recipe == 'fault-definition-next':
                flags = 1
            if recipe in ('retire-success', 'retire-hold'):
                flags = 1
            if recipe == 'eight-iteration-cap':
                flags, seconds, elapsed, duration = 1, 0, 0, 0
            memory = []

            def word(at, value, width=4):
                memory.append([at, value & ((1 << (width*8))-1), width])

            word(0x4f720c, POOL)
            word(POOL+0x44, (body-2*400) & NONE)
            word(body+2, flags, 2)
            word(body+4, 0x80010007)
            word(body+0x5c, n % 3, 2)
            word(body+0x5e, (n+1) % 3, 2)
            word(body+0x60, elapsed)
            word(body+0x64, duration)
            word(body+0x68, rng.getrandbits(32))
            word(0x4f0414, TAGS)
            word(TAGS+7*16+8, DEFINITION)
            word(DEFINITION+0x14, 3)
            word(DEFINITION+0x18, EVENTS)
            for event in range(3):
                lo = FLOATS[(n+event) % 11]
                hi = lo if n % 3 == 0 else FLOATS[(n+event+3) % 11]
                if recipe == 'begin-equal-range':
                    lo = hi = 0x3f000000
                elif recipe == 'begin-random-range':
                    lo, hi = 0x3e800000, 0x3f000000
                word(EVENTS+event*0x38+0x10, lo)
                word(EVENTS+event*0x38+0x14, hi)
            word(0x46a0dc, int.from_bytes(image.read(0x46a0dc, 4), 'little'))
            if recipe == 'fault-pool':
                word(0x4f720c, BAD)
            elif recipe == 'fault-body':
                word(POOL+0x44, BAD-2*400)
            elif recipe == 'fault-tags':
                word(0x4f0414, BAD)
            elif recipe.startswith('fault-definition-'):
                word(TAGS+7*16+8, BAD)
            elif recipe == 'fault-events':
                word(DEFINITION+0x18, BAD-(n % 3)*0x38)
            elif recipe == 'fault-event-maximum':
                word(DEFINITION+0x18, 0x2100000-0x14-(n % 3)*0x38)
                word(0x2100000-4, 0x7f812345)
            fp = dict(cw=CWS[n % len(CWS)],
                      status=((n % 8) << 11) | ((n*0x100) & 0x4700) | (n & 0x3f),
                      mxcsr=CSRS[n % len(CSRS)],
                      stack=[EXTENDED[2], EXTENDED[3]][:n % 3],
                      xmm=[rng.randbytes(16).hex() for _ in range(8)])
            host.write(STACK-64, rng.randbytes(128))
            for at, value, width in memory:
                host.write(at, value.to_bytes(width, 'little'))
            host.write(STACK, pack([STOP, handle, seconds]))
            initial_stack = host.read(STACK-64, 128).hex()
            host.prepare(ENTRY, STACK, dict(zip(ALIASES, gp)))
            putfp(host, fp)
            regs = host.registers()
            regs.eflags = 0x202 | (rng.getrandbits(12) & 0xcd5)
            host.trace(13, 0, ctypes.addressof(regs))
            initial_flags = regs.eflags
            calls, stores, trace, fault = [], [], [], []
            depth, defined = len(fp['stack']), NONE & ~DEBUG_FLAGS
            for _ in range(4000):
                pc = regs.rip
                if pc == STOP:
                    break
                if pc in SERVICES:
                    before = state(host, regs, depth)
                    ret = int.from_bytes(host.read(regs.rsp, 4), 'little')
                    returned = dict(gp=before['gp'].copy(), eflags=0x202 | (rng.getrandbits(12) & 0xcd5),
                                    fp=dict(before['fp']))
                    returned['fp']['stack'] = before['fp']['stack'].copy()
                    returned['fp']['xmm'] = [rng.randbytes(16).hex() for _ in range(8)]
                    for i in range(3):
                        returned['gp'][i] = rng.getrandbits(32)
                    if pc == 0x17c020:
                        returned['gp'][0] = [0, 1, 3, NONE][n % 4]
                        if recipe in ('retire-success', 'retire-hold'):
                            returned['gp'][0] = 3
                        elif recipe == 'eight-iteration-cap':
                            returned['gp'][0] = 0
                    elif pc == 0x17a4a0:
                        retired = (n // 4) % 2
                        if recipe in ('retire-success', 'retire-hold'):
                            retired = int(recipe == 'retire-success')
                        returned['gp'][0] = (returned['gp'][0] & ~255) | retired
                    elif pc == 0x259b0:
                        returned['fp']['cw'] = CWS[(n+3) % len(CWS)]
                        returned['fp']['mxcsr'] = CSRS[(n+1) % len(CSRS)]
                        status = returned['fp']['status']
                        returned['fp']['status'] = (status & ~0x3800) | ((((status >> 11)-1) & 7) << 11)
                        returned['fp']['stack'].insert(0, EXTENDED[n % len(EXTENDED)])
                        depth += 1
                    returned['gp'][4] += 4+SERVICES[pc]
                    putfp(host, returned['fp'])
                    for name, value in zip(ALIASES, returned['gp']):
                        setattr(regs, name, value)
                    regs.rip, regs.eflags = ret, returned['eflags']
                    host.trace(13, 0, ctypes.addressof(regs))
                    calls.append(dict(target=pc, before=before, flags_mask=defined, returned=returned))
                    defined = NONE & ~DEBUG_FLAGS
                    continue
                ins = next(decoder.disasm(image.read(pc, 15), pc, count=1))
                raw = bytes(ins.bytes)
                assert host.read(pc, len(raw)) == raw, (hex(pc), 'changed original')
                assert ENTRY <= pc <= 0x17c329, hex(pc)
                instructions[f'{pc:08x}'] = raw.hex()
                trace.append(pc)
                pending, operands = [], []

                def reg(name):
                    return 0 if not name else getattr(regs, REGS[name]) & NONE

                if ins.mnemonic in ('push', 'call'):
                    pending.append(((regs.rsp-4) & NONE, 4))
                for operand in ins.operands:
                    if operand.type != 3:
                        continue
                    at = (reg(ins.reg_name(operand.mem.base)) +
                          reg(ins.reg_name(operand.mem.index))*operand.mem.scale + operand.mem.disp) & NONE
                    operands.append((at, operand.size))
                    if operand.access & 2 or ins.mnemonic in ('fst', 'fstp'):
                        pending.append((at, operand.size))
                host.trace(9)
                _, status = os.waitpid(host.pid, 0)
                regs = host.registers()
                steps += 1
                if os.WIFSTOPPED(status) and os.WSTOPSIG(status) == signal.SIGSEGV:
                    assert recipe.startswith('fault-') and operands, (recipe, hex(pc), status)
                    fault = [dict(instruction=pc, address=operands[0][0], size=operands[0][1])]
                    break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status) == signal.SIGTRAP, (recipe, hex(pc), status)
                for at, width in pending:
                    assert STACK-64 <= at < STACK+128 or body <= at < body+400, hex(at)
                    stores.append([pc, at, width, int.from_bytes(host.read(at, width), 'little')])
                if ins.mnemonic == 'fstp':
                    depth -= 1
                if ins.mnemonic in ('cmp', 'comiss', 'ucomiss', 'add', 'sub'):
                    defined = NONE & ~DEBUG_FLAGS
                elif ins.mnemonic in ('test', 'xor', 'and', 'or'):
                    defined = (NONE & ~DEBUG_FLAGS) & ~0x10
                elif ins.mnemonic == 'imul':
                    defined = (NONE & ~DEBUG_FLAGS) & ~0xd4
                elif ins.mnemonic in ('shr', 'shl'):
                    defined = (NONE & ~DEBUG_FLAGS) & ~0x810
            else:
                raise ValueError('Original effect advancement instruction limit')
            assert bool(fault) == recipe.startswith('fault-'), (recipe, hex(regs.rip))
            if not fault:
                assert regs.rip == STOP and regs.rsp == STACK+12
            rows.append(dict(entry=ENTRY, kind=recipe, n=n, gp=gp, eflags=initial_flags, fp=fp,
                             memory=memory, initial_stack=initial_stack, calls=calls, stores=stores,
                             fault=fault, trace_offsets=b''.join(struct.pack('<H', pc-ENTRY) for pc in trace).hex(),
                             trace_base=ENTRY, expected=state(host, regs, depth), flags_mask=defined))
    data = (json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_ADVANCE_REFERENCE_V1',
                            original_sha256=pin, scope=__doc__, rows=rows), separators=(',', ':'))+'\n').encode()
    assert len(data) <= 8*1024*1024, len(data)
    compressed = gzip.compress(data, mtime=0)
    assert len(compressed) <= 1024*1024, len(compressed)
    args.fixture.write_bytes(compressed)
    report = dict(passed=True, original_sha256=pin, cases=len(rows), hardware_instructions=steps,
                  controlled_lower_calls=sum(len(row['calls']) for row in rows),
                  read_stops=sum(bool(row['fault']) for row in rows), instruction_map=instructions,
                  raw_bytes=len(data), raw_sha256=hashlib.sha256(data).hexdigest(),
                  fixture_bytes=len(compressed), fixture_sha256=hashlib.sha256(compressed).hexdigest(),
                  scratch_owner=str(scratch), backend='Physical Linux i386 PTRACE_SINGLESTEP',
                  instruction_patches=[], controlled_boundaries=[hex(pc) for pc in SERVICES],
                  physical_xbox_proof=False)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print(json.dumps({key: value for key, value in report.items() if key != 'instruction_map'}))


if __name__ == '__main__':
    main()
