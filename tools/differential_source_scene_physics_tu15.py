#!/usr/bin/env python3
"""Compare source-root publication with unchanged English TU1.5 BFC70.

Explicit root/parent inputs exercise presentation only, not solved contacts.
No original instructions, callees or expected result buffers are substituted.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, ROOT, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def words(raw):
    return list(struct.unpack('<'+'I'*(len(raw)//4),raw))


def floats(values):
    return words(struct.pack('<'+'f'*len(values),*values))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    code=_Code(image,'.text')
    # Literal caller operand sequence, including scale/+A0 and flag10. Keep
    # this independently authenticated rather than infer offsets from C++.
    listing=[];p=0xbf5c3
    while p<0xbf625:
        i=code.at(p);listing.append(dict(address=f'{p:08x}',bytes=i.bytes.hex(),instruction=i.mnemonic+' '+i.op_str));p+=i.size
    assert p==0xbf625 and listing[-1]['instruction']=='call 0xbfc70'
    assert next(r for r in listing if r['address']=='000bf600')['instruction']=='mov ecx, dword ptr [ebx + 0xa0]'
    assert next(r for r in listing if r['address']=='000bf614')['instruction']=='lea eax, [ebx + 0x7c]'
    assert next(r for r in listing if r['address']=='000bf617')['instruction']=='lea ecx, [ebx + 0x70]'
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x800000)
    for section in image.sections:guest.mem_write(section.va,image.section_bytes(section))
    base=0x70000000;guest.mem_map(base,0x10000)
    position,forward,up,parent,out,stack,stop=[base+x for x in (0,0x20,0x40,0x100,0x200,0xf000,0xff00)]
    instructions={}
    def observe(g,p,n,_):
        if p==stop:return
        original=image.read(p,n);actual=bytes(g.mem_read(p,n));assert actual==original
        instructions[p]=original.hex()
    guest.hook_add(UC_HOOK_CODE,observe)
    rng=random.Random(0xbf620);rows=[];input_lines=[]
    for case in range(256):
        angle=rng.uniform(-math.pi,math.pi)
        p=[rng.uniform(-32,32) for _ in range(3)]
        f=[math.cos(angle),math.sin(angle),0.];u=[0.,0.,1.]
        scale=[1.,.5,2.,1.125][case%4]
        mirrored=bool(case&4);attached=bool(case&8);parent_mirrored=bool(case&16)
        parent_angle=rng.uniform(-math.pi,math.pi)
        c,s=math.cos(parent_angle),math.sin(parent_angle)
        node=[1. if case%3==0 else 2.,c,s,0.,-s,c,0.,0.,0.,1.,*p]
        if case<32:p=[0.,-0.,1.];f=[1.,0.,-0.];u=[-0.,0.,1.];node=[1.,1.,0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.]
        row=floats(p+f+u)+floats([scale])+[int(mirrored),int(attached),int(parent_mirrored)]+floats(node)
        input_lines.append(' '.join(f'{v:08x}' for v in row))
        for address,values in ((position,row[:3]),(forward,row[3:6]),(up,row[6:9]),(parent,row[13:])):
            guest.mem_write(address,struct.pack('<'+'I'*len(values),*values))
        guest.mem_write(out,bytes([0xa5])*0x34)
        guest.mem_write(stack,struct.pack('<6I',stop,position,row[9],row[10],parent if attached else 0,out))
        for reg in (R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI):guest.reg_write(reg,0)
        guest.reg_write(R.UC_X86_REG_EAX,up);guest.reg_write(R.UC_X86_REG_ECX,forward)
        guest.reg_write(R.UC_X86_REG_EBX,int(parent_mirrored));guest.reg_write(R.UC_X86_REG_ESP,stack)
        guest.reg_write(R.UC_X86_REG_EFLAGS,2);guest.reg_write(R.UC_X86_REG_FPCW,0x27f)
        guest.reg_write(R.UC_X86_REG_FPTAG,0xffff);guest.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        guest.emu_start(0xbfc70,stop,count=10000)
        assert guest.reg_read(R.UC_X86_REG_EIP)==stop
        rows.append(dict(input=row,original=words(bytes(guest.mem_read(out,0x34)))))
    run=subprocess.run([str(args.binary.resolve()),'--probe'],input='\n'.join(input_lines)+'\n',text=True,capture_output=True)
    if run.returncode:raise RuntimeError(run.stderr)
    native=[[int(v,16) for v in line.split()]for line in run.stdout.splitlines()]
    assert len(native)==len(rows)
    failures=[]
    for n,(row,result)in enumerate(zip(rows,native)):
        row['native']=result
        if result!=row['original']:failures.append(n)
    report=dict(passed=not failures,original_sha256=NEW_PIN,entry='000bfc70',caller=listing,
        original_instructions=instructions,patched_original_instructions=0,supplied_original_callees=0,
        cases=rows,failed_cases=failures,native_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        limits='Explicit presentation root/parent inputs; no dynamic physics, contact, lift or vehicle execution. Original execution uses Unicorn, not physical Xbox hardware.')
    (args.output/'comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'{"PASS" if not failures else "FAIL"} TU1.5 source root matrices: {len(rows)-len(failures)}/{len(rows)} exact 13-word outputs')
    raise SystemExit(bool(failures))


if __name__=='__main__':main()
