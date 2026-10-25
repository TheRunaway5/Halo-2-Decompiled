#!/usr/bin/env python3
"""Measure complete original190460 audibility with declared camera inputs."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    cpu.mem_map(0x1000000,0x10000);sound=0x1001000;point=0x1003000;stack=0x100e000;sentinel=0x100f000
    cpu.mem_write(0x4f2c50,struct.pack('<I',sound))
    instructions={}
    def observe(_cpu,pc,size,_):
        if pc==sentinel:cpu.emu_stop();return
        insn=code.at(pc);assert 0x190460<=pc<=0x1904c8 and insn.mnemonic!='call'
        assert bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=insn.bytes.hex()
    cpu.hook_add(UC_HOOK_CODE,observe)
    def bits(*values):return list(struct.unpack('<'+'I'*len(values),struct.pack('<'+'f'*len(values),*values)))
    positions=[bits(0,0,0),bits(1,2,3),bits(-5,-4,-3),bits(1024,0,-2)]
    points=positions+[bits(1,2,4),bits(float('inf'),0,0),[0x7fc00000,0,0],bits(-0.,0,0),bits(1e20,1e20,1e20)]
    radii=bits(0,1,2,5,1e10,float('inf'),float('nan'),-1)
    results=bytearray()
    for variation in range(8):
      selected=[[word^(0x80000000 if variation&4 else 0) for word in positions[(slot+variation)%4]] for slot in range(4)]
      for mask in range(16):
        raw=bytearray(0x20c)
        for slot,position in enumerate(selected):
            raw[0x8e+slot*0x48]=(mask>>slot)&1
            struct.pack_into('<3I',raw,0xb8+slot*0x48,*position)
        cpu.mem_write(sound,bytes(raw))
        for radius in radii:
          for position in points:
            cpu.mem_write(point,struct.pack('<3I',*position));cpu.mem_write(stack,struct.pack('<I',sentinel))
            cpu.reg_write(R.UC_X86_REG_ESP,stack);cpu.reg_write(R.UC_X86_REG_ESI,point)
            cpu.reg_write(R.UC_X86_REG_XMM3,radius);cpu.reg_write(R.UC_X86_REG_EFLAGS,2)
            cpu.emu_start(0x190460,sentinel+1,count=500)
            assert cpu.reg_read(R.UC_X86_REG_EIP)==sentinel
            results.append(cpu.reg_read(R.UC_X86_REG_EAX)&255)
        assert bytes(cpu.mem_read(sound,len(raw)))==bytes(raw)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,entry='00190460',
        positions_bits=positions,points_bits=points,radii_bits=radii,variations=8,
        cases=len(results),results_sha256=hashlib.sha256(results).hexdigest(),
        order=['variation','active_mask','radius','point'],
        executed_original_instructions=instructions,source_instruction_patches=[],supplied_callee_results=[])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key] for key in ('entry','cases','results_sha256')}),flush=True)
if __name__=='__main__':main()
