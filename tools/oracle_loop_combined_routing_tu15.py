#!/usr/bin/env python3
"""Execute unchanged original2B9450, including every12DB00 call.

These are declared plugin/parameter/output component inputs. This observes
ordered dry/reverb mix-bin writes, not hardware playback or source lifecycle.
All original instructions and callees execute without patches/substitutions.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.output.exists():parser.error('Use a new original report path')
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    bank=0x1000000;cpu.mem_map(bank,0x40000)
    plugin,parameters,output=bank+0x1000,bank+0x2000,bank+0x3000
    stack,stop=bank+0x3e000,bank+0x3f000
    registers=[getattr(R,'UC_X86_REG_'+name) for name in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
    simd=[getattr(R,'UC_X86_REG_XMM'+str(n)) for n in range(8)]
    static=[]
    def write(address,raw,records):
        cpu.mem_write(address,raw);records.append([address,raw.hex()])
    def word(address,value,records):write(address,struct.pack('<I',value&0xffffffff),records)
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    for at in (0x455b0c,0x46a0ec,0x46a0d0,0x46a53c):static.append([at,image.read(at,4).hex()])
    instructions={};observed=[];context={}
    def observe(_,pc,size,__):
        if pc==stop:context['complete']=True;cpu.emu_stop();return
        instruction=code.at(pc)
        assert instruction and bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=instruction.bytes.hex();context['pc']=pc
    def store(_,access,address,size,value,__):
        if output<=address<output+80 or stack+4<=address<stack+20:
            observed.append([context['pc'],address,size,value&((1<<(8*size))-1)])
    cpu.hook_add(UC_HOOK_CODE,observe);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
    rng=0x1d4eac32
    def draw():
        nonlocal rng
        rng=(rng*1664525+1013904223)&0xffffffff;return rng
    rows=[]
    gains=[bits(v) for v in (-100,-64,-63.9999,-63,-32,-6,-.001,0,1,64,1e-40,-1e-40)]
    # Both bypass states, six class categories (including unsupported values
    # which append no send), all rounding modes and nonzero existing prefixes.
    for rounding in range(4):
      for kind in (0,1,2,3,0xffffffff,0x80000000):
       for bypass in range(2):
        for variant in range(8):
            writes=[]
            word(stack,stop,writes);word(stack+4,parameters,writes)
            word(stack+8,0xdeadbeef,writes);word(stack+12,kind,writes);word(stack+16,output,writes)
            write(plugin,bytes(0x48),writes);word(plugin+0x18,gains[(variant+2)%len(gains)],writes)
            write(parameters,bytes(0x30),writes)
            flags=(draw()&~8)|(bypass<<3);word(parameters,flags,writes)
            word(parameters+4,gains[variant],writes);word(parameters+8,gains[(variant+3)%len(gains)],writes)
            for k in range(4):word(parameters+0x18+4*k,gains[(variant+k*3)%len(gains)],writes)
            write(output,b''.join(struct.pack('<I',draw()) for _ in range(20)),writes)
            prefix=variant%3;word(output+0x40,prefix,writes)
            initial_gp=[draw() for _ in range(8)];initial_gp[1]=plugin;initial_gp[4]=stack
            initial_xmm=[[draw() for _ in range(4)] for _ in range(8)]
            for register,value in zip(registers,initial_gp):cpu.reg_write(register,value)
            for register,value in zip(simd,initial_xmm):cpu.reg_write(register,int.from_bytes(struct.pack('<4I',*value),'little'))
            cw=0x027f|(rounding<<10);mxcsr=0x1f80|(rounding<<13)
            for reg,value in ((R.UC_X86_REG_FPCW,cw),(R.UC_X86_REG_FPSW,0),
                (R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,mxcsr),(R.UC_X86_REG_EFLAGS,2)):cpu.reg_write(reg,value)
            observed.clear();context.clear();cpu.emu_start(0x2b9450,0,count=4000)
            assert context.get('complete')
            rows.append(dict(memory=writes,gp=initial_gp,xmm=initial_xmm,control=cw,mxcsr=mxcsr,
                kind=kind,bypass=bool(bypass),prefix=prefix,
                expected_gp=[cpu.reg_read(reg)&0xffffffff for reg in registers],
                expected_xmm=[list(struct.unpack('<4I',int(cpu.reg_read(reg)).to_bytes(16,'little'))) for reg in simd],
                expected_control=cpu.reg_read(R.UC_X86_REG_FPCW),expected_status=cpu.reg_read(R.UC_X86_REG_FPSW),
                expected_mxcsr=cpu.reg_read(R.UC_X86_REG_MXCSR),output=bytes(cpu.mem_read(output,80)).hex(),
                caller=bytes(cpu.mem_read(stack+4,16)).hex(),stores=list(observed)))
    result=dict(scope=__doc__,entry=0x2b9450,original_xbe_sha256=NEW_PIN,memory=static,cases=rows,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        plugin=plugin,parameters=parameters,output_address=output,stack=stack,
        executed_original_instructions=instructions,source_instruction_patches=[],substituted_calls=[],
        device_or_complete_loop_verified=False,retained_large_artifacts=[])
    raw=json.dumps(result,separators=(',',':'))+'\n';assert len(raw.encode())<1024*1024
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(raw)
    print(json.dumps(dict(cases=len(rows),instructions=len(instructions),bytes=len(raw.encode()))),flush=True)


if __name__=='__main__':main()
