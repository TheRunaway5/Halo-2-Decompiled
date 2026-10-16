#!/usr/bin/env python3
"""Whole TU1.5 2AA360/179700/1791C0 script marker-effect request construction.

BAD60 supplies declared completed marker records and counts.178E30 records the
complete96-byte parameters and two28-byte seed markers instead of allocating.
No original instructions change. This proves request construction/gating, not
object lookup, effect allocation, PVS, RNG, particles, lights, lenses or pixels.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    if a.output.exists():raise ValueError('Refusing to overwrite original evidence')
    x=Xbe(a.xbe);pin=hashlib.sha256(x.data).hexdigest()
    if pin!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':
        raise ValueError('Expected original Xbox TU1.5 English image')
    table=struct.unpack('<I',x.read(0x480c00+41*4,4))[0]
    if struct.unpack('<I',x.read(table+4,4))[0]!=0x2aae80:
        raise ValueError('Original script evaluator registration differs')
    cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,size in ((0x10000,0x600000),(0x2100000,0x100000),(0x3000000,4096)):
        cpu.mem_map(at,size)
    for section in x.sections:cpu.mem_write(section.va,x.section_bytes(section))
    stack,stop=0x21fff00,0x3000000
    def words(at,count):return list(struct.unpack('<'+'I'*count,cpu.mem_read(at,4*count)))
    def put(at,values):cpu.mem_write(at,struct.pack('<'+'I'*len(values),*values))
    def ret(pop,result=0):
        sp=cpu.reg_read(R.UC_X86_REG_ESP);pc=words(sp,1)[0]
        cpu.reg_write(R.UC_X86_REG_ESP,sp+4+pop)
        cpu.reg_write(R.UC_X86_REG_EAX,result);cpu.reg_write(R.UC_X86_REG_EIP,pc)
    active={};lookups=[];submissions=[];executed=set()
    def hook(uc,pc,size,user):
        if bytes(uc.mem_read(pc,size))!=x.read(pc,size):raise ValueError('Original instruction changed')
        executed.add(pc);sp=uc.reg_read(R.UC_X86_REG_ESP)
        if pc==0xbad60:
            obj,marker,maximum=words(sp+4,3)
            if maximum!=1 or uc.reg_read(R.UC_X86_REG_ECX)&255:
                raise ValueError('Original lookup no longer requests one noninterpolated marker')
            ordinal=len(lookups);lookups.append([obj,marker,maximum,0])
            raw=bytearray(112)
            raw[0x38:0x6c]=struct.pack('<13I',*active['world'][ordinal])
            uc.mem_write(uc.reg_read(R.UC_X86_REG_EBX),bytes(raw))
            ret(12,active['counts'][ordinal])
        elif pc==0x178e30:
            parameters=words(words(sp+4,1)[0],24)
            seeds=words(parameters[7],14)
            parameters[7]=0 # Native caller owns its actual backing address.
            submissions.append(dict(parameters=parameters,seeds=seeds));ret(4,0xffffffff)
    cpu.hook_add(UC_HOOK_CODE,hook)
    def bits(values):return list(struct.unpack('<'+'I'*len(values),struct.pack('<'+'f'*len(values),*values)))
    rng=random.Random(0x2aa360);cases=[]
    for n in range(192):
        effect=(0xea03088e,0xea070892,0xffffffff)[n%3]
        obj=(0x54000001,0x2345001a,0,0xffffffff)[(n//3)%4]
        marker=(0x130008fd,0x0700054e,0,0xffffffff)[(n//12)%4]
        counts=((1,1),(0,1),(1,0),(1,1))[(n//48)%4]
        world=[bits([1,1,0,0,0,1,0,0,0,1,*[rng.uniform(-100,100)for _ in range(3)]])for _ in range(2)]
        active=dict(world=world,counts=counts);lookups.clear();submissions.clear()
        put(stack,[stop,effect]);cpu.reg_write(R.UC_X86_REG_ESP,stack)
        cpu.reg_write(R.UC_X86_REG_EDI,obj);cpu.reg_write(R.UC_X86_REG_EAX,marker)
        cpu.reg_write(R.UC_X86_REG_EFLAGS,2);cpu.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        cpu.reg_write(R.UC_X86_REG_FPCW,0x027f)
        cpu.emu_start(0x2aa360,stop,count=100000)
        if cpu.reg_read(R.UC_X86_REG_EIP)!=stop:raise ValueError('Original request wrapper did not return')
        cases.append(dict(effect=effect,object=obj,marker=marker,counts=counts,world=world,
            lookups=lookups.copy(),submissions=submissions.copy()))
    report=dict(scope=__doc__,xbe_sha256=pin,
        generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        entry='002aa360',constructor='00179700',seed_entry='001791c0',
        instruction_patches=[],intercepted_callees=['000bad60','00178e30'],
        executed_instructions=len(executed),cases=cases)
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(len(cases),'original script-marker request cases;',sum(bool(c['submissions'])for c in cases),'complete parameter/seed packets')

if __name__=='__main__':main()
