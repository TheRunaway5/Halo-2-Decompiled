#!/usr/bin/env python3
"""Capture unchanged223670 centered-detail prefixes before the BSP query.

Declared caller, local-user and listener inputs;150900 and223870 execute
unchanged. No callee result, sound admission or source voice is supplied.
Run through tools/run_test.py. Only the small reference JSON is retained.
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
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.output.exists():raise ValueError('Preserve existing original reference evidence')
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    cpu.mem_map(0x1000000,0x20000)
    detail,random,out,pool,group,tags,sound_root,locals_,sound=[0x1001000+n*0x1000 for n in range(9)]
    stack,sentinel=0x101e000,0x101f000
    def put(at,value):cpu.mem_write(at,struct.pack('<I',value&0xffffffff))
    def word(at):return struct.unpack('<I',cpu.mem_read(at,4))[0]
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    for at,value in [(0x50ebe8,pool),(pool+0x44,group),(0x4f0414,tags),(tags+8,sound_root),
                     (detail+8,0x56780000),(0x4f3cd8,random),(0x4f54f0,locals_),(0x4f2c50,sound)]:put(at,value)
    parent=bytearray(68)
    for at,value in [(4,.7),(8,-3),(12,900),(16,-123),(20,44)]:struct.pack_into('<f',parent,at,value)
    instructions={};current={}
    def observe(_,pc,size,__):
        if pc in (sentinel,0x22381c):current['stop']=pc;cpu.emu_stop();return
        insn=code.at(pc);assert insn and bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=insn.bytes.hex()
    cpu.hook_add(UC_HOOK_CODE,observe)
    rows=[]
    for mask in range(16):
      points=[]
      for view in range(4):
        point=[bits(view*13.25),bits(-view*17.5),bits(3+view*.125)]
        cpu.mem_write(sound+0xb8+view*0x48,struct.pack('<3I',*point))
        put(locals_+0xc+view*4,view if mask&(1<<view) else 0xffffffff)
        if mask&(1<<view):points.append(point)
      cpu.mem_write(locals_+8,struct.pack('<h',len(points)))
      for seed in (0,1,0xffff,0xffffffff,0x9a725331):
       for values in ((0,0,0,0,0,0),(0,2.5,-.5,.5,0,2),(-3.141,3.141,-1,1,-1,1)):
        bounds=list(map(bits,values))
        for at,value in zip((0x1c,0x20,0x24,0x28,0x2c,0x30),bounds):put(detail+at,value)
        cpu.mem_write(group+0xc,bytes(parent));cpu.mem_write(out,bytes(160));cpu.mem_write(stack,bytes(256))
        for n,value in enumerate((sentinel,detail,0x12340000,out)):put(stack+n*4,value)
        put(random+4,seed);current.clear()
        for reg,value in [(R.UC_X86_REG_ESP,stack),(R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),
                          (R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)]:cpu.reg_write(reg,value)
        cpu.emu_start(0x223670,0,count=10000)
        assert current.get('stop')==(0x22381c if points else sentinel)
        rows.append(dict(seed=seed,bounds=bounds,listeners=points,parent=parent.hex(),
            result=[word(out+at) for at in (0x10,0x14,0x18,0x1c,0x20)] if points else None,
            seed_after=word(random+4)))
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,entry=0x223670,stop_before_bsp_query=0x22381c,
        source_instruction_patches=[],supplied_callee_results=[],cases=rows,executed_original_instructions=instructions,
        completed_admission_or_audio_output_verified=False)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,separators=(',',':'))+'\n')
    print(json.dumps(dict(cases=len(rows),bytes=args.output.stat().st_size,sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())))


if __name__=='__main__':main()
