#!/usr/bin/env python3
"""Unchanged BBE00 origin and1791C0 direction with no callees or patches.

Replay every prior location input on a physical i386 CPU, adding partial-vector
read/write faults, stack faults and live MMX. Record full GP/FP/flags, all PCs,
operand reads, source stores and the original128-byte live/dead stack domain.
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
from oracle_weapon_effect_dispatch_tu15 import STACK,STOP,NONE
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,EXTENDED
from weapon_effect_original_cpu_state import EffectCpuState
from weapon_reference_versions import images

BASE,END,BAD=0x1000000,0x2100000,0x6000000
POOL,HEADERS,OBJECT,PARENT,INPUT,OUTPUT=[BASE+i*0x3000 for i in range(6)]

class LocationState(EffectCpuState):
    def snapshot(self,regs):
        result=super().snapshot(regs);result['stack']=self.host.read(STACK-64,128).hex();return result


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--direction',action='store_true');parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);root=Path(__file__).resolve().parents[1];prior=root/'tests/fixtures/weapon_effect_location_tu15_original.json';entry=0x1791c0 if args.direction else 0xbbe00
    seeds=[r for r in json.loads(prior.read_text())['rows'] if r['entry']==entry];assert len(seeds)==96
    configs=[(n,'base',copy.deepcopy(seed),None) for n,seed in enumerate(seeds)]
    def recipe(kind,seed,sp=None):configs.append((len(configs),kind,copy.deepcopy(seed),sp))
    if args.direction:
        for kind in ['fault-first-global-partial','fault-second-global-partial','fault-input-second','fault-input-third','fault-output-header','fault-output-first-type','fault-output-second-header','fault-output-last-type']:recipe(kind,seeds[0])
        for kind,sp in [('fault-first-push',BASE),('fault-second-push',BASE+4),('fault-return',END)]:recipe(kind,seeds[0],sp)
    else:
        for kind in ['fault-origin-position-second','fault-origin-position-third','fault-origin-matrix','fault-origin-translation']:recipe(kind,seeds[1])
        for kind in ['fault-output-copy-first','fault-output-copy-second']:recipe(kind,seeds[0])
        for kind in ['fault-output-transform-first','fault-output-transform-second']:recipe(kind,seeds[1])
        for kind,seed,sp in [('fault-origin-push',seeds[1],BASE),('fault-copy-return',seeds[0],END),('fault-transform-return',seeds[1],END)]:recipe(kind,seed,sp)
    for i in [0,1,4,5]:recipe('active-mmx',seeds[i])
    _,image,pin=next(r for r in images() if r[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    rng=random.Random(entry);rows=[];instructions={};write_sites=set();steps=0
    with OriginalAudioCpu(image,scratch/'location-strict-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0);machine=LocationState(host)
        for n,kind,seed,sp in configs:
            gp=seed['gp'];memory=seed['memory'];blobs=seed['banks'];initial_stack=next(raw for at,raw in blobs if at==STACK-64)
            def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
            if kind in ('fault-first-global-partial','fault-second-global-partial'):
                word(0x474d1c if kind=='fault-first-global-partial' else 0x474d10,END-4);word(END-4,0x12345678)
            elif kind in ('fault-input-second','fault-input-third'):
                gp[1]=END-(4 if kind=='fault-input-second' else 8)
                for i in range(1 if kind=='fault-input-second' else 2):word(gp[1]+i*4,0x45670000+i)
            elif kind.startswith('fault-output-'):
                blobs.append([END-512,rng.randbytes(512).hex()])
                at=BAD if kind.endswith('-first') else END-4
                if args.direction:gp[0]=END-{'fault-output-header':0x10,'fault-output-first-type':0x18,'fault-output-second-header':0x2c,'fault-output-last-type':0x34}[kind]
                else:gp[6]=at
            elif kind.startswith('fault-origin-position-'):
                to=END-(0x68 if kind.endswith('second') else 0x6c)
                memory=[[at+to-OBJECT if OBJECT<=at<OBJECT+0x120 else at,v,w] for at,v,w in memory];memory=[r for r in memory if r[0]+r[2]<=END];word(HEADERS+3*12+8,to)
            elif kind in ('fault-origin-matrix','fault-origin-translation'):
                matrix=END-(0x10 if kind=='fault-origin-matrix' else 0x28);parent=matrix-0x49c
                word(HEADERS+5*12+8,parent);word(OBJECT+0x18,3,1);word(parent+0x116,0x400,2)
                raw=struct.pack('<I',0x3f800000)+struct.pack('<I',0x7f812345)*((END-matrix)//4-1);blobs.append([matrix,raw.hex()]);seed['mxcsr']&=~0x3f
            if sp is not None:
                gp[4]=sp;at=BASE if sp<BASE+4096 else END-512;blobs.append([at,rng.randbytes(512).hex()])
                if sp+4<=END:word(sp,STOP)
            for at,raw in blobs:host.write(at,bytes.fromhex(raw))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=seed['mxcsr'],stack=[EXTENDED[2],EXTENDED[3]][:n%3],xmm=seed['xmm'])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(entry,gp[4],dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];reads=[];stores=[];fault=[];defined=NONE&~DEBUG_FLAGS
            for _ in range(300):
                pc=regs.rip
                if pc==STOP:break
                assert entry<=pc<=(0x17922b if args.direction else 0xbbf1c),hex(pc)
                ins=next(decoder.disasm(image.read(pc,15),pc,count=1));assert host.read(pc,len(ins.bytes))==bytes(ins.bytes)
                instructions[f'{pc:08x}']=bytes(ins.bytes).hex();trace.append(pc);pending=[];operands=[]
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
                    assert (kind.startswith('fault-') or not seed['completed']) and (operands or pending),(kind,hex(pc),status)
                    access='read' if operands else 'write';at,width=(operands if operands else pending)[-1];fault=[dict(instruction=pc,address=at,size=width,access=access)];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(hex(pc),status)
                for at,width in pending:stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                if ins.mnemonic in ('cmp','add','ucomiss'):defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic in ('and','test'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
            else:raise ValueError('Original strict location instruction limit')
            assert bool(fault)==(kind.startswith('fault-') or not seed['completed']),(kind,hex(regs.rip))
            expected=machine.snapshot(regs)
            if kind in ('base','active-mmx'):
                assert expected['gp']==seed['expected_gp'] and expected['fp']['xmm']==seed['expected_xmm'] and expected['fp']['mxcsr']==seed['expected_mxcsr']
                assert all(host.read(at,len(bytes.fromhex(raw))).hex()==raw for at,raw in seed['expected_banks'])
            offsets=b''.join(struct.pack('<H',pc-entry) for pc in trace).hex()
            rows.append(dict(entry=entry,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,initial_stack=initial_stack,reads=reads,stores=stores,calls=[],fault=fault,trace_offsets=offsets,expected=expected,flags_mask=defined))
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_LOCATION_STRICT_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=sorted(write_sites),rows=rows),separators=(',',':'))+'\n').encode();assert len(data)<=1024*1024,len(data);args.fixture.write_bytes(data)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),state_helper_sha256=hashlib.sha256(Path(__file__).with_name('weapon_effect_original_cpu_state.py').read_bytes()).hexdigest(),legacy_input_sha256=hashlib.sha256(prior.read_bytes()).hexdigest(),legacy_inputs_replayed=96,direction_mode=args.direction,read_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='read' for r in rows),write_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='write' for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),instruction_map=instructions,fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],controlled_boundaries=[],physical_xbox_proof=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
