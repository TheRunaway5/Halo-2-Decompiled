#!/usr/bin/env python3
"""Execute original detail offsets and initial/subsequent deadline arithmetic."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text')
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    cpu.mem_map(0x1000000,0x20000)
    detail=0x1001000;random=0x1002000;out=0x1003000;root=0x1004000;sound=0x1005000;stack=0x101e000;sentinel=0x101f000
    def put(at,value):cpu.mem_write(at,struct.pack('<I',value&0xffffffff))
    def word(at):return struct.unpack('<I',cpu.mem_read(at,4))[0]
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    put(0x4f3cd8,random);put(0x4f2c50,sound);put(root+0x24,1);put(root+0x28,detail)
    assert word(0x46a160)==0x37800080 and word(0x46a1dc)==bits(1000)
    assert bytes(cpu.mem_read(word(0x474d04),12))==bytes(12)
    instructions={}
    def observe(_cpu,pc,size,_):
        if pc==sentinel:cpu.emu_stop();return
        insn=code.at(pc);assert insn and bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=insn.bytes.hex()
    cpu.hook_add(UC_HOOK_CODE,observe)
    def run(entry,end=sentinel):
        cpu.reg_write(R.UC_X86_REG_FPCW,0x27f);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80);cpu.reg_write(R.UC_X86_REG_EFLAGS,2)
        cpu.emu_start(entry,end,count=100000);assert cpu.reg_read(R.UC_X86_REG_EIP)==end
    rng=0x3b92e18d
    def draw():
        nonlocal rng
        rng=(rng*1664525+1013904223)&0xffffffff;return rng
    positions=[]
    for index in range(640):
        seed=draw()
        yaw=[bits((draw()%6284-3142)/1000) for _ in range(2)]
        pitch=[bits((draw()%3142-1571)/1000) for _ in range(2)]
        radius=[bits((draw()%10000)/1000) for _ in range(2)]
        if index%8==0:radius=[0,0]
        if index%16==1:radius=[bits(-1),bits(1)]
        for offset,value in zip((0x1c,0x20,0x24,0x28,0x2c,0x30),yaw+pitch+radius):put(detail+offset,value)
        put(random+4,seed);put(stack,sentinel);cpu.reg_write(R.UC_X86_REG_ESP,stack)
        cpu.reg_write(R.UC_X86_REG_ECX,detail);cpu.reg_write(R.UC_X86_REG_ESI,out);run(0x223870)
        positions.append([seed,yaw+pitch+radius,[word(out+n*4) for n in range(3)],word(random+4)])
    deadlines=[]
    periods=[(0,0),(0,.01),(.1,.5),(1,7),(30,120),(7,1),(-.1,.1),(-1,-.01)]
    clocks=[0,1,47,1000,0x00ffffff,0x01000001,0x7ffff000,0x80000000,0xfffffc18]
    for initial in (False,True):
      for lower,upper in periods:
       for clock in clocks:
        for immediate in (False,True):
         for seed in (0,0xffffffff,0x3b921e8d):
            put(detail+0xc,bits(lower));put(detail+0x10,bits(upper));put(detail+0x18,4 if immediate else 0)
            put(random+4,seed);put(sound+0x84,clock);cpu.mem_write(stack,bytes(0x100));cpu.reg_write(R.UC_X86_REG_ESP,stack)
            if initial:
                for reg,value in ((R.UC_X86_REG_EDI,root),(R.UC_X86_REG_EBP,0),(R.UC_X86_REG_EBX,random),(R.UC_X86_REG_EDX,out)):
                    cpu.reg_write(reg,value)
                cpu.reg_write(R.UC_X86_REG_XMM1,0);cpu.reg_write(R.UC_X86_REG_XMM2,bits(1000))
                run(0x221980,0x2219f3)
            else:
                cpu.reg_write(R.UC_X86_REG_ESI,detail);cpu.reg_write(R.UC_X86_REG_EBP,out)
                run(0x22216d,0x2221cb)
            deadlines.append([initial,[bits(lower),bits(upper)],clock,immediate,seed,word(out),word(random+4)])
    # Component input records only: run the whole original spatial-detail
    # request builder and callback without substituting any child result.
    pool=0x1006000;group=0x1007000;tags=0x1008000;sound_root=0x1009000
    put(0x50ebe8,pool);put(pool+0x38,1);put(pool+0x24,0xd4);put(pool+0x44,group)
    put(group,0x1234);put(0x4f0414,tags);put(tags+8,sound_root);put(detail+8,0x56780000)
    requests=[]
    for index,row in enumerate(positions):
        seed,bounds,offset,_=row
        parent=bytearray(68);parent[3]=1
        parent_words=[bits((draw()%10000)/1000),bits((draw()%64000-32000)/1000)]
        parent_words += [bits((draw()%400000-200000)/1000) for _ in range(3)]
        for at,value in zip((4,8,12,16,20),parent_words):struct.pack_into('<I',parent,at,value)
        cpu.mem_write(group+0xc,bytes(parent))
        for at,value in zip((0x1c,0x20,0x24,0x28,0x2c,0x30),bounds):put(detail+at,value)
        put(random+4,seed);cpu.mem_write(out,bytes(160));cpu.mem_write(stack,bytes(256))
        for at,value in enumerate((sentinel,detail,0x12340000,out)):put(stack+at*4,value)
        cpu.reg_write(R.UC_X86_REG_ESP,stack);run(0x223670)
        assert cpu.reg_read(R.UC_X86_REG_EAX)&255==1
        assert word(out)==0x2f and word(out+8)==0x10 and word(out+0x50)==0x12340000
        requests.append([seed,bounds,parent.hex(),[word(out+at) for at in (0x10,0x14,0x18,0x1c,0x20)],word(random+4)])
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,positions=positions,deadlines=deadlines,spatial_requests=requests,
        executed_original_instructions=instructions,source_instruction_patches=[],supplied_callee_results=[],
        detail_admission_or_playback_verified=False)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,separators=(',',':'))+'\n')
    print(json.dumps(dict(position_cases=len(positions),deadline_cases=len(deadlines),bytes=args.output.stat().st_size,sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())),flush=True)
if __name__=='__main__':main()
