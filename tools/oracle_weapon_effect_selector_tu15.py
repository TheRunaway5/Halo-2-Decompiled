#!/usr/bin/env python3
"""Complete unchanged106F90 selector ABI and reads on a physical i386 child.

Reuse every earlier selector input, add unmapped reads and live MMX controls,
and capture defined flags, every PC, reads, stack stores and full FP state.
There are no supplied callees or modified instructions. This is source-owner
evidence, not framebuffer or waveform certification. Use tools/run_test.py.
"""
import argparse
import ctypes
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import signal
import struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_dispatch_tu15 import STACK,STOP,NONE,pack
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,CSRS,EXTENDED
from weapon_effect_original_cpu_state import EffectCpuState
from weapon_reference_versions import images

BAD,END=0x4000000,0x2100000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);prior=Path(__file__).resolve().parents[1]/'tests/fixtures/weapon_effect_dispatch_tu15_original.json'
    seeds=[r for r in json.loads(prior.read_text())['rows'] if r['entry']==0x106f90];assert len(seeds)==256
    recipes=[(n,'base',copy.deepcopy(r)) for n,r in enumerate(seeds)]
    recipes += [(256+n,kind,copy.deepcopy(seeds[0])) for n,kind in enumerate(['fault-pool','fault-rows','fault-object','fault-tags','fault-definition','fault-object-flags','fault-parent','fault-negative-header'])]
    recipes += [(264+n,'active-mmx',copy.deepcopy(seeds[n])) for n in range(4)]
    _,image,pin=next(r for r in images() if r[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    rng=random.Random(0x106f90);rows=[];instructions={};write_sites=set();steps=0
    with OriginalAudioCpu(image,scratch/'effect-selector-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0);machine=EffectCpuState(host)
        for n,kind,seed in recipes:
            gp=seed['gp'];memory=seed['memory'];pool,headers,body,definition,tags=[0x1000000+i*0x2000 for i in range(5)]
            def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
            if kind=='fault-pool':word(0x4ecbd0,BAD)
            elif kind=='fault-rows':word(pool+0x44,BAD)
            elif kind=='fault-object':word(headers+3*12+8,BAD)
            elif kind=='fault-tags':word(0x4f0414,BAD)
            elif kind=='fault-definition':word(tags+7*16+8,BAD)
            elif kind=='fault-object-flags':word(headers+3*12+8,END-4);word(END-4,7)
            elif kind=='fault-parent':word(headers+3*12+8,END-0x14);word(END-0x14,7);word(END-0x10,1,1)
            elif kind=='fault-negative-header':word(pool+0x44,0x1000000-48)
            initial_stack=rng.randbytes(160);host.write(STACK-96,initial_stack);host.write(STACK,pack([STOP,*seed['arguments']]));initial_stack=host.read(STACK-96,160).hex()
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=CSRS[n%len(CSRS)],stack=[EXTENDED[2],EXTENDED[3]][:n%3],xmm=seed['xmm'])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(0x106f90,STACK,dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];reads=[];stores=[];fault=[];defined=NONE&~DEBUG_FLAGS
            for _ in range(100):
                pc=regs.rip
                if pc==STOP:break
                assert 0x106f90<=pc<=0x106fe4,hex(pc)
                ins=next(decoder.disasm(image.read(pc,15),pc,count=1));raw=bytes(ins.bytes);assert host.read(pc,len(raw))==raw
                instructions[f'{pc:08x}']=raw.hex();trace.append(pc);pending=[];operands=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                if ins.mnemonic=='push':pending.append(((regs.rsp-4)&NONE,4));write_sites.add(pc)
                if ins.mnemonic=='pop' or ins.mnemonic.startswith('ret'):reads.append([pc,regs.rsp&NONE,4]);operands.append((regs.rsp&NONE,4))
                for op in ins.operands:
                    if op.type!=3 or ins.mnemonic=='lea':continue
                    at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&NONE
                    if op.access&1:reads.append([pc,at,op.size]);operands.append((at,op.size))
                    if op.access&2:pending.append((at,op.size));write_sites.add(pc)
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
                    assert kind.startswith('fault-') and operands,(kind,hex(pc),status)
                    at,width=operands[-1];fault=[dict(instruction=pc,address=at,size=width,access='read')];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(hex(pc),status)
                for at,width in pending:stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                if ins.mnemonic=='cmp':defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic in ('test','and'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic=='shl':defined=(NONE&~DEBUG_FLAGS)&~0x810
            else:raise ValueError('Original selector instruction limit')
            assert bool(fault)==kind.startswith('fault-'),(kind,hex(regs.rip))
            expected=machine.snapshot(regs)
            if not fault:
                assert regs.rip==STOP and regs.rsp==STACK+4 and expected['gp']==seed['expected_gp']
            rows.append(dict(entry=0x106f90,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,initial_stack=initial_stack,reads=reads,stores=stores,fault=fault,trace_offsets=b''.join(struct.pack('<H',pc-0x106f90) for pc in trace).hex(),expected=expected,flags_mask=defined))
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_SELECTOR_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=sorted(write_sites),rows=rows),separators=(',',':'))+'\n').encode();assert len(data)<=1024*1024,len(data);args.fixture.write_bytes(data)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),state_helper_sha256=hashlib.sha256(Path(__file__).with_name('weapon_effect_original_cpu_state.py').read_bytes()).hexdigest(),legacy_input_sha256=hashlib.sha256(prior.read_bytes()).hexdigest(),legacy_selector_inputs_replayed=256,
        read_stops=sum(bool(r['fault']) for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),instruction_map=instructions,fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],supplied_callees=[],physical_xbox_proof=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
