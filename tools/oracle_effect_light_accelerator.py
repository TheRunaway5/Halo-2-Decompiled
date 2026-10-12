#!/usr/bin/env python3
"""Whole unchanged2DDBD0/2DCE60 spherical visibility queries.

Uses the456 independently retained query0 bytecode/header banks, including
authored render geometry and explicit opcode controls. Spheres and initial
lists are controlled service inputs, not original gameplay captures. No source
instruction or callee is replaced. Both ordered output lists are retained.
"""
import argparse, collections, hashlib, json, math, random, struct, subprocess, sys
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def sha(data):return hashlib.sha256(data).hexdigest()
def pack(values):return struct.pack('<'+'I'*len(values),*values)
def words(data):return struct.unpack('<'+'I'*(len(data)//4),data)
def bits(x):return words(struct.pack('<f',x))[0]
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
    source=ROOT/'tests/fixtures/source_bsp_accelerator_original.tsv';raw=source.read_bytes();assert sha(raw)=='1962360d46d6565d1c687b0444e66bb85d0cfc77de74ffcfccda438320cd2aae'
    banks={}
    for line in raw.decode().splitlines():
        if line.startswith('B '):_,name,data=line.split();banks[name]=bytes.fromhex(data)
    assert len(banks)==456
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
    for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
    base,tree,query,direct,ranges,stack,stop=0x2000000,0x2010000,0x2020000,0x2030000,0x2040000,0x20ff000,0x2100000
    g.mem_map(base,0x110000);calls=collections.Counter();executed={}
    def observe(u,pc,size,_):
        if pc==0x2d91d0:raise RuntimeError('source list requires allocation; increase explicit fixture capacity')
        if pc not in executed:
            assert bytes(u.mem_read(pc,size))==x.read(pc,size);executed[pc]=size
        if pc in (0x2ddbd0,0x2dce60,0x2ff570,0x2ff210):calls[f'{pc:08X}']+=1
    g.hook_add(UC_HOOK_CODE,observe)
    rng=random.Random(0x2ddbd0);rows=[];max_direct=max_ranges=0;checked=0
    for name,bank in banks.items():
        g.mem_write(tree,bank);info=struct.unpack_from('<4f',bank);cell=f32(65536/info[3]);extent=f32(cell*256)
        cases=[]
        if name.startswith('stock-'):
            for fraction,radius in [(0,0),(.5,0),(.5,.001),(.5,.01),(.5,.05),(.5,.25),(.5,1),(.5,2),(-.1,.1),(1.1,.1)]:
                cases.append([*[f32(v+extent*fraction)for v in info[:3]],f32(extent*radius)])
            for _ in range(6):cases.append([*[f32(v+extent*rng.uniform(-.1,1.1))for v in info[:3]],f32(extent*rng.choice([.001,.01,.1,.5]))])
        else:
            for center,radius in [((0,0,0),0),((0,0,0),1),((1,1,1),0),((1,1,1),1),((1,1,1),math.sqrt(3)),((1,1,1),2),((2,2,2),1),((4,4,4),.125),((0,0,0),-1),((0,0,0),512)]:
                cases.append([*[f32(info[i]+cell*center[i])for i in range(3)],f32(cell*radius)])
            for _ in range(6):cases.append([*[f32(v+cell*rng.uniform(-2,8))for v in info[:3]],f32(cell*rng.uniform(0,5))])
        for ordinal,sphere in enumerate(cases):
            cap=512;initial_direct=[42]if ordinal%3==0 else [];initial_ranges=[7,9]if ordinal%4==0 else []
            g.mem_write(query,struct.pack('<4f',*sphere));g.mem_write(direct,pack([direct+16,len(initial_direct),cap|0x80000000]));g.mem_write(ranges,pack([ranges+16,len(initial_ranges),cap|0x80000000]));g.mem_write(direct+16,pack(initial_direct));g.mem_write(ranges+16,pack(initial_ranges))
            g.mem_write(stack-0x10000,bytes(0x10000));g.mem_write(stack,pack([stop,tree,query,direct,ranges]));g.reg_write(R.UC_X86_REG_ESP,stack)
            for reg in (R.UC_X86_REG_EAX,R.UC_X86_REG_EBX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI,R.UC_X86_REG_EBP):g.reg_write(reg,0)
            for reg,value in [(R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x027f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)]:g.reg_write(reg,value)
            g.emu_start(0x2ddbd0,stop,count=3000000);assert g.reg_read(R.UC_X86_REG_EIP)==stop and g.reg_read(R.UC_X86_REG_ESP)==stack+4
            nd=words(g.mem_read(direct+4,4))[0];nr=words(g.mem_read(ranges+4,4))[0];assert nd<=cap and nr<=cap and nr%2==0
            out_direct=words(g.mem_read(direct+16,nd*4));out_ranges=words(g.mem_read(ranges+16,nr*4));max_direct=max(max_direct,nd);max_ranges=max(max_ranges,nr)
            checked+=2+nd+nr
            sections=[list(map(bits,sphere)),initial_direct,initial_ranges,out_direct,out_ranges]
            row=f'Q {name}-{ordinal} {name} {cap} {cap}'
            for values in sections:row+=' '+str(len(values))+' '+' '.join(f'{v:x}'for v in values)
            rows.append(row.rstrip())
    fixture=a.output/'fixture.tsv';fixture.write_text('# Whole unchanged source spherical visibility; explicit controlled service queries\n'+f'C {len(rows)} {len(banks)}\n'+''.join(f'B {name} {bank.hex()}\n'for name,bank in banks.items())+'\n'.join(rows)+'\n')
    executed_code=b''.join(pack([pc,size])+x.read(pc,size)for pc,size in sorted(executed.items()))
    report=dict(boundary=__doc__,xbe_sha256=PIN,input_banks_sha256=sha(raw),tool_sha256=sha(Path(__file__).read_bytes()),fixture_sha256=sha(fixture.read_bytes()),cases=len(rows),banks=len(banks),checked_output_words=checked,calls=dict(calls),executed_instruction_addresses=len(executed),executed_code_sha256=sha(executed_code),intercepted_instructions=0,max_direct=max_direct,max_range_words=max_ranges,list_capacity=512,fpcw='027F',mxcsr='1F80')
    if a.native:
        run=subprocess.run([str(a.native.resolve()),str(fixture.resolve()),'--sphere'],text=True,capture_output=True);report.update(native_sha256=sha(a.native.read_bytes()),native_exit=run.returncode,native_stdout=run.stdout,native_stderr=run.stderr)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return int(report.get('native_exit',0)!=0)
if __name__=='__main__':raise SystemExit(main())
