#!/usr/bin/env python3
"""Execute complete original18B580 sound-scenery angle packing on ABI inputs."""
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
    cpu.mem_map(0x1000000,0x10000);stack=0x100e000;stop=0x100f000;instructions={}
    def observe(_cpu,pc,size,_):
        if pc==stop:cpu.emu_stop();return
        instruction=code.at(pc);assert instruction.mnemonic!='call'
        assert bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=instruction.bytes.hex()
    cpu.hook_add(UC_HOOK_CODE,observe)
    inputs=[0,0x80000000,0x3f800000,0x40490fda,0x40490fdb,0x40490fdc,
        0xbf800000,0x7f800000,0xff800000,0x7fc00000,0x7f800001]
    rng=random.Random(0x18b580);inputs += [rng.getrandbits(32) for _ in range(2048)]
    cases=[]
    for bits in inputs:
        cpu.mem_write(stack,struct.pack('<I',stop));cpu.reg_write(R.UC_X86_REG_ESP,stack)
        cpu.reg_write(R.UC_X86_REG_EAX,0x71392481);cpu.reg_write(R.UC_X86_REG_XMM1,bits)
        cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80);cpu.reg_write(R.UC_X86_REG_EFLAGS,2)
        cpu.emu_start(0x18b580,stop+1,count=100)
        assert cpu.reg_read(R.UC_X86_REG_EIP)==stop
        cases.append([bits,cpu.reg_read(R.UC_X86_REG_EAX)&65535])
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=cases,
        operands={f'{at:08x}':image.read(at,4).hex() for at in (0x46a2ec,0x46a584)},
        executed_original_instructions=instructions,source_instruction_patches=[],supplied_callee_results=[])
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,separators=(',',':'))+'\n')
    print(json.dumps(dict(cases=len(cases),bytes=args.output.stat().st_size,sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())),flush=True)
if __name__=='__main__':main()
