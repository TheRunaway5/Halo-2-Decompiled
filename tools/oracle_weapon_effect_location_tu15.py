#!/usr/bin/env python3
"""Unchanged TU1.5 BBE00/1791C0, with no patched instructions or callees.

Inputs include signed node/offset operands, aliasing, exceptional float bits,
caller MXCSR controls and genuine unmapped-read prefixes. Owned scratch only.
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
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_weapon_effect_dispatch_tu15 import GP, STACK, STOP, NONE
from weapon_reference_versions import images
from original_audio_cpu import OriginalAudioCpu

POOL, HEADERS, OBJECT, PARENT, INPUT, OUTPUT = [0x1000000+i*0x3000 for i in range(6)]
BAD=0x6000000


def hardware_reference(image,rows,scratch,instructions):
    """Single-step the very same original bytes on a physical i386 child.

    Unicorn does not reliably publish SSE sticky exceptions. Hardware owns
    expected final registers/bytes/flags; emulator stores supply only the
    independently checked instruction/address/width observation schedule.
    """
    aliases=('rax','rcx','rdx','rbx','rsp','rbp','rsi','rdi')
    corrections=[];steps=0
    with OriginalAudioCpu(image,scratch/'original-location.elf',0xa7f,0x1f80,
                          source_bank_bytes=0x1100000) as host:
        host.trace(6,host.debug_offset+7*8,0)
        for ordinal,row in enumerate(rows):
            for at,raw in row['banks']:host.write(at,bytes.fromhex(raw))
            for at,value,width in row['memory']:host.write(at,value.to_bytes(width,'little'))
            host.mxcsr=row['mxcsr'];host.prepare(row['entry'],STACK,dict(zip(aliases,row['gp'])))
            fp=ctypes.create_string_buffer(512);host.trace(14,0,ctypes.addressof(fp));context=bytearray(fp.raw)
            for n,raw in enumerate(row['xmm']):context[160+n*16:176+n*16]=bytes.fromhex(raw)
            fp=ctypes.create_string_buffer(bytes(context),512);host.trace(15,0,ctypes.addressof(fp))
            stored=0;registers=host.registers()
            for _ in range(1000):
                pc=registers.rip
                if pc==STOP:break
                raw=bytes.fromhex(instructions[f'{pc:08x}'])
                if host.read(pc,len(raw))!=raw:raise ValueError('Physical original instruction changed')
                pending=row['stores'][stored] if stored<len(row['stores']) and row['stores'][stored][0]==pc else None
                host.trace(9);_,status=os.waitpid(host.pid,0);registers=host.registers();steps+=1
                if not os.WIFSTOPPED(status):raise ValueError('Original hardware child exited')
                if os.WSTOPSIG(status)==signal.SIGSEGV:
                    if row['completed'] or pc!=row['fault_pc']:raise ValueError('Different physical fault')
                    break
                if os.WSTOPSIG(status)!=signal.SIGTRAP:raise ValueError('Unexpected physical signal')
                if pending:
                    pending[3]=int.from_bytes(host.read(pending[1],pending[2]),'little');stored+=1
            else:raise ValueError('Physical original instruction limit')
            if (registers.rip==STOP)!=row['completed'] or stored!=len(row['stores']):raise ValueError('Physical completion/store schedule differs')
            actual_gp=[getattr(registers,name)&NONE for name in aliases]
            if actual_gp!=row['expected_gp']:raise ValueError(('Physical GP differs',ordinal))
            host.trace(14,0,ctypes.addressof(fp));context=fp.raw
            actual_xmm=[context[160+n*16:176+n*16].hex() for n in range(8)]
            actual_csr=struct.unpack_from('<I',context,24)[0]
            actual_banks=[[at,host.read(at,len(bytes.fromhex(raw))).hex()] for at,raw in row['banks']]
            changes=[key for key,actual in [('expected_xmm',actual_xmm),('expected_mxcsr',actual_csr),('expected_banks',actual_banks)] if row[key]!=actual]
            if changes:corrections.append(dict(case=ordinal,hardware_authoritative_fields=changes))
            row.update(expected_xmm=actual_xmm,expected_mxcsr=actual_csr,expected_banks=actual_banks)
    return dict(cases=len(rows),instructions=steps,backend='physical Linux i386 compatibility PTRACE_SINGLESTEP',
                emulator_differences=corrections,instruction_patches=[],supplied_callee_results=[])


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--fixture',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True)
    args=p.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _,image,pin=next(x for x in images() if x[0]=='TU1.5')
    rng=random.Random(0xbbe001791c0)
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    g.mem_map(0x10000,0x800000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(0x1000000,0x100000);g.mem_map(STACK-4096,8192);g.mem_map(STOP,4096)
    instructions={};stores=[];rows=[]
    def code(g,at,size,_):
        raw=bytes(g.mem_read(at,size))
        if raw!=image.read(at,size):raise ValueError('Original instruction changed')
        instructions[f'{at:08x}']=raw.hex()
    def write(g,access,at,size,value,_):
        stores.append([g.reg_read(R.UC_X86_REG_EIP),at,size,value&((1<<(size*8))-1)])
    g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,write)
    special=[0,0x80000000,1,0x80000001,0x7f800000,0xff800000,
             0x7fc12345,0x7f812345,0x3f800000,0x3f000000,0xbf800000,0x7f7fffff]
    for entry in (0xbbe00,0x1791c0):
        for n in range(96):
            banks={};memory=[]
            def bank(at,data):banks[at]=bytes(data);g.mem_write(at,bytes(data))
            def word(at,value,width=4):
                memory.append([at,value&((1<<(width*8))-1),width])
                g.mem_write(at,(value&((1<<(width*8))-1)).to_bytes(width,'little'))
            bank(STACK-64,rng.randbytes(128));word(STACK,STOP)
            bank(OBJECT,rng.randbytes(0x120));bank(OUTPUT,rng.randbytes(64));bank(INPUT,rng.randbytes(64))
            gp=[rng.getrandbits(32) for _ in GP];gp[4]=STACK
            node=(-2,-1,0,1,3)[n%5];offset=(0x400,-0x80)[n%2]
            matrix=(PARENT+node*0x34+offset)&NONE
            bank(matrix,b''.join(struct.pack('<I',special[(n+j)%len(special)] if n&4 else
                struct.unpack('<I',struct.pack('<f',rng.uniform(-20,20)))[0]) for j in range(13)))
            word(0x4ecbd0,POOL);word(POOL+0x44,HEADERS)
            word(HEADERS+3*12+8,OBJECT);word(HEADERS+5*12+8,PARENT)
            word(OBJECT+0x14,NONE if n%4==0 else 0xabc00005)
            word(OBJECT+0x18,node,1);word(PARENT+0x116,offset,2)
            for j in range(3):word(OBJECT+0x64+j*4,special[(n+j*3)%len(special)] if n&8 else
                struct.unpack('<I',struct.pack('<f',rng.uniform(-20,20)))[0])
            word(matrix,(0x3f800000,0x3f000000,0x7fc12345,0x7f812345)[(n//4)%4])
            word(0x46a0d0,int.from_bytes(image.read(0x46a0d0,4),'little'))
            gp[0]=0xbeef0003;gp[6]=[OUTPUT,matrix+8,OBJECT+0x68,STACK-12][(n//8)%4]
            if entry==0x1791c0:
                gp[0]=OUTPUT;gp[1]=[INPUT,OUTPUT,OUTPUT+4,OUTPUT+0x24][n%4]
                word(0x474d1c,[INPUT+16,OUTPUT+8,OUTPUT+16][(n//4)%3])
                word(0x474d10,[INPUT+32,OUTPUT+0x24,OUTPUT+0x28][(n//12)%3])
            fault_case=n>=88
            if fault_case:
                if entry==0x1791c0:
                    if n&1:gp[1]=BAD
                    else:word(0x474d10 if n&2 else 0x474d1c,BAD)
                elif n&1:word(HEADERS+3*12+8,BAD)
                elif n&2:word(HEADERS+5*12+8,BAD);word(OBJECT+0x14,0xabc00005)
                else:word(POOL+0x44,BAD)
            xmm=[rng.randbytes(16).hex() for _ in range(8)]
            csr=[0x1f80,0x9f80,0x1fc0,0xdfc0,0x3f80,0x5f80,0x7f80][n%7]|(n%64)
            for reg,value in zip(GP,gp):g.reg_write(reg,value)
            for i,raw in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(bytes.fromhex(raw),'little'))
            g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_MXCSR,csr)
            stores.clear();fault=None
            try:g.emu_start(entry,STOP,count=1000)
            except UcError as error:fault=str(error)
            completed=g.reg_read(R.UC_X86_REG_EIP)==STOP
            if completed==fault_case:raise ValueError('Unexpected original completion')
            rows.append(dict(entry=entry,gp=gp,xmm=xmm,mxcsr=csr,banks=[[at,raw.hex()] for at,raw in banks.items()],
                memory=memory,completed=completed,fault=fault,
                fault_pc=None if completed else g.reg_read(R.UC_X86_REG_EIP),
                expected_gp=[g.reg_read(r) for r in GP],
                expected_xmm=[g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little').hex() for i in range(8)],
                expected_mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),stores=list(stores),
                expected_banks=[[at,bytes(g.mem_read(at,len(raw))).hex()] for at,raw in banks.items()]))
    hardware=hardware_reference(image,rows,Path(os.environ['HALO2_TEST_SCRATCH']),instructions)
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_LOCATION_REFERENCE_V1',original_sha256=pin,
        reference_backend='physical Linux i386 compatibility PTRACE_SINGLESTEP',rows=rows),separators=(',',':'))+'\n').encode()
    if len(data)>1024*1024:raise ValueError('Fixture exceeds 1MiB')
    args.fixture.write_bytes(data)
    report=dict(passed=True,cases=len(rows),unmapped_prefixes=sum(not row['completed'] for row in rows),
        fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),original_sha256=pin,
        instructions=instructions,hardware=hardware,instruction_patches=[],declared_service_boundaries=[],full_output_parity=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','cases','unmapped_prefixes','fixture_bytes','fixture_sha256')}))


if __name__=='__main__':main()
