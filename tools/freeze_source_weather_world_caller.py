#!/usr/bin/env python3
"""Freeze unchanged36B50 through its first real draw call over saved RAM.

The actual case compares complete producer banks with an independent original
return. Other cases change only explicit view/visibility/stopped inputs. No
original instruction, service, readiness, RNG or draw result is replaced.
RSQRT answers are retained only as fixture inputs, never as a live lookup.
"""
from pathlib import Path
import collections
import hashlib
import json
import re
import struct
import sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from audit_source_weather_particle_simulation_original import load,reg,xmm,GPRS,snapshot_record
from differential_source_object_admission_snapshot import uint,words,packed,PIN
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def sha(raw):return hashlib.sha256(raw).hexdigest()

def main():
    base=ROOT/'analysis/multiplayer-shading/original-weather-20261009/session1'
    entry=load(base/'lockout-weather-full-entry');actual=load(base/'lockout-weather-full-return')
    image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
    regs=entry.report['registers_before'];stack=reg(regs,'ESP');ret=uint(entry.read(stack,4))
    assert reg(regs,'EIP')==0x36b50 and ret==0x2c16e
    bsp=uint(entry.read(0x4e0348,4));palette=uint(entry.read(bsp+0x88,4))
    datum=uint(entry.read(palette+0x24,4));tags=uint(entry.read(0x4e3b44,4))
    weather=uint(entry.read(tags+(datum&0xffff)*16+8,4))
    assert uint(entry.read(weather,4))==uint(entry.read(weather+8,4))==1
    system=uint(entry.read(weather+4,4));plate=uint(entry.read(weather+12,4))
    group=uint(entry.read(system+0x68,4));count,render,_,state=words(entry.read(group,16))
    auxiliary=uint(entry.read(0x4e7408,4));record=uint(entry.read(0x50942c,4))
    view_pool=uint(entry.read(0x4e8c20,4));visible=uint(entry.read(0x547f94,4))
    clusters=words(entry.read(uint(entry.read(visible+12,4)),struct.unpack('<h',entry.read(visible+4,2))[0]*4))
    containing=struct.unpack('<h',entry.read(0x547f90,2))[0]
    states=[0x509420,0x4670fc,0x467100,0x509424,0x509428]
    fixture=bytearray(b'H2WCF001'+packed([7]));records=[]
    for case in range(7):
        u=Uc(UC_ARCH_X86,UC_MODE_32);pages={};seen=set();calls=[];isa=[];pending=[None]
        instructions=[0];draws=[0]
        def read(address,size):
            if not size:return b''
            for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):
                if p not in pages:
                    raw=entry.read(p,4096);u.mem_map(p,4096);u.mem_write(p,raw);pages[p]=raw
            return bytes(u.mem_read(address,size))
        def write(address,raw):read(address,len(raw));u.mem_write(address,raw)
        def missing(g,access,address,size,value,_):read(address,size);return True
        def code(g,pc,size,_):
            if pc==ret:g.emu_stop();return
            if pc==0x37a60:draws[0]+=1;g.emu_stop();return
            assert read(pc,size)==image.read(pc,size),f'changed original instruction {pc:08x}'
            seen.add(pc);instructions[0]+=1
            if pc in (0x398d0,0x4b3d0,0x37260,0x38450,0x12de70):calls.append(f'{pc:08X}')
            if pc==0x38b02:
                index=(g.reg_read(UC_X86_REG_EBP)-state)//32
                assert state+index*32==g.reg_read(UC_X86_REG_EBP)
                pending[0]=[index,uint(read(g.reg_read(UC_X86_REG_ESP)+0x40,4))]
            if pc==0x38b08:
                assert pending[0] is not None
                isa.append(pending[0]+[g.reg_read(UC_X86_REG_XMM0)&0xffffffff]);pending[0]=None
        def writing(g,access,address,size,value,_):
            assert not any(pc<=address<pc+16 for pc in seen),'original code overwritten'
            read(address,size)
        u.hook_add(UC_HOOK_MEM_UNMAPPED,missing);u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,writing)
        ga,gl=(int(v,16) for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',regs).groups())
        read(ga,gl+1);u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
        for name in ('CS','SS','DS','ES','FS','GS'):
            u.reg_write(globals()['UC_X86_REG_'+name],int(re.search(r'\b'+name+r'\s*=([0-9a-fA-F]+)',regs)[1],16))
        for name in GPRS:u.reg_write(globals()['UC_X86_REG_'+name],reg(regs,name))
        u.reg_write(UC_X86_REG_EFLAGS,reg(regs,'EFL'))
        for i in range(8):
            u.reg_write(globals()['UC_X86_REG_XMM'+str(i)],xmm(regs,i))
            f=re.search(r'FPR%d=([0-9a-fA-F]{16})\s+([0-9a-fA-F]{4})'%i,regs)
            u.reg_write(globals()['UC_X86_REG_FP'+str(i)],(int(f[1],16),int(f[2],16)))
        assert reg(regs,'FTW')==0
        u.reg_write(UC_X86_REG_FPCW,reg(regs,'FCW'));u.reg_write(UC_X86_REG_FPSW,reg(regs,'FSW'))
        u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,reg(regs,'MXCSR'))
        # Case0 is the actual call. Cases1..4 independently partition its
        # actual banks four ways; cases5/6 supply empty visibility/stopped state.
        if 1<=case<=4:
            write(0x485600,struct.pack('<h',case-1));write(view_pool+8,struct.pack('<h',4))
        if case==5:write(visible+4,struct.pack('<h',0))
        if case==6:
            write(0x509420,packed([1]));write(0x509424,packed([0]))
            if record:write(record,packed([1,*[uint(read(a,4)) for a in states[1:3]],0,uint(read(states[4],4)),0xdeadbeef]))
        current_clusters=[] if case==5 else clusters
        views=struct.unpack('<h',read(view_pool+8,2))[0]
        bank_regions=[(0x485600,244),(weather,176),(plate,936),(system,140),
                      (render,count*20),(state,count*32)]
        inputs=b''.join(read(a,n) for a,n in bank_regions)
        state_before=b''.join(read(a,4) for a in states)
        clock_before=read(0x50941c,4)+read(0x509418,4)
        record_before=read(record,24) if record else bytes(24)
        time=read(0x4858a0,8);aux_before=read(auxiliary+4,4)
        u.emu_start(0x36b50,ret,count=4_000_000)
        assert u.reg_read(UC_X86_REG_EIP) in (ret,0x37a60) and pending[0] is None
        expected=b''.join(read(a,n) for a,n in bank_regions[1:])
        state_after=b''.join(read(a,4) for a in states)
        clock_after=read(0x50941c,4)+read(0x509418,4)
        record_after=read(record,24) if record else bytes(24)
        independent={}
        if case==0:
            for name,a,n in [('weather',weather,176),('plate',plate,936),('system',system,140),
                             ('render',render,count*20),('state',state,count*32),('auxiliary',auxiliary+4,4),
                             ('clock',0x509418,8)]:
                independent[name]=sum(a!=b for a,b in zip(read(a,n),actual.read(a,n)))
            assert all(n==0 for n in independent.values()),independent
        header=[case,datum,count,views&0xffff,containing&0xffff,int(bool(record)),draws[0],reg(regs,'FCW'),reg(regs,'MXCSR')]
        fixture+=packed(header)+aux_before+state_before+clock_before+record_before+time
        fixture+=packed([len(current_clusters),*current_clusters])+inputs
        fixture+=packed([len(isa),*[word for row in isa for word in row]])
        fixture+=read(auxiliary+4,4)+state_after+clock_after+record_after+expected
        records.append({'case':case,'view_count':views,'slot':struct.unpack('<h',inputs[:2])[0],
            'actual_call':case==0,'original_instructions':instructions[0],'original_addresses':len(seen),
            'source_call_order':calls,'stopped_before_first_draw':bool(draws[0]),'rsqrt_pairs':len(isa),
            'independent_actual_return_differing_bytes':independent,
            'input_banks_sha256':sha(inputs),'expected_banks_sha256':sha(expected)})
    path=ROOT/'tests/fixtures/retail_source_weather_world_caller.bin';path.write_bytes(fixture)
    report={'authority':__doc__,'source_executable_sha256':PIN,
        'entry':snapshot_record(entry),'actual_return':snapshot_record(actual),
        'fixture':str(path.relative_to(ROOT)),'fixture_sha256':sha(fixture),
        'tool_sha256':sha(Path(__file__).read_bytes()),'cases':records,
        'original_instruction_patches':[],'services_replaced':[],
        'native_live_rsqrt_lookup':False,'native_live_activation':False,
        'scope':'Complete original clock/intensity/admission/wind/plate/ready-particle prefix; stop before the actual first draw. No GPU completion, launch RNG or original async IO claim.'}
    output=ROOT/'analysis/map-world-features/weather-world-caller-original.json'
    output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'cases':len(records),'fixture_bytes':len(fixture),
        'instructions':sum(r['original_instructions'] for r in records),
        'actual_return_differences':records[0]['independent_actual_return_differing_bytes']},indent=2))

if __name__=='__main__':main()
