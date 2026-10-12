#!/usr/bin/env python3
"""Compare native hand IK and aiming state against untouched retail instructions."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT/'analysis/first-person/ik-differential.json')
    args = parser.parse_args()
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    xbe = Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(xbe.data).hexdigest()!=EXPECTED: raise ValueError('unexpected retail XBE')
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
    for section in xbe.sections:g.mem_write(section.va,xbe.section_bytes(section))
    g.mem_map(0x1000000,0x200000);g.mem_map(0x3000000,4096)
    stop=0x3000000;stack=0x11fff00;base=0x1000000
    def word(a,v):g.mem_write(a,struct.pack('<I',v&0xffffffff))
    def call(a,registers,arguments=()):
        g.mem_write(stack,struct.pack('<'+'I'*(1+len(arguments)),stop,*arguments));g.reg_write(UC_X86_REG_ESP,stack)
        for r,v in registers.items():g.reg_write(r,v)
        g.emu_start(a,stop,count=20000)
        if g.reg_read(UC_X86_REG_EIP)!=stop:raise RuntimeError(f'original {a:x} failed to return')
    def floats(a,n):return list(struct.unpack('<'+'f'*n,bytes(g.mem_read(a,n*4))))
    def put(a,f):g.mem_write(a,struct.pack('<'+'f'*len(f),*f))
    def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
    def identity(position=(0,0,0)):return [1,1,0,0,0,1,0,0,0,1,*position]
    rng=random.Random(0x168644);inputs=[];expected=[];kinds=[]
    def add(kind,data,answer):inputs.append(' '.join(map(str,[kind,*data])));expected.append(answer);kinds.append(kind)
    # 1421B0 calls the complete source quaternion/translation constructor.
    for i in range(1024):
        p=[f32(rng.uniform(-100,100))for _ in range(3)]
        q=[f32(rng.uniform(-2,2))for _ in range(4)] if i else [0,0,0,0]
        put(base+0x100,p+q);call(0x1421b0,{UC_X86_REG_EDX:base,UC_X86_REG_ESI:base+0x100,UC_X86_REG_ECX:base+0x10c})
        add(1,p+q,floats(base,13))
    # 120220: ECX=root, EAX=middle, stack=target,end.
    for i in range(1024):
        origin=[f32(rng.uniform(-50,50))for _ in range(3)]
        transforms=[identity(origin)]
        for _ in range(3):transforms.append(identity([f32(x+rng.uniform(-1,1))for x in origin]))
        if i<4:transforms[i]=transforms[0].copy()
        for j,t in enumerate(transforms):put(base+j*0x100,t)
        call(0x120220,{UC_X86_REG_ECX:base,UC_X86_REG_EAX:base+0x100},(base+0x300,base+0x200))
        add(0,sum(transforms,[]),floats(base,13)+floats(base+0x100,13)+floats(base+0x200,13))
    # Full 1D9240/1D9320/1D9370; source selects before the requested advances.
    word(0x510c54,base+0x1000)
    for i in range(2048):
        count=rng.randrange(0,127);value=rng.randrange(count+1);curve=rng.randrange(5);flags=rng.randrange(4)
        eligible=rng.randrange(2);ticks=rng.choice([15,30,60,120]);advance=rng.randrange(9)
        g.mem_write(base,struct.pack('<bbbB',value,count,curve,flags));g.mem_write(base+0x1002,struct.pack('<h',ticks))
        duration=struct.unpack('<I',struct.pack('<f',.267 if eligible else .06675))[0]
        call(0x1d9240,{UC_X86_REG_ECX:base,UC_X86_REG_EBX:eligible},(duration,))
        changed=0
        for _ in range(advance):
            call(0x1d9320,{UC_X86_REG_ECX:base});changed|=g.reg_read(UC_X86_REG_EAX)&255
        state=list(struct.unpack('<bbbB',bytes(g.mem_read(base,4))))
        call(0x1d9370,{UC_X86_REG_EAX:base})
        weight=struct.unpack('<f',struct.pack('<I',g.reg_read(UC_X86_REG_XMM0)&0xffffffff))[0]
        add(2,[value,count,curve,flags,eligible,ticks,advance],state+[weight,changed])
    # 1112F0 and its actual C7070 /111590 /1DAEA0 callees, with two source
    # animation records and source unit/control channels. No branch is hooked.
    unit=base+0x3000;table=base+0x4000;graph=base+0x6000;animation=base+0x7000
    word(0x4e0300,base+0x2000);word(base+0x2044,base+0x2800);word(base+0x2808,unit)
    word(0x4e3b44,table);word(table+24,graph);word(graph+0x30,animation)
    g.mem_write(unit+0x12a,struct.pack('<h',0x600));g.mem_write(unit+0x33e,struct.pack('<h',0x800))
    word(unit+0x600,1);word(unit+0x604,0x0000ffff)
    word(unit+0x89c,1);word(unit+0x8a0,0x0001ffff)
    g.mem_write(0x510c20,b'\0\0')
    admitted=[0xffffffff,0x500000a,0x6000006,0x6000007,0x9000004,0x9000005,0x9000008,0x9000009,0xa000040,0xa0005b9,0xa0005bb,0xb0005b2,0xc000073,0xc000075,0xc000077,0xc0006b3,0xc0006cd,0xe000038,0xe000607,0xe000608,0xe000609,0xe00060a,0xe00067d,0x11000074,0x11000076,0x130005bc,0x140005b3]
    denied=[0x5000049,0x7000679,0x700067a,0x700067b,0x700067c,0x400004a,0x600008c,0xa00003e,0xb000059,0xb00005a,0xc000043,0xc000058,0xc00005b,0xd00003f,0xd000042,0xe00003b,0xe0000c2,0xf00003a,0x1000003c]
    for i in range(2048):
        state=rng.choice([0xffffffff,0x0400000c,*denied]);action=rng.choice([*admitted,0x0b000065,0x0c000064,0xdeadbeef])
        flags0=rng.randrange(16);flags1=rng.randrange(16);dual=rng.randrange(2)
        word(unit+0x67c,state);word(unit+0x8a4,action)
        g.mem_write(animation+0x18,struct.pack('<H',flags0));g.mem_write(animation+0x6c+0x18,struct.pack('<H',flags1))
        call(0x1112f0,{UC_X86_REG_EDI:0});answer=g.reg_read(UC_X86_REG_EAX)&255
        g.mem_write(unit+0x212,b'\0'+(b'\1'if dual else b'\xff'))
        call(0xcd660,{UC_X86_REG_EAX:0});answer=answer and not(g.reg_read(UC_X86_REG_EAX)&255)
        add(4,[state,action,flags0,flags1,dual],[int(answer)])
    # Complete marker attachment and recursive descendant propagation, using
    # retained authored SMG idle nodes from the original-frame investigation.
    sys.path.insert(0,str(ROOT/'tools'));from inspect_multiplayer import Cache
    cache=Cache(ROOT/'assets/disc/maps/cyclotron.map');arm=cache.tags[0xeb1a0995]
    armbytes=bytes(cache.data[arm['offset']:arm['offset']+arm['size']]);va=arm['offset']+cache.magic
    g.mem_map(va&~4095,((va+len(armbytes)+4095)&~4095)-(va&~4095));g.mem_write(va,armbytes);word(table+0x995*16+8,va)
    parents=[cache.h(n+4)for n in cache.block(arm['offset']+0x48,0x60)]
    p=ROOT/'analysis/multiplayer-shading/original-xemu'
    rows=[list(map(float,l.split()))for l in (p/'fp-idle-matrices.tsv').read_text().splitlines()]
    def row_transform(r):return [1,r[0],r[4],r[8],r[1],r[5],r[9],r[2],r[6],r[10],r[3],r[7],r[11]]
    markers=[]
    for tag in [arm,cache.tags[0xe6b70542]]:
        group=next(a for a in cache.block(tag['offset']+0x58,12)if cache.u(a)==0x9000100)
        marker=cache.block(group+4,36)[0];g.mem_write(base+0x100,bytes(cache.data[marker+4:marker+32]))
        call(0x1421b0,{UC_X86_REG_EDX:base,UC_X86_REG_ESI:base+0x100,UC_X86_REG_ECX:base+0x10c});markers.append(floats(base,13))
    for frame in [0,1,16,32,48,64,80,88]:
        selected=[r for r in rows if r[0]==frame]
        arms=[row_transform(next(r[2:14]for r in selected if r[1]==1000+n))for n in range(37)]
        weapons=[row_transform(next(r[2:14]for r in selected if r[1]==n))for n in range(4)]
        for weight in [0,.00001,.001,.1,.5,.9,1,1.1]:
            for n,t in enumerate(arms):put(base+0x10000+n*52,t)
            if abs(weight)>=.0001:
                put(base+0x9000,weapons[0]);put(base+0x9100,markers[1]);put(base+0x9300,markers[0])
                call(0x142a60,{UC_X86_REG_ECX:base+0x9000,UC_X86_REG_EDX:base+0x9100},(base+0x9200,))
                call(0x1d90e0,{UC_X86_REG_EAX:0xeb1a0995,UC_X86_REG_ESI:5,UC_X86_REG_EBX:base+0x10000},(base+0x9300,base+0x9200,struct.unpack('<I',struct.pack('<f',weight))[0],37))
            add(3,[37,4,*parents,*sum(arms,[]),*sum(weapons,[]),5,0,*markers[0],*markers[1],weight],floats(base+0x10000,37*13))
    # Execute the retail sorted lookup and full category iterator, using
    # Chief's authored unit graph. The resource residency side effects of
    # 1DB170/1DBC80 are outside this metadata/animation selection fixture.
    unit_graph_id=0xea4e08c9;unit_graph=cache.tags[unit_graph_id];unit_va=unit_graph['offset']+cache.magic
    unit_bytes=bytes(cache.data[unit_graph['offset']:unit_graph['offset']+unit_graph['size']])
    g.mem_map(unit_va&~4095,((unit_va+len(unit_bytes)+4095)&~4095)-(unit_va&~4095));g.mem_write(unit_va,unit_bytes)
    modes=[cache.u(m)for m in cache.block(unit_graph['offset']+0x34,20)]
    categories=[]
    for m in cache.block(unit_graph['offset']+0x34,20):
        for c in cache.block(m+4,20):
            for t in cache.block(c+4,0x34):categories.append((cache.u(m),cache.u(c),cache.u(t)))
    for i in range(1024):
        mode,weapon_class,type=rng.choice(categories)
        if i%3==0:type=0x03000dcc
        if i%5==0:mode=0x06000086
        action=rng.choice([0x0400000c,0x06000006,0x08000002]);overlay=int(action!=0x0400000c)
        iterator=base+0xb000;found_mode=base+0xb100;found_class=base+0xb104;found_type=base+0xb108
        g.mem_write(iterator,struct.pack('<7I',unit_va,mode,weapon_class,type,0,0,0));answer=[0xffffffff]*4
        if weapon_class!=0xffffffff:
            while True:
                call(0x2963f0,{UC_X86_REG_ESI:iterator,UC_X86_REG_EBX:found_type},(found_mode,found_class))
                entry=g.reg_read(UC_X86_REG_EAX)
                if not entry:break
                call(0x1dd560,{},(entry+(12 if overlay else 4),action,8));named=g.reg_read(UC_X86_REG_EAX)
                if not named:continue
                inheritance,index=struct.unpack('<hh',bytes(g.mem_read(named+4,4)))
                if index==-1:continue
                resolved=unit_graph_id
                if inheritance!=-1:resolved=cache.u(cache.block(unit_graph['offset']+0x4c,32)[inheritance]+4)
                animation=cache.block(cache.tags[resolved]['offset']+0x2c,0x6c)[index]
                answer=[resolved,index,cache.h(animation+0x18)&65535,cache.h(animation+0x14)];break
        add(5,[unit_graph_id,mode,weapon_class,type,action,overlay],answer)
    result=subprocess.run([str(args.probe.resolve()),'--probe'],input='\n'.join(inputs)+'\n',text=True,capture_output=True,check=True)
    lines=result.stdout.splitlines()
    if len(lines)!=len(expected):raise RuntimeError('native probe output count mismatch')
    report={};failures=[]
    for index,(kind,ref,line)in enumerate(zip(kinds,expected,lines)):
        got=list(map(float,line.split()));entry=report.setdefault(str(kind),dict(cases=0,bit_mismatched_cases=0,max_absolute_error=0))
        entry['cases']+=1
        if len(got)!=len(ref):raise RuntimeError('native probe field count mismatch')
        error=max(abs(x-y)for x,y in zip(got,ref));entry['max_absolute_error']=max(entry['max_absolute_error'],error)
        bitdiff=sum(struct.pack('<f',a)!=struct.pack('<f',b)for a,b in zip(got,ref))
        if bitdiff:
            entry['bit_mismatched_cases']+=1
            if len(failures)<12:failures.append(dict(index=index,kind=kind,error=error,fields=[(j,a,b)for j,(a,b)in enumerate(zip(got,ref))if struct.pack('<f',a)!=struct.pack('<f',b)][:8]))
    output=dict(executable_sha256=EXPECTED,original_functions=['1421B0','120220','1D9240','1D9320','1D9370','1112F0','C7070','CD660','111590','1DAEA0','1D90E0','16D8D0','2963F0','1DD560'],groups=report,first_mismatches=failures)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(output,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    if any(e['bit_mismatched_cases']for e in report.values()):raise SystemExit(1)

if __name__=='__main__':main()
