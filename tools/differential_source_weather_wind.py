#!/usr/bin/env python3
"""Compare native wind banks with untouched retail4B3D0/36A40/30BF0.

The caller supplies the source elapsed time and auxiliary RNG state. This does
not replace36B50's per-view visibility admission or weather particle rendering.
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
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ESI, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
DISC='ascension beavercreek burial_mounds coagulation colossus cyclotron foundation headlong lockout midship waterworks zanzibar'.split()
PACK='containment warlock deltatap turf backwash dune elongation gemini triplicate'.split()
ACTOR,POOL,STACK,STOP=0x2100000,0x2101000,0x21fff00,0x3000000

def f(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-mp-shading/source_weather_wind_tests')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/weather-wind-differential.json')
    p.add_argument('--write-fixture',type=Path)
    args=p.parse_args();image=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(image.data).hexdigest()!=PIN:raise ValueError('unsupported original executable')
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    for section in image.sections:guest.mem_write(section.va,image.section_bytes(section))
    guest.mem_map(ACTOR,0x100000);guest.mem_map(STOP,0x1000)
    def put(at,value):guest.mem_write(at,struct.pack('<I',value&0xffffffff))
    put(0x4e7408,POOL)
    def original(row):
        put(POOL,0x11223344);put(POOL+4,row[0]);put(0x509418,row[1])
        guest.mem_write(ACTOR,struct.pack('<39I',*row[2:]))
        guest.reg_write(UC_X86_REG_ESI,ACTOR);guest.reg_write(UC_X86_REG_ESP,STACK);put(STACK,STOP)
        guest.reg_write(UC_X86_REG_FPCW,0x027f);guest.reg_write(UC_X86_REG_MXCSR,0x1f80)
        guest.emu_start(0x4b3d0,STOP,count=10000)
        if guest.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('original wind did not complete')
        if struct.unpack('<I',guest.mem_read(POOL,4))[0]!=0x11223344:raise RuntimeError('wind altered primary stream')
        return [struct.unpack('<I',guest.mem_read(POOL+4,4))[0],*struct.unpack('<39I',guest.mem_read(ACTOR,156))]
    rows=[];assets=[]
    for name in DISC+PACK:
        path=ROOT/('assets/disc/maps' if name in DISC else 'assets/map-pack/maps')/(name+'.map')
        cache=Cache(path)
        try:
            weather=[]
            for datum,tag in cache.tags.items():
                if tag['group']!='weat':continue
                _,offset=cache.tag(datum,'weat')
                bank=list(struct.unpack_from('<39I',cache.data,offset+0x10))
                weather.append({'datum':f'{datum:08x}','name':tag['name'],'size':tag['size'],
                    'particle_systems':cache.u32(offset),'background_plates':cache.u32(offset+8),'wind_words':bank})
                # Retain actual authored state across100 consecutive source steps.
                aux=0x12345678
                for i in range(100):
                    row=[aux,f([0.,1./120.,1./30.,.125][i%4]),*bank];rows.append(row)
                    result=original(row);aux=result[0];bank=result[1:]
            assets.append({'map':name,'cache_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'weather':weather})
        finally:cache.close()
    rng=random.Random(0x4b3d0)
    for i in range(1024):
        bank=[f(rng.uniform(-2,2)) for _ in range(39)];bank[0]=rng.getrandbits(32)
        # Include exactly zero and near-zero normalization, both clamp limits,
        # zero elapsed and large admitted scalar inputs without generating NaNs.
        if i%16==0:
            for field in [11,12,13,18,22,23,24,25,26,27]:bank[field]=f(0.)
        elif i%16==1:
            for field in [11,12,13,18,22,23,24,25,26,27]:bank[field]=f(.00001)
        rows.append([rng.getrandbits(32),f([0.,1./120.,1./30.,.5,2.][i%5]),*bank])
    expected=[original(row) for row in rows]
    encoded=[' '.join(f'{word:x}' for word in row) for row in rows]
    result=subprocess.run([str(args.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(word,16) for word in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(rows):raise RuntimeError('native wind probe lost source rows')
    failures=[{'case':i,'input':row,'source':source,'native':native} for i,(row,source,native) in enumerate(zip(rows,expected,actual)) if source!=native]
    args.output.parent.mkdir(parents=True,exist_ok=True)
    (args.output.parent/'weather-asset-inventory.json').write_text(json.dumps(assets,indent=2)+'\n')
    if args.write_fixture and not failures:
        lines=['# Untouched retail4B3D0 wind banks; original executable '+PIN,
            '# 41 hexadecimal input words(aux,elapsed,39bank) then40 output words(aux,39bank).']
        selected=list(range(0,len(rows),16))
        lines.extend(encoded[i]+' '+' '.join(f'{word:08x}' for word in expected[i]) for i in selected)
        args.write_fixture.write_text('\n'.join(lines)+'\n')
    report={'original_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),
        'original_entries':['0004B3D0','00036A40','00030BF0'],'cases':len(rows),
        'bit_exact_cases':len(rows)-len(failures),'compared_words':len(rows)*40,'primary_stream_unchanged':True,
        'auxiliary_draws_per_case':4,'multiplayer_maps':len(assets),'weather_maps':[a['map'] for a in assets if a['weather']],
        'failures':failures[:12],'scope':'Explicit wind bank/elapsed/auxiliary input; visibility, initial auxiliary chronology and GPU particles remain caller services.'}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
