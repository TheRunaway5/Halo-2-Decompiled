#!/usr/bin/env python3
"""Complete unmodified retail36F50 background weather-plate initialization."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
from differential_source_weather_initialization import ROOT,PIN,fw,words,Xbe,Cache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
BANK,RNG,STACK,STOP=0x2100000,0x2102000,0x21fff00,0x3000000
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--write-fixture',type=Path);a=p.parse_args()
    xbe=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(xbe.data).hexdigest()!=PIN:raise ValueError('unsupported original executable')
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in xbe.sections:u.mem_write(s.va,xbe.section_bytes(s))
    u.mem_map(BANK,0x100000);u.mem_map(STOP,0x1000)
    def put(p,v):u.mem_write(p,struct.pack('<I',v&0xffffffff))
    put(0x4e7408,RNG);verified=set()
    def code(g,pc,size,_):
        if not 0x36f50<=pc<0x37251:raise RuntimeError(f'unexpected original instruction {pc:08x}')
        if bytes(g.mem_read(pc,size))!=xbe.read(pc,size):raise RuntimeError('changed original instruction')
        verified.add(pc)
    u.hook_add(UC_HOOK_CODE,code)
    def original(row):
        put(RNG,0x11223344);put(RNG+4,row[0]);u.mem_write(BANK,struct.pack('<234I',*row[1:]))
        put(STACK,STOP);u.reg_write(UC_X86_REG_EDX,BANK);u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.emu_start(0x36f50,STOP,count=10000)
        if u.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('original plate did not return')
        if words(u.mem_read(RNG,4))!=[0x11223344]:raise RuntimeError('primary RNG altered')
        return [*words(u.mem_read(RNG+4,4)),*words(u.mem_read(BANK,936))]
    rows=[];assets=[]
    for name in ['lockout','warlock']:
        path=ROOT/('assets/disc/maps' if name=='lockout' else 'assets/map-pack/maps')/(name+'.map')
        c=Cache(path)
        try:
            plates=[]
            for datum,tag in c.tags.items():
                if tag['group']!='weat':continue
                _,at=c.tag(datum,'weat')
                for ordinal,r in enumerate(c.block(at+8,0x3a8)):
                    raw=c.data[r:r+936]
                    plates.append({'datum':f'{datum:08x}','ordinal':ordinal,'sha256':hashlib.sha256(raw).hexdigest()})
                    for seed in [0,1,0xffffffff,0x12345678]:rows.append([seed,*words(raw)])
            assets.append({'map':name,'cache_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'plates':plates})
        finally:c.close()
    rng=random.Random(0x36f50)
    for i in range(1536):
        bank=[rng.getrandbits(32) for _ in range(234)]
        for j in range(9,18):bank[j]=fw(rng.uniform(-64,64))
        if i%8==0:bank[9:18]=[fw(0)]*9
        if i%8==1:bank[9:18]=[fw(.00001)]*9
        if i%8==2:bank[9:18]=[fw(-100000)]*9
        rows.append([rng.getrandbits(32),*bank])
    expected=[original(row) for row in rows]
    encoded=[' '.join(f'{v:x}' for v in row) for row in rows]
    run=subprocess.run([str(a.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(v,16) for v in line.split()] for line in run.stdout.splitlines()]
    if len(actual)!=len(rows):raise RuntimeError('native plate probe lost rows')
    failures=[{'case':i,'different_words':[j for j,(x,y) in enumerate(zip(s,t)) if x!=y]} for i,(s,t) in enumerate(zip(expected,actual)) if s!=t]
    if a.write_fixture and not failures:
        a.write_fixture.parent.mkdir(parents=True,exist_ok=True)
        a.write_fixture.write_text('# Complete unchanged36F50. Input: auxiliary seed,234 bank words; output: auxiliary seed,234 bank words.\n' +
            '\n'.join(x+' '+' '.join(f'{v:08x}' for v in y) for x,y in zip(encoded,expected))+'\n')
    report={'original_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(a.binary.read_bytes()).hexdigest(),
        'cases':len(rows),'exact_cases':len(rows)-len(failures),'compared_words':len(rows)*235,
        'verified_original_instructions':len(verified),'draws_per_case':36,'primary_rng_unchanged':True,'assets':assets,'failures':failures[:8],
        'scope':'Complete36F50 with explicit auxiliary stream and owned plate bank. Launch RNG chronology, later37260 simulation, plate visibility and GPU submission remain separate.'}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='assets'},indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
