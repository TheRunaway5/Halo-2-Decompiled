#!/usr/bin/env python3
"""Unchanged TU1.5 16E6A0 raw allocator ABI on a physical i386 CPU.

Replay the 162 completed typed-allocator inputs, then add real page faults,
backward REP stores, stack aliases and live MMX. No patched instructions,
emulator outcomes or supplied callees. This is bounded CPU-owner evidence.
Run through tools/run_test.py; the compressed fixture is a required reference.
"""
import argparse
import copy
import ctypes
import gzip
import hashlib
import json
import os
from pathlib import Path
import random
import signal
import struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_dispatch_tu15 import STOP,NONE
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS
from weapon_effect_original_cpu_state import EffectCpuState
from weapon_reference_versions import images

BASE,END,BAD,STACK,ENTRY=0x1000000,0x2100000,0x6000000,0x200f000,0x16e6a0


class State(EffectCpuState):
    def snapshot(self,regs):
        result=super().snapshot(regs);result['stack']=self.host.read(STACK-64,128).hex();return result


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);root=Path(__file__).resolve().parents[1];prior=root/'tests/fixtures/native_tu15_data_array_allocate.json.gz'
    seeds=[r for r in json.loads(gzip.decompress(prior.read_bytes()))['cases'] if r['completed'] and 'fault' not in r];assert len(seeds)==162,len(seeds)
    configs=[(n,'base',copy.deepcopy(seed),{}) for n,seed in enumerate(seeds)]
    def recipe(kind,seed=seeds[24],**settings):configs.append((len(configs),kind,copy.deepcopy(seed),settings))
    for i in range(4):recipe('fault-push-'+str(i),sp=BASE+i*4)
    recipe('fault-return',sp=END)
    recipe('fault-pool',pool=BAD)
    recipe('fault-extent',pool=END-0x38)
    recipe('fault-scan-bitmap',cursor=0,extent=1,bits=BAD)
    recipe('fault-allocation-bitmap',cursor=0,extent=0,bits=BAD)
    for stride in (0,1,2,3,4,5,8,9):recipe('fault-record-'+str(stride),stride=stride,record=BAD)
    for stride in (5,8,9):recipe('fault-record-partial-'+str(stride),stride=stride,record=END-4)
    for stride in (5,8,9):recipe('fault-record-backward-'+str(stride),stride=stride,record=BASE,df=True)
    for stride in range(18):recipe('backward-rep',stride=stride,df=True)
    for offset in (-16,-12,-8,-4,0,4):recipe('stack-record-alias',stride=9,record=STACK+offset)
    for i in (0,1,4,5):recipe('active-mmx',seeds[i])
    _,image,pin=next(r for r in images() if r[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    rng=random.Random(ENTRY);rows=[];instructions={};write_sites=set();steps=0
    with OriginalAudioCpu(image,scratch/'datum-allocate-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0);machine=State(host)
        for n,kind,seed,settings in configs:
            gp=seed['gp'];memory=[];blobs=[[bank['address'],bank['before']] for bank in seed['banks']];initial_stack=rng.randbytes(128).hex();blobs.append([STACK-64,initial_stack])
            def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
            pool=settings.get('pool',gp[2]);gp[2]=pool
            if pool==END-0x38:blobs.append([END-4096,rng.randbytes(4096).hex()]);word(END-4,0)
            if pool!=BAD and pool<END-0x38:
                for name,offset in [('cursor',0x34),('extent',0x38),('bits',0x48),('record',0x44),('stride',0x24)]:
                    if name in settings:word(pool+offset,settings[name])
            if settings.get('record')==END-4:blobs.append([END-4096,rng.randbytes(4096).hex()])
            if 'sp' in settings:
                gp[4]=settings['sp'];at=BASE if gp[4]<BASE+4096 else END-4096;blobs.append([at,rng.randbytes(4096).hex()])
                # The first page is also the pool for near-BASE PUSH faults.
                if at==BASE:blobs.append([BASE,seed['banks'][0]['before'][:8192]])
            if gp[4]+4<=END:word(gp[4],STOP)
            # Stack-record aliases may overwrite the RET target. Stop at the
            # exact returned address, never execute caller-owned data as code.
            for at,raw in blobs:host.write(at,bytes.fromhex(raw))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            fp=dict(cw=seed['before_fp'][0],status=seed['before_fp'][1],mxcsr=seed['before_fp'][2],stack=seed['live_x87'],xmm=[rng.randbytes(16).hex() for _ in range(8)])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(ENTRY,gp[4],dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0x8d5)|(0x400 if settings.get('df') else 0);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];reads=[];stores=[];fault=[];defined=NONE&~DEBUG_FLAGS;returned=False
            for _ in range(1000):
                pc=regs.rip
                if returned:break
                assert ENTRY<=pc<=0x16e760,(kind,hex(pc))
                ins=next(decoder.disasm(image.read(pc,15),pc,count=1));assert host.read(pc,len(ins.bytes))==bytes(ins.bytes)
                instructions[f'{pc:08x}']=bytes(ins.bytes).hex();trace.append(pc);pending=[];operands=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                if ins.mnemonic=='push':pending.append(((regs.rsp-4)&NONE,4));write_sites.add(pc)
                if ins.mnemonic=='pop' or ins.mnemonic.startswith('ret'):reads.append([pc,regs.rsp&NONE,4]);operands.append((regs.rsp&NONE,4))
                for op in ins.operands:
                    if op.type!=3 or ins.mnemonic=='lea' or (ins.mnemonic.startswith('rep ') and not regs.rcx&NONE):continue
                    at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&NONE
                    # Capstone omits TEST's real memory access for 85 2c 8b.
                    access=1 if pc==0x16e6c5 else op.access
                    if pc==0x16e6c5:assert bytes(ins.bytes).hex()=='852c8b'
                    if access&1:reads.append([pc,at,op.size]);operands.append((at,op.size))
                    if access&2:pending.append((at,op.size));write_sites.add(pc)
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
                    assert kind.startswith('fault-') and (operands or pending),(kind,hex(pc),status)
                    access='read' if operands else 'write';at,width=(operands if operands else pending)[-1];fault=[dict(instruction=pc,address=at,size=width,access=access)];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(kind,hex(pc),status)
                for at,width in pending:stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                mn=ins.mnemonic
                if mn in ('cmp','add'):defined=NONE&~DEBUG_FLAGS
                elif mn in ('and','or','xor','test'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif mn=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
                elif mn=='inc':defined=(NONE&~DEBUG_FLAGS)&(~1|defined)
                elif mn in ('shr','sar','shl'):
                    count=(reg('ecx') if ins.operands[1].type==1 else ins.operands[1].imm)&31
                    if count:defined=(NONE&~DEBUG_FLAGS)&~(0x10|(0x800 if count>1 else 0))
                returned=mn.startswith('ret')
            else:raise ValueError('Original allocator instruction limit')
            assert bool(fault)==kind.startswith('fault-'),(kind,hex(regs.rip))
            expected=machine.snapshot(regs)
            if kind=='base':
                assert expected['gp']==seed['boundary_gp'],(n,expected['gp'],seed['boundary_gp'])
                assert all(host.read(bank['address'],len(bytes.fromhex(bank['after']))).hex()==bank['after'] for bank in seed['banks'])
            offsets=b''.join(struct.pack('<H',pc-ENTRY) for pc in trace).hex()
            rows.append(dict(entry=ENTRY,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,initial_stack=initial_stack,reads=reads,stores=stores,fault=fault,trace_offsets=offsets,expected=expected,flags_mask=defined))
    assert len(instructions)==78,len(instructions)
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_DATUM_ALLOCATE_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=sorted(write_sites-{0x16e743}),rows=rows),separators=(',',':'))+'\n').encode();compressed=gzip.compress(data,mtime=0);assert len(compressed)<=1024*1024,len(compressed);args.fixture.write_bytes(compressed)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),state_helper_sha256=hashlib.sha256(Path(__file__).with_name('weapon_effect_original_cpu_state.py').read_bytes()).hexdigest(),legacy_input_sha256=hashlib.sha256(prior.read_bytes()).hexdigest(),legacy_inputs_replayed=len(seeds),read_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='read' for r in rows),write_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='write' for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),backward_rep_cases=sum(r['kind']=='backward-rep' for r in rows),instruction_map=instructions,fixture_bytes=len(compressed),fixture_sha256=hashlib.sha256(compressed).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],controlled_boundaries=[],physical_xbox_proof=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
