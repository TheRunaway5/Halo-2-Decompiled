#!/usr/bin/env python3
"""Unchanged TU1.5 18BE00 local-sound decision, with no lower boundaries.

Physical i386 reference covers every object type byte, sound flag exclusions,
NONE datums, arbitrary upper registers, live FP/MMX and genuine memory faults.
This is owner CPU evidence; it does not certify speaker output or Xbox DSP.
Run through tools/run_test.py and keep only the compressed reference/report.
"""
import argparse
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
from oracle_weapon_effect_dispatch_tu15 import STACK,STOP,NONE
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,EXTENDED
from weapon_effect_original_cpu_state import EffectCpuState
from weapon_reference_versions import images

BASE,END,BAD,ENTRY=0x1000000,0x2100000,0x6000000,0x18be00
TAGS,DATA,POOL,HEADERS=[BASE+0x3000*i for i in range(1,5)]


class State(EffectCpuState):
    def snapshot(self,regs):
        result=super().snapshot(regs);result['stack']=self.host.read(STACK-64,128).hex();return result


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);_,image,pin=next(r for r in images() if r[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    configs=[('type-byte',dict(type=n)) for n in range(256)]
    configs += [('sound-flag-excluded',dict(type=n,flags=(n*7)&~64)) for n in range(32)]
    configs += [('NONE-sound',dict(sound=NONE)) for _ in range(8)]
    configs += [('NONE-owner',dict(owner=NONE)) for _ in range(8)]
    configs += [('active-mmx',dict(type=n)) for n in (0,2,34,255)]
    configs += [(kind,{}) for kind in ('fault-tag-table','fault-sound-flags','fault-object-pool','fault-object-header','fault-first-push','fault-second-push','fault-return','fault-NONE-return')]
    rng=random.Random(ENTRY);rows=[];instructions={};write_sites=set();steps=0
    with OriginalAudioCpu(image,scratch/'sound-local-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0);machine=State(host)
        for n,(kind,settings) in enumerate(configs):
            gp=[rng.getrandbits(32) for _ in ALIASES];gp[4]=STACK+(n%4)*4;gp[2]=settings.get('sound',((n*13&65535)<<16)|3);gp[1]=settings.get('owner',((n*19&65535)<<16)|5)
            memory=[];initial_stack=rng.randbytes(128).hex();blobs=[[STACK-64,initial_stack]]
            def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
            word(0x4f0414,TAGS);word(TAGS+3*16+8,DATA);word(DATA,settings.get('flags',64),1)
            word(0x4ecbd0,POOL);word(POOL+0x44,HEADERS);word(HEADERS+5*12+3,settings.get('type',2),1)
            if kind=='fault-tag-table':word(0x4f0414,BAD-3*16-8)
            elif kind=='fault-sound-flags':word(TAGS+3*16+8,BAD)
            elif kind=='fault-object-pool':word(0x4ecbd0,BAD-0x44)
            elif kind=='fault-object-header':word(POOL+0x44,BAD-5*12-3)
            elif kind in ('fault-first-push','fault-second-push'):gp[4]=BASE+(4 if kind=='fault-second-push' else 0);blobs.append([BASE,rng.randbytes(4096).hex()])
            elif kind in ('fault-return','fault-NONE-return'):
                gp[4]=END;blobs.append([END-4096,rng.randbytes(4096).hex()])
                if kind=='fault-NONE-return':gp[2]=NONE
            if gp[4]+4<=END:word(gp[4],STOP)
            for at,raw in blobs:host.write(at,bytes.fromhex(raw))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4],stack=[EXTENDED[(n+i)%len(EXTENDED)] for i in range((0,1,3,8)[n%4])],xmm=[rng.randbytes(16).hex() for _ in range(8)])
            if kind=='active-mmx':fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(ENTRY,gp[4],dict(zip(ALIASES,gp)));machine.load(fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];reads=[];stores=[];fault=[];defined=NONE&~DEBUG_FLAGS
            for _ in range(100):
                pc=regs.rip
                if pc==STOP:break
                assert ENTRY<=pc<=0x18be4c,(kind,hex(pc))
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
                    assert kind.startswith('fault-') and (operands or pending),(kind,hex(pc),status)
                    access='read' if operands else 'write';at,width=(operands if operands else pending)[-1];fault=[dict(instruction=pc,address=at,size=width,access=access)];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(kind,hex(pc),status)
                for at,width in pending:stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                mn=ins.mnemonic
                if mn=='cmp':defined=NONE&~DEBUG_FLAGS
                elif mn in ('and','or','xor','test'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif mn=='shl':
                    count=(reg('ecx') if ins.operands[1].type==1 else ins.operands[1].imm)&31
                    if count:defined=(NONE&~DEBUG_FLAGS)&~(0x10|(0x800 if count>1 else 0))
            else:raise ValueError('Original local-sound instruction limit')
            assert bool(fault)==kind.startswith('fault-'),(kind,hex(regs.rip))
            offsets=b''.join(struct.pack('<H',pc-ENTRY) for pc in trace).hex()
            rows.append(dict(entry=ENTRY,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,blobs=blobs,initial_stack=initial_stack,reads=reads,stores=stores,fault=fault,trace_offsets=offsets,expected=machine.snapshot(regs),flags_mask=defined))
    expected_sites={i.address for i in decoder.disasm(image.read(ENTRY,0x4d),ENTRY)}
    assert {int(pc,16) for pc in instructions}==expected_sites,(len(instructions),len(expected_sites))
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_SOUND_LOCAL_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=sorted(write_sites),rows=rows),separators=(',',':'))+'\n').encode();compressed=gzip.compress(data,mtime=0);assert len(compressed)<=1024*1024;args.fixture.write_bytes(compressed)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,original_instruction_sites=len(instructions),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),state_helper_sha256=hashlib.sha256(Path(__file__).with_name('weapon_effect_original_cpu_state.py').read_bytes()).hexdigest(),type_byte_values=256,read_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='read' for r in rows),write_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='write' for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),instruction_map=instructions,fixture_bytes=len(compressed),fixture_sha256=hashlib.sha256(compressed).hexdigest(),scratch_owner=str(scratch),backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],controlled_boundaries=[],physical_xbox_proof=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
