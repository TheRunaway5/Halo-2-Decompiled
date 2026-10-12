#!/usr/bin/env python3
"""Validate the complete TU1.5 triangle contact callback on original host i386.

Geometry, pair records and transforms are declared procedure inputs. Installed
shape methods bind through original constructor stores. Nothing constructs a
live world or supplies a contact outcome. The emulator discovers initial reads;
unchanged original CPU instructions provide the final comparison authority.
"""
import argparse
import itertools
import json
import random
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_vehicle_motion_math_20261011 import capture
from export_mp_physics_contact_lifecycle import segments
from oracle_physics_game_frame_hardware_20261011 import execute as hardware
from original_physics_file_hash import StatCheckedSha256

sha = StatCheckedSha256()
BASE, STACK, STOP = 0x1000000, 0x1010000, 0x100f000
PAIR, A, B, SA, SB, TA, TB, D = (BASE+n for n in range(0x20000, 0x28000, 0x1000))
ENTRY = 0x282fd0
PARENT = ROOT/'analysis/multiplayer/sandbox-source/phantom-world-tu15-20261011/research-verified/report.json'
PARENT_PIN = '23443dc99ed0c1ad6f280307d389c10d698231fe5d4a11e423392ea9ca063b9e'


def controls():
    rows = []
    # Adjacent binary32 values straddle the binary64 retail threshold. Include
    # exceptional direction inputs without altering the original constant.
    values = (0, 0x80000000, 0x3f13b644, 0x3f13b645, 0x3f13b646,
              0xbf13b645, 0x3f800000, 0x7f800000, 0xff800000,
              0x7fc12345, 0x7f812345, 1)
    for types, bits, rounding in itertools.product(((24, 8), (8, 24), (24, 24), (8, 8)), values, range(4)):
        rows.append(dict(types=types, direction_bits=bits, rounding=rounding,
                         precision=3, top=0, scale=1., rotate=False))
    for precision, rounding, top, scale, rotate in itertools.product((0, 2, 3), range(4), (0, 3), (0.25, 3.), (False, True)):
        rows.append(dict(types=(24, 8), direction_bits=0x3f13b645, rounding=rounding,
                         precision=precision, top=top, scale=scale, rotate=rotate))
    return rows


def original(pair, image):
    code = _Code(image, '.text')
    sites = set(pair['instruction_map'].values())
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x7f0000)
    g.mem_map(BASE, 0x30000)
    for section in image.sections:
        g.mem_write(section.va, image.section_bytes(section))
    state = {}
    def on_code(_g, p, _n, _u):
        if p not in sites:
            state['exit_entry'] = p
            g.emu_stop()
        else:
            state['instructions'].append(p)
    def on_read(_g, _a, p, n, _v, _u):
        state['events'].append(dict(kind='R', instruction=g.reg_read(R.UC_X86_REG_EIP), address=p, raw=bytes(g.mem_read(p, n)).hex()))
    def on_write(_g, _a, p, n, value, _u):
        state['events'].append(dict(kind='W', instruction=g.reg_read(R.UC_X86_REG_EIP), address=p,
            before=bytes(g.mem_read(p, n)).hex(), written=(value & ((1 << (n*8))-1)).to_bytes(n, 'little').hex()))
    g.hook_add(UC_HOOK_CODE, on_code)
    g.hook_add(UC_HOOK_MEM_READ, on_read)
    g.hook_add(UC_HOOK_MEM_WRITE, on_write)
    rng, cases = random.Random(0x2830fe), []
    def w(p, value): g.mem_write(p, struct.pack('<I', value))
    def floats(p, values): g.mem_write(p, struct.pack('<'+'f'*len(values), *values))
    for index, row in enumerate(controls()):
        g.mem_write(STACK-0x200, rng.randbytes(0x400))
        g.mem_write(PAIR, bytes(0x8000))
        w(STACK, STOP)
        w(PAIR+8, A); w(PAIR+12, B); w(PAIR+20, D)
        for record, shape, transform, kind in ((A, SA, TA, row['types'][0]), (B, SB, TB, row['types'][1])):
            w(record, shape); w(record+8, transform)
            w(shape, 0x4620d0 if kind == 24 else 0x461fe0)
            floats(shape+0x20, (0., 0., 0., 0.))
            floats(shape+0x30, (row['scale'], 0., 0., 0.))
            floats(shape+0x40, (0., row['scale'], 0., 0.))
            rotation = (0., 0., 1., 0., 0., 1., 0., 0., -1., 0., 0., 0.) if row['rotate'] else (1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0.)
            floats(transform+0x20, rotation)
        floats(D+0x10, (0.25, -0.5, 0., 0.)); w(D+0x18, row['direction_bits'])
        for n, name in enumerate(('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')):
            g.reg_write(getattr(R, 'UC_X86_REG_'+name), STACK if name == 'ESP' else PAIR if name == 'ESI' else rng.getrandbits(32))
        for n in range(8): g.reg_write(getattr(R, f'UC_X86_REG_XMM{n}'), rng.getrandbits(128))
        g.reg_write(R.UC_X86_REG_EFLAGS, 2)
        g.reg_write(R.UC_X86_REG_FPCW, 0x7f | (row['precision'] << 8) | (row['rounding'] << 10))
        g.reg_write(R.UC_X86_REG_MXCSR, 0x1f80 | (row['rounding'] << 13))
        g.reg_write(R.UC_X86_REG_FPSW, row['top'] << 11)
        g.reg_write(R.UC_X86_REG_FPTAG, 0xffff)
        declared = {}
        for p, n in ((STACK-0x200, 0x400), (PAIR, 0x8000)):
            declared.update({p+k: b for k, b in enumerate(bytes(g.mem_read(p, n)))})
        state.update(instructions=[], events=[], exit_entry=None)
        entry_abi = capture(g)
        g.emu_start(ENTRY, 0, count=10000)
        assert state['exit_entry'] == STOP, (index, state['exit_entry'])
        first, current = {}, {}
        for event in state['events']:
            raw = bytes.fromhex(event['raw'] if event['kind'] == 'R' else event['before'])
            for k, value in enumerate(raw):
                p = event['address']+k
                if p not in current: first[p] = current[p] = value
                assert current[p] == value
            if event['kind'] == 'W':
                current.update({event['address']+k: value for k, value in enumerate(bytes.fromhex(event['written']))})
        for p, value in first.items():
            assert p not in declared or declared[p] == value
            declared[p] = value
        banks = [dict(address=p, hex=b.hex()) for p, b in segments(declared)]
        cases.append(dict(id=index, entry=ENTRY, entry_abi=entry_abi, exit=capture(g),
            complete=True, exit_entry=STOP, return_address=STOP, x87_defined_status_mask=0x383f,
            initial_banks=banks, final_banks=[dict(address=b['address'], hex=bytes(g.mem_read(b['address'], len(b['hex'])//2)).hex()) for b in banks],
            instructions=state['instructions'], writes=[e for e in state['events'] if e['kind']=='W'],
            estimates=[], timestamps=[], packets=[], declared_input=row))
    return cases


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN and sha(PARENT) == PARENT_PIN
    image, parent = Xbe(NEW_DEFAULT), json.loads(PARENT.read_text())
    # Identity keys keep all new observations in actual TU1.5 addresses. No
    # older executable is opened or executed by this oracle.
    pair = {name: {f'{q:08x}': q for q in parent[name].values()} for name in ('entries','instruction_map','address_map')}
    code = _Code(image, '.text')
    bindings = []
    for constructor, store, table, getter, kind in ((0x1f5fe0,0x1f6021,0x4620d0,0xab4b0,24),(0x1f2c70,0x1f2edf,0x461fe0,0xad390,8)):
        s = code.at(store)
        assert store in _trace(code, constructor, set(pair['entries'].values()) | {constructor}).code
        assert s.mnemonic == 'mov' and s.operands[1].imm == table
        assert struct.unpack('<I', image.read(table+0x14, 4))[0] == getter
        ins = [code.at(p) for p in sorted(_trace(code, getter, set(pair['entries'].values())).code)]
        assert [(i.mnemonic,i.op_str) for i in ins] == [('mov', f'eax, {hex(kind)}' if kind > 9 else f'eax, {kind}'), ('ret','')]
        bindings.append(dict(constructor=constructor,store=store,table=table,getter=getter,type=kind,store_bytes=s.bytes.hex()))
    assert image.read(0x2830fe, 6).hex() == 'dd0588a64600'
    assert image.read(0x46a688, 8).hex() == '105839b4c876e23f'
    a.output.mkdir(parents=True, exist_ok=False)
    fixture = dict(schema=1, image='1.5', parity_target='English TU1.5 only', cases=original(pair,image),
        tool_sha256=sha(__file__), original_sha256=NEW_PIN, parent_correspondence_sha256=PARENT_PIN,
        shape_bindings=bindings, native_expected_data_used=False, live_physics_proof=False,
        raw_inputs_not_source_constructed=True, original_instruction_patches=[])
    (a.output/'original.json').write_text(json.dumps(fixture, sort_keys=True, separators=(',',':'))+'\n')
    result = hardware(fixture, pair, a.output/'hardware')
    target = a.output/'u5-native.json'
    target.write_text(json.dumps(result, sort_keys=True, separators=(',',':'))+'\n')
    report = dict(fixture_sha256=sha(target), cases=len(result['cases']),
        instructions=result['single_stepped_original_instructions'], writes=sum(len(c['writes']) for c in result['cases']),
        qword_load_cases=sum(0x2830fe in c['instructions'] for c in result['cases']),
        return_values=sorted({c['exit']['gp'][0]&255 for c in result['cases']}),
        original_sha256=NEW_PIN, tool_sha256=sha(__file__), live_physics_proof=False)
    (a.output/'report.json').write_text(json.dumps(report, sort_keys=True, indent=2)+'\n')
    print(json.dumps(report))


if __name__ == '__main__': main()
