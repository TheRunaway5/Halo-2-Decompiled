#!/usr/bin/env python3
"""Unchanged17DFF0/17A1A0 lifecycle and179610 attached-effect requests.

Physical i386 flags, full GP/XMM/live80/MMX, ordered reads/stores, complete
PC traces and genuine faults. Lower services return explicit declared inputs,
not allocation/audio/child-owner proof. No original instruction is patched.
Run through tools/run_test.py; retain only the bounded required references.
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
SERVICES={0x17a1a0:4,0x17b130:0,0x17dce0:12,0x18d540:4,0xbbe00:0,0x1791c0:0,0x178e30:4}
POOL,BODIES,POSITION,TAGS,DEFINITION=0x1000000,0x1002000,0x1004000,0x1006000,0x1008000
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
    parser.add_argument('--attached',action='store_true');parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    root=Path(__file__).resolve().parents[1];prior=root/'tests/fixtures/weapon_effect_request_tu15_original.json';restart=root/'tests/fixtures/weapon_effect_restart_tu15_original.json'
    request_rows=json.loads(prior.read_text())['rows'];seeds=([r for r in request_rows if r['entry']==0x179610] if args.attached else [r for r in request_rows if r['entry']==0x17dff0]+json.loads(restart.read_text())['rows']);assert len(seeds)==(32 if args.attached else 96)
    configs=[(n,'base',copy.deepcopy(seed),None) for n,seed in enumerate(seeds)]
    def recipe(kind,seed,sp=None):configs.append((len(configs),kind,copy.deepcopy(seed),sp))
    if args.attached:
        for kind in ['fault-position-first','fault-position-second','fault-position-third']:recipe(kind,seeds[1])
        for kind,sp in [('fault-attached-first-argument',END-4),('fault-attached-fourth-argument',END-16),('fault-attached-first-push',BASE+0xa4),('fault-attached-second-push',BASE+0xa8),('fault-attached-rep',BASE+0xac),('fault-attached-call',BASE+0xac)]:recipe(kind,seeds[0],sp)
        for i in range(8):recipe('alias-position-'+str(i),seeds[1])
        for i in [0,1,16,17]:recipe('active-mmx',seeds[i])
    else:
        refresh=next(r for r in seeds if r['entry']==0x17dff0 and r['calls']);renew=next(r for r in seeds if r['entry']==0x17a1a0 and len(r['calls'])==3)
        for value in [0,0xffffffff,0x80000000,0x7fffffff]:recipe('edge-refresh-bound-'+str(value),refresh)
        for value in [0,0x8000,0x7fff]:recipe('edge-refresh-generation-'+str(value),refresh)
        for kind in ['fault-refresh-pool','fault-refresh-bound','fault-refresh-multiply-generation','fault-refresh-generation','fault-refresh-definition']:recipe(kind,refresh)
        for kind,sp in [('fault-refresh-argument',END-4),('fault-refresh-first-push',BASE),('fault-refresh-second-push',BASE+4),('fault-refresh-call',BASE+12)]:recipe(kind,refresh,sp)
        for kind in ['fault-restart-pool','fault-restart-tags','fault-restart-definition','fault-restart-flags','fault-restart-sound','fault-restart-index','fault-restart-sound-handle','fault-restart-location']:recipe(kind,renew)
        for kind,sp in [('fault-restart-argument',END-4),('fault-restart-first-push',BASE),('fault-restart-call',BASE+16)]:recipe(kind,renew,sp)
        for kind in ['alias-restart-saved-register','alias-restart-slot-handle','alias-restart-slot-definition','alias-restart-slot-generation']:recipe(kind,renew)
        for i in [1,9,64,72]:recipe('active-mmx',seeds[i])
    _,image,pin=next(r for r in images() if r[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    rng=random.Random(0x179610 if args.attached else 0x17dff0);rows=[];instructions={};write_sites=set();read_sites=set();steps=0
    with OriginalAudioCpu(image,scratch/'lifecycle-strict-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0);machine=AudioState(host)
        for n,kind,seed,sp in configs:
            entry=seed['entry'];gp=seed['gp'];memory=seed['memory'];blobs=[];stride=0x70 if entry==0x17dff0 else 0x190;body=BODIES+3*stride
            def word(at,value,width=4):memory.append([at,value&((1<<(8*width))-1),width])
            def relocate(start,end,to):
                nonlocal memory
                memory=[[at+to-start if start<=at<end else at,value,width] for at,value,width in memory]
                memory=[v for v in memory if v[0]<BASE or v[0]+v[2]<=END]
            def move_body(to):
                nonlocal body,memory
                memory=[[at+to-body if body<=at<body+0x100 else at,value,width] for at,value,width in memory]
                memory=[v for v in memory if not body<=v[0]<body+0x100 and (not (to<=v[0]<to+0x100) or BASE<=v[0] and v[0]+v[2]<=END)]
                body=to;word(POOL+0x44,body-3*stride)
            if kind.startswith('edge-refresh-bound-'):word(POOL+0x38,int(kind.split('-')[-1]))
            elif kind.startswith('edge-refresh-generation-'):
                value=int(kind.split('-')[-1]);gp[7]=(value<<16)|3;word(body,value,2)
            elif kind in ('fault-refresh-pool','fault-restart-pool'):word(0x4f720c,BAD)
            elif kind=='fault-refresh-bound':
                relocate(POOL,POOL+0x100,END-0x38);memory=[v for v in memory if v[0]+v[2]<=END];word(0x4f720c,END-0x38)
            elif kind=='fault-refresh-multiply-generation':word(POOL+0x24,0x80000000)
            elif kind=='fault-refresh-generation':move_body(END)
            elif kind in ('fault-refresh-definition','fault-restart-definition'):move_body(END-4)
            elif kind=='fault-restart-tags':word(0x4f0414,BAD)
            elif kind=='fault-restart-flags':move_body(BASE-4)
            elif kind in ('fault-restart-sound','fault-restart-index'):
                to=END-(0x20 if kind=='fault-restart-sound' else 0x24);relocate(DEFINITION,DEFINITION+0x100,to);memory=[v for v in memory if v[0]+v[2]<=END];word(TAGS+7*16+8,to)
            elif kind=='fault-restart-sound-handle':move_body(END-8)
            elif kind=='fault-restart-location':move_body(END-0x100);word(DEFINITION+0x24,0x7fff,2)
            elif kind=='alias-restart-saved-register':move_body(STACK-20);gp[7]=0x34560007;gp[6]=NONE
            elif kind.startswith('alias-restart-slot-'):
                slot={'alias-restart-slot-handle':-29,'alias-restart-slot-definition':-30,'alias-restart-slot-generation':-31}[kind];word(DEFINITION+0x24,slot,2);word(body,0x9834,2)
            elif kind.startswith('fault-position-'):
                gp[3]=BAD if kind=='fault-position-first' else END-(4 if kind=='fault-position-second' else 8)
                for i in range(3):
                    at=gp[3]+4*i
                    if at<END:word(at,0x12340000+i)
            elif kind.startswith('alias-position-'):gp[3]=STACK-[0xa4,0x98,0x94,0x90,0x38,0x40,0x34,0x30][int(kind.split('-')[-1])]
            if kind=='fault-attached-rep':seed['eflags']|=0x400
            if sp is not None:
                gp[4]=sp;at=BASE if sp<BASE+4096 else END-512;blobs=[[at,rng.randbytes(512).hex()]]
                for i,value in enumerate([STOP,*seed['arguments']]):
                    at=sp+i*4
                    if at+4<=END:word(at,value)
            initial_stack=seed['initial_stack'];host.write(STACK-512,bytes.fromhex(initial_stack))
            for at,raw in blobs:host.write(at,bytes.fromhex(raw))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=CSRS[n%len(CSRS)],stack=[EXTENDED[2],EXTENDED[3]][:n%3],xmm=seed['xmm'])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(entry,gp[4],dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0x8d5)|(seed['eflags']&0x400);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];reads=[];stores=[];calls=[];fault=[];events=[];defined=NONE&~DEBUG_FLAGS
            for _ in range(300):
                pc=regs.rip
                if pc==STOP:break
                if pc in SERVICES and pc!=entry:
                    before=machine.snapshot(regs);ret=int.from_bytes(host.read(regs.rsp,4),'little');legacy=seed['calls'][len(calls)];assert legacy['target']==pc,(kind,hex(pc))
                    writes=[[at+gp[4]-STACK if STACK-4096<=at<STACK+4096 else at,raw] for at,raw in legacy['writes']]
                    for at,raw in writes:host.write(at,bytes.fromhex(raw))
                    returned=dict(gp=before['gp'].copy(),eflags=0x202|(rng.getrandbits(12)&0xcd5),fp=copy.deepcopy(before['fp']))
                    returned['gp'][:3]=seed['clobber'];returned['gp'][4]+=4+SERVICES[pc]
                    returned['fp']['xmm'][0]=seed['service_xmm0'];returned['fp']['cw']=CWS[(n+len(calls)+1)%len(CWS)];returned['fp']['mxcsr']=CSRS[(n+len(calls)+1)%len(CSRS)]
                    machine.load(returned['fp'])
                    for name,value in zip(ALIASES,returned['gp']):setattr(regs,name,value)
                    regs.rip=ret;regs.eflags=returned['eflags'];host.trace(13,0,ctypes.addressof(regs));calls.append(dict(target=pc,tail=False,before=packed(before,fp),flags_mask=defined,writes=writes,returned=packed(returned,fp)));defined=NONE&~DEBUG_FLAGS;continue
                assert entry<=pc<={0x17dff0:0x17e04f,0x17a1a0:0x17a228,0x179610:0x1796f0}[entry],hex(pc)
                ins=next(decoder.disasm(image.read(pc,15),pc,count=1));raw=bytes(ins.bytes);assert host.read(pc,len(raw))==raw
                instructions[f'{pc:08x}']=raw.hex();trace.append(pc);pending=[];operands=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                if ins.mnemonic in ('push','call'):pending.append(((regs.rsp-4)&NONE,4));write_sites.add(pc)
                if ins.mnemonic=='pop' or ins.mnemonic.startswith('ret'):reads.append([pc,regs.rsp&NONE,4]);operands.append((regs.rsp&NONE,4))
                for op in ins.operands:
                    if op.type!=3 or ins.mnemonic=='lea':continue
                    at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&NONE
                    if op.access&1:reads.append([pc,at,op.size]);operands.append((at,op.size));read_sites.add(pc)
                    if op.access&2:pending.append((at,op.size));write_sites.add(pc)
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
                    assert kind.startswith('fault-') and (operands or pending),(kind,hex(pc),status)
                    access='read' if operands else 'write';at,width=(operands if operands else pending)[-1];fault=[dict(instruction=pc,address=at,size=width,access=access)];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(hex(pc),status)
                for at,width in pending:stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                if ins.mnemonic in ('cmp','sub','add'):defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic in ('test','and','or','xor'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic in ('shl','sar'):defined=(NONE&~DEBUG_FLAGS)&~0x810
                elif ins.mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
            else:raise ValueError('Original strict lifecycle instruction limit')
            assert bool(fault)==kind.startswith('fault-'),(kind,hex(regs.rip))
            expected=machine.snapshot(regs)
            if not fault:
                assert regs.rip==STOP and regs.rsp==gp[4]+(20 if entry==0x179610 else 8)
                if kind in ('base','active-mmx'):assert expected['gp']==seed['expected_gp'] and expected['fp']['xmm']==seed['expected_xmm'] and expected['stack']==seed['expected_stack']
            offsets=b''.join(struct.pack('<H',pc-entry) for pc in trace).hex()
            rows.append(dict(entry=entry,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,initial_stack=initial_stack,reads=reads,stores=stores,calls=calls,fault=fault,trace_offsets=offsets,expected=packed(expected,fp),flags_mask=defined))
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_LIFECYCLE_STRICT_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=sorted(write_sites-read_sites),read_modify_write_sites=sorted(write_sites&read_sites),rows=rows),separators=(',',':'))+'\n').encode();assert len(data)<=1024*1024,len(data);args.fixture.write_bytes(data)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),state_helper_sha256=hashlib.sha256(Path(__file__).with_name('weapon_effect_original_cpu_state.py').read_bytes()).hexdigest(),legacy_inputs_sha256={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in ([prior] if args.attached else [prior,restart])},legacy_inputs_replayed=len(seeds),attached_mode=args.attached,alias_cases=sum(r['kind'].startswith('alias-') for r in rows),controlled_lower_calls=sum(len(r['calls']) for r in rows),read_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='read' for r in rows),write_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='write' for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),instruction_map=instructions,fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],controlled_boundaries=[hex(k) for k in SERVICES if any(c['target']==k for r in rows for c in r['calls'])],physical_xbox_proof=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
