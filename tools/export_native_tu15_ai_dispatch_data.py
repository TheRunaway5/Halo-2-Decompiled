#!/usr/bin/env python3
"""Authenticate the compiled AI frame's bounded immutable switch tables.

Use the pinned compiled-frame snapshot and original TU1.5 image offline. Each
recovered function must contain only compiled original instruction sites, and
each DWORD table target must belong to that same recovered body. Writable
arrays, tags, clock imports and computed state are never exported.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

FRAME_PIN = '0faad001eff14edaaa31df28eddcde059ab855a5a3639fff4b81e3526ee15308'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--frame-source', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--fixture', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    ap.add_argument('--verify', action='store_true')
    args = ap.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.output.resolve().is_relative_to(scratch)
    raw = args.frame_source.read_bytes(); assert hashlib.sha256(raw).hexdigest() == FRAME_PIN
    source = raw.decode()
    entries = {int(p,16) for p in re.findall(r'static void run_([0-9a-f]+)\(WorldState &s\)', source)}
    sites = {int(p,16) for p in re.findall(r'services\.instruction\(0x([0-9a-f]+)u?\)', source)}
    image = Xbe(NEW_DEFAULT); assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text'); tables = {}; excluded_sdk = []
    for entry in sorted(entries):
        if not code.inside(entry): excluded_sdk.append(entry); continue
        body = _trace(code, entry, entries)
        assert set(body.code) <= sites, hex(entry)
        for address, width, count in body.tables:
            section = next(s for s in image.sections if s.va <= address < address+width*count <= s.va+s.vsize)
            assert not section.flags & 1 and width in (1,4) and count > 0
            data = image.read(address, width*count)
            if width == 4:
                assert all(int.from_bytes(data[i:i+4], 'little') in body.code for i in range(0,len(data),4))
            row = tables.setdefault((address,width,count), dict(address=address,width=width,count=count,
                entries=[],data_hex=data.hex()))
            assert row['data_hex'] == data.hex(); row['entries'].append(entry)
    rows = [row for _, row in sorted(tables.items())]
    assert len(rows) == 49 and sum(row['width']*row['count'] for row in rows) == 1836
    reached = next(row for row in rows if row['address'] == 0x1fc0e8)
    assert reached['width'] == 4 and reached['count'] == 6
    assert code.at(0x1fbcc0).op_str == 'eax, word ptr [esi + 0x270]'
    assert code.at(0x1fbcc7).op_str == 'eax, 5'
    assert code.at(0x1fbcca).mnemonic == 'ja'
    guard_cases = []
    for value in (-32768,-1,0,1,2,3,4,5,6,32767):
        g = Uc(UC_ARCH_X86, UC_MODE_32); g.mem_map(0x10000, 0x600000)
        for section in image.sections: g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(0x2000000, 4096); g.mem_write(0x2000270, struct.pack('<h', value))
        g.reg_write(R.UC_X86_REG_ESI, 0x2000000); g.reg_write(R.UC_X86_REG_EFLAGS,0x202)
        trace, reads = [], []
        def instruction(_g, pc, size, _user):
            assert bytes(g.mem_read(pc,size)) == image.read(pc,size)
            if pc not in (0x1fbcc0,0x1fbcc7,0x1fbcca,0x1fbcd0): g.emu_stop(); return
            trace.append(pc)
        def read(_g, _kind, address, size, _value, _user):
            if 0x1fc0e8 <= address < 0x1fc100: reads.append([address,size])
        g.hook_add(UC_HOOK_CODE,instruction); g.hook_add(UC_HOOK_MEM_READ,read)
        g.emu_start(0x1fbcc0,0x3000000,count=10)
        assert len(reads) == (1 if 0 <= value <= 5 else 0)
        guard_cases.append(dict(value=value,trace=trace,table_reads=reads,selected_original_pc=g.reg_read(R.UC_X86_REG_EIP)))
    lines = ['// Authenticated TU1.5 compiled AI-frame dispatch tables; no runtime state.']
    for row in rows:
        data = bytes.fromhex(row['data_hex'])
        lines.append(f'literal(memory,0x{row["address"]:x}u,std::array<std::uint8_t,{len(data)}>{{' +
            ','.join(f'0x{b:02x}' for b in data) + '});')
    output = ('\n'.join(lines)+'\n').encode(); args.output.write_bytes(output)
    fixture = dict(scope=__doc__,original_xbe_sha256=NEW_PIN,pinned_frame_sha256=FRAME_PIN,
        tables=rows,reached_guard_prefix_cases=guard_cases,instruction_patches=[],supplied_dependency_returns=[])
    data = (json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists(): assert args.fixture.read_bytes() == data, 'Preserve original dispatch evidence'
    else: args.fixture.parent.mkdir(parents=True,exist_ok=True); args.fixture.write_bytes(data)
    report = dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,pinned_frame_sha256=FRAME_PIN,
        compiled_functions=len(entries),excluded_sdk_entries=excluded_sdk,tables=len(rows),literal_bytes=1836,
        guard_prefix_cases=len(guard_cases),guard_prefix_only=True,complete_behavior_callback_verified=False,
        fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),
        output_sha256=hashlib.sha256(output).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True); args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__ == '__main__': main()
