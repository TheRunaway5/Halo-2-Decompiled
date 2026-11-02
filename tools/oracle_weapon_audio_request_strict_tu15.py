#!/usr/bin/env python3
"""Unchanged18BE50/18C4B0 with explicit lower-service ABI controls.

Physical i386 execution, complete FP/MMX/flags/PC/read/store/stack snapshots,
and genuine read/write faults. Helper writes and returned GP/XMM0/flags/FP
controls are declared inputs, not proof of those lower owners. Instructions
are never patched. Run through tools/run_test.py; keep the bounded reference.
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
SERVICES={0x18c090:8,0x18be00:0,0x18c130:24,0x18cdd0:4}
BASE=0x1000000

class AudioState(EffectCpuState):
    def snapshot(self,regs):
        result=super().snapshot(regs);result['stack']=self.host.read(STACK-512,544).hex();return result


def packed(state,initial):
    # These encodings omit only fields physically verified to equal the input.
    state=copy.deepcopy(state);fp=state['fp'];state['fp']={k:v for k,v in fp.items() if initial.get(k)!=v}
    if 'xmm' in state['fp']:
        assert fp['xmm'][1:]==initial['xmm'][1:];state['fp']['xmm0']=state['fp'].pop('xmm')[0]
    return state


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    prior=Path(__file__).resolve().parents[1]/'tests/fixtures/weapon_effect_request_tu15_original.json';seeds=[r for r in json.loads(prior.read_text())['rows'] if r['entry'] in (0x18be50,0x18c4b0)];assert len(seeds)==48
    configs=[(n,'base',copy.deepcopy(seed),None) for n,seed in enumerate(seeds)]
    for i in range(6):configs.append((len(configs),'fault-direct-push-'+str(i),copy.deepcopy(seeds[0]),BASE+0x28+i*4))
    for kind,sp in [('fault-direct-first-argument',END-8),('fault-direct-last-argument',END-12)]:configs.append((len(configs),kind,copy.deepcopy(seeds[0]),sp))
    for kind,sp in [('fault-unowned-argument',END-4),('fault-unowned-first-store',BASE+0x55),('fault-unowned-second-store',BASE+0x56),('fault-unowned-push',BASE+0x58),('fault-unowned-call',BASE+0x5c)]:configs.append((len(configs),kind,copy.deepcopy(seeds[32]),sp))
    for i in [0,1,32,33]:configs.append((len(configs),'active-mmx',copy.deepcopy(seeds[i]),None))
    _,image,pin=next(r for r in images() if r[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    rng=random.Random(0x18be50);rows=[];instructions={};write_sites=set();steps=0
    with OriginalAudioCpu(image,scratch/'audio-request-strict-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0);machine=AudioState(host)
        for n,kind,seed,sp in configs:
            entry=seed['entry'];gp=seed['gp'];memory=seed['memory'];blobs=[]
            if sp is not None:
                gp[4]=sp;at=BASE if sp<BASE+4096 else END-512;blobs=[[at,rng.randbytes(512).hex()]]
                for i,value in enumerate([STOP,*seed['arguments']]):
                    at=sp+i*4
                    if at+4<=END:memory.append([at,value,4])
            initial_stack=seed['initial_stack'];host.write(STACK-512,bytes.fromhex(initial_stack))
            for at,raw in blobs:host.write(at,bytes.fromhex(raw))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=CSRS[n%len(CSRS)],stack=[EXTENDED[2],EXTENDED[3]][:n%3],xmm=seed['xmm'])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(entry,gp[4],dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];reads=[];stores=[];calls=[];fault=[];events=[];defined=NONE&~DEBUG_FLAGS
            for _ in range(300):
                pc=regs.rip
                if pc==STOP:break
                if pc in SERVICES:
                    before=machine.snapshot(regs);ret=int.from_bytes(host.read(regs.rsp,4),'little');legacy=seed['calls'][len(calls)];assert legacy['target']==pc,(kind,hex(pc))
                    writes=[[at+gp[4]-STACK if STACK-4096<=at<STACK+4096 else at,raw] for at,raw in legacy['writes']]
                    for at,raw in writes:host.write(at,bytes.fromhex(raw))
                    returned=dict(gp=before['gp'].copy(),eflags=0x202|(rng.getrandbits(12)&0xcd5),fp=copy.deepcopy(before['fp']))
                    returned['gp'][:3]=seed['clobber'];returned['gp'][4]+=4+SERVICES[pc]
                    returned['fp']['xmm'][0]=seed['service_xmm0'];returned['fp']['cw']=CWS[(n+len(calls)+1)%len(CWS)];returned['fp']['mxcsr']=CSRS[(n+len(calls)+1)%len(CSRS)]
                    machine.load(returned['fp'])
                    for name,value in zip(ALIASES,returned['gp']):setattr(regs,name,value)
                    regs.rip=ret;regs.eflags=returned['eflags'];host.trace(13,0,ctypes.addressof(regs));calls.append(dict(target=pc,tail=False,before=packed(before,fp),flags_mask=defined,writes=writes,returned=packed(returned,fp)));defined=NONE&~DEBUG_FLAGS;continue
                assert entry<=pc<=entry+0x53,hex(pc)
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
                    assert kind.startswith('fault-') and (operands or pending),(kind,hex(pc),status)
                    access='write' if pending else 'read';at,width=(pending if pending else operands)[-1];fault=[dict(instruction=pc,address=at,size=width,access=access)];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(hex(pc),status)
                for at,width in pending:stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                if ins.mnemonic in ('cmp','sub','add'):defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic in ('test','and','or','xor'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic=='shl':defined=(NONE&~DEBUG_FLAGS)&~0x810
                elif ins.mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
            else:raise ValueError('Original strict reset instruction limit')
            assert bool(fault)==kind.startswith('fault-'),(kind,hex(regs.rip))
            expected=machine.snapshot(regs)
            if not fault:
                assert regs.rip==STOP and regs.rsp==gp[4]+(16 if entry==0x18be50 else 8)
                if kind in ('base','active-mmx'):assert expected['gp']==seed['expected_gp'] and expected['fp']['xmm']==seed['expected_xmm'] and expected['stack']==seed['expected_stack']
            offsets=b''.join(struct.pack('<H',pc-entry) for pc in trace).hex()
            rows.append(dict(entry=entry,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,initial_stack=initial_stack,reads=reads,stores=stores,calls=calls,fault=fault,trace_offsets=offsets,expected=packed(expected,fp),flags_mask=defined))
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_AUDIO_REQUEST_STRICT_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=sorted(write_sites),rows=rows),separators=(',',':'))+'\n').encode();assert len(data)<=1024*1024,len(data);args.fixture.write_bytes(data)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),state_helper_sha256=hashlib.sha256(Path(__file__).with_name('weapon_effect_original_cpu_state.py').read_bytes()).hexdigest(),legacy_input_sha256=hashlib.sha256(prior.read_bytes()).hexdigest(),legacy_audio_inputs_replayed=48,controlled_lower_calls=sum(len(r['calls']) for r in rows),read_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='read' for r in rows),write_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='write' for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),instruction_map=instructions,fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],controlled_boundaries=[hex(k) for k in SERVICES],physical_xbox_proof=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
