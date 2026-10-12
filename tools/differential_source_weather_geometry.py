#!/usr/bin/env python3
"""Independent untouched retail weather input/pixel/visibility oracle.

GPU entry hooks only supply resource residency and retain immediate commands;
rain/point arithmetic, __ftol2, pixel words, cluster/portal admission execute
original instruction bytes. This does not emulate rasterization or simulate
weather clocks/particles. Authored asset fixture is read by the Python cache
parser independently of the native decoder.
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
from unicorn.x86_const import (UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,
    UC_X86_REG_EBX,UC_X86_REG_FPCW,UC_X86_REG_MXCSR)

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
sys.path.insert(0,str(ROOT/'tools'))
from xbe import Xbe
from audit_rendering import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x22fff00,0x3000000

def f(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def fnv(data):
    result=14695981039346656037
    for byte in data:result=((result^byte)*1099511628211)&0xffffffffffffffff
    return result
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer-shading/resize-build/source_weather_assets_tests')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/weather-geometry-differential.json')
    parser.add_argument('--write-fixture',type=Path)
    parser.add_argument('--write-assets',type=Path)
    args=parser.parse_args();image=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(image.data).hexdigest()!=PIN:raise ValueError('unsupported original executable')
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    for section in image.sections:guest.mem_write(section.va,image.section_bytes(section))
    guest.mem_map(BASE,0x200000);guest.mem_map(STOP,0x1000)
    def put(at,*words):guest.mem_write(at,struct.pack('<'+'I'*len(words),*(v&0xffffffff for v in words)))
    def get(at,n=1):return list(struct.unpack('<'+'I'*n,guest.mem_read(at,n*4)))
    def return_from_hook(pop=0,value=1):
        sp=guest.reg_read(UC_X86_REG_ESP);target=get(sp)[0]
        guest.reg_write(UC_X86_REG_ESP,sp+4+pop);guest.reg_write(UC_X86_REG_EAX,value)
        guest.reg_write(UC_X86_REG_EIP,target)
    device,commands=BASE+0x10000,BASE+0x20000
    call_mode=0
    def hook(uc,address,size,data):
        nonlocal call_mode
        # Only external device/resource services are stubbed. The authored
        # pixel builder and float-to-integer conversion run unmodified.
        services={0x12de70:0,0x16b10:0,0x1c590:4,0x12fa0:8,
                  0x3f5cf0:0,0x3f54f0:4,0x143c0:4,0x1cf80:0,
                  0x400fa0:0,0x3f5730:4,0x3fa3d0:0,0x3fa680:4,0x12d50:4,0x3f7330:4}
        if address in services and not(address==0x12fa0 and call_mode==3):
            return_from_hook(services[address]);return
        if address==0x3fb3e0:
            sp=guest.reg_read(UC_X86_REG_ESP);color=get(sp+8)[0];at=get(device)[0]
            put(at,0xc010c010,color);put(device,at+8);return_from_hook(8)
        if address==0x3faae0:raise RuntimeError('oracle command buffer overflow')
    guest.hook_add(UC_HOOK_CODE,hook)
    def reset():
        guest.mem_write(BASE,bytes(0x200000));put(0x407488,device)
        put(device,commands,commands+0x10000,0)
        guest.reg_write(UC_X86_REG_FPCW,0x027f);guest.reg_write(UC_X86_REG_MXCSR,0x1f80)
        put(STACK,STOP);guest.reg_write(UC_X86_REG_ESP,STACK)
    def original(row):
        nonlocal call_mode
        reset();mode=row[0];call_mode=mode
        if mode==1:
            guest.reg_write(UC_X86_REG_EBX,row[1]);guest.emu_start(0x39780,STOP,count=10000)
            return get(get(device+0x92c)[0],60)
        if mode==0:
            _,kind,opacity0,opacity1,streak,width,elapsed,views,view,count,*particles=row
            p,records,render,state,globals_,shaders,players=[BASE+x for x in (0x1000,0x2000,0x3000,0x5000,0x7000,0x8000,0x9000)]
            put(p+0x68,records);guest.mem_write(p+0x6c,struct.pack('<H',kind))
            put(p+0x70,opacity0,opacity1,streak,width);put(records,count,render,count,state)
            put(0x485a80,globals_);put(globals_+0x5c,shaders);put(shaders+0xb4,0xffffffff)
            put(0x4e8c20,players);guest.mem_write(players+8,struct.pack('<H',views))
            guest.mem_write(0x485600,struct.pack('<h',view));put(0x509418,elapsed)
            for i in range(count):put(render+i*20,*particles[i*13:i*13+5]);put(state+i*32,*particles[i*13+5:i*13+13])
            put(STACK+4,p);guest.emu_start(0x391f0,STOP,count=100000)
            if guest.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('original391F0 did not finish')
            end=get(device)[0];at=commands;color=0;vertices=[];primitive=None
            while at<end:
                method=get(at)[0]
                if method==0x417fc:
                    value=get(at+4)[0]
                    if value:primitive=value
                    at+=8
                elif method==0xc010c010:color=get(at+4)[0];at+=8
                elif method==0x101518:vertices.extend(get(at+4,4)+[color]);at+=20
                else:raise RuntimeError(f'unexpected source command {method:08x}')
            return [int(primitive==2),width,len(vertices)//5,*vertices]
        if mode==2:
            at=1;current=row[at];eye=row[at+1:at+4];palettes,clusters,portals,visible=row[at+4:at+8];at+=8
            bsp,palette,cluster,portal,planes,collision,view,indices=[BASE+x for x in (0x1000,0x2000,0x3000,0x6000,0x7000,0x8000,0x9000,0xa000)]
            put(0x4e0348,bsp);put(bsp+0x84,palettes,palette);put(bsp+0x9c,clusters,cluster)
            put(bsp+0x60,portal);put(bsp+0x18,collision);put(collision+0xc,planes)
            put(0x547f90,current);put(0x547f94,view);guest.mem_write(view+4,struct.pack('<h',visible));put(view+0xc,indices)
            put(0x509420,0);guest.mem_write(0x485600,struct.pack('<h',1));put(0x485618,*eye)
            for i in range(palettes):put(palette+i*0x88+0x24,row[at]);at+=1
            for i in range(clusters):
                index,n=row[at:at+2];at+=2;guest.mem_write(cluster+i*0xb0+0x76,struct.pack('<H',index&0xffff))
                address=BASE+0xb000+i*0x100;put(cluster+i*0xb0+0x8c,n,address)
                guest.mem_write(address,struct.pack('<'+'H'*n,*(v&0xffff for v in row[at:at+n])));at+=n
            for i in range(portals):
                back,front,*plane=row[at:at+6];at+=6
                guest.mem_write(portal+i*36,struct.pack('<hh',back if back<0x80000000 else back-0x100000000,front if front<0x80000000 else front-0x100000000))
                put(portal+i*36+4,i);put(planes+i*16,*plane)
            put(indices,*row[at:at+visible]);guest.emu_start(0x36b50,0x36dd9,count=100000)
            if guest.reg_read(UC_X86_REG_EIP)!=0x36dd9:raise RuntimeError('source weather admission did not reach boundary')
            sp=guest.reg_read(UC_X86_REG_ESP);result=[]
            for i in range(palettes):result.extend([guest.mem_read(sp+0x30+i,1)[0],get(sp+0x40+i*4)[0]])
            return result
        if mode==3:
            _,left,top,right,bottom,render_mode=row
            camera,matrix=BASE+0x1000,BASE+0x2000
            guest.mem_write(camera+0x30,struct.pack('<4H',top,left,bottom,right))
            guest.reg_write(UC_X86_REG_EAX,matrix)
            from unicorn.x86_const import UC_X86_REG_ECX
            guest.reg_write(UC_X86_REG_ECX,camera);put(STACK+4,0,render_mode)
            guest.mem_write(0x404fe8,b'\0');guest.emu_start(0x12fa0,STOP,count=10000)
            return get(0x407738+10*16,8)
        raise ValueError('unknown oracle case')
    rng=random.Random(0x391f0);rows=[[1,0],[1,1]]
    for i in range(384):
        kind=i%8;views=(i%6)-1;effective=max(1,min(4,views));view=i%effective;count=(i%17)+1
        row=[0,kind,f(rng.uniform(-.2,1.2)),f(rng.uniform(-.2,1.2)),f(rng.uniform(-2,4)),f(rng.uniform(.1,8)),f(rng.choice([0.,1/30,1/120,.25])),views&0xffff,view,count]
        for j in range(count):row += [f(rng.uniform(-100,100)) for _ in range(3)]+[f(rng.uniform(.001,.02)),rng.getrandbits(32)]+[f(rng.uniform(-9,9)) for _ in range(6)]+[f(rng.uniform(.1,8)),f(rng.choice([-1.,-0.,0.,.01,1.,9.]))]
        rows.append(row)
    for i in range(256):
        palettes,clusters=3,5;current=i%clusters;eye=[f(rng.uniform(-50,50)) for _ in range(3)]
        ports=[(0,1),(0,2),(1,3),(2,4),(3,4)];members=[[] for _ in range(clusters)]
        for j,(a,b) in enumerate(ports):members[a].append(j);members[b].append(j)
        visible=rng.sample(range(clusters),i%6)
        row=[2,current,*eye,palettes,clusters,len(ports),len(visible),0xe0000000,0xffffffff if i%4==0 else 0xe0000001,0xe0000002]
        for cluster,m in enumerate(members):row += [rng.choice([0,1,2,0xffff]),len(m),*m]
        for a,b in ports:row += [a,b]+[f(rng.uniform(-1,1)) for _ in range(3)]+[f(rng.uniform(-30,30))]
        row+=visible;rows.append(row)
    for width,height in [(640,480),(1280,720),(3840,2160),(1920,1080)]:
        for views in [1,2,4]:
            rectangles=[(0,0,width,height)] if views==1 else ([(0,0,width,height//2),(0,height//2,width,height)] if views==2 else
                [(x*width//2,y*height//2,(x+1)*width//2,(y+1)*height//2) for y in range(2) for x in range(2)])
            for left,top,right,bottom in rectangles:
                for mode in [0,1,2,3]:rows.append([3,left,top,right,bottom,mode])
    expected=[original(row) for row in rows]
    encoded=[' '.join(f'{v:x}' for v in row) for row in rows]
    native=subprocess.run([str(args.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(v,16) for v in line.split()] for line in native.stdout.splitlines()]
    failures=[{'case':i,'input':row,'source':source,'native':result} for i,(row,source,result) in enumerate(zip(rows,expected,actual)) if source!=result]
    if len(actual)!=len(expected):raise RuntimeError('native weather probe lost cases')
    if args.write_fixture and not failures:
        args.write_fixture.write_text('# Untouched retail391F0/39780/36B50..36DD9; source '+PIN+'\n'+'\n'.join(encoded[i]+' | '+' '.join(f'{w:x}' for w in expected[i]) for i in range(len(rows)))+'\n')
    assets=[]
    for directory in ['assets/disc/maps','assets/map-pack/maps']:
        names=['ascension','beavercreek','burial_mounds','coagulation','colossus','cyclotron','foundation','headlong','lockout','midship','waterworks','zanzibar'] if 'disc' in directory else ['containment','warlock','deltatap','turf','backwash','dune','elongation','gemini','triplicate']
        for name in names:
            path=Path(directory)/(name+'.map');c=Cache(ROOT/path)
            _,scenario=c.tag(c.scenario,'scnr');ref=c.block(scenario+0x210,68)[0]
            bsp_address,bsp_size,bsp_memory=c.u32(ref),c.u32(ref+4),c.u32(ref+8)
            b=bsp_address+16;magic=bsp_memory-bsp_address
            palettes=c.block(b+0x84,0x88,magic);clusters=c.block(b+0x9c,0xb0,magic);polyhedra=c.block(b+0x8c,24,magic)
            result=[len(palettes),len(clusters),len(polyhedra)];datums=set()
            for p in palettes:
                datum=c.u32(p+0x24);result += [datum,fnv(c.data[p:p+0x88])]
                if datum!=0xffffffff:datums.add(datum)
            result.append(fnv(b''.join(c.data[p+0x76:p+0x78] for p in clusters)))
            for p in polyhedra:
                planes_=c.block(p+16,16,magic);result += list(struct.unpack_from('<4I',c.data,p))+[len(planes_)]
                for v in planes_:result += list(struct.unpack_from('<4I',c.data,v))
            result.append(len(datums))
            for datum in sorted(datums):
                _,p=c.tag(datum,'weat');particles=c.block(p,0x8c);plates=c.block(p+8,0x3a8)
                result += [datum,len(particles),len(plates),fnv(c.data[p+0x10:p+0xac]),c.u32(p+0xac)]
                matg=next(t for t in c.tags.values() if t['group']=='matg');_,g=c.tag(matg['datum'],'matg');raster=c.block(g+0x108,0x108)[0];shaders=c.block(raster+0x58,8)
                for j in [21,22]:
                    shader=c.u32(shaders[j]+4);_,root=c.tag(shader,'vrtx');cls=c.block(root+4,28)[0]
                    code=c.pointer(c.u32(cls+16),c.magic,c.u32(cls+12));result += [shader,fnv(c.data[code:code+c.u32(cls+12)])]
                for r in particles:
                    ptr,size,header=c.u32(r+0x40),c.u32(r+0x44),c.u32(r+0x48)
                    location=ptr>>30;files=['','mainmenu.map','shared.map','single_player_shared.map']
                    resource=(ROOT/path).parent/files[location] if location else ROOT/path
                    with resource.open('rb') as stream:stream.seek(ptr&0x3fffffff);raw=stream.read(size)
                    descriptors=c.block(r+0x50,16);payloads={}
                    for d in descriptors:
                        start=header+8+c.u32(d+12);payloads[c.u16(d+4)]=raw[start:start+c.u32(d+8)]
                    result += [fnv(c.data[r:r+0x8c]),c.u32(r+4),c.u16(r+0x6c),c.u32(r+0x2c),len(payloads[0])//20,len(payloads[8])//32,fnv(payloads[0]),fnv(payloads[8]),len(descriptors)]
                    for d in descriptors:
                        start=header+8+c.u32(d+12);result += [fnv(c.data[d:d+16]),fnv(raw[start:start+c.u32(d+8)])]
                for p in plates:result.append(fnv(c.data[p:p+0x3a8]))
            assets.append(str(path)+'|'+' '.join(f'{v:x}' for v in result));c.close()
    if args.write_assets:args.write_assets.write_text('# Independent Python retail-cache bytes/resources; all21 supplied MP maps.\n'+'\n'.join(assets)+'\n')
    report={'source_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),
            'cases':len(rows),'bit_exact_cases':len(rows)-len(failures),'compared_words':sum(map(len,expected)),
            'failures':failures[:8],'asset_maps':len(assets),
            'boundary':'Actual instruction arithmetic and pixel words; residency/device calls supply external services, no weather simulation or GPU rasterization claim.'}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
