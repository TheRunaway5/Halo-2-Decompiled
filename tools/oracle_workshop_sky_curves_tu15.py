#!/usr/bin/env python3
"""Execute unchanged TU1.5 scalar curves over actual authored sky functions.

The image, complete instruction reads and encoded lookup tables are pinned.
Only declared curve/argument storage is supplied; no code, call or result is
substituted. Run under run_test.py with raw outputs inside its scratch owner.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_INVALID
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from export_mcc_native_sky_fixture import SkyExporter
from functions import _Code, _trace
from research_mp_inventory_switch_u5 import prefix, raw_prefix_candidates, norm


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--prepared-root', type=Path, required=True)
    p.add_argument('--item', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--fixture', type=Path, required=True)
    a=p.parse_args();root=Path(__file__).resolve().parents[1]
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert a.output.resolve().is_relative_to(scratch) and a.fixture.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    exporter=SkyExporter(root,a.prepared_root,dict(old=a.prepared_root/'halo2/h2_maps_win64_dx11/03a_oldmombasa.map',
        alpha=a.item/'maps/alphamoon.map'))
    records=[]
    try:
        for name, shader in [('old','scenarios\\skies\\solo\\earthcity\\shaders\\clouds_low2'),
                             ('alpha','digsite\\scenarios\\skies\\solo\\alphamoon\\shaders\\moon_clouds')]:
            row=exporter.row(name,shader,False);case=exporter.fixture(row,len(records),'pc-geometry-owner')
            records.append(dict(map=name,shader=shader,overlays=case['expected_overlays']))
    finally:
        exporter.close()
    curves=sorted({o['curve_hex'] for row in records for o in row['overlays']})
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(0x2100000,0x100000);g.mem_map(0x3000000,4096)
    # These samples are literal encoded TU1.5 .data, not initialized outputs.
    tables=(a.prepared_root/'native-shaders/function-tables.bin').read_bytes()
    assert image.read(0x4752a8,20*1024)==tables[:20*1024]
    # Establish the complete TU1.5 correspondence; addresses from another
    # update are not accepted as an ABI or algorithm identity.
    original=Xbe(root/'assets/disc/default.xbe')
    assert hashlib.sha256(original.data).hexdigest()=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    reference_code=_Code(original,'.text');body=_trace(reference_code,0x13b390,{0x13b390})
    instructions=[reference_code.at(at) for at in sorted(body.code)]
    positions={i.address:n for n,i in enumerate(instructions)}
    expected=[norm(i,original,positions) for i in instructions]
    code=_Code(image,'.text')
    starts={int(row['address'],16) for row in csv.DictReader((root/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
    candidates=[e for e in starts if prefix(code,image,e)==prefix(reference_code,original,0x13b390)]
    candidates=sorted(set(candidates+raw_prefix_candidates(reference_code,original,code,0x13b390)))
    matches=[]
    for candidate in candidates:
        actual=_trace(code,candidate,starts);sequence=[code.at(at) for at in sorted(actual.code)]
        positions={i.address:n for n,i in enumerate(sequence)}
        if [norm(i,image,positions) for i in sequence]==expected:matches.append(candidate)
    assert len(matches)==1, matches
    entry,stack,descriptor,curve,stop=matches[0],0x21fff00,0x2100000,0x2101000,0x3000000
    executed=set();calls=set()
    def instruction(vm,pc,size,_):
        assert bytes(vm.mem_read(pc,size))==image.read(pc,size)
        executed.add(pc)
        if image.read(pc,1)==b'\xe8':calls.add(pc)
    g.hook_add(UC_HOOK_CODE,instruction)
    def invalid(vm, access, address, size, value, _):
        raise RuntimeError(f'Original sky curve memory fault pc={vm.reg_read(R.UC_X86_REG_EIP):x} address={address:x} bytes={size} descriptor={descriptor:x}')
    g.hook_add(UC_HOOK_MEM_INVALID,invalid)
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    samples=[-10000.,-31.,-1.,-.01,0.,.0001,.01,.125,.25,.5,.75,.9999,1.,1.125,2.,16.,64.,10000.]
    rows=[]
    for raw in curves:
        data=bytes.fromhex(raw);g.mem_write(curve,data);g.mem_write(descriptor,struct.pack('<II',len(data),curve))
        for value in samples:
            for range_ in (0.,.5,1.):
                g.mem_write(stack,struct.pack('<III',stop,bits(value),bits(range_)))
                for reg in (R.UC_X86_REG_EAX,R.UC_X86_REG_EBX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI):g.reg_write(reg,0)
                g.reg_write(R.UC_X86_REG_EDI,descriptor);g.reg_write(R.UC_X86_REG_ESP,stack)
                g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
                g.emu_start(entry,stop,count=100000)
                assert g.reg_read(R.UC_X86_REG_EIP)==stop and g.reg_read(R.UC_X86_REG_ESP)==stack+12
                result=g.reg_read(R.UC_X86_REG_XMM0)&0xffffffff
                rows.append(f'{raw} {bits(value):08x} {bits(range_):08x} {result:08x}')
    raw=(f'# Unchanged English TU1.5 {entry:08x}; literal authored sky curves; no patches or result substitutions.\n'+'\n'.join(rows)+'\n').encode()
    a.fixture.write_bytes(raw)
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,entry=entry,cases=len(rows),curves=len(curves),records=records,
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),
        encoded_lookup_rows_exact=True,instruction_patches=[],intercepted_calls=[],
        complete_original_body_correspondence=True,reference_entry=0x13b390,normalized_instructions=len(expected),
        executed_instructions=len(executed),executed_direct_call_sites=sorted(calls),
        generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(rows),curves=len(curves),original_instructions=len(executed))),flush=True)


if __name__=='__main__':
    main()
