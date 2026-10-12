#!/usr/bin/env python3
"""Complete unchanged37260 with actual authored plates and explicit caller banks.

All source instructions execute unchanged. Resource publication, view camera,
elapsed/intensity and initial auxiliary chronology remain supplied inputs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from differential_source_weather_initialization import PIN,fw,words,Xbe,Cache
BANK,WEATHER,RNG,STACK,STOP=0x2100000,0x2101000,0x2102000,0x21fff00,0x3000000
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--write-fixture',type=Path)
    a=p.parse_args();xbe=Xbe(ROOT/'assets/disc/default.xbe')
    assert sha(xbe.data)==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in xbe.sections:u.mem_write(s.va,xbe.section_bytes(s))
    u.mem_map(BANK,0x100000);u.mem_map(STOP,0x1000)
    def put(p,v):u.mem_write(p,pack([v&0xffffffff]))
    put(0x4e7408,RNG);verified=set();initializing=[False]
    def code(g,pc,size,_):
        begin,end=(0x36f50,0x37251) if initializing[0] else (0x37260,0x37a53)
        if not begin<=pc<end:raise RuntimeError(f'unexpected source instruction {pc:08X}')
        if bytes(g.mem_read(pc,size))!=xbe.read(pc,size):raise RuntimeError('changed source instruction')
        if not initializing[0]:verified.add(pc)
    u.hook_add(UC_HOOK_CODE,code)
    def reset():
        for r in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(r,0)
        u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2)
        u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_FPTAG,0xffff)
        u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
    def initialize(plate,seed):
        reset();put(RNG,0x11223344);put(RNG+4,seed);u.mem_write(BANK,pack(plate));put(STACK,STOP)
        u.reg_write(UC_X86_REG_EDX,BANK);initializing[0]=True
        u.emu_start(0x36f50,STOP,count=10000);initializing[0]=False
        assert u.reg_read(UC_X86_REG_EIP)==STOP
        return words(u.mem_read(BANK,936)),words(u.mem_read(RNG+4,4))[0]
    def original(row):
        seed,elapsed,intensity=row[:3];plate=row[3:237];view=row[237:298];weather=row[298:]
        reset();put(RNG,0x11223344);put(RNG+4,seed)
        u.mem_write(BANK,pack(plate));u.mem_write(WEATHER,pack(weather))
        u.mem_write(0x485600,pack(view));put(0x4670fc,intensity)
        u.mem_write(STACK,pack([STOP,BANK,elapsed,WEATHER]))
        u.emu_start(0x37260,STOP,count=10000)
        if u.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('original plate step did not return')
        if words(u.mem_read(RNG,4))!=[0x11223344]:raise RuntimeError('original changed primary stream')
        assert u.reg_read(UC_X86_REG_ESP)==STACK+16
        return [words(u.mem_read(RNG+4,4))[0],u.reg_read(UC_X86_REG_EAX)&255,
            *words(u.mem_read(BANK,936)),*words(u.mem_read(0x485600,244)),*words(u.mem_read(WEATHER,172))]
    rng=random.Random(0x37260);rows=[];assets=[]
    def view_bank(slot):
        view=[0]*61;view[0]=slot
        for at,v in [(0x18,3.),(0x1c,5.),(0x20,7.),(0x24,1.),(0x40,.785),
            (0x8c,1.),(0x90,1.),(0xa0,1.),(0xb0,1.),(0xc0,1.),(0xc4,1.),(0xd4,1.),(0xe4,1.),
            (0xe8,3.),(0xec,5.),(0xf0,7.),(0xb4,-3.),(0xb8,-5.),(0xbc,-7.)]:view[at//4]=fw(v)
        return view
    for name in ['lockout','warlock']:
        path=ROOT/('assets/disc/maps' if name=='lockout' else 'assets/map-pack/maps')/(name+'.map');c=Cache(path)
        try:
            records=[]
            for datum,tag in c.tags.items():
                if tag['group']!='weat':continue
                _,at=c.tag(datum,'weat');weather=words(c.data[at:at+172])
                for ordinal,r in enumerate(c.block(at+8,0x3a8)):
                    raw=c.data[r:r+936];records.append({'datum':f'{datum:08X}','ordinal':ordinal,'plate_sha256':sha(raw),'weather_root_sha256':sha(c.data[at:at+172])})
                    for seed in [0,1,0xffffffff,0x12345678]:
                        plate,after=initialize(words(raw),seed)
                        for slot in range(4):
                            rows.append([after,fw(1./30.),fw(.75),*plate,*view_bank(slot),*weather])
            assets.append({'map':name,'cache_sha256':sha(path.read_bytes()),'plates':records})
        finally:c.close()
    for i in range(1536):
        plate=[rng.getrandbits(32) for _ in range(234)]
        for at in range(6,50):plate[at]=fw(rng.uniform(-4,4))
        plate[0x94//4]=i%8
        for at in range(0xc8//4,234):plate[at]=fw(rng.uniform(-4,4))
        if i%16==0:plate[0x7c//4]=plate[0x78//4]
        if i%16==1:plate[0x7c//4]=fw(1);plate[0x78//4]=fw(-1)
        if i%16==2:
            for at in range(0xbc//4,0xc8//4):plate[at]=fw(.0001)
        view=view_bank(i%4)
        for at in [0x18,0x1c,0x20,0x24,0x28,0x2c,0x90,0x94,0x9c,0xa0,0xa8,0xac,0xb4,0xb8,0xc4,0xc8,0xcc,0xd0,0xd4,0xd8,0xdc,0xe0,0xe4,0xe8,0xec,0xf0]:view[at//4]=fw(rng.uniform(-4,4))
        view[0x8c//4]=fw([1,0,-1,.25,2][i%5]);view[0xc0//4]=fw([1,0,-1,.5,3][i%5])
        view[0x40//4]=fw(rng.uniform(-3,3))
        weather=[rng.getrandbits(32) for _ in range(43)]
        for at in [0x30,0x34,0x38,0x90,0x94,0x98,0x9c,0xa8]:weather[at//4]=fw(rng.uniform(-4,4))
        rows.append([rng.getrandbits(32),fw(rng.uniform(-1,1)),fw(rng.uniform(-1,2)),*plate,*view,*weather])
    classification=[]
    # Independent exceptional and cancellation controls. These preserve real
    # authored initialization but explicitly replace one consumed caller field.
    special=[0x7fc12345,0x7fa12345,0x7f800000,0xff800000,1,0x80000000,
        0x38d1b716,0x38d1b717,0x38d1b718,0x4effffff,0x4f000000,0xcf000000,0xcf000001]
    for kind,offset in [('plate',0x78),('plate',0x7c),('plate',0x140),('plate',0xbc),
        ('plate',0x174),('plate',0x24),('weather',0xa8),('weather',0x90),
        ('view',0x18),('view',0x40),('view',0x8c),('view',0xc0),('elapsed',0),('intensity',0)]:
        for bits in special:
            row=rows[0].copy();row[3+0x94//4]=7
            row[3+0x78//4]=fw(-1);row[3+0x7c//4]=fw(1)
            if kind=='plate':row[3+offset//4]=bits
            elif kind=='view':row[237+offset//4]=bits
            elif kind=='weather':row[298+offset//4]=bits
            else:row[1 if kind=='elapsed' else 2]=bits
            classification.append({'case':len(rows),'bank':kind,'offset':f'{offset:X}','bits':f'{bits:08X}'})
            rows.append(row)
    for bits in [0x5f800000,0xdf800000]:
        row=rows[0].copy();row[237+0x40//4]=bits
        classification.append({'case':len(rows),'bank':'view','offset':'40','bits':f'{bits:08X}'})
        rows.append(row)
    sequences=[]
    for initial_index in [0,16]:
        state=rows[initial_index].copy();first=len(rows)
        for step in range(32):
            state[237]=(step%4)
            state[237+0x18//4]=fw(3.+step*.125)
            state[237+0x1c//4]=fw(5.-step*.0625)
            row=state.copy();rows.append(row);out=original(row)
            state[0]=out[0];state[3:237]=out[2:236]
        sequences.append({'first_case':first,'count':32,'initial_authored_case':initial_index,
            'scope':'Explicit supplied step/view/position sequence; source output banks and auxiliary word feed the next input. No original gameplay chronology is asserted.'})
    expected=[original(row) for row in rows]
    def admitted(row):
        angle=struct.unpack('<f',pack([row[237+0x40//4]]))[0]
        import math
        return math.isfinite(angle) and abs(angle*.5)<9223372036854775808.
    indices=[i for i,row in enumerate(rows) if admitted(row)]
    encoded=[' '.join(f'{v:x}' for v in rows[i]) for i in indices]
    run=subprocess.run([str(a.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(v,16) for v in row.split()] for row in run.stdout.splitlines()]
    assert len(actual)==len(indices)
    failures=[{'case':i,'differences':[{'word':j,'source':f'{x:08X}','native':f'{y:08X}'} for j,(x,y) in enumerate(zip(expected[i],t)) if x!=y]} for i,t in zip(indices,actual) if expected[i]!=t]
    if a.write_fixture and not failures:
        a.write_fixture.parent.mkdir(parents=True,exist_ok=True)
        a.write_fixture.write_text('# Complete unchanged37260. Input seed,elapsed,intensity,234 plate,61 view,43 weat words; output seed,AL,all banks.\n'+'\n'.join(x+' '+' '.join(f'{v:08x}' for v in expected[i]) for x,i in zip(encoded,indices))+'\n')
    report={'original_executable_sha256':PIN,'native_binary_sha256':sha(a.binary.read_bytes()),'original_cases':len(rows),'cases':len(indices),'exact_cases':len(indices)-len(failures),'compared_words':len(indices)*340,'verified_original_instructions':len(verified),'draws_per_case':9,'primary_rng_unchanged':True,'assets':assets,'sequences':sequences,'classification_controls':classification,'source_only_exceptional_angles':[{'case':i,'input':rows[i],'output':expected[i]} for i in range(len(rows)) if i not in indices],'failures':failures[:20],'scope':'Complete37260, explicit owned plate/view/owning-weat/elapsed/intensity/auxiliary inputs. Nonfinite/out-of-range FPTAN angles retain independent source-only executions and are rejected before native mutation. Original36F50 supplies authored initialization controls, not launch RNG or camera/time chronology. No source services/instructions replaced; no GPU/runtime activation.'}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ['assets','classification_controls','source_only_exceptional_angles']},indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
