#!/usr/bin/env python3
"""Offline unchanged TU1.5 execution for the three native static startup owners.

Only the native exit-registration ABI is external. It retains real callback
identities and returns the native registration outcome; the Xbox CRT table is
not claimed to run. All game constructors and both shutdown callbacks execute
unchanged original instructions. No constructor result or live bank is an input
to the native runtime. This is not a map/world or Xbox kernel parity proof.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

REGIONS=((0x48d3d8,8),(0x486080,24),(0x4860a0,0x4000),(0x4822a0,16),
         (0x47d174,4),(0x47a2f8,4),(0x56dfa0,44))
ENTRIES=(0x3835f0,0x381370,0x3825d0,0x383970,0x383c90)
REGS=(R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
      R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI)

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    pair_path=ROOT/'analysis/multiplayer/sandbox-source/physics-game-frame-runtime-20261011/correspondence-v2/complete-report.json'
    assert sha(pair_path)=='92379da9dc9d29baea9eaca194cebdab7d69f8be319aecf6cd0b759e26e744cc'
    pair=json.loads(pair_path.read_text())
    imap={k:v for k,v in pair['instruction_map'].items() if 0xdd970<=int(k,16)<=0xdd99a}
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    stack,sp,stop=0x60000000,0x600f0000,0x60300000
    g.mem_map(stack,0x100000);g.mem_map(stop,4096)
    exits=[];instructions=[];stores=[];registrations=[];pc=[0]
    def u(p):return int.from_bytes(g.mem_read(p,4),'little')
    def hook(g,p,n,_):
        if p==0x32b0b4:
            current=g.reg_read(R.UC_X86_REG_ESP);ret=u(current);callback=u(current+4)
            assert (ret,callback) in ((0x383604,0x383c90),(0x38138e,0x383970))
            exits.append(callback);registrations.append(callback)
            g.reg_write(R.UC_X86_REG_EAX,0);g.reg_write(R.UC_X86_REG_ESP,current+4)
            g.reg_write(R.UC_X86_REG_EIP,ret);return
        # Disassemble the actual source instruction and authenticate every
        # executed byte. Original constructors/callbacks have no patched code.
        instruction=code.at(p)
        assert instruction.size==n and bytes(g.mem_read(p,n))==image.read(p,n)
        instructions.append(p);pc[0]=p
    def write(g,_,address,size,value,__):
        if stack<=address<stack+0x100000:return
        assert any(p<=address and address+size<=p+n for p,n in REGIONS),(hex(address),size)
        stores.append(dict(instruction=pc[0],address=address,
            before=bytes(g.mem_read(address,size)).hex(),written=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
    g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
    def buffers():return [dict(address=p,bytes=n,sha256=hashlib.sha256(g.mem_read(p,n)).hexdigest())for p,n in REGIONS]
    initial=buffers();steps=[]
    for entry in ENTRIES:
        if entry in ENTRIES[3:]:assert exits.pop()==entry
        g.mem_write(sp,struct.pack('<I',stop))
        for register in REGS:g.reg_write(register,0)
        g.reg_write(R.UC_X86_REG_ESP,sp);g.reg_write(R.UC_X86_REG_EFLAGS,2)
        g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_FPSW,0)
        g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),0)
        first_instruction,first_store=len(instructions),len(stores)
        g.emu_start(entry,stop,count=10000)
        assert g.reg_read(R.UC_X86_REG_EIP)==stop
        steps.append(dict(entry=entry,returned_gp=[g.reg_read(r)for r in REGS],
            returned_eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
            instructions=instructions[first_instruction:],stores=stores[first_store:],buffers=buffers()))
    assert not exits and registrations==[0x383c90,0x383970]
    report=dict(schema='H2_NATIVE_TU15_STATIC_ORIGINAL_V1',source_sha256=NEW_PIN,
        oracle_sha256=sha(__file__),correspondence_sha256=sha(pair_path),canonical_instruction_map=imap,
        initial_fields=initial,steps=steps,original_instructions=len(instructions),ordered_original_stores=len(stores),
        native_exit_registrations=registrations,instruction_patches=[],supplied_gameplay_results=[],
        reference_executable_is_runtime_input=False,world_startup_complete=False,boundaries=__doc__)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(f'Unchanged TU1.5 static owners: {len(ENTRIES)} complete constructor/exit calls, {len(instructions)} instructions, {len(stores)} ordered game stores')

if __name__=='__main__':main()
