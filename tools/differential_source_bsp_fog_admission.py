#!/usr/bin/env python3
"""Untouched16E210 with literal native atmosphere fields and explicit globals."""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from audit_rendering import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BSP,SCENARIO,CLUSTERS,PLANES,PALETTES,STACK,STOP=0x2100000,0x2101000,0x2102000,0x2120000,0x2130000,0x21fff00,0x3000000
def pack(values):return struct.pack('<'+'I'*len(values),*(x&0xffffffff for x in values))
def bank(designators,planes,palettes):return [len(designators),*[v&255 for v in designators],len(planes),*[v&65535 for v in planes],len(palettes),*palettes]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer-shading/source_bsp_admission-standalone');p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/bsp-fog-admission-differential.json');p.add_argument('--write-fixture',type=Path);p.add_argument('--write-assets',type=Path);p.add_argument('--count',type=int,default=2048);args=p.parse_args()
    image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN;u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in image.sections:u.mem_write(s.va,image.section_bytes(s))
    u.mem_map(BSP,0x100000);u.mem_map(STOP,0x1000)
    def put(at,*v):u.mem_write(at,pack(v))
    rows=[];expected=[];labels=[];maps=[];assets=[];paths=[];calls=0
    def add(designators,planes,palettes,datum,flags=0,scenario=True,bsp=True,label=''):
        nonlocal calls
        u.mem_write(BSP,bytes(0x40000));put(BSP+0xa0,CLUSTERS);put(BSP+0x68,PLANES);put(SCENARIO+0x34c,PALETTES);put(0x4e0350,SCENARIO if scenario else 0);put(0x4e0348,BSP if bsp else 0);put(0x4e64c8,flags)
        for i,d in enumerate(designators):u.mem_write(CLUSTERS+i*0xb0+0x70,bytes([d&255]))
        for i,v in enumerate(planes):u.mem_write(PLANES+i*24,struct.pack('<H',v&65535))
        for i,v in enumerate(palettes):put(PALETTES+i*16+8,v)
        out=[]
        for cluster in [*range(len(designators)),-1]:
            put(STACK,STOP);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_ECX,cluster&0xffffffff);u.reg_write(UC_X86_REG_EDI,datum);u.emu_start(0x16e210,STOP,count=1000);calls+=1
            if u.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('16E210 did not return')
            out.append(u.reg_read(UC_X86_REG_EAX)&255)
        rows.append([int(scenario),int(bsp),datum,flags,*bank(designators,planes,palettes)]);expected.append(out);labels.append(label)
    rng=random.Random(0x16e210);palettes=[rng.getrandbits(32)for i in range(128)];palettes[0]=0xffffffff
    for i in range(args.count):
        nd=1+i%512;designators=[rng.randrange(-128,128)for j in range(nd)];planes=[rng.choice([-1,*range(128)])for j in range(128)]
        datum=rng.choice([0xffffffff,rng.getrandbits(32),*palettes]);flags=rng.getrandbits(32) if i%3==0 else rng.choice([0,0x10,0x8,0x20]);add(designators,planes,palettes,datum,flags,bool(i%5),bool(i%7),f'synthetic{i}')
    # Every signedbyte designator and its aliasing128-plane index, missing
    # palette sentinel, NONE bypass, unrelated flags and forcedmembership.
    for designator in range(-128,128):
        for datum in [0xffffffff,palettes[1],0x1234]:
            planes=[1]*128;planes[(designator&127)]= -1 if designator%3==0 else 1
            add([designator],planes,palettes,datum,label=f'designator{designator}:{datum:x}')
    for scenario in [False,True]:
        for bsp in [False,True]:
            for flags in [0,0x10]:add([-1,0,-128],[],[],0xffffffff,flags,scenario,bsp,'availability/NONE')
    for group,names in [('disc',['ascension','beavercreek','burial_mounds','coagulation','colossus','cyclotron','foundation','headlong','lockout','midship','waterworks','zanzibar']),('map-pack',['containment','warlock','deltatap','turf','backwash','dune','elongation','gemini','triplicate'])]:
        for name in names:
            path=ROOT/'assets'/group/'maps'/(name+'.map');c=Cache(path);_,s=c.tag(c.scenario,'scnr');ref=c.block(s+0x210,68)[0];at,size,mem=struct.unpack_from('<III',c.data,ref);b=at+16;magic=mem-at
            designators=[struct.unpack_from('<b',c.data,q+0x70)[0]for q in c.block(b+0x9c,0xb0,magic)];planes=[struct.unpack_from('<h',c.data,q)[0]for q in c.block(b+0x64,24,magic)];palettes=[c.u32(q+8)for q in c.block(s+0x348,16)]
            values=bank(designators,planes,palettes);assets.append(str(path.relative_to(ROOT))+' | '+' '.join(f'{v:x}'for v in values));paths.append(str(path));start=len(rows)
            for datum in sorted(set([0xffffffff,0x1234,*palettes])):
                for flags in [0,0x10,0x8,0x20]:add(designators,planes,palettes,datum,flags,label=f'{name}:literal:{datum:x}:flags{flags:x}')
            maps.append({'map':name,'clusters':len(designators),'negative_designators':sorted(set(v for v in designators if v< -1)),'fog_planes':len(planes),'palette_datums':[f'{d:08x}'for d in palettes],'cases':len(rows)-start,'cache_sha256':hashlib.sha256(path.read_bytes()).hexdigest()});c.close()
    encoded=[' '.join(f'{v:x}'for v in r)for r in rows];native=subprocess.run([str(args.binary.resolve()),'--fog-probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True);actual=[[int(v,16)for v in line.split()]for line in native.stdout.splitlines()]
    if len(actual)!=len(expected):raise RuntimeError('native16E210 probe lost cases')
    failed=[{'case':i,'label':labels[i],'input':rows[i],'source':s,'native':n}for i,(s,n)in enumerate(zip(expected,actual))if s!=n]
    nativeassets=subprocess.run([str(args.binary.resolve()),'--fog-assets'],input='\n'.join(paths)+'\n',text=True,capture_output=True,check=True);actualbanks=[' '.join(f'{int(v,16):x}'for v in line.split())for line in nativeassets.stdout.splitlines()];expectedbanks=[row.split('|')[1].strip()for row in assets]
    if actualbanks!=expectedbanks:raise RuntimeError('native atmosphere metadata differs from source cache bytes')
    if args.write_fixture and not failed:
        selected=sorted(set([*range(min(128,args.count)),*range(args.count,len(rows))]));args.write_fixture.write_text('# Untouched16E210; source '+PIN+'\n'+'\n'.join(encoded[i]+' | '+' '.join(f'{v:x}'for v in expected[i])for i in selected)+'\n')
    if args.write_assets and not failed:args.write_assets.write_text('# Literal cluster+70, BSP fogplane+0, scenario fogpalette+8 datums\n'+'\n'.join(assets)+'\n')
    report={'source_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),'cases':len(rows),'bit_exact_cases':len(rows)-len(failed),'compared_words':sum(map(len,expected)),'original16E210_calls':calls,'maps':maps,'failures':failed[:8],'scope':'Untouched16E210, no service hooks. Native consumes actual BspAtmosphere signed designators/plane palettes/datums independently compared against all21 cache banks. Explicit4E0350/4E0348 availability,4B9F8C datum and4E64C8 flags; no guessed live override producer.'};args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='failures'},indent=2));print('failures',json.dumps(failed[:8]));return bool(failed)
if __name__=='__main__':raise SystemExit(main())
