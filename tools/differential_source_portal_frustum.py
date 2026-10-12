#!/usr/bin/env python3
"""Untouched Xbox173520/173380/23A2B0/23A430 portal projection oracle.

No source function is replaced. The original empty-bound initializer376F00
runs before the probes. Synthetic finite64-vertex inputs and actual authored
multiplayer portal streams are provided as explicit source context fields.
"""
import argparse,hashlib,json,math,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_EDX,UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_EIP,UC_X86_REG_XMM1)
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from audit_rendering import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x22fff00,0x3000000
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def scalar(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def flat(points):return [word(v) for p in points for v in p]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer-shading/source_portal_frustum-standalone')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/portal-frustum-differential.json')
    parser.add_argument('--write-fixture',type=Path);parser.add_argument('--skip-maps',action='store_true')
    args=parser.parse_args();image=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(image.data).hexdigest()!=PIN:raise ValueError('unsupported original executable')
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    for section in image.sections:guest.mem_write(section.va,image.section_bytes(section))
    guest.mem_map(BASE,0x200000);guest.mem_map(STOP,0x1000)
    calls={a:0 for a in [0x376f00,0x173520,0x1429d0,0x11fc80,0x173890,0x173910,0x120850,0x23a220,0x173380,0x23a2b0,0x23a430,0x320560]}
    initial_facing=[0]
    def hook(uc,address,size,data):
        if address in calls:calls[address]+=1
        if address==0x1735ca:initial_facing[0]=uc.reg_read(UC_X86_REG_EAX)
    guest.hook_add(UC_HOOK_CODE,hook)
    def put(at,*values):guest.mem_write(at,struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values)))
    def get(at,n=1):return list(struct.unpack('<'+'I'*n,guest.mem_read(at,n*4)))
    def start(address):
        put(STACK,STOP);guest.reg_write(UC_X86_REG_ESP,STACK);guest.reg_write(UC_X86_REG_FPCW,0x027f);guest.reg_write(UC_X86_REG_MXCSR,0x1f80)
        guest.emu_start(address,STOP,count=1000000)
        if guest.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'original{address:x} did not return')
    start(0x376f00)
    if get(0x5476cc,4)!=[0x7f7fffff,0xff7fffff,0x7f7fffff,0xff7fffff]:raise RuntimeError('source empty-bound startup differs')
    def original(row):
        guest.mem_write(BASE,bytes(0x200000));initial_facing[0]=0
        context,bsp,collision,view,portal,plane,polygon,clip,output=[BASE+x for x in (0x1000,0x11000,0x12000,0x13000,0x14000,0x15000,0x16000,0x18000,0x1a000)]
        mode,*v=row
        if mode==0:
            normal=v[:3];epsilon,capacity,count=v[3:6];points=v[6:];put(plane,*normal);put(polygon,*points)
            put(STACK+4,capacity,output);guest.reg_write(UC_X86_REG_ECX,polygon);guest.reg_write(UC_X86_REG_EAX,count);guest.reg_write(UC_X86_REG_EBX,plane);guest.reg_write(UC_X86_REG_XMM1,epsilon)
            start(0x23a430);n=guest.reg_read(UC_X86_REG_EAX)&65535
            return [n,*get(output,n*2)]
        if mode in [1,3]:
            epsilon,capacity,count=v[:3];points=v[3:3+count*2];v=v[3+count*2:];nclip=v[0];clip_points=v[1:]
            put(polygon,*points);put(clip,*clip_points)
            if mode==1:
                put(STACK+4,nclip,clip,capacity,output,epsilon);guest.reg_write(UC_X86_REG_ECX,polygon);guest.reg_write(UC_X86_REG_EDX,count)
                start(0x23a2b0);n=guest.reg_read(UC_X86_REG_EAX)&65535;return [n,*get(output,n*2)]
            if capacity!=64 or epsilon!=word(.0001):raise ValueError('173380 actual fixed arguments required')
            put(portal+0x10,count,polygon);put(view+0x10,nclip,clip);put(bsp+0x14,output)
            guest.reg_write(UC_X86_REG_EAX,portal);guest.reg_write(UC_X86_REG_ECX,view);guest.reg_write(UC_X86_REG_EBX,bsp)
            start(0x173380);n=get(bsp+0x10)[0]&65535
            return [guest.reg_read(UC_X86_REG_EAX),n,*get(output,n*2)]
        if mode!=2:raise ValueError('unknown portal projection oracle')
        transform=v[:13];eye=v[13:16];rectangle=v[16:20];clipping_plane=v[20:24];far_enabled,far=v[24:26];initial_n=v[26];initial_points=v[27:27+initial_n*2];v=v[27+initial_n*2:]
        flags,back,front=v[:3];authored_plane=v[3:7];count=v[7];vertices=v[8:]
        put(context,bsp,collision,view,*clipping_plane);put(bsp+0x60,portal);put(collision+0xc,plane)
        put(view+4,*transform);put(view+0x60,*eye);put(view+0x84,far_enabled,far);put(view+0xa0,*rectangle);put(view+0x198,initial_n,*initial_points)
        put(portal,(back&65535)|((front&65535)<<16),0);put(portal+0x18,flags,count,polygon);put(polygon,*vertices);put(plane,*authored_plane)
        put(STACK+4,context);guest.reg_write(UC_X86_REG_EAX,0);start(0x173520)
        record=context+0x5c
        if guest.reg_read(UC_X86_REG_EAX)!=record:raise RuntimeError('original projection unexpectedly exhausts arena')
        literal=get(record,10);n=literal[8]&65535;arena=get(context+0x505c)[0]
        actual_points=get(literal[9],n*2) if initial_facing[0] else []
        return [literal[0],literal[1]&255,(literal[1]>>16)&65535,literal[2]&65535,*literal[3:8],n,arena,len(actual_points)//2,*actual_points]
    rng=random.Random(0x173520);groups=[[],[],[],[]];metadata=[]
    for i in range(2048):
        n=i%65;normal=[rng.uniform(-3,3) for j in range(3)];epsilon=rng.choice([0,.0001,.01,-.0001]);capacity=(i*17+3)%65
        if i%8==0:normal=[1,0,0]
        points=[]
        for j in range(n):
            p=[rng.uniform(-100,100),rng.uniform(-100,100)]
            if i%8==0:p[0]=rng.choice([-1,1,0,.0001,-.0001])
            points.append(p)
        groups[0].append([0,*map(word,normal),word(epsilon),capacity,n,*flat(points)])
    for i in range(2048):
        n=i%65;nclip=1+(i*13)%64
        points=[[rng.uniform(-10,10),rng.uniform(-10,10)] for j in range(n)]
        clipping=[]
        for j in range(nclip):
            a=2*math.pi*j/nclip;clipping.append([math.cos(a)*rng.uniform(.5,10),math.sin(a)*rng.uniform(.5,10)])
        if i%16==0:clipping=[[0.,0.]]*nclip
        if i%16==1:clipping=[[.00003*j,.00004*j] for j in range(nclip)]
        groups[1].append([1,word(.0001),64,n,*flat(points),nclip,*flat(clipping)])
        groups[3].append([3,word(.0001),64,n,*flat(points),nclip,*flat(clipping)])
    transform=list(map(word,[1,1,0,0,0,1,0,0,0,1,0,0,0]));rect=list(map(word,[-1,1,-1,1]));initial=[[-1,-1],[1,-1],[1,1],[-1,1]]
    def projection(t,eye,rect_,nearplane,far_enabled,far,plane,points,flags=0,back=0,front=1):
        return [2,*t,*map(word,eye),*rect_,*map(word,nearplane),far_enabled,word(far),4,*flat(initial),flags,back&65535,front&65535,*map(word,plane),len(points),*flat(points)]
    for i in range(2048):
        n=i%65;plane=[rng.uniform(-1,1) for j in range(4)];eye=[rng.uniform(-10,10) for j in range(3)]
        points=[[rng.uniform(-20,20),rng.uniform(-20,20),rng.choice([-.0078125,.0078125,-1.,-1024.,rng.uniform(-50,50)])] for j in range(n)]
        if i%8==0:plane=[0,0,1,0];eye=[0,0,rng.choice([.015625,-.015625,0.,.0156251,-.0156251])]
        t=transform if i%2==0 else [word(rng.uniform(.1,2)),*[word(rng.uniform(-1,1)) for j in range(9)],*[word(rng.uniform(-20,20)) for j in range(3)]]
        groups[2].append(projection(t,eye,rect,[0,0,-1,0],i%2,rng.choice([.015625,1,1024]),plane,points,flags=i%32))
    maps=[]
    if not args.skip_maps:
        for group,names in [('disc',['ascension','beavercreek','burial_mounds','coagulation','colossus','cyclotron','foundation','headlong','lockout','midship','waterworks','zanzibar']),('map-pack',['containment','warlock','deltatap','turf','backwash','dune','elongation','gemini','triplicate'])]:
            for name in names:
                path=ROOT/'assets'/group/'maps'/(name+'.map');c=Cache(path);_,s=c.tag(c.scenario,'scnr');ref=c.block(s+0x210,68)[0]
                at,size,memory=struct.unpack_from('<III',c.data,ref);b=at+16;magic=memory-at;collision=c.block(b+0x14,64,magic)[0];planes=c.block(collision+8,16,magic);portals=c.block(b+0x5c,36,magic)
                first=len(groups[2]);counts=[]
                for j,p in enumerate(portals):
                    vertices=c.block(p+0x1c,12,magic);points=[struct.unpack_from('<fff',c.data,x) for x in vertices];counts.append(len(points));plane=struct.unpack_from('<ffff',c.data,planes[c.u32(p+4)])
                    center=struct.unpack_from('<fff',c.data,p+8);eye=list(center);eye[2]+=5;translation=[-x for x in eye];t=[*transform[:10],*map(word,translation)]
                    groups[2].append(projection(t,eye,rect,[0,0,-1,0],1,1024,plane,points,c.u32(p+0x18),c.u16(p),c.u16(p+2)))
                maps.append({'map':name,'portal_cases':len(portals),'maximum_authored_vertices':max(counts,default=0),'first_projection_case':first,'cache_sha256':hashlib.sha256(path.read_bytes()).hexdigest()});c.close()
    rows=sum(groups,[]);expected=[]
    for row in rows:expected.append(original(row))
    encoded=[' '.join(f'{v:x}' for v in row) for row in rows]
    native=subprocess.run([str(args.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(v,16) for v in line.split()] for line in native.stdout.splitlines()]
    if len(actual)!=len(expected):raise RuntimeError('native portal frustum probe lost cases')
    failures=[{'case':i,'input':row,'source':source,'native':result} for i,(row,source,result) in enumerate(zip(rows,expected,actual)) if source!=result]
    if args.write_fixture and not failures:
        selected=[];offset=0
        for mode,group in enumerate(groups):
            selected.extend(range(offset,offset+256))
            if mode==2:selected.extend(range(offset+2048,offset+len(group)))
            offset+=len(group)
        args.write_fixture.write_text('# Untouched source173520/173380/23A2B0/23A430; source '+PIN+'\n'+'\n'.join(encoded[i]+' | '+' '.join(f'{v:x}' for v in expected[i]) for i in selected)+'\n')
    report={'source_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),'cases':len(rows),'mode_cases':[len(g) for g in groups],'bit_exact_cases':len(rows)-len(failures),'compared_words':sum(map(len,expected)),'original_calls':{f'{a:08x}':n for a,n in calls.items()},'maps':maps,'failures':failures[:8],'boundary':'Finite64vertex pure source projection/convex clipping; original startup376F00 executed. No source service hooks. Explicit view/open-mask services remain required for full173130.'}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
