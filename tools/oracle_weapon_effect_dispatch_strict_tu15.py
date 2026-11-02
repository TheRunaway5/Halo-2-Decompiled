#!/usr/bin/env python3
"""Unchanged108A70 with actual106F90 and explicit refresh/create/sound ABIs.

Every earlier dispatcher input is replayed on a physical i386 CPU child.
The three lower services are arbitrary declared GP/flags/FP results, not
allocation or audio proof. No original instruction is patched. Capture full
call/return ABI, ordered operand reads, stores, PC traces and stopped reads.
Run through tools/run_test.py; retain only the bounded required reference.
"""
import argparse
import copy
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
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,CSRS,EXTENDED
from weapon_effect_original_cpu_state import EffectCpuState
from weapon_reference_versions import images

BAD,END=0x4000000,0x2100000
SERVICES={0x17dff0:4,0x179610:16,0x18be50:12}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    prior=Path(__file__).resolve().parents[1]/'tests/fixtures/weapon_effect_dispatch_tu15_original.json';seeds=[r for r in json.loads(prior.read_text())['rows'] if r['entry']==0x108a70];assert len(seeds)==256
    recipes=[(n,'base',copy.deepcopy(seed)) for n,seed in enumerate(seeds)]
    recipes += [(256+n,kind,copy.deepcopy(seeds[1 if kind=='fault-sound-argument' else 4])) for n,kind in enumerate(['fault-pool','fault-owner-flags','fault-model-owner','fault-group','fault-signed-group','fault-argument','fault-sound-argument','fault-object'])]
    recipes += [(264+n,'active-mmx',copy.deepcopy(seeds[n+1])) for n in range(4)]
    _,image,pin=next(r for r in images() if r[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    rng=random.Random(0x108a70);rows=[];instructions={};write_sites=set();steps=0
    with OriginalAudioCpu(image,scratch/'dispatch-strict-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0);machine=EffectCpuState(host)
        for n,kind,seed in recipes:
            gp=seed['gp'];memory=seed['memory'];pool,headers,body,definition,tags=[0x1000000+i*0x2000 for i in range(5)];blobs=[]
            def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
            if kind=='fault-pool':word(0x4ecbd0,BAD)
            elif kind in ('fault-owner-flags','fault-model-owner'):
                body=END-(0x120 if kind=='fault-owner-flags' else 0x150);word(headers+3*12+8,body);word(body,7);word(body+4,0,1)
                if kind=='fault-model-owner':word(body+0x12c,1,1)
            elif kind=='fault-group':word(0x4f0414,END-128);word(END-8,definition)
            elif kind=='fault-signed-group':gp[0]=0x92348001
            elif kind=='fault-object':word(headers+3*12+8,BAD)
            elif kind in ('fault-argument','fault-sound-argument'):
                gp[4]=END-(12 if kind=='fault-argument' else 8);blobs=[[END-256,rng.randbytes(256).hex()]];word(gp[4],STOP)
                for off,value in enumerate(seed['arguments']):
                    at=gp[4]+4+off*4
                    if at<END:word(at,value)
            initial_stack=rng.randbytes(160);host.write(STACK-96,initial_stack);host.write(STACK,pack([STOP,*seed['arguments']]));initial_stack=host.read(STACK-96,160).hex()
            for at,raw in blobs:host.write(at,bytes.fromhex(raw))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=CSRS[n%len(CSRS)],stack=[EXTENDED[2],EXTENDED[3]][:n%3],xmm=seed['xmm'])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(0x108a70,gp[4],dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];reads=[];stores=[];calls=[];fault=[];defined=NONE&~DEBUG_FLAGS
            for _ in range(300):
                pc=regs.rip
                if pc==STOP:break
                if pc in SERVICES:
                    before=machine.snapshot(regs);ret=int.from_bytes(host.read(regs.rsp,4),'little');returned=dict(gp=before['gp'].copy(),eflags=0x202|(rng.getrandbits(12)&0xcd5),fp=dict(before['fp']))
                    returned['gp'][:3]=seed['clobber'];returned['gp'][4]+=4+SERVICES[pc]
                    if pc==0x17dff0:returned['gp'][0]=seed['refresh_result']
                    elif pc==0x179610:returned['gp'][0]=seed['create_result']
                    returned['fp']['cw']=CWS[(n+len(calls)+1)%len(CWS)];returned['fp']['mxcsr']=CSRS[(n+len(calls)+1)%len(CSRS)]
                    machine.load(returned['fp'])
                    for name,value in zip(ALIASES,returned['gp']):setattr(regs,name,value)
                    regs.rip=ret;regs.eflags=returned['eflags'];host.trace(13,0,ctypes.addressof(regs));calls.append(dict(target=pc,before=before,flags_mask=defined,returned=returned));defined=NONE&~DEBUG_FLAGS;continue
                assert 0x108a70<=pc<=0x108b28 or 0x106f90<=pc<=0x106fe4,hex(pc)
                ins=next(decoder.disasm(image.read(pc,15),pc,count=1));raw=bytes(ins.bytes);assert host.read(pc,len(raw))==raw
                instructions[f'{pc:08x}']=raw.hex();trace.append(pc);pending=[];operands=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                if ins.mnemonic in ('push','call'):pending.append(((regs.rsp-4)&NONE,4));write_sites.add(pc)
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
                elif ins.mnemonic in ('test','and','or','xor'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic=='shl':defined=(NONE&~DEBUG_FLAGS)&~0x810
            else:raise ValueError('Original strict dispatcher instruction limit')
            assert bool(fault)==kind.startswith('fault-'),(kind,hex(regs.rip))
            expected=machine.snapshot(regs)
            if not fault:assert regs.rip==STOP and regs.rsp==STACK+16 and expected['gp']==seed['expected_gp'] and expected['fp']['xmm']==seed['expected_xmm']
            # Signed16 offsets cover both actual original owners without a
            # supplied selector result or a fabricated return boundary.
            offsets=b''.join(struct.pack('<h',pc-0x108a70) for pc in trace).hex()
            rows.append(dict(entry=0x108a70,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,initial_stack=initial_stack,reads=reads,stores=stores,calls=calls,fault=fault,trace_offsets=offsets,expected=expected,flags_mask=defined))
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_DISPATCH_STRICT_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=sorted(write_sites),rows=rows),separators=(',',':'))+'\n').encode();args.fixture.write_bytes(data)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),state_helper_sha256=hashlib.sha256(Path(__file__).with_name('weapon_effect_original_cpu_state.py').read_bytes()).hexdigest(),legacy_input_sha256=hashlib.sha256(prior.read_bytes()).hexdigest(),legacy_dispatcher_inputs_replayed=256,controlled_lower_calls=sum(len(r['calls']) for r in rows),actual_selector_cases=sum(any(pc<0 for pc in struct.unpack('<'+'h'*(len(r['trace_offsets'])//4),bytes.fromhex(r['trace_offsets']))) for r in rows),
        read_stops=sum(bool(r['fault']) for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),instruction_map=instructions,fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],controlled_boundaries=[hex(k) for k in SERVICES],actual_selector=True,physical_xbox_proof=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
