#!/usr/bin/env python3
"""Execute original layer attenuation/admission and gain-control slew owners."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    cpu.mem_map(0x1000000,0x10000);table=0x1001000;interval=0x1002000;definition=0x1003000
    row=0x1004000;track=0x1005000;sound=0x1006000;parent=0x1007000;stack=0x100e000;sentinel=0x100f000
    def put(at,value):cpu.mem_write(at,struct.pack('<I',value&0xffffffff))
    def word(at):return struct.unpack('<I',cpu.mem_read(at,4))[0]
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    put(0x52d664,table);put(table+0x1c,interval);put(0x4f2c50,sound)
    instructions={}
    def observe(_cpu,pc,size,_):
        if pc==sentinel:cpu.emu_stop();return
        insn=code.at(pc);assert insn and bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=insn.bytes.hex()
    cpu.hook_add(UC_HOOK_CODE,observe)
    def run(entry,end=sentinel):
        cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        cpu.reg_write(R.UC_X86_REG_EFLAGS,2);cpu.emu_start(entry,end,count=200000)
        assert cpu.reg_read(R.UC_X86_REG_EIP)==end,(hex(entry),hex(cpu.reg_read(R.UC_X86_REG_EIP)))
    layers=[]
    intervals=[(0,-1200,1200,-600,600),(500,0,1000,100,900),
        (0,-32768,32767,-12000,12000),(0,0,0,0,0),
        (0,-1,1,0,0),(0,1000,-1000,100,-100),
        (0,-1000,1000,-1000,1000),(0,-1000,1000,2000,3000)]
    pitches=[-32000,-1200,-1000,-900,-600,-100,-1,-.00005,0,.00005,1,100,500,600,900,1000,1200,32000,float('inf'),float('nan')]
    for bounds in intervals:
      cpu.mem_write(interval,struct.pack('<5h',*bounds))
      for layered in (0,1):
        put(definition,0x2000 if layered else 0)
        for pitch in pitches:
            raw=bits(pitch);put(stack,sentinel);put(stack+4,raw)
            cpu.reg_write(R.UC_X86_REG_ESP,stack);cpu.reg_write(R.UC_X86_REG_ECX,definition)
            cpu.reg_write(R.UC_X86_REG_EDX,row);run(0x220990);gain=cpu.reg_read(R.UC_X86_REG_EAX)
            put(stack+0x40,raw);put(stack+0x30,0)
            cpu.reg_write(R.UC_X86_REG_ESP,stack);cpu.reg_write(R.UC_X86_REG_EBX,interval+2)
            cpu.reg_write(R.UC_X86_REG_ESI,0);cpu.reg_write(R.UC_X86_REG_EAX,gain)
            run(0x221eaa,0x221ee6)
            layers.append(dict(interval=list(bounds),layered=bool(layered),pitch_bits=raw,
                               gain_bits=gain,active=bool(word(stack+0x30)&1)))
    slew=[]
    for current in (-1,0,.08,.5,1,2):
      for target in (-1,0,.08,.5,1,2):
       for rate in (-1,0,.00005,.0001,1,4):
        for elapsed in (0,1/30,.5):
            for at,value in ((definition+0xc,rate),(track+0x10,current),(parent+0x10,target),(sound+0x1e0,elapsed)):put(at,bits(value))
            put(stack+0x14,parent);cpu.reg_write(R.UC_X86_REG_ESP,stack)
            cpu.reg_write(R.UC_X86_REG_EAX,definition);cpu.reg_write(R.UC_X86_REG_ECX,track)
            run(0x221c3c,0x221ca7)
            slew.append(dict(current_bits=bits(current),target_bits=bits(target),rate_bits=bits(rate),
                             elapsed_bits=bits(elapsed),result_bits=word(track+0x10)))
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,layers=layers,slew=slew,
        executed_original_instructions=instructions,source_instruction_patches=[],supplied_callee_results=[])
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,separators=(',',':'))+'\n')
    print(json.dumps(dict(layer_cases=len(layers),slew_cases=len(slew),fixture_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())),flush=True)
if __name__=='__main__':main()
