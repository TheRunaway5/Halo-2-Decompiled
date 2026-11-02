#!/usr/bin/env python3
"""Physical unchanged TU1.5 17B130/17DC60 with declared lower ABI controls.

Only 17AAB0 and259B0 are supplied external boundaries, with explicit arbitrary
GP/XMM/flags/FP results. No original instruction is patched. Hardware owns all
stores, complete caller/callee states and traces, including SSE/x87 status.
This is owner-component evidence, not actual RNG allocation or rendered output.
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
from weapon_reference_versions import images

POOL,BODY,TAGS,DEFINITION,EVENTS,COMPONENT_POOL,COMPONENTS=[0x1000000+i*0x4000 for i in range(7)]
BAD=0x4000000
ALIASES=('rax','rcx','rdx','rbx','rsp','rbp','rsi','rdi')
REGS=dict(zip(('eax','ecx','edx','ebx','esp','ebp','esi','edi'),ALIASES))
SERVICES={0x17aab0:0,0x259b0:8}
DEBUG_FLAGS=0x10100 # PTRACE TF/RF do not belong to the caller's logical flags.
FLOATS=[0,0x80000000,1,0x80000001,0x3f800000,0x3f000000,0xbf800000,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0x7f7fffff]
CWS=[0x7f,0x47f,0x87f,0xc7f,0x27f,0x67f,0xa7f,0xe7f,0x37f,0x77f,0xb7f,0xf7f]
CSRS=[0x1f80,0x3f80,0x5f80,0x7f80,0x9fc0,0xdfc0,0x1fc0,0xdf80]


def extended(significand,exponent):return struct.pack('<QH',significand,exponent).hex()
EXTENDED=[extended(0,0),extended(0,0x8000),extended(1<<63,0x3fff),extended(1<<63,0xbfff),
          extended(0xaaaaaaaaaaaaaaab,0x3ffd),extended((1<<63)+(1<<39)+(1<<15),0x3fff),
          extended(1<<63,0x3f69),extended(0xffffff8000000000,0x407e),extended(1<<63,0x7fff),
          extended(0xc000000012345678,0x7fff),extended(0x8000000012345678,0x7fff)]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _,image,pin=next(x for x in images() if x[0]=='TU1.5');decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    rng=random.Random(0x17b130);rows=[];instructions={};steps=0
    def getfp(host,depth):
        buffer=ctypes.create_string_buffer(512);host.trace(14,0,ctypes.addressof(buffer));raw=buffer.raw
        return dict(cw=struct.unpack_from('<H',raw,0)[0],status=struct.unpack_from('<H',raw,2)[0],mxcsr=struct.unpack_from('<I',raw,24)[0],
                    stack=[raw[32+i*16:42+i*16].hex() for i in range(depth)],xmm=[raw[160+i*16:176+i*16].hex() for i in range(8)])
    def putfp(host,fp):
        buffer=ctypes.create_string_buffer(512);host.trace(14,0,ctypes.addressof(buffer));raw=bytearray(buffer.raw);top=(fp['status']>>11)&7
        struct.pack_into('<HHH',raw,0,fp['cw'],fp['status'],sum(1<<((top+i)&7) for i in range(len(fp['stack']))))
        struct.pack_into('<I',raw,24,fp['mxcsr'])
        for i,value in enumerate(fp['stack']):raw[32+i*16:42+i*16]=bytes.fromhex(value)
        for i,value in enumerate(fp['xmm']):raw[160+i*16:176+i*16]=bytes.fromhex(value)
        buffer=ctypes.create_string_buffer(bytes(raw),512);host.trace(15,0,ctypes.addressof(buffer))
    def state(host,regs,depth):
        return dict(gp=[getattr(regs,n)&NONE for n in ALIASES],eflags=regs.eflags&~DEBUG_FLAGS,
                    fp=getfp(host,depth),stack=host.read(STACK-64,128).hex())
    with OriginalAudioCpu(image,scratch/'effect-state-original.elf',0x27f,0x1f80,source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0)
        recipes=[(entry,n,'base') for entry in [0x17b130,0x17dc60] for n in range(128 if entry==0x17b130 else 64)]
        recipes += [(0x17b130,128+n,kind) for n,kind in enumerate(['fault-pool','fault-body','fault-negative-tag','fault-definition','fault-events','fault-max','fault-table','fault-component-pool'])]
        recipes += [(0x17dc60,n,'alias-duration-flags') for n in range(4)]
        recipes += [(0x17dc60,68+n,kind) for n,kind in enumerate(['fault-head','fault-component-pool','fault-component-rows','fault-current'])]
        for entry,n,recipe in recipes:
            salt=[0x1234,0x8001,0xffff,0x7fff][n%4];handle=(salt<<16)|2;selector=n%2
            gp=[rng.getrandbits(32) for _ in ALIASES];gp[4]=STACK;gp[2]=handle if entry==0x17b130 else BODY;gp[7]=(gp[7]&0xffff0000)|selector
            memory=[]
            def word(at,value,width=4):memory.append([at,value&((1<<(width*8))-1),width])
            word(0x4f720c,POOL);word(POOL+0x38,8);word(POOL+0x24,0x190);word(POOL+0x44,BODY-2*0x190)
            word(BODY,salt,2);word(BODY+2,rng.randrange(256),1);word(BODY+4,0x80010007)
            word(0x4f0414,TAGS);word(TAGS+7*16+8,DEFINITION);word(DEFINITION+0x14,2);word(DEFINITION+0x18,EVENTS)
            lower=FLOATS[n%len(FLOATS)];upper=lower if n%4==0 else FLOATS[(n+3)%len(FLOATS)]
            if n%16==1:lower,upper=0,0x80000000
            word(EVENTS+selector*0x38+8,lower);word(EVENTS+selector*0x38+12,upper)
            current=[0,1,0x8001,0xffff][n%4] if entry==0x17dc60 else selector
            previous=[0xffff,2,0x8000,1][n%4]
            word(BODY+0x5c,current,2);word(BODY+0x5e,previous,2);word(BODY+0x60,rng.getrandbits(32));word(BODY+0x64,FLOATS[(n+2)%len(FLOATS)])
            word(0x51df74,COMPONENT_POOL);word(COMPONENT_POOL+0x44,COMPONENTS);word(0x46a0d0,int.from_bytes(image.read(0x46a0d0,4),'little'))
            length=n%4;nodes=[3,1,2];word(BODY+0xfc,NONE if not length else 0xdead0003)
            for i in range(length):
                at=COMPONENTS+nodes[i]*0x54;index=[current,previous,7][i]
                word(at+0x14,(index if index<0x8000 else index-0x10000)&NONE);word(at+0xc,rng.randrange(256),1)
                word(at+4,rng.getrandbits(32));word(at+8,rng.getrandbits(32));word(at+0x38,NONE if i+1==length else 0xbeef0000|nodes[i+1])
            kind='valid'
            if entry==0x17b130 and n<16:
                kind=['NONE','high-index','negative-count','empty-salt','wrong-salt','negative-event','outside-event','valid'][n%8]
                if kind=='NONE':gp[2]=NONE
                elif kind=='high-index':word(POOL+0x38,2)
                elif kind=='negative-count':word(POOL+0x38,0x80000000)
                elif kind=='empty-salt':word(BODY,0,2)
                elif kind=='wrong-salt':word(BODY,salt^1,2)
                elif kind=='negative-event':gp[7]=(gp[7]&0xffff0000)|0xffff
                elif kind=='outside-event':gp[7]=(gp[7]&0xffff0000)|2
            if recipe!='base':
                kind=recipe
                if recipe=='fault-pool':word(0x4f720c,BAD)
                elif recipe=='fault-body':word(POOL+0x44,BAD)
                elif recipe=='fault-negative-tag':word(0x4f0414,BAD);gp[7]|=0xffff
                elif recipe=='fault-definition':word(TAGS+7*16+8,BAD)
                elif recipe=='fault-events':word(DEFINITION+0x18,BAD-selector*0x38)
                elif recipe=='fault-max':
                    word(DEFINITION+0x18,0x2100000-12-selector*0x38);word(0x2100000-4,0x7f812345)
                elif recipe=='fault-table':
                    word(0x4f720c,0x2100000-0x40);word(0x2100000-8,8);word(0x2100000-0x1c,0x190)
                elif recipe=='fault-component-pool':
                    word(BODY+0xfc,0xdead0003);word(0x51df74,BAD)
                    word(EVENTS+selector*0x38+8,0x3f800000);word(EVENTS+selector*0x38+12,0x3f800000)
                elif recipe=='fault-head':gp[2]=BAD
                elif recipe=='fault-component-rows':word(BODY+0xfc,0xdead0003);word(COMPONENT_POOL+0x44,BAD)
                elif recipe=='fault-current':
                    gp[2]=0x1000000-0x80;word(gp[2]+0xfc,0xdead0003)
                elif recipe=='alias-duration-flags':
                    body=COMPONENTS+3*0x54-0x58;gp[2]=body
                    memory=[[body+(at-BODY) if BODY<=at<BODY+400 else at,value,width] for at,value,width in memory]
                    word(body+0xfc,0xdead0003);word(body+0x5c,0,2);word(body+0x5e,2,2)
                    word(body+0x64,[0,0x80000000,1,0x3f800000][n]);word(COMPONENTS+3*0x54+0x14,0);word(COMPONENTS+3*0x54+0x38,NONE)
            fp=dict(cw=CWS[n%len(CWS)],status=((n%8)<<11)|((n*0x100)&0x4700)|(n&0x3f),mxcsr=CSRS[n%len(CSRS)],
                    stack=[EXTENDED[2],EXTENDED[3]][:n%3],xmm=[rng.randbytes(16).hex() for _ in range(8)])
            initial_stack=rng.randbytes(128);host.write(STACK-64,initial_stack);host.write(STACK,pack([STOP]));initial_stack=host.read(STACK-64,128).hex()
            for at,value,width in memory:host.write(at,value.to_bytes(width,'little'))
            host.prepare(entry,STACK,dict(zip(ALIASES,gp)));putfp(host,fp);regs=host.registers();regs.eflags=0x202|(rng.getrandbits(12)&0xcd5);host.trace(13,0,ctypes.addressof(regs));initial_flags=regs.eflags
            calls=[];stores=[];trace=[];fault=[];depth=len(fp['stack']);defined=NONE&~DEBUG_FLAGS
            for _ in range(2000):
                pc=regs.rip
                if pc==STOP:break
                if pc in SERVICES:
                    before=state(host,regs,depth);ret=int.from_bytes(host.read(regs.rsp,4),'little')
                    returned=before.copy();returned['gp']=before['gp'].copy();returned['fp']=dict(before['fp']);returned['fp']['stack']=before['fp']['stack'].copy()
                    for i in range(3):returned['gp'][i]=rng.getrandbits(32)
                    returned['eflags']=0x202|(rng.getrandbits(12)&0xcd5);returned['fp']['xmm']=[rng.randbytes(16).hex() for _ in range(8)]
                    if pc==0x259b0:
                        returned['fp']['cw']=CWS[(n+3)%len(CWS)];returned['fp']['mxcsr']=CSRS[(n+1)%len(CSRS)]
                        returned['fp']['status']=(returned['fp']['status']&~0x3800)|((((returned['fp']['status']>>11)-1)&7)<<11)
                        returned['fp']['stack'].insert(0,EXTENDED[n%len(EXTENDED)]);depth+=1
                    returned['gp'][4]+=4+SERVICES[pc];putfp(host,returned['fp'])
                    for name,value in zip(ALIASES,returned['gp']):setattr(regs,name,value)
                    regs.rip=ret;regs.eflags=returned['eflags'];host.trace(13,0,ctypes.addressof(regs))
                    calls.append(dict(target=pc,before=before,flags_mask=defined,returned=returned));defined=NONE&~DEBUG_FLAGS;continue
                ins=next(decoder.disasm(image.read(pc,15),pc,count=1));raw=bytes(ins.bytes)
                assert host.read(pc,len(raw))==raw,(hex(pc),'changed original')
                assert 0x17b130<=pc<=0x17b1ed or 0x17dc60<=pc<=0x17dcd6,hex(pc)
                instructions[f'{pc:08x}']=raw.hex();trace.append(pc);pending=[]
                def reg(name):return 0 if not name else getattr(regs,REGS[name])&NONE
                if ins.mnemonic in ('push','call'):pending.append(((regs.rsp-4)&NONE,4))
                operands=[]
                for op in ins.operands:
                    if op.type==3:
                        at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&NONE;operands.append((at,op.size))
                    if op.type==3 and (op.access&2 or ins.mnemonic in ('fst','fstp')):
                        at=(reg(ins.reg_name(op.mem.base))+reg(ins.reg_name(op.mem.index))*op.mem.scale+op.mem.disp)&NONE;pending.append((at,op.size))
                host.trace(9);_,status=os.waitpid(host.pid,0);regs=host.registers();steps+=1
                if os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGSEGV:
                    assert recipe.startswith('fault-') and operands,(recipe,hex(pc),status)
                    fault=[dict(instruction=pc,address=operands[0][0],size=operands[0][1])];break
                assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(hex(pc),status)
                for at,width in pending:
                    assert STACK-64<=at<STACK+64 or BODY<=at<BODY+400 or COMPONENTS-128<=at<COMPONENTS+4*0x54,hex(at)
                    stores.append([pc,at,width,int.from_bytes(host.read(at,width),'little')])
                if ins.mnemonic=='fstp':depth-=1
                if ins.mnemonic in ('cmp','comiss','ucomiss'):defined=NONE&~DEBUG_FLAGS
                elif ins.mnemonic in ('test','xor','and','or'):defined=(NONE&~DEBUG_FLAGS)&~0x10
                elif ins.mnemonic=='imul':defined=(NONE&~DEBUG_FLAGS)&~0xd4
                elif ins.mnemonic in ('sar','shl'):defined=(NONE&~DEBUG_FLAGS)&~0x810
            else:raise ValueError('Original effect-state instruction limit')
            assert bool(fault)==recipe.startswith('fault-'),(recipe,hex(regs.rip))
            if not fault:assert regs.rip==STOP and regs.rsp==STACK+4
            rows.append(dict(entry=entry,kind=kind,n=n,gp=gp,eflags=initial_flags,fp=fp,memory=memory,initial_stack=initial_stack,
                calls=calls,stores=stores,fault=fault,trace_offsets=b''.join(struct.pack('<H',pc-entry if pc>=entry else pc-0x17b130) for pc in trace).hex(),
                trace_base=entry,expected=state(host,regs,depth),flags_mask=defined))
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_STATE_REFERENCE_V1',original_sha256=pin,scope=__doc__,rows=rows),separators=(',',':'))+'\n').encode()
    assert len(data)<=1024*1024,len(data);a.fixture.write_bytes(data)
    report=dict(passed=True,original_sha256=pin,cases=len(rows),hardware_instructions=steps,controlled_lower_calls=sum(len(r['calls']) for r in rows),
        read_stops=sum(bool(r['fault']) for r in rows),alias_cases=sum(r['kind'].startswith('alias-') for r in rows),
        instruction_map=instructions,fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),scratch_owner=str(scratch),
        backend='Physical Linux i386 PTRACE_SINGLESTEP',instruction_patches=[],controlled_boundaries=['17AAB0 RNG owner','259B0 bounded-real owner'],physical_xbox_proof=False)
    a.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
