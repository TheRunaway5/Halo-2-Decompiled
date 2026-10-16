#!/usr/bin/env python3
"""Offline source-only configuration plans and interrupt-bit7 CPU behavior.

This executes authenticated original TU1.5 instructions. The offline MMIO
destinations collect original requests; they are not a native register bank,
implemented policy, clock, DSP, AC97 backend or successful runtime initializer.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import threading
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

STACK,STOP,APU,FS=0x2008000,0x2100000,0x19000000,0x2001000
GP=[getattr(R,'UC_X86_REG_'+name)for name in ['EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI']]


def run(image,fill,initialized,count,bit7=False):
    cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    for p,n in [(0x10000,0x600000),(0x2000000,0x20000),(STOP,4096),(0x2200000,4096),
                (APU,0x1000),(0x80000000,4096),(0xfe800000,0x60000)]:cpu.mem_map(p,n)
    for section in image.sections:
        raw=image.section_bytes(section)
        if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
    cpu.mem_write(0x2000000,bytes([fill])*0x20000);cpu.mem_write(APU,bytes([fill])*0x1000)
    def segment(base,access):return struct.pack('<Q',(base&65535)<<16|(base&0xff0000)<<16|(base&0xff000000)<<32|65535|0xf<<48|access<<40|0xc<<52)
    cpu.mem_write(0x2200008,segment(0,0x9b)+segment(0,0x93)+segment(FS,0x93))
    cpu.reg_write(R.UC_X86_REG_GDTR,(0,0x2200000,4095,0))
    for reg,value in [(R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),
                      (R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)]:cpu.reg_write(reg,value)
    def word(p):return struct.unpack('<I',cpu.mem_read(p,4))[0]
    def put(p,v):cpu.mem_write(p,struct.pack('<I',v&0xffffffff))
    put(FS+0x24,0);put(0x391460,initialized);put(0x38f848,count)
    put(0xfe801510,1);put(0xfe801100,0x80);put(0xfe802000,0)
    initial=[0x11000000+i*0x111111 for i in range(8)];initial[1]=APU+8;initial[4]=STACK
    instructions=[];stores=[];reads=[];platform=[];executed={};level=0;lock=threading.RLock();ack=False
    def hook(m,pc,size,_):
        nonlocal level,ack
        if ack:
            assert pc==0x3892e8;put(0xfe801000,0);ack=False
        if pc in [0x800000a0,0x800000a1]:
            sp=m.reg_read(R.UC_X86_REG_ESP);ret=word(sp)
            if pc==0x800000a0:
                assert ret==0x383d8e and level==0 and m.reg_read(R.UC_X86_REG_ECX)&255==2
                lock.acquire();level=2;old=m.reg_read(R.UC_X86_REG_EAX);m.reg_write(R.UC_X86_REG_EAX,old&0xffffff00)
            else:
                assert ret==0x383da3 and level==2 and m.reg_read(R.UC_X86_REG_ECX)&255==0
                level=0;lock.release()
            put(FS+0x24,level);platform.append([pc,ret,level])
            m.reg_write(R.UC_X86_REG_ESP,sp+4);m.reg_write(R.UC_X86_REG_EIP,ret);return
        raw=bytes(m.mem_read(pc,size));assert raw==image.read(pc,size)
        assert executed.setdefault(f'{pc:08x}',raw.hex())==raw.hex();instructions.append(pc)
    def store(m,access,p,n,value,_):
        nonlocal ack
        pc=m.reg_read(R.UC_X86_REG_EIP)
        stores.append([pc,p,n,bytes(m.mem_read(p,n)).hex(),(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()])
        if bit7 and p==0xfe801000:assert pc==0x3892e2;ack=True
    def read(m,access,p,n,value,_):
        if p>=0xfe800000:reads.append([m.reg_read(R.UC_X86_REG_EIP),p,n,bytes(m.mem_read(p,n)).hex()])
    cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,store);cpu.hook_add(UC_HOOK_MEM_READ,read)
    def call(entry,ecx,args=()):
        for r,v in zip(GP,initial):cpu.reg_write(r,v)
        cpu.reg_write(R.UC_X86_REG_ECX,ecx);cpu.reg_write(R.UC_X86_REG_ESP,STACK)
        cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202);cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        cpu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args))
        cpu.emu_start(entry,STOP,count=100000)
        assert cpu.reg_read(R.UC_X86_REG_EIP)==STOP and level==0
        return dict(entry=entry,instructions=instructions.copy(),stores=stores.copy(),hardware_reads=reads.copy(),
            returned_gp=[cpu.reg_read(r)&0xffffffff for r in GP],returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS))
    if bit7:
        put(0xfe801000,0x81);put(APU+0x4b8,0);put(APU+0x4c0,0)
        irq=call(0x3892d0,APU)
        assert word(APU+0x4b8)==0x81 and word(APU+0x4c0)==0 and word(0xfe801000)==0
        assert not any(pc in instructions for pc in [0x3891b3,0x388a1d,0x80000077])
        instructions.clear();stores.clear();reads.clear()
        aggregate=call(0x3889f1,0,(APU,))
        assert word(APU+0x4b8)==0 and word(APU+0x4c0)==0x81
        return dict(fill=fill,declared_status=0x81,irq=irq,aggregate=aggregate,
            hardware_bit7_producer_implemented=False,executed_instruction_bytes=executed)
    result=call(0x38e8c9,APU+8)
    expected=count if initialized&1 else 0x800
    assert word(0x391460)==initialized|1 and word(0x38f848)==expected
    assert word(0xfe801004)==0xd8 and word(0xfe801100)==0x100f and word(0xfe802000)==7 and word(0xfe801510)==0
    assert result['returned_gp'][0]==0 and result['returned_gp'][4]==STACK+4
    result.update(fill=fill,declared_initialized=initialized,declared_prior_count=count,
        source_initialized=word(0x391460),source_count=word(0x38f848),native_policy_installed=False,
        final_request_words={f'{p:08x}':word(p)for p in [0xfe801004,0xfe801100,0xfe802000,0xfe801510,
            0xfe802008,0xfe80200c,0xfe802010,0xfe802014,0xfe802018,0xfe80201c,0xfe802020,0xfe802024,0xfe802028,
            0xfe80402c,0xfe80403c]},platform_exclusion=platform,executed_instruction_bytes=executed)
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();assert not args.output.exists();image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'DSOUND')
    args.output.mkdir(parents=True)
    entries=[0x38e8c9,0x38e75b,0x388a1d,0x3892d0,0x3889f1,0x3890f0,0x38e53d,0x38e574,0x38e5ab,0x38e606]
    bodies={}
    for entry in entries:
        body=_trace(code,entry,set(entries));rows=[]
        for pc in sorted(body.code):
            i=code.at(pc);assert bytes(i.bytes)==image.read(pc,i.size)
            rows.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
        path=args.output/f'original-{entry:08x}.asm';path.write_text(''.join(rows))
        bodies[f'{entry:08x}']=dict(instructions=len(body.code),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
            calls=[dict(instruction=pc,target=target,kind=kind)for pc,target,kind in body.sites])
    cases=[run(image,fill,initialized,count)for fill in [0,0x5a,0xa5]
        for initialized,count in [(0,0xdeadbeef),(0x80,0x12345678),(1,0x800),(0x81,0x1000)]]
    bit7=[run(image,fill,0,0,True)for fill in [0,0x5a,0xa5]]
    report=dict(schema='H2_TU15_CONFIGURATION_REFERENCE_ONLY_V1',scope=__doc__,passed=True,
        original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        original_bodies=bodies,cases=cases,bit7_cases=bit7,instruction_patches=[],supplied_sdk_or_game_results=[],
        source_configuration_requests_recovered=True,native_configuration_owner_implemented=False,
        native_interrupt_bit3_recovery_implemented=False,native_interrupt_bit7_producer_implemented=False,
        cold_initializer45_complete=False,native_dsp_or_pcm_parity=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Configuration source-only: {len(cases)} exact cold/retained plans; {len(bit7)} bit7 ISR/aggregate cases; no native config success')


if __name__=='__main__':main()
