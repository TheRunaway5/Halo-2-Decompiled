#!/usr/bin/env python3
"""Inspect unchanged D1E10/241C0 over retained diagnostic object contacts.

All current PRT, resident geometry and tag banks remain literal snapshot bytes.
The contact input comes from the separately recorded unchanged 14A8E0 audit;
it is a diagnostic invocation, not a captured game query or native producer.
"""
from pathlib import Path
import collections, hashlib, json, struct, sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED
from unicorn.x86_const import *
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words, packed, fw
sys.path.insert(0, str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
SYNTH, CONTACT, OUT, STACK, STOP = 0x20000000, 0x20000100, 0x20000200, 0x200fe000, 0x200ff000

def main():
    snapshot = Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    image = Xbe(ROOT/'assets/disc/default.xbe')
    assert hashlib.sha256(image.data).hexdigest() == PIN
    u = Uc(UC_ARCH_X86, UC_MODE_32); u.mem_map(SYNTH, 0x100000)
    pages = {}; verified = set(); calls = collections.Counter(); observations = []; encoded_points = []; mix = {}
    def page(address):
        base = address & ~4095
        if base not in pages:
            raw = snapshot.read(base, 4096); u.mem_map(base, 4096); u.mem_write(base, raw); pages[base] = raw
    def read(address, size):
        for base in range(address & ~4095, ((address+size-1) & ~4095)+4096, 4096):
            if not SYNTH <= base < SYNTH+0x100000: page(base)
        return bytes(u.mem_read(address, size))
    def unmapped(g, access, address, size, value, data):
        for base in range(address & ~4095, ((address+size-1) & ~4095)+4096, 4096): page(base)
        return True
    def code(g, address, size, data):
        if address == STOP: g.emu_stop(); return
        if address in [0xd1e10, 0x24490, 0x241c0, 0x12de70, 0xd33a0, 0xd31c0, 0xd30d0]: calls[f'{address:08X}'] += 1
        if address == 0xd2278:
            encoded_points[:] = [list(struct.unpack('<5h', read(g.reg_read(reg), 10))) for reg in [UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_ECX]]
        if address == 0xd247d:
            esp=g.reg_read(UC_X86_REG_ESP)
            mix.update({'prt_valid':bool(read(esp+0x13,1)[0]),'baked_valid':bool(g.reg_read(UC_X86_REG_EAX)&255),
                'prt_incident':words(read(esp+0x58,12)),'prt_color':words(read(esp+0x28,12)),
                'baked_incident':words(read(esp+0x34,12)),'baked_color':words(read(esp+0x14,12)),
                'default_incident':words(read(uint(read(0x4687b0,4)),12))})
        if address == 0xd292e:mix['output_words']=words(read(OUT,12))+[uint(read(OUT+16,4))]
        if address == 0x241c0:
            args = words(read(g.reg_read(UC_X86_REG_ESP)+4, 28))
            observations.append({'owner': g.reg_read(UC_X86_REG_EDX), 'palette': g.reg_read(UC_X86_REG_ECX), 'section': args[0],
                'points': [words(read(p, 20)) for p in args[1:4]], 'fractions': args[4:6], 'encoded_short_points': list(encoded_points)})
        if address in verified or SYNTH <= address < SYNTH+0x100000: return
        for section in image.sections:
            if section.va <= address and address+size <= section.va+section.rsize:
                assert read(address, size) == image.section_bytes(section)[address-section.va:address-section.va+size], f'changed source instruction {address:08X}'
                verified.add(address); return
        raise RuntimeError(f'uncaptured source instruction service {address:08X}')
    u.hook_add(UC_HOOK_MEM_UNMAPPED, unmapped); u.hook_add(UC_HOOK_CODE, code)
    inputs = json.loads((ROOT/'analysis/multiplayer-shading/dynamic-object-attribution-snapshot-original.json').read_text())
    records = []
    for entry in inputs['records']:
        if not entry.get('attributed'): continue
        for address, raw in pages.items(): u.mem_write(address, raw)
        observations.clear(); encoded_points.clear(); mix.clear()
        u.mem_write(CONTACT, packed(entry['output_words'])); u.mem_write(OUT, packed([0xcdcdcdcd]*5))
        u.mem_write(STACK, packed([STOP, CONTACT, OUT, fw(1.), 0]))
        for reg in [UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]: u.reg_write(reg, 0)
        u.reg_write(UC_X86_REG_ESP, STACK); u.reg_write(UC_X86_REG_EFLAGS, 2)
        u.reg_write(UC_X86_REG_FPCW, 0x023f); u.reg_write(UC_X86_REG_FPSW, 0); u.reg_write(UC_X86_REG_FPTAG, 0xffff); u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        record = {'contact_index': entry['index'], 'owner': entry['owner']}
        try:
            u.emu_start(0xd1e10, STOP+1, count=3000000)
            assert u.reg_read(UC_X86_REG_EIP) == STOP and u.reg_read(UC_X86_REG_ESP) == STACK+20
            record['status'] = u.reg_read(UC_X86_REG_EAX); record['output_words'] = words(read(OUT, 20))
        except Exception as error:
            record['unresolved'] = {'eip': f'{u.reg_read(UC_X86_REG_EIP):08X}', 'error': str(error)}
        count = struct.unpack('<i', snapshot.read(0x5093e8, 4))[0]
        assert read(0x4b89b0, count*104) == snapshot.read(0x4b89b0, count*104)
        assert read(0x487b1c, 0x27000) == snapshot.read(0x487b1c, 0x27000)
        record['prt_calls'] = list(observations); record['light_mix']=dict(mix); records.append(record)
    count = struct.unpack('<i', snapshot.read(0x5093e8, 4))[0]
    report = {'source_xbe_sha256': PIN, 'source_unchanged': True, 'snapshot_ram_sha256': hashlib.sha256(snapshot.ram).hexdigest(),
        'snapshot_tick': snapshot.report['clock_tick'], 'prt_cache_count': count, 'prt_cache_rows': words(snapshot.read(0x4b89b0, count*104)),
        'calls': dict(calls), 'instructions_verified': len(verified), 'records': records,
        'boundary': 'Offline explicit diagnostic contacts. No guest actions, patched code, replaced services or live native PRT cache producer.'}
    output = ROOT/'analysis/multiplayer-shading/object-prt-snapshot-original.json'
    output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({'cases': len(records), 'calls': dict(calls), 'unresolved': [r['unresolved'] for r in records if 'unresolved' in r]}, indent=2))
if __name__ == '__main__': main()
