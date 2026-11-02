#!/usr/bin/env python3
"""Unchanged TU1.5 178E30 effect creation on a physical i386 CPU child.

The fifteen lower owners are declared, arbitrary ABI/memory controls. They
are not RNG, allocation, attachment, audio, or rendered-effect evidence. No
original instruction is changed. Keep generated execution files in the owned
run_test.py scratch directory and retain only this bounded expected-data input.
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
from oracle_weapon_effect_state_tu15 import ALIASES,REGS,DEBUG_FLAGS,CWS,CSRS,EXTENDED
from weapon_reference_versions import images

POOL,BODY,TAGS,DEFINITION,REQUEST,SETTINGS,OBJECT_POOL,OBJECT_ROWS,OBJECT,OBJECT_DEFINITION,TINT=[0x1000000+i*0x4000 for i in range(11)]
BAD=0x4000000
SERVICES={0x1790a0:0,0x17afb0:8,0x165b00:0,0x169112:4,0xbcdf0:4,
          0x169151:4,0x17b0d0:4,0x178de0:0,0x17e5e0:0,0x17b1f0:16,
          0x17b880:0,0x17e460:8,0x17a1a0:4,0x17c6e0:4,0x16e940:0}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _,image,pin=next(x for x in images() if x[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    rng=random.Random(0x178e30);rows=[];instructions={};steps=0
    def getfp(host,depth,mmx=False):
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
    def state(host,regs,depth,mmx=False):
        return dict(gp=[getattr(regs,n)&NONE for n in ALIASES],eflags=regs.eflags&~DEBUG_FLAGS,fp=getfp(host,depth,mmx),stack=host.read(STACK-96,160).hex())
    with OriginalAudioCpu(image,scratch/'effect-creation-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0)
        recipes=[(n,'base') for n in range(48)]
        recipes += [(48+n,kind) for n,kind in enumerate(['fault-request','fault-pool','fault-rows','fault-tags','fault-settings','fault-object-pool','fault-object-body','fault-tint'])]
        recipes += [(56+n,'alias-position') for n in range(4)]
        recipes += [(60+n,'active-mmx') for n in range(4)]
        for n,kind in recipes:
            request=REQUEST if kind!='alias-position' else BODY+[8,12,16,20][n-56]
            gp=[rng.getrandbits(32) for _ in ALIASES];gp[4]=STACK
            memory=[]
            def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
            datum=0x80010002;flags=n%8;definition_flags=((n>>1)&1)*2|((n>>2)&1)*8|((n>>3)&1)*16
            word(0x4f720c,POOL);word(POOL+0x44,BODY-2*0x190)
            word(BODY,0x8001,2);word(BODY+2,(n%4)*2|((n>>2)&1)*0x200,2);word(BODY+4,0xdead0007)
            word(BODY+0x48,NONE);word(BODY+0x58,0xbeef0005)
            word(0x4f0414,TAGS);word(TAGS+7*16+8,DEFINITION);word(DEFINITION,definition_flags)
            word(0x4f3218,SETTINGS);word(SETTINGS+8,2 if n%3 else 1)
            word(0x4ecbd0,OBJECT_POOL);word(OBJECT_POOL+0x44,OBJECT_ROWS);word(OBJECT_ROWS+3*12+8,OBJECT)
            word(OBJECT,0x12340008);word(TAGS+8*16+8,OBJECT_DEFINITION);word(OBJECT_DEFINITION+2,0x2000 if n&8 else 0,2)
            word(0x51df7c,1 if n&16 else 0,1);word(0x51df78,rng.getrandbits(32));word(TINT,rng.getrandbits(32));word(TINT+4,rng.getrandbits(32))
            # All fields are caller-owned bits, not invented gameplay values.
            for off in range(0,0x60,4):word(request+off,rng.getrandbits(32))
            if kind=='alias-position':flags=4
            word(request,flags);word(request+4,0xabcd0007);word(request+8,0x12340003 if n%5 else NONE)
            word(request+0x50,TINT if n&1 else 0)
            if kind=='fault-request':request=BAD
            elif kind=='fault-pool':flags=4;word(REQUEST,flags);word(0x4f720c,BAD)
            elif kind=='fault-rows':flags=4;word(REQUEST,flags);word(POOL+0x44,BAD-2*0x190)
            elif kind=='fault-tags':flags=4;word(REQUEST,flags);word(0x4f0414,BAD)
            elif kind=='fault-settings':flags=4;word(REQUEST,flags);word(0x4f3218,BAD)
            elif kind=='fault-object-pool':flags=4;word(REQUEST,flags);word(SETTINGS+8,2);word(REQUEST+8,0x12340003);word(0x4ecbd0,BAD)
            elif kind=='fault-object-body':flags=4;word(REQUEST,flags);word(SETTINGS+8,2);word(REQUEST+8,0x12340003);word(OBJECT_ROWS+3*12+8,BAD)
            elif kind=='fault-tint':flags=4;word(REQUEST,flags);word(REQUEST+0x50,BAD)
            initial_stack=rng.randbytes(160);host.write(STACK-96,initial_stack);host.write(STACK,pack([STOP,request]));initial_stack=host.read(STACK-96,160).hex()
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|(n&0x3f),mxcsr=CSRS[n%len(CSRS)],
                    stack=[EXTENDED[2],EXTENDED[3]][:n%3],xmm=[rng.randbytes(16).hex() for _ in range(8)])
            mmx=kind=='active-mmx'
            if mmx:
                fp.update(status=n&0x3f,stack=[],mmx=[struct.pack('<QH',0x5a00000000000000+n*31+i,0xffff).hex() for i in range(8)])
            host.prepare(0x178e30,STACK,dict(zip(ALIASES,gp)));putfp(host,fp)
            regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            calls=[];stores=[];trace=[];fault=[];depth=len(fp['stack']);defined=NONE&~DEBUG_FLAGS
            for _ in range(1000):
                pc=regs.rip
                if pc==STOP:break
                if pc in SERVICES:
                    before=state(host,regs,depth,mmx);ret=int.from_bytes(host.read(regs.rsp,4),'little')
                    returned=dict(gp=before['gp'].copy(),eflags=0x202|(rng.getrandbits(12)&0xcd5),fp=dict(before['fp']))
                    for i in range(3):returned['gp'][i]=rng.getrandbits(32)
                    returned['fp']['xmm']=[rng.randbytes(16).hex() for _ in range(8)]
                    returned['fp']['cw']=CWS[(n+len(calls))%len(CWS)];returned['fp']['mxcsr']=CSRS[(n+len(calls))%len(CSRS)]
                    writes=[]
                    if pc==0x1790a0:returned['gp'][0]=(returned['gp'][0]&0xffffff00)|(0 if kind=='base' and n%16==0 else 1)
                    elif pc==0x17afb0:returned['gp'][0]=NONE if kind=='base' and n%16==1 else datum
                    elif pc==0x165b00:returned['gp'][0]=(returned['gp'][0]&0xffffff00)|(1 if kind.startswith('fault-object') or n&2 else 0)
                    elif pc==0x169112:returned['gp'][0]=NONE if n%4==1 else 0xcdef0005
                    elif pc==0xbcdf0:returned['gp'][0]=OBJECT if n&8 else 0
                    elif pc in (0x169151,0x178de0):returned['gp'][0]=(returned['gp'][0]&0xffffff00)|((n>>2)&1)
                    elif pc==0x16e940:returned['gp'][0]=BODY if n&16 else 0
                    elif pc==0x17a1a0:writes=[[BODY+2,(n%4)*2|((n>>2)&1)*0x200,2]]
                    for at,value,width in writes:host.write(at,value.to_bytes(width,'little'))
                    returned['gp'][4]+=4+SERVICES[pc];putfp(host,returned['fp'])
                    for name,value in zip(ALIASES,returned['gp']):setattr(regs,name,value)
                    regs.rip=ret;regs.eflags=returned['eflags'];host.trace(13,0,ctypes.addressof(regs))
                    calls.append(dict(target=pc,before=before,flags_mask=defined,returned=returned,writes=writes));defined=NONE&~DEBUG_FLAGS;continue
                ins=next(decoder.disasm(image.read(pc,15),pc,count=1));raw=bytes(ins.bytes)
                assert 0x178e30<=pc<=0x17909b,hex(pc)
                assert host.read(pc,len(raw))==raw,(hex(pc),'changed original')
                instructions[f'{pc:08x}']=raw.hex();trace.append(pc);pending=[];operands=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                if ins.mnemonic in ('push','call'):pending.append(((regs.rsp-4)&NONE,4))
                for op in ins.operands:
                    if op.type==3:
                        at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&NONE;operands.append((at,op.size))
                        if op.access&2:pending.append((at,op.size))
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
                    assert kind.startswith('fault-') and operands,(kind,hex(pc),status)
                    fault=[dict(instruction=pc,address=operands[0][0],size=operands[0][1])];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(hex(pc),status)
                for at,width in pending:
                    assert STACK-96<=at<STACK+64 or BODY<=at<BODY+0x190 or at==0x51df78,hex(at)
                    stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                if ins.mnemonic in ('cmp','add','sub'):defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic in ('test','xor','and','or'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
                elif ins.mnemonic in ('shr','shl'):defined=(NONE&~DEBUG_FLAGS)&~(0x10 if ins.operands[1].imm==1 else 0x810)
            else:raise ValueError('Original creation instruction limit')
            assert bool(fault)==kind.startswith('fault-'),(kind,hex(regs.rip))
            if not fault:assert regs.rip==STOP and regs.rsp==STACK+8
            rows.append(dict(entry=0x178e30,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,initial_stack=initial_stack,
                calls=calls,stores=stores,fault=fault,trace_offsets=b''.join(struct.pack('<H',pc-0x178e30) for pc in trace).hex(),
                expected=state(host,regs,depth,mmx),flags_mask=defined))
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_CREATION_REFERENCE_V1',original_sha256=pin,scope=__doc__,rows=rows),separators=(',',':'))+'\n').encode()
    assert len(data)<=1024*1024,len(data)
    args.fixture.write_bytes(data)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,controlled_lower_calls=sum(len(r['calls']) for r in rows),
        generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        read_stops=sum(bool(r['fault']) for r in rows),alias_cases=sum(r['kind'].startswith('alias-') for r in rows),
        active_mmx_cases=sum(r['kind']=='active-mmx' for r in rows),
        instruction_map=instructions,fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),scratch_owner=str(scratch),
        backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],controlled_boundaries={hex(k):v for k,v in SERVICES.items()},physical_xbox_proof=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
