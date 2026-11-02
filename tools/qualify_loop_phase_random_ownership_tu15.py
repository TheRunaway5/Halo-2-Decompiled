#!/usr/bin/env python3
"""Measure unchanged original loop RNG/range ownership branches.

Declared component records supply addresses and initial masks. The outside
21FED0 range lookup alone receives an explicit read-only component result.
No voice, source admission, driver state or sound instruction is replaced.
Run through tools/run_test.py; retain only this bounded policy report.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    args.output=args.output.resolve()
    if args.output.exists():parser.error('Use a new bounded report path; reference inputs are never overwritten')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    bank=0x1000000;cpu.mem_map(bank,0x20000)
    definition,parent,track,common,sound,voice,gestalt,ranges,stack=[bank+n*0x1000 for n in range(1,10)]
    sentinel=bank+0x1f000
    def put(address,value):cpu.mem_write(address,struct.pack('<I',value&0xffffffff))
    def word(address):return struct.unpack('<I',cpu.mem_read(address,4))[0]
    def reg(register):return cpu.reg_read(register)&0xffffffff
    instructions={};context={};ports=[]
    def observe(_,pc,size,__):
        if pc==context['stop']:context['complete']=True;cpu.emu_stop();return
        instruction=code.at(pc)
        assert instruction and bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=instruction.bytes.hex()
        if pc==0x21fed0:
            # Actual CALL completed. This lookup is outside the ownership
            # branch; its explicit mask is a component input, never a voice.
            ports.append(dict(target=pc,sound=reg(R.UC_X86_REG_ECX),range=reg(R.UC_X86_REG_EDX)))
            sp=reg(R.UC_X86_REG_ESP);ret=word(sp)
            cpu.reg_write(R.UC_X86_REG_EAX,0x13579bdf)
            cpu.reg_write(R.UC_X86_REG_ESP,sp+8);cpu.reg_write(R.UC_X86_REG_EIP,ret)
    cpu.hook_add(UC_HOOK_CODE,observe)
    def run(entry,stop,registers):
        context.clear();context['stop']=stop;ports.clear()
        for register,value in registers.items():cpu.reg_write(register,value)
        cpu.reg_write(R.UC_X86_REG_EFLAGS,2)
        cpu.emu_start(entry,0,count=200)
        assert context.get('complete'),hex(entry)
    put(0x4f3cd8,common);put(0x52d664,gestalt);put(gestalt+0x24,ranges)
    cpu.mem_write(sound+8,struct.pack('<h',0))
    rows=[]
    for flags in (0,8,0x18,0x20,0x30,0x80):
        put(definition,flags)
        run(0x220f9e,0x220fbd,{R.UC_X86_REG_EAX:definition,R.UC_X86_REG_EBP:parent,
            R.UC_X86_REG_ECX:0,R.UC_X86_REG_EDX:0,R.UC_X86_REG_ESP:stack})
        creation=word(stack+0x5c);assert creation==(parent+8 if flags&0x30 else common+4)
        for primary in (False,True):
            cpu.mem_write(stack+0x13,bytes([primary]));put(stack+0x1c,track);put(stack+0x2c,definition)
            run(0x2231f4,0x223227,{R.UC_X86_REG_EDX:definition,R.UC_X86_REG_ESI:voice,R.UC_X86_REG_ESP:stack})
            frame=word(stack+0x28);assert frame==((track+8 if primary else voice+0x64) if flags&0x30 else common+4)
            for synchronized in (False,True):
                for reset in (False,True):
                    put(parent+0x54,0x12340001 if synchronized else 0xffffffff)
                    put(parent+0x8c,0x0f00af53)
                    for offset,value in ((0,sentinel),(4,definition),(8,0),(12,int(reset))):put(stack+offset,value)
                    run(0x2235a0,sentinel,{R.UC_X86_REG_EAX:0,R.UC_X86_REG_ECX:sound,
                        R.UC_X86_REG_EDX:parent,R.UC_X86_REG_ESP:stack})
                    private=bool(flags&0x30 and synchronized)
                    assert bool(ports)!=private
                    assert reg(R.UC_X86_REG_EAX)==(0x0f00af53 if private else 0x13579bdf)
                    assert word(parent+0x8c)==(0 if private and reset else 0x0f00af53)
                    if ports:assert ports==[dict(target=0x21fed0,sound=sound,range=ranges)]
                    rows.append(dict(flags=flags,primary=primary,synchronization_owner=synchronized,reset=reset,
                        creation_random_address=creation,frame_random_address=frame,
                        used_range_owner='track' if private else 'shared pitch range',
                        returned_mask=reg(R.UC_X86_REG_EAX),stored_track_mask=word(parent+0x8c)))
    report=dict(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,cases=rows,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        original_instruction_patches=[],executed_original_instructions=instructions,
        qualified_native_adapter_profiles=[0,0x18],component_lookup_return=0x13579bdf,
        native_device_or_whole_controller_verified=False,owned_scratch=str(owner),retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    raw=json.dumps(report,indent=2)+'\n';assert len(raw.encode())<1024*1024
    args.output.write_text(raw)
    print(json.dumps(dict(passed=True,cases=len(rows),instructions=len(instructions))),flush=True)


if __name__=='__main__':main()
