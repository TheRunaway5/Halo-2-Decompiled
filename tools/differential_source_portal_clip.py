#!/usr/bin/env python3
"""Untouched retail11FC80/173890/173910 portal math differential.

The source routines and all their pure callees run directly without service
hooks. Input memory supplies literal plane/eye/polygon fields. This oracle does
not construct original visibility context, admit portals or replace173130.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys

from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EBX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_EIP)

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x22fff00,0x3000000
def word(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def scalar(value):return struct.unpack('<f',struct.pack('<I',value))[0]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer-shading/resize-build/source_portal_clip_tests')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/portal-clip-differential.json')
    parser.add_argument('--write-fixture',type=Path)
    args=parser.parse_args();image=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(image.data).hexdigest()!=PIN:raise ValueError('unsupported original executable')
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    for section in image.sections:guest.mem_write(section.va,image.section_bytes(section))
    guest.mem_map(BASE,0x200000);guest.mem_map(STOP,0x1000)
    calls={address:0 for address in [0x11fc80,0x173890,0x173910,0x120850,0x23a220]}
    def count_call(uc,address,size,data):
        if address in calls:calls[address]+=1
    guest.hook_add(UC_HOOK_CODE,count_call)
    def put(at,*words):guest.mem_write(at,struct.pack('<'+'I'*len(words),*(v&0xffffffff for v in words)))
    def get(at,n=1):return list(struct.unpack('<'+'I'*n,guest.mem_read(at,n*4)))
    def original(row):
        guest.mem_write(BASE,bytes(0x200000));guest.reg_write(UC_X86_REG_FPCW,0x027f)
        guest.reg_write(UC_X86_REG_MXCSR,0x1f80);put(STACK,STOP);guest.reg_write(UC_X86_REG_ESP,STACK)
        mode,*values=row;plane=values[:4];values=values[4:]
        context,collision,frustum,portal,planes,polygon,output=[BASE+x for x in (0x1000,0x2000,0x3000,0x4000,0x5000,0x6000,0x7000)]
        put(planes,*plane)
        if mode==0:
            positive,epsilon,capacity,count,*vertices=values;put(polygon,*vertices)
            put(STACK+4,planes,positive,epsilon,count,polygon,capacity,output)
            guest.emu_start(0x11fc80,STOP,count=500000)
            n=guest.reg_read(UC_X86_REG_EAX)
            if n>capacity:raise RuntimeError('original clip exceeds output capacity')
            result=[n,*get(output,n*3)]
        else:
            eye=values[:3];values=values[3:]
            clipped=values.pop(0) if mode==1 else 0
            count,*vertices=values;put(polygon,*vertices)
            put(context+4,collision,frustum);put(collision+0xc,planes)
            put(frustum+0x60,*eye);put(portal+4,0);put(portal+0x1c,count,polygon)
            guest.reg_write(UC_X86_REG_EAX,context)
            if mode==1:
                guest.reg_write(UC_X86_REG_ESI,portal);guest.reg_write(UC_X86_REG_EBX,clipped)
                guest.reg_write(UC_X86_REG_EDI,output);guest.mem_write(output,b'\xa5')
                guest.emu_start(0x173890,STOP,count=500000)
                result=[guest.reg_read(UC_X86_REG_EAX),guest.mem_read(output,1)[0]]
            else:
                put(STACK+4,portal);guest.emu_start(0x173910,STOP,count=500000)
                result=[guest.reg_read(UC_X86_REG_EAX)]
        if guest.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('original portal routine did not return')
        return result
    rng=random.Random(0x173890);groups=[[],[],[]]
    for i in range(2048):
        count=i%65;capacity=(i*17+3)%65
        plane=[rng.uniform(-3,3) for _ in range(4)]
        if i%8==0:plane=[0,0,1,0]
        elif i%8==1:plane=[1,1,1,.0078125]
        epsilon=rng.choice([0,.0078125,.0001,-.0078125])
        vertices=[]
        for j in range(count):
            p=[rng.uniform(-100,100) for _ in range(3)]
            if i%8==0:p[2]=rng.choice([0,.0078125,-.0078125,1,-1])
            if i%8==1:p=[rng.choice([65536,-65536]),rng.uniform(-.1,.1),rng.choice([65536,-65536])]
            vertices.extend(map(word,p))
        groups[0].append([0,*map(word,plane),i%2,word(epsilon),capacity,count,*vertices])
    # Explicit capacity saturation, inclusive edges and empty input cases.
    for count in [0,1,2,3,4,5,7,8,63,64]:
        for capacity in [0,1,2,3,63,64]:
            for positive in [0,1]:
                points=[[float(j),float(j%3),(-1.)**j] for j in range(count)]
                groups[0].append([0,0,0,word(1.),0,positive,word(.0078125),capacity,count,
                    *[word(v) for p in points for v in p]])
    axes=[(2,1),(1,2),(0,2),(2,0),(1,0),(0,1)]
    for i in range(2048):
        major=i%3;normal=[rng.uniform(-.5,.5) for _ in range(3)]
        normal[major]=rng.choice([-1.,1.]);length=math.sqrt(sum(v*v for v in normal))
        normal=[v/length for v in normal];plane=normal+[rng.uniform(-50,50)]
        if i%16==0:plane=[1,1,1,0] # Explicit major-axis ties.
        elif i%16==1:plane=[0,-1,-1,0]
        elif i%16==2:plane=[-1,0,-1,0]
        elif i%16==3:plane=[0,0,0,0]
        major=max(range(3),key=lambda j:(abs(plane[j]),j))
        a,b=axes[major*2+(plane[major]>0)];count=i%65
        points=[]
        for j in range(count):
            angle=2*math.pi*j/max(count,1);p=[0.,0.,0.];p[a]=math.cos(angle)*10;p[b]=math.sin(angle)*10
            if plane[major]:p[major]=(plane[3]-plane[a]*p[a]-plane[b]*p[b])/plane[major]
            points.append(p)
        eye=[rng.choice([0.,5.,10.,20.]),rng.choice([0.,5.,10.,20.]),rng.choice([0.,5.,10.,20.])]
        if i%2==0 and plane[major]:
            delta=scalar(rng.choice([word(0.),word(.015625),word(.015625)-1,word(.015625)+1,
                word(-.015625),word(-.015625)-1,word(-.015625)+1]))
            eye[major]=(plane[3]+delta-plane[a]*eye[a]-plane[b]*eye[b])/plane[major]
        flattened=[word(v) for p in points for v in p]
        groups[1].append([1,*map(word,plane),*map(word,eye),i%65,count,*flattened])
        groups[2].append([2,*map(word,plane),*map(word,eye),count,*flattened])
    # Near-plane decisions must use extended distance, not the SSE copy.
    for sign in [-1,1]:
        for distance in [.015625,scalar(word(.015625)-1),scalar(word(.015625)+1),0.,-.015625]:
            for clipped in [0,1,2,3,64]:
                groups[1].append([1,word(16777216),word(-16777216),word(1),0,
                    word(1),word(1),word(distance*sign),clipped,0])
    # Exact projected-edge tolerance and positive/negative normal orientation.
    for sign in [-1,1]:
        a,b=axes[4+(sign>0)];points=[]
        for x,y in [(-1,-1),(1,-1),(1,1),(-1,1)]:
            p=[0.,0.,0.];p[a]=x;p[b]=y;points.append(p)
        for x in [1.,scalar(word(1.00005)-1),scalar(word(1.00005)),scalar(word(1.00005)+1),1.0001]:
            eye=[0.,0.,0.];eye[a]=x
            plane=[0,0,sign,0];flattened=[word(v) for p in points for v in p]
            groups[1].append([1,*map(word,plane),*map(word,eye),0,4,*flattened])
            groups[2].append([2,*map(word,plane),*map(word,eye),4,*flattened])
    rows=sum(groups,[]);expected=[original(row) for row in rows]
    encoded=[' '.join(f'{v:x}' for v in row) for row in rows]
    native=subprocess.run([str(args.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(v,16) for v in line.split()] for line in native.stdout.splitlines()]
    if len(actual)!=len(expected):raise RuntimeError('native portal probe lost cases')
    failures=[{'case':i,'input':row,'source':source,'native':result} for i,(row,source,result) in enumerate(zip(rows,expected,actual)) if source!=result]
    if args.write_fixture and not failures:
        # Retain every curated endpoint case and bounded samples of each mode.
        selected=[];offset=0
        for mode,group in enumerate(groups):
            selected.extend(range(offset,offset+([256,256,128][mode])))
            selected.extend(range(offset+2048,offset+len(group)))
            offset+=len(group)
        args.write_fixture.write_text('# Untouched retail11FC80/173890/173910 and pure callees; source '+PIN+'\n'+
            '\n'.join(encoded[i]+' | '+' '.join(f'{v:x}' for v in expected[i]) for i in selected)+'\n')
    report={'source_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),
        'cases':len(rows),'mode_cases':[len(g) for g in groups],'bit_exact_cases':len(rows)-len(failures),
        'compared_words':sum(map(len,expected)),'original_calls':{f'{a:08x}':n for a,n in calls.items()},
        'failures':failures[:8],'boundary':'Finite64vertex pure math inputs; no source service hooks, full173130 visibility admission/construction remains separate.'}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
