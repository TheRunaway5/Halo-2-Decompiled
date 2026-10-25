#!/usr/bin/env python3
"""Execute original222690 insertion and2217C0 retirement on declared ABI rings.

Component data only: no live original voices, readiness, callee results or
instruction patches. Detached intro rings and signed full parent salts vary.
"""
import argparse,hashlib,json,os,random,struct
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
    pool=0x1001000;parent=0x1002000;vp=0x1003000;voices=0x1004000;stack=0x101e000;stop=0x101f000
    def put(at,value):cpu.mem_write(at,struct.pack('<I',value&0xffffffff))
    def word(at):return struct.unpack('<I',cpu.mem_read(at,4))[0]
    def vid(index):return 0xffffffff if index<0 else 0x12340000+index
    def ordinal(value):return -1 if value==0xffffffff else value&65535
    put(0x50ebe8,pool);put(pool+0x24,0xd4);put(pool+0x38,1);put(pool+0x44,parent)
    put(0x4f2c4c,vp);put(vp+0x44,voices)
    instructions={}
    def observe(_cpu,pc,size,_):
        if pc==stop:cpu.emu_stop();return
        instruction=code.at(pc);assert instruction and bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=instruction.bytes.hex()
    cpu.hook_add(UC_HOOK_CODE,observe)
    def run(entry,end=stop):
        cpu.reg_write(R.UC_X86_REG_EFLAGS,2);cpu.emu_start(entry,end,count=10000)
        assert cpu.reg_read(R.UC_X86_REG_EIP)==end
    rng=random.Random(0x381c21);cases=[]
    for case in range(768):
        remove=bool(case%2);count=1+rng.randrange(8) if remove else rng.randrange(9)
        indices=list(range(count));rng.shuffle(indices);initial={};start=0
        while start<count:
            size=1+rng.randrange(count-start);part=indices[start:start+size]
            for index,item in enumerate(part):initial[item]=[part[(index+1)%size],part[index-1]]
            start+=size
        primary=rng.randrange(-1,count) if count else -1;selected=rng.randrange(count) if remove else count
        salt=0xe96c if case%3 else 0x5678;group=salt<<16
        cpu.mem_write(parent,bytes(0xd4));cpu.mem_write(voices,bytes(12*0xbc));cpu.mem_write(stack,bytes(256))
        cpu.mem_write(parent,struct.pack('<H',salt));cpu.mem_write(parent+0x51,bytes([count]));put(parent+0x88,vid(primary))
        for index in range(count+int(not remove)):
            at=voices+index*0xbc;put(at+0x5c,0xffffffff);put(at+0x6c,vid(index));put(at+0x70,vid(index))
            if index in initial:put(at+0x6c,vid(initial[index][0]));put(at+0x70,vid(initial[index][1]))
        put(stack,stop);cpu.reg_write(R.UC_X86_REG_ESP,stack)
        if remove:
            for n,value in enumerate((group,vid(selected),0)):put(stack+4+n*4,value)
            run(0x2217c0)
        else:
            for register,value in ((R.UC_X86_REG_EAX,group),(R.UC_X86_REG_ECX,0),(R.UC_X86_REG_ESI,vid(selected))):cpu.reg_write(register,value)
            run(0x222690)
            # Complete caller's real count store, with its intervening POP.
            cpu.reg_write(R.UC_X86_REG_EBX,parent);cpu.reg_write(R.UC_X86_REG_ESP,stack)
            run(0x22266f,0x222678)
        active=sorted((set(range(count))-{selected}) if remove else set(range(count+1)))
        after=[[i,ordinal(word(voices+i*0xbc+0x6c)),ordinal(word(voices+i*0xbc+0x70))] for i in active]
        cases.append(dict(remove=remove,input_primary=primary,input=[[i,*initial[i]] for i in sorted(initial)],
            selected=selected,parent_salt=salt,output_primary=ordinal(word(parent+0x88)),output=after,
            output_count=cpu.mem_read(parent+0x51,1)[0]))
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=cases,executed_original_instructions=instructions,
        source_instruction_patches=[],supplied_callee_results=[],live_source_admission_verified=False)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,separators=(',',':'))+'\n')
    print(json.dumps(dict(cases=len(cases),bytes=args.output.stat().st_size,sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())),flush=True)
if __name__=='__main__':main()
