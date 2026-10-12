#!/usr/bin/env python3
"""Complete unchanged38450 with explicit controls over owned actual ready banks.

The first row is the independently captured whole original call. Other rows are
declared small controlled input copies, never claimed as original game history.
All resource, exclusion, conversion and RSQRT instructions execute unchanged;
no readiness, arithmetic, kernel or call result is supplied. Native comparison
receives literal ISA operand/results as an explicit external producer bank.
"""
from pathlib import Path
import argparse
import collections
import hashlib
import json
import re
import struct
import subprocess
import sys
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from audit_source_weather_particle_simulation_original import load,reg,xmm,SESSION,GPRS,sha
from differential_source_object_admission_snapshot import uint,words,PIN
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def fw(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def packed(v):return struct.pack('<'+'I'*len(v),*(a&0xffffffff for a in v))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/retail_source_weather_particle_simulation_controls.tsv')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/weather-particle-simulation-original/controls.json')
    args=parser.parse_args()
    source=load(SESSION/'lockout-weather-particle-simulation-entry')
    image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
    registers=source.report['registers_before'];sp=reg(registers,'ESP')
    stop,owner=words(source.read(sp,8));system=reg(registers,'EBX')
    group=uint(source.read(system+0x68,4));render=uint(source.read(group+4,4));state=uint(source.read(group+12,4))
    rng=uint(source.read(0x4e7408,4));view_pool=uint(source.read(0x4e8c20,4))
    bsp=uint(source.read(0x4e0348,4));volumes,volume_pointer=words(source.read(bsp+0x8c,8))
    volume_words=[]
    for i in range(volumes):
        raw=source.read(volume_pointer+i*24,24);n,p=words(raw[16:24])
        volume_words+=words(raw[:20])+words(source.read(p,n*16))
    cases=[dict(label='controlled_same_inputs_small_owned_group',changes={})]
    for v in [0.,.001,.2,1.]:cases.append(dict(label=f'elapsed_{v}',changes={0x509418:fw(v)}))
    for v in [-1.,0.,.03125,.25,.5,1.,2.]:cases.append(dict(label=f'intensity_{v}',changes={0x4670fc:fw(v)}))
    for count,slot in [(0,0),(2,0),(2,1),(3,2),(4,0),(4,3),(7,3)]:
        cases.append(dict(label=f'view_partition_{count}_{slot}',changes={view_pool+8:count,0x485600:slot},shorts={view_pool+8,0x485600},count=35))
    for v in [0.,.125,1.,2.,8.]:cases.append(dict(label=f'portal_distance_{v}',changes={},portal=fw(v)))
    for v in [0.,.0001,1.,8.]:cases.append(dict(label=f'near_exclusion_{v}',changes={system+0x14:fw(v)}))
    for v in [0.,.01,-1.,1000.]:cases.append(dict(label=f'velocity_limit_{v}',changes={system+0x18:fw(v)}))
    for v in [0.,.0001,-1.]:cases.append(dict(label=f'box_dimension_{v}',changes={system+8:fw(v)}))
    cases += [dict(label='mass_recovery_first_row',changes={state+24:0}),
        dict(label='negative_mass_first_row',changes={state+24:fw(-1)}),
        dict(label='respawn_first_row',changes={state+28:fw(-1)}),
        dict(label='respawn_first_row_no_intensity',changes={state+28:fw(-1),0x4670fc:0}),
        dict(label='rain_type_skips_point_opacity',changes={system+0x6c:2},shorts={system+0x6c}),
        dict(label='zero_rows',changes={},count=0),
        dict(label='sparse_partition_zero_quotient',changes={view_pool+8:4,0x485600:3},shorts={view_pool+8,0x485600},count=3),
        dict(label='zero_portal_limit',changes={owner+0xac:0},portal=fw(1)),
        dict(label='mirrored_source_transform_scale',changes={0x48568c:fw(-1),0x4856c0:fw(-1)}),
        dict(label='scaled_source_transform',changes={0x48568c:fw(2),0x4856c0:fw(.5)})]
    assert len(cases)==45
    output_lines=(ROOT/'tests/fixtures/retail_source_weather_particle_simulation.tsv').read_text().splitlines()
    output_lines.insert(0,'# Firstrow actual independent38450 call;45 explicit32-row source input controls follow.')
    records=[]
    for case in cases:
        u=Uc(UC_ARCH_X86,UC_MODE_32);pages={};seen={};boundaries=[];counts=collections.Counter();isa=[];pending=[None]
        def page(address):
            at=address&~4095
            if at not in pages:
                raw=source.read(at,4096);u.mem_map(at,4096);u.mem_write(at,raw);pages[at]=raw
        def read(address,n):
            for at in range(address&~4095,((address+n-1)&~4095)+4096,4096):page(at)
            return bytes(u.mem_read(address,n))
        def missing(g,access,address,n,value,_):
            try:read(address,n);return True
            except ValueError as e:boundaries.append(dict(pc=f'{g.reg_read(UC_X86_REG_EIP):08X}',reason=str(e)));g.emu_stop();return False
        def code(g,pc,n,_):
            if pc==stop:g.emu_stop();return
            try:raw=image.read(pc,n)
            except (ValueError,KeyError):boundaries.append(dict(pc=f'{pc:08X}',reason='External/kernel entry, not replaced'));g.emu_stop();return
            assert read(pc,n)==raw;seen[pc]=n;counts['instructions']+=1
            if pc==0x38b02:
                esp=g.reg_read(UC_X86_REG_ESP);index=(g.reg_read(UC_X86_REG_EBP)-state)//32
                assert state+index*32==g.reg_read(UC_X86_REG_EBP)
                pending[0]=[index,uint(read(esp+0x40,4))]
            if pc==0x38b08 and pending[0] is not None:
                isa.append([*pending[0],g.reg_read(UC_X86_REG_XMM0)&0xffffffff]);pending[0]=None
        def writing(g,access,address,n,value,_):
            assert not any(pc<address+n and address<pc+size for pc,size in seen.items())
            read(address,n)
            if address==rng+4:assert n==4;counts['draws']+=1
        u.hook_add(UC_HOOK_MEM_UNMAPPED,missing);u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,writing)
        ga,gl=(int(v,16)for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',registers).groups());read(ga,gl+1)
        u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
        for name in ['CS','SS','DS','ES','FS','GS']:
            u.reg_write(globals()['UC_X86_REG_'+name],int(re.search(r'\b'+name+r'\s*=([0-9a-fA-F]+)',registers)[1],16))
        for name in GPRS:u.reg_write(globals()['UC_X86_REG_'+name],reg(registers,name))
        for i in range(8):u.reg_write(globals()['UC_X86_REG_XMM'+str(i)],xmm(registers,i))
        u.reg_write(UC_X86_REG_EFLAGS,reg(registers,'EFL'));u.reg_write(UC_X86_REG_FPCW,reg(registers,'FCW'))
        u.reg_write(UC_X86_REG_FPSW,reg(registers,'FSW'));u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,reg(registers,'MXCSR'))
        count=case.get('count',32)
        read(group,4);u.mem_write(group,packed([count]))
        # Include retained near-origin/respawn inputs while preserving rawstate
        # and renderrow pairing; this is an explicit control, not an owner pose.
        for i,old in [(0,389),(1,117),(2,480)]:
            read(render+i*20,20);read(state+i*32,32)
            u.mem_write(render+i*20,source.read(render+old*20,20));u.mem_write(state+i*32,source.read(state+old*32,32))
        for at,value in case['changes'].items():
            n=2 if at in case.get('shorts',set()) else 4
            read(at,n);u.mem_write(at,int(value).to_bytes(n,'little'))
        portal=case.get('portal',xmm(registers,0)&0xffffffff)
        u.reg_write(UC_X86_REG_XMM0,(xmm(registers,0)&~0xffffffff)|portal)
        values=[uint(read(rng+4,4)),uint(read(0x509418,4)),uint(read(0x4670fc,4)),portal,
            struct.unpack('<H',read(view_pool+8,2))[0],reg(registers,'FCW'),reg(registers,'MXCSR'),count]
        values+=words(read(system,140)+read(0x485600,244)+read(owner,176)+read(render,count*20)+read(state,count*32))
        values+=[volumes,*volume_words]
        try:u.emu_start(0x38450,stop,count=4_000_000)
        except UcError:
            if not boundaries:raise
        assert not boundaries and u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==sp+8
        values += [len(isa),*(v for row in isa for v in row)]
        expected=words(read(system,140)+read(render,count*20)+read(state,count*32))
        values += [uint(read(rng+4,4)),u.reg_read(UC_X86_REG_EAX)&255,counts['draws'],len(isa),*expected]
        output_lines.append(' '.join(f'{v:08X}'for v in values))
        records.append(dict(label=case['label'],count=count,changes={f'{a:08X}':f'{v:08X}'for a,v in case['changes'].items()},
            portal=f'{portal:08X}',source_instructions=counts['instructions'],distinct_instructions=len(seen),
            source_auxiliary_draws=counts['draws'],source_RSQRT_pairs=len(isa),expected_words=4+len(expected)))
    args.fixture.write_text('\n'.join(output_lines)+'\n')
    report=dict(authority=__doc__,actual_complete_original_calls=1,controlled_complete_original_calls=len(cases),
        source_executable_sha256=PIN,actual_entry_RAM_sha256=sha(source.ram),
        source_instruction_patches=[],service_replacements=[],source_ready_result_supplied=False,
        original_Xbox_RSQRT_precision_claimed=False, GPU_jobs=0,guest_IO=False,live_activation=False,
        controls=records,expected_native_words=13351+sum(r['expected_words']for r in records),
        source_auxiliary_draws=3168+sum(r['source_auxiliary_draws']for r in records),
        explicit_RSQRT_pairs=1009+sum(r['source_RSQRT_pairs']for r in records),
        fixture_sha256=sha(args.fixture.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),
        prior_actual_fixture_sha256=sha((ROOT/'tests/fixtures/retail_source_weather_particle_simulation.tsv').read_bytes()))
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases)+1,words=report['expected_native_words'],rsqrt=report['explicit_RSQRT_pairs'],report_sha256=sha(args.output.read_bytes()))))


if __name__=='__main__':main()
