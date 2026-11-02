#!/usr/bin/env python3
"""Original17AFB0 with actual17B960 and17B130/17DC60 children on i386.

Only16E6A0 allocation,17AEF0 pressure,17AAB0 RNG and259B0 bounded-real
results are declared arbitrary ABI/storage controls. The initialization and
event-reset children execute unchanged original instructions. No instruction
is patched. This is composed owner evidence, not actual allocator/audio/GPU
qualification. Run through tools/run_test.py and retain only bounded data.
"""
import argparse
import ctypes
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
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,CSRS,EXTENDED,FLOATS
from weapon_effect_original_cpu_state import EffectCpuState
from weapon_reference_versions import images

POOL,BODY,TAGS,DEFINITION,EVENTS,INPUT=0x1000000,0x1004000,0x1008000,0x100c000,0x1010000,0x1014000
END,BAD=0x2100000,0x4000000
SERVICES={0x16e6a0:0,0x17aef0:0,0x17aab0:0,0x259b0:8}
NATURAL={0x17b960,0x17b130}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _,image,pin=next(x for x in images() if x[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    rng=random.Random(0x17afb0);rows=[];instructions={};steps=0
    recipes=[(n,'base') for n in [*range(32),*range(48,64)]]
    recipes += [(64+n,'alias-vector') for n in range(4)]
    recipes += [(68+n,kind) for n,kind in enumerate(['fault-tags','fault-definition','fault-count','fault-pool','fault-rows','fault-vector','fault-vector-second','fault-child-0','fault-child-1','fault-child-8','fault-child-15','fault-child-second'])]
    recipes += [(80+n,'active-mmx') for n in range(4)]
    with OriginalAudioCpu(image,scratch/'effect-allocation-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0);machine=EffectCpuState(host)
        for n,kind in recipes:
            gp=[rng.getrandbits(32) for _ in ALIASES];gp[4]=STACK;gp[0]=0x80010007
            salt=[0x1234,0x8001,0xffff,0x7fff][n%4];datum=(salt<<16)|2
            flags=4 if n&4 else 0;count=2;force=[0,1,0x80,0xff][n%4];position=INPUT if n&1 else 0;body=BODY
            if kind=='base' and n<8:
                if n==0:gp[0]=NONE
                elif n==1:count=0
                elif n==2:count=0xffffffff
                elif n==3:count=0x80000000
                elif n==4:force=0;flags=4
            if kind=='alias-vector':force=1;flags=0;position=BODY+[0x48,0x4c,0x50,0x54][n-64]
            if kind.startswith('fault-'):force=1;flags=4;position=0
            if kind=='fault-definition':force=0
            if kind=='fault-vector':position=BAD
            if kind=='fault-vector-second':position=END-4
            if kind.startswith('fault-child-'):
                ending=kind.rsplit('-',1)[1];body=END-0x114 if ending=='second' else END-0x110-int(ending)*8
            memory=[]
            def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
            word(0x4f720c,BAD if kind=='fault-pool' else POOL);word(POOL+0x38,8);word(POOL+0x24,0x190)
            word(POOL+0x44,(BAD if kind=='fault-rows' else body)-2*0x190)
            word(0x4f0414,BAD if kind=='fault-tags' else TAGS);word(TAGS+7*16+8,BAD if kind in ('fault-definition','fault-count') else DEFINITION)
            word(DEFINITION,flags);word(DEFINITION+0x14,count);word(DEFINITION+0x18,EVENTS)
            low=FLOATS[n%len(FLOATS)];high=low if n%4==0 or kind=='active-mmx' else FLOATS[(n+3)%len(FLOATS)]
            word(EVENTS+8,low);word(EVENTS+12,high)
            for off in range(0,12,4):word(INPUT+off,rng.getrandbits(32))
            if kind=='fault-vector-second':word(END-4,rng.getrandbits(32))
            raw_body=rng.randbytes(min(0x190,END-body));blobs=[[body,raw_body.hex()]]
            initial_stack=rng.randbytes(160);host.write(STACK-96,initial_stack);host.write(STACK,pack([STOP,position,force]));initial_stack=host.read(STACK-96,160).hex()
            for at,raw in blobs:host.write(at,bytes.fromhex(raw))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=CSRS[n%len(CSRS)],stack=[EXTENDED[2],EXTENDED[3]][:n%3],xmm=[rng.randbytes(16).hex() for _ in range(8)])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(0x17afb0,STACK,dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            calls=[];pending_natural=[];stores=[];trace=[];fault=[];defined=NONE&~DEBUG_FLAGS;allocations=0
            for _ in range(2000):
                pc=regs.rip
                if pending_natural and pc==pending_natural[-1]['returned_pc']:
                    index=pending_natural.pop()['index'];calls[index].update(returned=machine.snapshot(regs),returned_flags_mask=defined)
                if pc==STOP:break
                if pc in NATURAL:
                    before=machine.snapshot(regs);ret=int.from_bytes(host.read(regs.rsp,4),'little');index=len(calls)
                    calls.append(dict(target=pc,natural=True,before=before,flags_mask=defined,returned=None))
                    pending_natural.append(dict(index=index,returned_pc=ret))
                if pc in SERVICES:
                    before=machine.snapshot(regs);ret=int.from_bytes(host.read(regs.rsp,4),'little')
                    returned=dict(gp=before['gp'].copy(),eflags=0x202|(rng.getrandbits(12)&0xcd5),fp=dict(before['fp']))
                    for i in range(3):returned['gp'][i]=rng.getrandbits(32)
                    returned['fp']['xmm']=[rng.randbytes(16).hex() for _ in range(8)]
                    returned['fp']['cw']=CWS[(n+len(calls))%len(CWS)];returned['fp']['mxcsr']=CSRS[(n+len(calls))%len(CSRS)]
                    writes=[]
                    if pc==0x16e6a0:
                        refusal=kind=='base' and bool(n&(8 if allocations==0 else 32));allocations+=1
                        returned['gp'][0]=NONE if refusal else datum
                        if not refusal and kind!='fault-rows':writes=[[body,salt,2]]
                    elif pc==0x17aef0:returned['gp'][0]=(returned['gp'][0]&0xffffff00)|((n>>4)&1)
                    elif pc==0x259b0:
                        # This declared bounded-real result is an x87 return,
                        # including an explicit MMX-to-x87 ABI transition.
                        returned['fp'].pop('mmx',None)
                        old=returned['fp']['status'];returned['fp']['status']=(old&~0x3800)|((((old>>11)-1)&7)<<11)
                        returned['fp']['stack']=returned['fp']['stack'].copy();returned['fp']['stack'].insert(0,EXTENDED[n%len(EXTENDED)])
                    for at,value,width in writes:host.write(at,value.to_bytes(width,'little'))
                    returned['gp'][4]+=4+SERVICES[pc];machine.load(returned['fp'])
                    for name,value in zip(ALIASES,returned['gp']):setattr(regs,name,value)
                    regs.rip=ret;regs.eflags=returned['eflags'];host.trace(13,0,ctypes.addressof(regs))
                    calls.append(dict(target=pc,natural=False,before=before,flags_mask=defined,returned=returned,writes=writes));defined=NONE&~DEBUG_FLAGS;continue
                assert 0x17afb0<=pc<=0x17b0cc or 0x17b960<=pc<=0x17b97f or 0x17b130<=pc<=0x17b1ed or 0x17dc60<=pc<=0x17dcd6,hex(pc)
                ins=next(decoder.disasm(image.read(pc,15),pc,count=1));raw=bytes(ins.bytes);assert host.read(pc,len(raw))==raw
                instructions[f'{pc:08x}']=raw.hex();trace.append(pc);pending=[];operands=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                if ins.mnemonic in ('push','call'):pending.append(((regs.rsp-4)&NONE,4))
                if ins.mnemonic=='pop' or ins.mnemonic.startswith('ret'):operands.append((regs.rsp&NONE,4,'read'))
                for op in ins.operands:
                    if op.type!=3 or ins.mnemonic=='lea':continue
                    at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&NONE
                    written=bool(op.access&2) or ins.mnemonic in ('fst','fstp')
                    operands.append((at,op.size,'write' if written else 'read'))
                    if written:pending.append((at,op.size))
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
                    assert kind.startswith('fault-') and operands,(kind,hex(pc),status)
                    at,width,access=operands[-1];fault=[dict(instruction=pc,address=at,size=width,access=access)];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(hex(pc),status)
                for at,width in pending:stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                if ins.mnemonic=='fstp':machine.depth-=1
                if ins.mnemonic in ('cmp','add','sub','comiss','ucomiss'):defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic in ('test','xor','and','or'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
                elif ins.mnemonic in ('shr','sar','shl'):defined=(NONE&~DEBUG_FLAGS)&~(0x10 if ins.operands[1].imm==1 else 0x810)
                elif ins.mnemonic in ('dec','inc'):defined=(NONE&~DEBUG_FLAGS)&~(0 if defined&1 else 1)
            else:raise ValueError('Original composed allocation instruction limit')
            assert bool(fault)==kind.startswith('fault-'),(kind,hex(regs.rip))
            if not fault:assert regs.rip==STOP and regs.rsp==STACK+12 and not pending_natural
            rows.append(dict(entry=0x17afb0,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,initial_stack=initial_stack,calls=calls,stores=stores,fault=fault,
                trace_offsets=b''.join(struct.pack('<H',pc-0x17afb0) for pc in trace).hex(),expected=machine.snapshot(regs),flags_mask=defined))
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_ALLOCATION_REFERENCE_V1',original_sha256=pin,scope=__doc__,rows=rows),separators=(',',':'))+'\n').encode()
    assert len(data)<=1024*1024,len(data);args.fixture.write_bytes(data)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),state_helper_sha256=hashlib.sha256(Path(__file__).with_name('weapon_effect_original_cpu_state.py').read_bytes()).hexdigest(),
        controlled_lower_calls=sum(not c['natural'] for r in rows for c in r['calls']),actual_child_calls=sum(c['natural'] for r in rows for c in r['calls']),fault_stops=sum(bool(r['fault']) for r in rows),alias_cases=sum(r['kind'].startswith('alias-') for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),instruction_map=instructions,
        fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],controlled_boundaries=[hex(k) for k in SERVICES],actual_children=[hex(k) for k in sorted(NATURAL)],physical_xbox_proof=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
