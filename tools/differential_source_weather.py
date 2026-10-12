#!/usr/bin/env python3
"""Run untouched retail weather commands,398D0 and36B50's first-view prefix.

Native rendering never executes Xbox code. This oracle verifies state, retained
records and clocks; it does not substitute for weather particle/GPU validation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW,
                              UC_X86_REG_MXCSR, UC_X86_REG_XMM1, UC_X86_REG_XMM3)

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
MEMORY,STACK,STOP=0x2100000,0x21fff00,0x3000000

def f(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def d(value):return struct.unpack('<Q',struct.pack('<d',value))[0]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-mp-shading/source_weather_tests')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/weather-differential.json')
    p.add_argument('--write-fixture',type=Path)
    args=p.parse_args();image=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(image.data).hexdigest()!=PIN:raise ValueError('unsupported original executable')
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    for section in image.sections:guest.mem_write(section.va,image.section_bytes(section))
    guest.mem_map(MEMORY,0x100000);guest.mem_map(STOP,0x1000)
    addresses=[0x509420,0x4670fc,0x467100,0x509424,0x509428]
    def put(at,word):guest.mem_write(at,struct.pack('<I',word&0xffffffff))
    def words(at,count):return list(struct.unpack('<'+'I'*count,guest.mem_read(at,count*4)))
    def original(row):
        for at,value in zip(addresses,row[1:6]):put(at,value)
        guest.mem_write(MEMORY,struct.pack('<6I',*row[7:13]));put(0x50942c,MEMORY if row[6] else 0)
        put(0x50941c,row[13]);put(0x509418,row[14]);guest.mem_write(0x4858a0,struct.pack('<Q',row[15]))
        guest.mem_write(0x485600,struct.pack('<H',row[18]&0xffff))
        guest.reg_write(UC_X86_REG_FPCW,0x027f);guest.reg_write(UC_X86_REG_MXCSR,0x1f80)
        put(STACK,STOP);put(STACK+4,row[16]);guest.reg_write(UC_X86_REG_ESP,STACK)
        guest.reg_write(UC_X86_REG_XMM1,row[16] if row[0] in (0,1) else row[17])
        guest.reg_write(UC_X86_REG_XMM3,row[16])
        entry=[0x2a0aa0,0x2a0b20,0x2a0ba0,0x398d0,0x36b50][row[0]]
        end=0x36bb3 if row[0]==4 else STOP
        guest.emu_start(entry,end,count=10000)
        if guest.reg_read(UC_X86_REG_EIP)!=end:raise RuntimeError(f'original {entry:08x} did not finish')
        return [words(at,1)[0] for at in addresses]+words(MEMORY,6)+words(0x50941c,1)+words(0x509418,1)
    rng=random.Random(0x36b50);rows=[]
    durations=[-1.,-0.,0.,.00001,.0001,1./30.,.5,10.,20.]
    targets=[-1.,-0.,0.,.3,1.,2.]
    for op in range(5):
        for i in range(384):
            stopped=rng.choice([0,0,1,2,0xffffffff]);current=rng.choice([0.,1.,.3,rng.uniform(-.2,1.2)])
            target=rng.choice(targets);transition=rng.choice([0,1,2,3,4,0xffffffff]);rate=rng.uniform(-4.,4.)
            source=[stopped,f(current),f(target),transition,f(rate)]
            retained=[rng.choice([0,1]),f(rng.random()),f(rng.choice(targets)),rng.choice([0,1,2,3]),f(rng.uniform(-2.,2.)),rng.choice([0,0xdeadbeef])]
            prior=rng.choice([0.,-0.,1234.25,16777216.,-1.]);now=prior+rng.choice([-.1,0.,1./120.,1./30.,.125,2.])
            row=[op,*source,i%3!=0,*retained,f(prior),f(.25),d(now),f(durations[i%len(durations)]),f(targets[i%len(targets)]),i%4]
            rows.append([int(value) for value in row])
    # Correlated start/change/stop and original frame-clock continuations.
    state=[0,f(1),f(1),0,f(0)];record=state+[0];clock=[0,0]
    for op,seconds,target,now,slot in [(2,0,.3,0,0),(4,0,0,300.125,0),(4,0,0,300.158333333,1),
         (4,0,0,300.158333333,0),(1,10,0,0,0),(3,10,0,0,0),(0,20,0,0,0),
         (3,10,0,0,0),(2,20,1,0,0),(3,10,0,0,0),(3,10,0,0,0)]:
        row=[op,*state,1,*record,*clock,d(now),f(seconds),f(target),slot];rows.append(row)
        result=original(row);state=result[:5];record=result[5:11];clock=result[11:]
    source_results=[original(row) for row in rows]
    encoded=[' '.join(f'{word:x}' for word in row) for row in rows]
    native=subprocess.run([str(args.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    results=[[int(word,16) for word in line.split()] for line in native.stdout.splitlines()]
    if len(results)!=len(rows):raise RuntimeError('native weather probe lost source cases')
    failures=[{'case':i,'input':row,'source':expected,'native':actual} for i,(row,expected,actual) in enumerate(zip(rows,source_results,results)) if expected!=actual]
    if args.write_fixture and not failures:
        args.write_fixture.parent.mkdir(parents=True,exist_ok=True)
        selected=list(range(0,1920,16))+list(range(1920,len(rows)))
        lines=['# Untouched retail weather commands/retained record/first-view clock; source executable '+PIN,
               '# 19 hexadecimal input words then13 original output words; double realtime is one64-bit word.']
        lines.extend(encoded[i]+' '+' '.join(f'{v:08x}' for v in source_results[i]) for i in selected)
        args.write_fixture.write_text('\n'.join(lines)+'\n')
    report={'original_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),
            'entries':['002A0AA0','002A0B20','002A0BA0','000398D0','00036B50..00036BB3'],
            'cases':len(rows),'bit_exact_cases':len(rows)-len(failures),'compared_words':len(rows)*13,'failures':failures[:12],
            'scope':'Finite weather commands, state, retained-record admission, first-view realtime clock; particle generation/rendering is separate.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
