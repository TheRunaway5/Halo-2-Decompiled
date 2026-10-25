#!/usr/bin/env python3
"""Execute whole original31B30 and its30BD0 tail on declared finite vectors."""
import argparse,hashlib,json,os,random,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    cpu.mem_map(0x1000000,0x20000);up=0x1001000;forward=0x1002000;stack=0x101e000;stop=0x101f000
    executed={}
    def observe(_cpu,pc,size,_):
        ins=code.at(pc);assert ins and bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        executed[f'{pc:08x}']=ins.bytes.hex()
    cpu.hook_add(UC_HOOK_CODE,observe)
    rng=random.Random(0x31b30);cases=[]
    special=[(0.,0.,0.),(1.,0.,0.),(0.,1.,0.),(0.,0.,1.),(1e-12,0.,0.),(1e-6,1e-6,0.),(-1.,0.,0.)]
    vectors=[(a,b) for a in special for b in special]
    vectors += [(tuple(rng.uniform(-4,4) for _ in range(3)),tuple(rng.uniform(-1,1) for _ in range(3))) for _ in range(384)]
    for a,b in vectors:
        raw=struct.pack('<6f',*a,*b);cpu.mem_write(up,raw[:12]);cpu.mem_write(forward,raw[12:])
        cpu.mem_write(stack,struct.pack('<I',stop));cpu.reg_write(R.UC_X86_REG_ESP,stack)
        cpu.reg_write(R.UC_X86_REG_EAX,up);cpu.reg_write(R.UC_X86_REG_ECX,forward)
        cpu.reg_write(R.UC_X86_REG_EFLAGS,2);cpu.reg_write(R.UC_X86_REG_FPCW,0x27f)
        cpu.reg_write(R.UC_X86_REG_FPSW,0);cpu.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        cpu.emu_start(0x31b30,stop,count=10000)
        assert cpu.reg_read(R.UC_X86_REG_EIP)==stop and cpu.reg_read(R.UC_X86_REG_ESP)==stack+4
        cases.append(dict(input=raw.hex(),output=bytes(cpu.mem_read(up,12)).hex(),forward=bytes(cpu.mem_read(forward,12)).hex()))
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,original_constant_page=image.read(0x46a000,4096).hex(),
        cases=cases,executed_original_instructions=executed,source_instruction_patches=[],supplied_callee_results=[])
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,separators=(',',':'))+'\n')
    print(json.dumps(dict(cases=len(cases),bytes=args.output.stat().st_size,sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())),flush=True)
if __name__=='__main__':main()
