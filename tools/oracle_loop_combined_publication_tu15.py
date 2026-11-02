#!/usr/bin/env python3
"""Execute complete original2216D0 and its closed CPU gain dependencies.

Declared component pools, sound-class/scale/ducking banks and caller backing
are inputs. No instruction, call result, voice admission or driver flag is
substituted. This proves no device routing or complete loop lifecycle.
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
    if args.output.exists():parser.error('Use a new report path; original evidence is never overwritten')
    image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text')
    cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    bank=0x1000000
    cpu.mem_map(bank,0x40000)
    pool,parents,tags,definition,gestalt,scales,globals_,reference,classes,class_rows,runtime,ducking,output=[
        bank+0x1000*n for n in range(1,14)]
    stack,stop=bank+0x3e000,bank+0x3f000
    gp_registers=[getattr(R,'UC_X86_REG_'+name) for name in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
    xmm_registers=[getattr(R,'UC_X86_REG_XMM'+str(n)) for n in range(8)]
    static=[]

    def write(address,raw,records):
        cpu.mem_write(address,raw)
        records.append([address,raw.hex()])

    def word(address,value,records=static):write(address,struct.pack('<I',value&0xffffffff),records)
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    def real(address,value,records=static):word(address,bits(value),records)

    word(0x50ebe8,pool)
    for at,value in ((0x24,0xd4),(0x38,4),(0x44,parents)):word(pool+at,value)
    word(0x4f0414,tags);word(tags+8,definition);word(tags+24,classes)
    word(0x52d664,gestalt);word(gestalt+0xc,scales)
    word(0x4ecc1c,globals_);word(globals_+0xc0,1);word(globals_+0xc4,reference);word(reference+4,1)
    word(classes+4,class_rows);word(0x50ebf0,runtime);word(0x4f2c50,ducking)
    # Sound class/scale indices are zero; all original reads remain real.
    write(definition,bytes(20),static)
    for kind in range(2):
        for at,value in ((0,-12-6*kind),(4,.4),(8,.5),(12,.8)):
            real(class_rows+0x28+kind*16+at,value)
    word(stack,stop);word(stack+8,0x76540000);word(stack+12,0xabc10203);word(stack+16,output)
    # Retain only literal operands of executed original instructions, not the
    # executable or a synthetic live runtime bank.
    instructions={};observed=[];current={}

    def observe(_,pc,size,__):
        if pc==stop:
            current['complete']=True;cpu.emu_stop();return
        instruction=code.at(pc)
        assert instruction and bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=instruction.bytes.hex()
        current['pc']=pc

    def store(_,access,address,size,value,__):
        if output<=address<output+80 or stack+4<=address<stack+20:
            observed.append([current['pc'],address,size,value&((1<<(size*8))-1)])

    cpu.hook_add(UC_HOOK_CODE,observe);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
    rng=0x719f3ca2

    def draw():
        nonlocal rng
        rng=(rng*1664525+1013904223)&0xffffffff
        return rng

    rows=[]
    for n in range(320):
        writes=[]
        mode=n%8
        generation=[0x1234,0x7fff,0x8001,0xffff][n%4]
        datum=(generation<<16) if mode!=1 else 0xffffffff
        if mode==2:datum=(generation<<16)|4
        if mode==3:datum=((generation^1)<<16)
        word(stack+4,datum,writes)
        #2216D0 reuses its caller's first two argument slots as scratch.
        #Each independent invocation owns fresh explicit argument backing.
        word(stack+8,0x76540000,writes);word(stack+12,0xabc10203,writes);word(stack+16,output,writes)
        write(parents,struct.pack('<H',0 if mode==4 else generation),writes)
        word(parents+4,0x65430002,writes)
        word(parents+8,0x65436002 if mode!=5 else 0x65430002,writes)
        real(parents+0x10,[0,.125,.5,1,2,-1][n%6],writes)
        real(parents+0x14,[-6,0,-.001,3][n%4],writes)
        for k in range(4):
            real(parents+0xc4+4*k,[0,1,.25,2,-.5,1e-7,.70710677][(n+k)%7],writes)
        real(scales,[-6,-12,3,-100][n%4],writes)
        real(scales+4,[0,-3,6,-1][n%4],writes)
        real(class_rows+0x20,[-1,-6,0][n%3],writes)
        real(class_rows+0x24,[0,-2,1][n%3],writes)
        real(runtime+4,[-3,0,-.01][n%3],writes)
        write(runtime+0xc,bytes([int(n%3==0)]),writes)
        active=[0xffffffff,0,1,0][n%4]
        fading=[0xffffffff,0xffffffff,1,1][n%4]
        word(ducking+0x1f8,active,writes);word(ducking+0x200,fading,writes)
        real(ducking+0x1fc,[.1,.5,1,2][n%4],writes)
        real(ducking+0x204,[.2,.6,.9,1.2][n%4],writes)
        write(output,b''.join(struct.pack('<I',draw()) for _ in range(20)),writes)
        initial_gp=[draw() for _ in range(8)];initial_gp[4]=stack
        initial_xmm=[[draw() for _ in range(4)] for _ in range(8)]
        for register,value in zip(gp_registers,initial_gp):cpu.reg_write(register,value)
        for register,value in zip(xmm_registers,initial_xmm):
            cpu.reg_write(register,int.from_bytes(struct.pack('<4I',*value),'little'))
        rounding=(n//8)%4
        cw=0x027f|(rounding<<10);mxcsr=0x1f80|(rounding<<13)
        cpu.reg_write(R.UC_X86_REG_FPCW,cw);cpu.reg_write(R.UC_X86_REG_FPSW,0)
        cpu.reg_write(R.UC_X86_REG_FPTAG,0xffff);cpu.reg_write(R.UC_X86_REG_MXCSR,mxcsr)
        cpu.reg_write(R.UC_X86_REG_EFLAGS,2)
        observed.clear();current.clear()
        try:cpu.emu_start(0x2216d0,0,count=10000)
        except Exception as error:
            raise RuntimeError(f'Original component case {n} at {cpu.reg_read(R.UC_X86_REG_EIP):08x}, '
                f'GP={[hex(cpu.reg_read(register)) for register in gp_registers]}') from error
        assert current.get('complete'),(n,hex(cpu.reg_read(R.UC_X86_REG_EIP)))
        rows.append(dict(memory=writes,gp=initial_gp,xmm=initial_xmm,control=cw,mxcsr=mxcsr,
            expected_gp=[cpu.reg_read(register)&0xffffffff for register in gp_registers],
            expected_xmm=[list(struct.unpack('<4I',int(cpu.reg_read(register)).to_bytes(16,'little'))) for register in xmm_registers],
            expected_control=cpu.reg_read(R.UC_X86_REG_FPCW),expected_status=cpu.reg_read(R.UC_X86_REG_FPSW),
            expected_mxcsr=cpu.reg_read(R.UC_X86_REG_MXCSR),output=bytes(cpu.mem_read(output,80)).hex(),
            stores=list(observed)))
    # The native group folds these exact read-only original operands.
    for at in (0x455b0c,0x455b14,0x46a0d0,0x46a0e8,0x46a118,0x46a12c,0x46a180):
        static.append([at,image.read(at,4).hex()])
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,entry=0x2216d0,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),memory=static,cases=rows,
        stack=stack,output_address=output,executed_original_instructions=instructions,
        source_instruction_patches=[],substituted_calls=[],device_or_complete_loop_verified=False)
    raw=json.dumps(report,separators=(',',':'))+'\n'
    assert len(raw.encode())<1024*1024
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(raw)
    print(json.dumps(dict(cases=len(rows),instructions=len(instructions),bytes=len(raw.encode()),
        sha256=hashlib.sha256(raw.encode()).hexdigest())),flush=True)


if __name__=='__main__':main()
