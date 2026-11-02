#!/usr/bin/env python3
"""Physical unchanged TU1.5 effect frame179DF0, pool scan16EA60 and actual lower calls.

Caller-owned raw effect/component banks include empty/sparse bitmaps, salted
identities, linked zero-system definitions, aliases and fault prefixes. The
original16EA60/179ED0/178070/177830 bodies really execute. No instruction,
callee return, game-service result, allocation or admission is supplied. This
is CPU/ABI evidence for these declared inputs, not full effect/output parity.
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
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from original_audio_cpu import OriginalAudioCpu
from oracle_weapon_effect_dispatch_tu15 import STACK, STOP, NONE, pack
from oracle_weapon_effect_state_tu15 import ALIASES, REGS, DEBUG_FLAGS, CWS, CSRS, EXTENDED
from weapon_effect_original_cpu_state import EffectCpuState
from weapon_reference_versions import images

ENTRY=0x179df0
POOL,ROWS,BITS=0x1000000,0x1004000,0x1002000
PARTICLE_POOL,PARTICLES,PARTICLE_BITS=0x1010000,0x1014000,0x1012000
TAGS,DEFINITION,EVENTS,SYSTEMS=0x1020000,0x1022000,0x1024000,0x1028000
BAD,READONLY=0x4000000,0x4100000
STACK_BYTES=1024
CALLEES={0x16ea60:0x179e01,0x179ed0:0x179e56,0x177830:STOP}
RANGES=((0x179df0,0x179ecd),(0x16ea60,0x16ea99),(0x179ed0,0x17a021),
        (0x178070,0x178108),(0x177830,0x1778cd))
EXTENTS=(0,1,2,3,31,32,33,63,64,65,97,129)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _,image,pin=next(x for x in images() if x[0]=='TU1.5')
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    rng=random.Random(ENTRY);rows=[];instruction_map={};steps=0
    recipes=[('base',n) for n in range(144)]
    recipes += [('raw-stride',n) for n in range(7)]
    recipes += [('raw-limit',n) for n in (0xffffffff,0x80000000,0x7fffffff)]
    recipes += [('alias-pool',n) for n in (4,8,12,0x24,0x38,0x44,0x48)]
    recipes += [('alias-rows',n) for n in (4,8,12,0x100)]
    recipes += [(name,n) for n,name in enumerate(('fault-pool','fault-bitmap',
        'fault-rows','fault-head-edge','fault-salt-edge','fault-bitmap-edge',
        'readonly-first-push','readonly-second-push','readonly-call-push',
        'readonly-scan-push','readonly-tail-push','fault-tail-pool'))]
    recipes += [('slot-scan',n) for n in range(144)]
    recipes += [(name,n) for n,name in enumerate(('slot-fault-pool','slot-fault-bitmap',
        'slot-readonly-first','slot-readonly-second','slot-readonly-third','slot-readonly-negative'))]
    with tempfile.TemporaryDirectory(prefix='effect-frame-original-',dir=scratch) as temporary, \
         OriginalAudioCpu(image,Path(temporary)/'original.elf',0x27f,0x1f80,
             extra_declared_regions=[(READONLY-4096,4096),(READONLY+4096,4096)],
             read_only_declared_regions=[(READONLY,4096)],source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0)
        for case,(kind,n) in enumerate(recipes):
            entry=0x16ea60 if kind.startswith('slot-') else ENTRY
            stack=STACK
            if kind in ('readonly-first-push','slot-readonly-first','slot-readonly-negative'):stack=READONLY+128
            elif kind in ('readonly-second-push','slot-readonly-second'):stack=READONLY+4100
            elif kind in ('readonly-call-push','slot-readonly-third'):stack=READONLY+4104
            elif kind=='readonly-scan-push':stack=READONLY+4108
            elif kind=='readonly-tail-push':stack=READONLY+4112
            extent=EXTENTS[n%len(EXTENTS)] if kind in ('base','slot-scan') else 3
            if kind=='readonly-tail-push':extent=0
            elif kind=='raw-limit':extent=n
            mode=(n//len(EXTENTS))%6
            occupied=[] if not extent or mode==0 else sorted(set(
                [0] if mode==1 else [extent-1] if mode==2 else
                [0,extent-1] if mode==3 else [min(1,extent-1),min(31,extent-1),extent-1]
                if mode==4 else [0,min(32,extent-1),extent-1]))
            if kind not in ('base','slot-scan'):occupied=[0] if extent else []
            pool=stack-n if kind=='alias-pool' else POOL
            records=stack-n if kind=='alias-rows' else ROWS
            bitmap=BITS;stride=0x190
            if kind=='raw-stride':stride=(0,2,8,0x18f,0x191,0x80000000,0xfffffe70)[n];occupied=[0,1,2]
            if kind=='fault-rows':records=BAD
            elif kind=='fault-head-edge':records=0x2100000-0xfc
            elif kind=='fault-salt-edge':records=0x2100000-1
            elif kind in ('fault-bitmap','slot-fault-bitmap'):bitmap=BAD
            elif kind=='fault-bitmap-edge':bitmap=0x2100000-2
            regions=[(0x1000000,0x1100000)]
            for at,size in regions:host.write(at,bytes(size))
            host.write(stack-512,rng.randbytes(STACK_BYTES))
            memory=[]
            def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
            word(0x4f720c,BAD if kind=='fault-pool' else pool)
            word(pool+0x24,stride);word(pool+0x38,extent)
            word(pool+0x44,records);word(pool+0x48,bitmap)
            masks=[0]*5
            for index in occupied:masks[index//32]|=1<<(index%32)
            if bitmap==BITS:
                for index,value in enumerate(masks):word(bitmap+index*4,value)
            elif kind=='fault-bitmap-edge':word(bitmap,masks[0],2)
            word(0x51df74,BAD if kind=='fault-tail-pool' else PARTICLE_POOL)
            word(PARTICLE_POOL+0x24,0x54);word(PARTICLE_POOL+0x38,0)
            word(PARTICLE_POOL+0x44,PARTICLES);word(PARTICLE_POOL+0x48,PARTICLE_BITS)
            word(0x4f0414,TAGS);word(TAGS+7*16,0x65666665)
            word(TAGS+7*16+8,DEFINITION);word(DEFINITION+0x18,EVENTS)
            word(EVENTS+0x34,SYSTEMS);word(SYSTEMS+8,0);word(SYSTEMS+0xc,0,2)
            children=(n//6)%3 if kind=='base' else 0
            for position,index in enumerate(occupied):
                salt=(0,1,0x7fff,0x8000,0xffff,0xa123,0x1234,0xfffe)[(n+position)%8]
                if records!=BAD:
                    salt_at=(records+index*stride)&NONE
                    if 0x1000000<=salt_at<0x2100000:
                        word(salt_at,salt,1 if kind=='fault-salt-edge' else 2)
                    if kind not in ('fault-salt-edge','fault-head-edge'):
                        word((records+index*0x190+0xfc)&NONE,
                             (0xb1230000|position*2) if children else NONE)
                for child in range(children):
                    at=PARTICLES+(position*2+child)*0x54
                    word(at+0x10,0x90010007);word(at+0x14,0)
                    word(at+0xe,0,2)
                    word(at+0x38,(0xb1230000|position*2+child+1) if child+1<children else NONE)
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            argument=(0,0x80000000,0x3d088889,0x3f800000,0x7fc12345)[n%5]
            host.write(stack,pack([STOP,argument]));initial_stack=host.read(stack-512,STACK_BYTES).hex()
            gp=[rng.getrandbits(32) for _ in ALIASES];gp[4]=stack
            mmx=case%7==6;depth=0 if mmx else n%7
            fp=dict(cw=CWS[case%len(CWS)],status=0 if mmx else ((n%8)<<11)|((n*0x100)&0x4700)|(n&0x3f),
                mxcsr=CSRS[case%len(CSRS)],stack=[EXTENDED[(n+j)%len(EXTENDED)] for j in range(depth)],
                xmm=[rng.randbytes(16).hex() for _ in range(8)])
            if mmx:fp['mmx']=[rng.randbytes(8).hex()+'ffff' for _ in range(8)]
            if entry==0x16ea60:
                gp[1]=BAD if kind=='slot-fault-pool' else pool
                gp[2]=(0,1,31,32,33,extent-1,extent,extent+1,NONE,0x80000000,0x7fffffff,0x80000001)[n//12] if kind=='slot-scan' else (NONE if kind=='slot-readonly-negative' else 0)
            host.prepare(entry,stack,dict(zip(ALIASES,gp)));cpu=EffectCpuState(host);cpu.load(fp)
            regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5)
            host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            trace=[];stores=[];fault=[];calls=[];boundary={};active=None;defined=NONE&~DEBUG_FLAGS
            def state():
                return dict(gp=[getattr(regs,name)&NONE for name in ALIASES],
                    eflags=regs.eflags&~DEBUG_FLAGS,fp=cpu.floating_point(),
                    stack=host.read(stack-512,STACK_BYTES).hex())
            for iteration in range(16000):
                pc=regs.rip
                if active is not None and pc==active['return_address']:
                    active['returned']=state();active['returned_flags_mask']=defined
                    active=None
                if pc==STOP:break
                if not(iteration==0 and pc==entry) and pc in CALLEES and (not calls or calls[-1].get('returned') is not None):
                    active=dict(target=pc,before=state(),flags_mask=defined,
                        return_address=CALLEES[pc],tail=pc==0x177830)
                    calls.append(active)
                assert any(lo<=pc<hi for lo,hi in RANGES),hex(pc)
                instruction=next(decoder.disasm(image.read(pc,15),pc,count=1))
                raw=bytes(instruction.bytes);assert host.read(pc,len(raw))==raw
                instruction_map[f'{pc:08x}']=raw.hex();trace.append(pc)
                pending=[];operands=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                if instruction.mnemonic in ('push','call'):pending.append(((regs.rsp-4)&NONE,4))
                for operand in instruction.operands:
                    if operand.type!=3:continue
                    at=(reg(instruction.reg_name(operand.mem.base))+
                        reg(instruction.reg_name(operand.mem.index))*operand.mem.scale+operand.mem.disp)&NONE
                    operands.append((at,operand.size,bool(operand.access&2)))
                    if operand.access&2:pending.append((at,operand.size))
                if instruction.mnemonic in ('push','call'):
                    operands.append((((regs.rsp-4)&NONE),4,True))
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
                    info=ctypes.create_string_buffer(128);host.trace(0x4202,0,ctypes.addressof(info))
                    address=struct.unpack_from('<Q',info.raw,16)[0]
                    assert operands,(kind,hex(pc))
                    fault=[dict(instruction=pc,address=address,operand_address=operands[0][0],
                        size=operands[0][1],kind='write' if operands[0][2] else 'read')]
                    if active is not None:active['fault']=True;active['returned']=state();active['returned_flags_mask']=defined
                    break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(kind,hex(pc),status)
                for at,width in pending:
                    assert stack-512<=at and at+width<=stack+512,(kind,hex(pc),hex(at))
                    stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                mnemonic=instruction.mnemonic
                if mnemonic in ('add','sub','cmp'):defined=NONE&~DEBUG_FLAGS
                elif mnemonic in ('test','and','xor','or'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
                elif mnemonic in ('sar','shr','shl'):defined=(NONE&~DEBUG_FLAGS)&~0x810
                elif mnemonic in ('inc','dec'):defined=(NONE&~DEBUG_FLAGS&~1)|(defined&1)
                assert cpu.floating_point()==fp,'Integer owner or actual bounded lower call changed FP file'
            else:
                boundary=dict(kind='instruction-budget',instructions=16000,next_instruction=regs.rip)
                if active is not None:
                    active['interrupted']=True;active['returned']=state();active['returned_flags_mask']=defined
            if not fault and not boundary:assert regs.rip==STOP and regs.rsp==stack+(4 if entry==0x16ea60 else 8)
            if kind in ('base','slot-scan'):assert not fault and not boundary,(kind,n,fault,boundary)
            rows.append(dict(kind=kind,n=n,entry=entry,regions=regions,memory=memory,
                stack_base=stack,readonly=[READONLY,4096],initial_stack=initial_stack,
                gp=gp,eflags=initial_flags,fp=fp,calls=calls,stores=stores,
                trace=b''.join(struct.pack('<I',pc) for pc in trace).hex(),fault=fault,boundary=boundary,
                expected=state(),flags_mask=defined))
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_FRAME_REFERENCE_V2',
        original_sha256=pin,scope=__doc__,rows=rows),separators=(',',':'))+'\n').encode()
    compressed=gzip.compress(data,mtime=0)
    assert len(data)<=16*1024*1024 and len(compressed)<=1024*1024
    args.fixture.write_bytes(compressed)
    report=dict(passed=True,scope=__doc__,cases=len(rows),hardware_instructions=steps,
        original_instruction_sites=len(instruction_map),instruction_map=instruction_map,
        original_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        controlled_lower_calls=0,instruction_patches=[],supplied_game_services=[],
        actual_lower_calls={hex(target):sum(c['target']==target for r in rows for c in r['calls']) for target in CALLEES},
        read_stops=sum(bool(r['fault']) and r['fault'][0]['kind']=='read' for r in rows),
        bounded_observation_prefixes=sum(bool(r['boundary']) for r in rows),
        returned_frames=sum(not r['fault'] and not r['boundary'] for r in rows),
        write_stops=sum(bool(r['fault']) and r['fault'][0]['kind']=='write' for r in rows),
        fault_instructions={hex(pc):sum(bool(r['fault']) and r['fault'][0]['instruction']==pc for r in rows) for pc in sorted({r['fault'][0]['instruction'] for r in rows if r['fault']})},
        raw_bytes=len(data),raw_sha256=hashlib.sha256(data).hexdigest(),fixture_bytes=len(compressed),
        fixture_sha256=hashlib.sha256(compressed).hexdigest(),scratch_owner=str(scratch),
        backend='Physical Linux i386 PTRACE_SINGLESTEP',physical_xbox_proof=False,full_user_goal_complete=False)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('scope','instruction_map')}))

if __name__=='__main__':main()
