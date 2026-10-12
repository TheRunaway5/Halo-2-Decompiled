#!/usr/bin/env python3
"""Run untouched 443B2..443DF feature admission with explicit source banks."""
import argparse, hashlib, json, struct, subprocess, sys
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESP

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-mp-shading/source_render_policy_tests')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/render-policy-differential.json')
    p.add_argument('--write-fixture',type=Path)
    a=p.parse_args();x=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(x.data).hexdigest()!=PIN:raise RuntimeError('unsupported original executable')
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    u.mem_map(0x2100000,0x100000);u.mem_map(0x3000000,0x1000)
    def put(at,value):u.mem_write(at,struct.pack('<I',value))
    stack=0x21fff00
    banks=[]
    for players in range(6):
        put(stack,0x3000000);u.reg_write(UC_X86_REG_ESP,stack);u.reg_write(UC_X86_REG_EAX,players)
        u.emu_start(0x2c490,0x3000000,count=1000)
        banks.append(list(u.mem_read(0x4ba020,6)))
    banks.extend([[255 if bits&(1<<i) else 0 for i in range(6)] for bits in range(64)])
    rows=[[layer,*bank] for bank in banks for layer in range(32)]
    expected=[]
    for layer,*bank in rows:
        u.mem_write(0x4ba020,bytes(bank));u.mem_write(stack+15,b'\1')
        u.reg_write(UC_X86_REG_ESP,stack);u.reg_write(UC_X86_REG_EDI,layer)
        u.emu_start(0x443b2,0x443df,count=100)
        if u.reg_read(UC_X86_REG_EIP)!=0x443df:raise RuntimeError('source admission did not finish')
        expected.append(int(u.mem_read(stack+15,1)[0]!=0))
    encoded=[' '.join(f'{value:x}' for value in row) for row in rows]
    result=subprocess.run([str(a.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    actual=[int(line) for line in result.stdout.splitlines()]
    if len(actual)!=len(rows):raise RuntimeError('native admission cases missing')
    failures=[{'case':i,'input':row,'source':s,'native':n} for i,(row,s,n) in enumerate(zip(rows,expected,actual)) if s!=n]
    if a.write_fixture and not failures:
        a.write_fixture.write_text('# Untouched443B2..443DF feature slice; source '+PIN+'\n'+
            '\n'.join(row+' '+str(value) for row,value in zip(encoded,expected))+'\n')
    report={'original_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(a.binary.read_bytes()).hexdigest(),
        'entries':['2C490','443B2..443DF'],'source_code_hooks':0,'cases':len(rows),
        'bit_exact_cases':len(rows)-len(failures),'failures':failures[:12],
        'boundary':'Original feature admission only. Available-layer masks, context flags, source owner traversal, layer17 alpha and postprocess admission remain independent source services.'}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
