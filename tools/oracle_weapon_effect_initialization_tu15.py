#!/usr/bin/env python3
"""Unchanged TU1.5 17B0D0/17E5E0 on a physical Linux i386 CPU child.

These complete leaves have no supplied callees or instruction patches. The
reference includes ordered operand reads, stores, stopped accesses, complete
GP/XMM/defined flags, live x87/MMX and source controls. This is owner evidence,
not a rendered effect or a physical Xbox capture. Use tools/run_test.py.
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
from weapon_reference_versions import images

BODY,SOURCE,AUX=0x1000000,0x1004000,0x1008000
END,BAD=0x2100000,0x4000000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _,image,pin=next(x for x in images() if x[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    rng=random.Random(0x17e5e0);rows=[];instructions={};write_sites=set();steps=0
    def getfp(host,depth,mmx):
        buffer=ctypes.create_string_buffer(512);host.trace(14,0,ctypes.addressof(buffer));raw=buffer.raw
        result=dict(cw=struct.unpack_from('<H',raw,0)[0],status=struct.unpack_from('<H',raw,2)[0],mxcsr=struct.unpack_from('<I',raw,24)[0],
                    stack=[raw[32+i*16:42+i*16].hex() for i in range(depth)],xmm=[raw[160+i*16:176+i*16].hex() for i in range(8)])
        if mmx:
            assert raw[4]==255 and not result['status']&0x3800,'Original MMX tags/TOP changed'
            result['mmx']=[raw[32+i*16:42+i*16].hex() for i in range(8)]
        return result
    def putfp(host,fp):
        buffer=ctypes.create_string_buffer(512);host.trace(14,0,ctypes.addressof(buffer));raw=bytearray(buffer.raw);top=(fp['status']>>11)&7
        struct.pack_into('<HHH',raw,0,fp['cw'],fp['status'],sum(1<<((top+i)&7) for i in range(len(fp['stack']))))
        struct.pack_into('<I',raw,24,fp['mxcsr'])
        for i,value in enumerate(fp['stack']):raw[32+i*16:42+i*16]=bytes.fromhex(value)
        if 'mmx' in fp:
            assert not fp['stack'] and top==0
            struct.pack_into('<H',raw,4,255)
            for i,value in enumerate(fp['mmx']):raw[32+i*16:42+i*16]=bytes.fromhex(value)
        for i,value in enumerate(fp['xmm']):raw[160+i*16:176+i*16]=bytes.fromhex(value)
        buffer=ctypes.create_string_buffer(bytes(raw),512);host.trace(15,0,ctypes.addressof(buffer))
    def state(host,regs,depth,mmx):
        return dict(gp=[getattr(regs,n)&NONE for n in ALIASES],eflags=regs.eflags&~DEBUG_FLAGS,fp=getfp(host,depth,mmx),stack=host.read(STACK-96,160).hex())
    recipes=[(0x17b0d0,n,'base') for n in range(64)]
    recipes += [(0x17b0d0,64+n,'alias-vector') for n in range(16)]
    recipes += [(0x17b0d0,80+n,kind) for n,kind in enumerate(['fault-input-vector','fault-default-vector','fault-vector-second','fault-vector-third','fault-second-input','fault-second-vector-second','fault-second-vector-third','fault-argument'])]
    recipes += [(0x17b0d0,88+n,'active-mmx') for n in range(4)]
    recipes += [(0x17e5e0,n,'base') for n in range(64)]
    recipes += [(0x17e5e0,64+n,'alias-fill') for n in range(4)]
    recipes += [(0x17e5e0,68+n,kind) for n,kind in enumerate(['fault-fill-0','fault-fill-1','fault-fill-16','fault-fill-31','fault-fill-bad','fault-return-bad','fault-return-end','fault-return-far'])]
    recipes += [(0x17e5e0,76+n,'active-mmx') for n in range(4)]
    with OriginalAudioCpu(image,scratch/'effect-initialization-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0)
        for entry,n,kind in recipes:
            gp=[rng.getrandbits(32) for _ in ALIASES];gp[4]=STACK
            memory=[]
            def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
            for at in range(BODY,BODY+144,4):word(at,rng.getrandbits(32))
            for base in [SOURCE,AUX]:
                for off in range(0,12,4):word(base+off,rng.getrandbits(32))
            word(0x474c70,SOURCE)
            if entry==0x17b0d0:
                gp[0]=BODY;gp[1]=SOURCE if n&1 else 0;gp[2]=AUX if n&2 else 0
                if kind=='alias-vector':
                    offsets=[0x20,0x24,0x28,0x2c,0x64,0x68,0x6c,0x70]
                    gp[1]=BODY+offsets[n%8] if n%4 else 0
                    word(0x474c70,BODY+offsets[n%8]);gp[2]=BODY+[0x38,0x3c,0x40,0x44][n%4] if n%5 else 0
                elif kind=='fault-input-vector':gp[1]=BAD
                elif kind=='fault-default-vector':gp[1]=0;word(0x474c70,BAD)
                elif kind in ('fault-vector-second','fault-vector-third'):
                    gp[1]=END-(4 if kind.endswith('second') else 8)
                    for at in range(gp[1],END,4):word(at,rng.getrandbits(32))
                elif kind=='fault-second-input':gp[1]=SOURCE;gp[2]=BAD
                elif kind in ('fault-second-vector-second','fault-second-vector-third'):
                    gp[1]=SOURCE;gp[2]=END-(4 if kind.endswith('second') else 8)
                    for at in range(gp[2],END,4):word(at,rng.getrandbits(32))
                elif kind=='fault-argument':gp[4]=END-4;word(END-4,STOP)
            else:
                gp[0]=[NONE,0,rng.getrandbits(32),0x80010002][n%4];gp[1]=BODY+n%4
                if kind=='alias-fill':gp[1]=[STACK-192,STACK+16,0x474c70,BODY+64][n-64]
                elif kind.startswith('fault-fill-'):
                    gp[1]=BAD if kind.endswith('bad') else END-int(kind.rsplit('-',1)[1])*4
                elif kind.startswith('fault-return-'):gp[4]={'bad':BAD,'end':END,'far':BAD+4096}[kind.rsplit('-',1)[1]]
                for at in range(gp[1],gp[1]+128,4):
                    if 0x1000000<=at<END or 0x10000<=at<0x810000:word(at,rng.getrandbits(32))
            initial_stack=rng.randbytes(160);host.write(STACK-96,initial_stack);host.write(STACK,pack([STOP,rng.getrandbits(32)]))
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            initial_stack=host.read(STACK-96,160).hex()
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=CSRS[n%len(CSRS)],
                    stack=[EXTENDED[2],EXTENDED[3]][:n%3],xmm=[rng.randbytes(16).hex() for _ in range(8)])
            fp['xmm'][0]=(struct.pack('<I',FLOATS[n%len(FLOATS)])+bytes.fromhex(fp['xmm'][0])[4:]).hex()
            mmx=kind=='active-mmx'
            if mmx:fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(entry,gp[4],dict(zip(ALIASES,gp)));putfp(host,fp)
            regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            stores=[];reads=[];trace=[];fault=[];depth=len(fp['stack']);defined=NONE&~DEBUG_FLAGS
            for _ in range(100):
                pc=regs.rip
                if pc==STOP:break
                assert entry<=pc<({0x17b0d0:0x17b12b,0x17e5e0:0x17e640}[entry]),hex(pc)
                ins=next(decoder.disasm(image.read(pc,15),pc,count=1));raw=bytes(ins.bytes);assert host.read(pc,len(raw))==raw
                instructions[f'{pc:08x}']=raw.hex();trace.append(pc);pending=[];operands=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                if ins.mnemonic=='push':pending.append(((regs.rsp-4)&NONE,4));write_sites.add(pc)
                if ins.mnemonic=='pop' or ins.mnemonic.startswith('ret'):
                    reads.append([pc,regs.rsp&NONE,4]);operands.append((regs.rsp&NONE,4,'read'))
                for op in ins.operands:
                    if op.type!=3 or ins.mnemonic=='lea':continue
                    at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&NONE
                    # CMOV's memory operand is an eager source read even when
                    # its destination is retained by the condition.
                    if op.access&1:reads.append([pc,at,op.size]);operands.append((at,op.size,'read'))
                    if op.access&2:pending.append((at,op.size));write_sites.add(pc);operands.append((at,op.size,'write'))
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
                    assert kind.startswith('fault-') and operands,(kind,hex(pc),status)
                    at,width,access=operands[-1];fault=[dict(instruction=pc,address=at,size=width,access=access)];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(hex(pc),status)
                for at,width in pending:stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                if ins.mnemonic=='add':defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic=='test':defined=(NONE&~DEBUG_FLAGS)&~0x10
            else:raise ValueError('Original initialization instruction limit')
            assert bool(fault)==kind.startswith('fault-'),(kind,hex(regs.rip))
            if not fault:assert regs.rip==STOP and regs.rsp==STACK+(8 if entry==0x17b0d0 else 4)
            rows.append(dict(entry=entry,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,initial_stack=initial_stack,
                reads=reads,stores=stores,fault=fault,trace_offsets=b''.join(struct.pack('<H',pc-entry) for pc in trace).hex(),
                expected=state(host,regs,depth,mmx),flags_mask=defined))
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_INITIALIZATION_REFERENCE_V1',original_sha256=pin,scope=__doc__,write_only_sites=sorted(write_sites),rows=rows),separators=(',',':'))+'\n').encode()
    assert len(data)<=1024*1024,len(data);args.fixture.write_bytes(data)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        original_instruction_sites=len(instructions),read_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='read' for r in rows),write_stops=sum(bool(r['fault']) and r['fault'][0]['access']=='write' for r in rows),
        alias_cases=sum(r['kind'].startswith('alias-') for r in rows),active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),
        instruction_map=instructions,fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),scratch_owner=str(scratch),
        backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],supplied_callees=[],physical_xbox_proof=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
