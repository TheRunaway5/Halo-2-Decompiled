#!/usr/bin/env python3
"""Execute original189617..18963D held secondary-intent flag production."""
import argparse,hashlib,json,os,random,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text')
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    cpu.mem_map(0x1000000,0x1000);output=0x1000100;instructions={}
    def observe(_cpu,pc,size,_):
        assert 0x189617<=pc<0x18963d
        instruction=code.at(pc);assert bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=instruction.bytes.hex()
    cpu.hook_add(UC_HOOK_CODE,observe)
    rng=random.Random(0x189617);cases=[]
    for flags in [0,0x4800,0x220000,0xffffffff]+[rng.getrandbits(32) for _ in range(1024)]:
        for held in (False,True):
            cpu.mem_write(output+0x18,struct.pack('<I',flags));cpu.reg_write(R.UC_X86_REG_EDI,output)
            cpu.reg_write(R.UC_X86_REG_EDX,int(held));cpu.reg_write(R.UC_X86_REG_EFLAGS,2)
            cpu.emu_start(0x189617,0x18963d,count=30);assert cpu.reg_read(R.UC_X86_REG_EIP)==0x18963d
            result=struct.unpack('<I',cpu.mem_read(output+0x18,4))[0];cases.append([flags,held,result])
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=cases,
        executed_original_instructions=instructions,source_instruction_patches=[],supplied_callee_results=[])
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,separators=(',',':'))+'\n')
    print(json.dumps(dict(cases=len(cases),bytes=args.output.stat().st_size,sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())),flush=True)
if __name__=='__main__':main()
