#!/usr/bin/env python3
"""Unchanged CABC0 reset with explicit actual helper and tail-sound ABIs.

Replay all earlier reset inputs on a physical i386 CPU. No instructions are
patched. Helper results are declared controls, not proof of lower owners.
Capture defined flags, every PC/read/store, committed reset publication and
complete live FP/MMX state, including genuine read and partial-write faults.
Run through tools/run_test.py; retain only the required bounded reference.
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
SERVICES={0xbcdf0:4,0x106040:0,0x18c4b0:4}
OBJECT_POOL,HEADERS,UNIT,WEAPON,PLAYERS,PLAYER_ROWS,USERS,TAGS,WEAP_DEF,GLOBALS,INFO=[0x1000000+n*0x2000 for n in range(11)]

class ResetState(EffectCpuState):
    def snapshot(self,regs):
        result=super().snapshot(regs);result['stack']=self.host.read(STACK-64,96).hex();return result


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
    prior=Path(__file__).resolve().parents[1]/'tests/fixtures/weapon_zoom_reset_tu15_original.json';seeds=json.loads(prior.read_text())['rows'];assert len(seeds)==144
    default=next(i for i,r in enumerate(seeds) if any(c['tail'] for c in r['calls']) and not r['predicate'])
    scoped=next(i for i,r in enumerate(seeds) if any(c['tail'] for c in r['calls']) and r['events'] and r['events'][0]['weapon_scoped'])
    local=next(i for i,r in enumerate(seeds) if r['events'] and r['events'][0]['source_local_user']==0 and not r['predicate'] and r['events'][0]['weapon']!=NONE and not r['request'])
    configs=[(n,'base',copy.deepcopy(seed)) for n,seed in enumerate(seeds)]
    fault_kinds=['fault-pool','fault-headers','fault-object','fault-player-pool','fault-player-row','fault-slot','fault-global-info','fault-tag-table','fault-weapon-definition','fault-argument','fault-previous-selector','fault-reset-second-store','fault-reset-timer-store','fault-camera-first-store','fault-camera-second-store']
    for kind in fault_kinds:configs.append((len(configs),kind,copy.deepcopy(seeds[scoped if kind in ('fault-tag-table','fault-weapon-definition') else local if kind.startswith('fault-camera-') else default])))
    for i in range(8):configs.append((len(configs),'alias-'+str(i),copy.deepcopy(seeds[local])))
    for i in range(4):configs.append((len(configs),'active-mmx',copy.deepcopy(seeds[i+1])))
    _,image,pin=next(r for r in images() if r[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    rng=random.Random(0xcabc0);rows=[];instructions={};write_sites=set();steps=0
    with OriginalAudioCpu(image,scratch/'zoom-reset-strict-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0);machine=ResetState(host)
        for n,kind,seed in configs:
            gp=seed['gp'];memory=seed['memory'];blobs=[];body=UNIT
            def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
            if kind=='fault-pool':word(0x4ecbd0,BAD)
            elif kind=='fault-headers':word(OBJECT_POOL+0x44,BAD)
            elif kind=='fault-object':word(HEADERS+3*12+8,BAD)
            elif kind=='fault-player-pool':word(0x4f54f4,BAD)
            elif kind=='fault-player-row':word(PLAYERS+0x44,BAD)
            elif kind=='fault-global-info':word(0x4ecc1c,BAD)
            elif kind=='fault-tag-table':word(0x4f0414,BAD)
            elif kind=='fault-weapon-definition':word(TAGS+9*16+8,END-0x278)
            elif kind in ('fault-slot','fault-previous-selector','fault-reset-second-store','fault-reset-timer-store'):
                body=END-{'fault-slot':0x212,'fault-previous-selector':0x240,'fault-reset-second-store':0x241,'fault-reset-timer-store':0x26c}[kind]
                memory=[[at+body-UNIT if UNIT<=at<UNIT+0x1000 else at,v,w] for at,v,w in memory]
                memory=[v for v in memory if v[0]+v[2]<=END];word(HEADERS+3*12+8,body)
            elif kind=='fault-argument':
                gp[4]=END-4;blobs=[[END-256,rng.randbytes(256).hex()]];word(gp[4],STOP)
            elif kind.startswith('fault-camera-') or kind.startswith('alias-'):
                users=BAD if kind=='fault-camera-first-store' else END-0x9d if kind=='fault-camera-second-store' else [UNIT+0x240,UNIT+0x241,UNIT+0x26c,HEADERS+3*12+8,UNIT+0x212,UNIT+0x214,UNIT+0x218,UNIT+0x13c][int(kind.split('-')[1])]-0x42
                if kind.startswith('fault-camera-'):memory=[[at+users-USERS if USERS<=at<USERS+0x1000 else at,v,w] for at,v,w in memory]
                memory=[v for v in memory if not BAD<=v[0]<BAD+0x1000 and v[0]+v[2]<=END];word(0x4f9b54,users)
            initial_stack=seed['initial_stack'];host.write(STACK-64,bytes.fromhex(initial_stack))
            for at,raw in blobs:host.write(at,bytes.fromhex(raw))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=CSRS[n%len(CSRS)],stack=[EXTENDED[2],EXTENDED[3]][:n%3],xmm=seed['xmm'])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(0xcabc0,gp[4],dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];reads=[];stores=[];calls=[];fault=[];events=[];defined=NONE&~DEBUG_FLAGS;player=NONE;weapon_scoped=False;previous=0xff;request=seed['request']
            for _ in range(300):
                pc=regs.rip
                if pc==STOP:break
                if pc in SERVICES:
                    before=machine.snapshot(regs);ret=int.from_bytes(host.read(regs.rsp,4),'little');legacy=seed['calls'][len(calls)];assert legacy['target']==pc,(n,kind,len(calls),hex(pc),hex(legacy['target']))
                    returned=dict(gp=before['gp'].copy(),eflags=0x202|(rng.getrandbits(12)&0xcd5),fp=copy.deepcopy(before['fp']))
                    returned['gp'][:3]=legacy['returned_gp'];returned['gp'][4]+=4+SERVICES[pc]
                    if pc==0xbcdf0 and returned['gp'][0]==UNIT:returned['gp'][0]=body
                    returned['fp']['xmm'][0]=legacy['returned_xmm0'];returned['fp']['cw']=CWS[(n+len(calls)+1)%len(CWS)];returned['fp']['mxcsr']=CSRS[(n+len(calls)+1)%len(CSRS)]
                    machine.load(returned['fp'])
                    for name,value in zip(ALIASES,returned['gp']):setattr(regs,name,value)
                    regs.rip=ret;regs.eflags=returned['eflags'];host.trace(13,0,ctypes.addressof(regs));calls.append(dict(target=pc,tail=ret==STOP,before=packed(before,fp),flags_mask=defined,returned=packed(returned,fp)));defined=NONE&~DEBUG_FLAGS;continue
                assert 0xcabc0<=pc<=0xcad35,hex(pc)
                if pc==0xcac17:player=regs.rcx&NONE
                if pc==0xcac60:weapon_scoped=bool(regs.rax&255)
                if pc==0xcac77:previous=regs.rdx&255
                if pc==0xcac96 and regs.rdx&255:
                    events.append(dict(unit=gp[0],player=player,weapon=regs.rdi&NONE,previous_selector=previous,request=request,weapon_scoped=weapon_scoped,source_local_user=ctypes.c_int32(regs.rbx&NONE).value,committed_stores=len(stores)))
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
                if ins.mnemonic=='cmp':defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic in ('test','and','or','xor'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic=='shl':defined=(NONE&~DEBUG_FLAGS)&~0x810
                elif ins.mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
            else:raise ValueError('Original strict reset instruction limit')
            assert bool(fault)==kind.startswith('fault-'),(kind,hex(regs.rip))
            expected=machine.snapshot(regs)
            if not fault:
                assert regs.rip==STOP and regs.rsp==gp[4]+8
                if kind in ('base','active-mmx'):assert expected['gp']==seed['expected_gp'] and expected['fp']['xmm']==seed['expected_xmm'] and events==seed['events']
            offsets=b''.join(struct.pack('<H',pc-0xcabc0) for pc in trace).hex()
            rows.append(dict(entry=0xcabc0,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,initial_stack=initial_stack,reads=reads,stores=stores,calls=calls,fault=fault,events=events,trace_offsets=offsets,expected=packed(expected,fp),flags_mask=defined))
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_ZOOM_RESET_STRICT_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=sorted(write_sites),rows=rows),separators=(',',':'))+'\n').encode();assert len(data)<=1024*1024,len(data);args.fixture.write_bytes(data)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),state_helper_sha256=hashlib.sha256(Path(__file__).with_name('weapon_effect_original_cpu_state.py').read_bytes()).hexdigest(),legacy_input_sha256=hashlib.sha256(prior.read_bytes()).hexdigest(),legacy_reset_inputs_replayed=144,controlled_lower_calls=sum(len(r['calls']) for r in rows),read_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='read' for r in rows),write_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='write' for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),alias_cases=sum(r['kind'].startswith('alias-') for r in rows),committed_reset_events=sum(len(r['events']) for r in rows),tail_calls=sum(c['tail'] for r in rows for c in r['calls']),instruction_map=instructions,fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],controlled_boundaries=[hex(k) for k in SERVICES],physical_xbox_proof=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
