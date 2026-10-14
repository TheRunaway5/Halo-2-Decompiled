#!/usr/bin/env python3
"""Untouched TU1.5 17B2EC..17B2FF attachment-name selection.

Declared caller inputs come from all 21 authored map attachment/location rows.
This proves the alias branch, not the complete location resolver or lifecycle.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from audit_rendering import Cache
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EBP, UC_X86_REG_EDI,
                               UC_X86_REG_ESP, UC_X86_REG_EFLAGS,
                               UC_X86_REG_EIP)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), 'Preserve original execution evidence'
    root = Path(__file__).resolve().parents[1]
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    original = Xbe(NEW_DEFAULT)
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x600000)
    for section in original.sections:
        cpu.mem_write(section.va, original.section_bytes(section))
    cpu.mem_map(0x2400000, 0x2000)
    trace = []

    def instruction(uc, address, size, _):
        assert bytes(uc.mem_read(address, size)) == original.read(address, size)
        trace.append(address)

    cpu.hook_add(UC_HOOK_CODE, instruction)
    inventory = json.loads((root / 'analysis/map-world-features/inventory.json').read_text())
    rows, maps = [], {}
    for item in inventory['maps']:
        path = root / item['cache']
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        assert digest == item['cache_sha256']
        maps[item['map']] = digest
        cache = Cache(path)

        def name(sid):
            if sid == 0xffffffff:
                return '<NONE>'
            offset = cache.u32(368) + cache.u32(cache.u32(364) + (sid & 65535) * 4)
            return cache.string(offset, 256)

        assert name(0x0700054e) == 'replace'
        seen = set()
        for placement in item['placements']:
            for attachment in placement['attachments']:
                tag = attachment['definition']
                if not tag or tag['group'] != 'effe':
                    continue
                _, at = cache.tag(int(tag['datum'], 16), 'effe')
                locations = [cache.u32(p) for p in cache.block(at + 0xc, 4)]
                # The reserved-name cases are explicit caller inputs; the
                # remaining names are actual authored location rows.
                for sid in [*locations, 0x0700054e]:
                    for supplied in (attachment['marker'], 0, sid):
                        if (sid, supplied) in seen:
                            continue
                        seen.add((sid, supplied))
                        cpu.mem_write(0x240000c, struct.pack('<I', supplied))
                        cpu.mem_write(0x2401000, bytes([0xcd]) * 64)
                        cpu.reg_write(UC_X86_REG_EBP, 0x2400000)
                        cpu.reg_write(UC_X86_REG_ESP, 0x2401000)
                        cpu.reg_write(UC_X86_REG_EDI, sid)
                        cpu.reg_write(UC_X86_REG_EFLAGS, 2)
                        trace.clear()
                        cpu.emu_start(0x17b2ec, 0x17b2ff, count=20)
                        assert cpu.reg_read(UC_X86_REG_EIP) == 0x17b2ff
                        result = cpu.reg_read(UC_X86_REG_EDI)
                        # Compare the native string-key representation with
                        # the name selected by the actual original prefix.
                        native_name = name(supplied) if name(sid) == name(0x0700054e) else name(sid)
                        assert native_name == name(result)
                        assert bytes(cpu.mem_read(0x2401010, 4)) == bytes(4)
                        rows.append(dict(map=item['map'], input_sid=sid,
                                         supplied_sid=supplied, original_sid=result,
                                         input_name=name(sid), result_name=name(result),
                                         instructions=trace.copy()))
        cache.close()
    assert len(maps) == 21 and any(row['input_name'] == 'marker' for row in rows)
    assert any(row['input_sid'] == 0x0700054e for row in rows)
    args.output.mkdir(parents=True)
    (args.output / 'cases.json').write_text(json.dumps(rows, indent=2) + '\n')
    report = dict(passed=True, original_sha256=NEW_PIN, maps=maps,
                  cases=len(rows), authored_marker_cases=sum(r['input_name'] == 'marker' for r in rows),
                  alias_cases=sum(r['input_sid'] == 0x0700054e for r in rows),
                  entry=0x17b2ec, stop=0x17b2ff,
                  original_bytes=original.read(0x17b2ec, 0x13).hex(),
                  instruction_patches=[], replaced_services=[],
                  boundary=__doc__, full_goal_complete=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS original attachment alias:', len(rows), 'cases across 21 maps')


if __name__ == '__main__':
    main()
