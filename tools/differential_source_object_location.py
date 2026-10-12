#!/usr/bin/env python3
"""Untouched11BED0/14A280, BF510 and B8600 location-prefix oracle.

1DDE10/1DDEA0 execute unchanged, including actual surface callees in map cases.
Only their independent third leaf bank is compared; contact outputs/status are
not reimplemented. Object registration/creation chronology stays explicit.
"""
import argparse,collections,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
sys.path.insert(0,str(ROOT/'tools'));from inspect_multiplayer import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x24fff00,0x3000000
BSP,COLL,NODES,PLANES,LEAVES,RENDER,POINT,OUTPUT,HEADER,HANDLES,OBJECT,PACKET=[BASE+x for x in (0,0x1000,0x10000,0x20000,0x30000,0x40000,0x50000,0x51000,0x52000,0x53000,0x54000,0x60000)]
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def scalar(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def serialize(b,selected,queries):
    out=[]
    if b is not None:
        out.append(len(b['nodes']))
        for n in b['nodes']:out.extend(n)
        out.append(len(b['planes']))
        for p in b['planes']:out.extend(map(word,p))
        out.extend([len(b['leaves']),*[v&65535 for v in b['leaves']]])
    out.extend([selected&65535,len(queries)])
    for q in queries:
        out.extend([q['kind'],*map(word,q['center'])])
        if q['kind'] in [1,2]:out.append(word(q['radius']))
        if q['kind'] in [2,3]:out.extend(map(word,q['root']))
    return out
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,required=True);p.add_argument('--count',type=int,default=1024);p.add_argument('--write-fixture',type=Path,required=True);args=p.parse_args()
    image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in image.sections:u.mem_write(s.va,image.section_bytes(s))
    u.mem_map(BASE,0x400000);u.mem_map(STOP,0x1000)
    calls=collections.Counter();source=[0x11bed0,0x14a280,0xbf510,0x1dde10,0x1ddea0,0x11be90,0xb8600]
    def hook(_u,at,_size,_data):
        if at in source:calls[at]+=1
    u.hook_add(UC_HOOK_CODE,hook)
    def put(at,*v):u.mem_write(at,struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v)))
    def run(at,args=(),regs={},end=STOP):
        for r in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(r,regs.get(r,0))
        put(STACK,STOP,*args);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.emu_start(at,end,count=100000000)
        if u.reg_read(UC_X86_REG_EIP)!=end:raise RuntimeError(f'original{at:x} exceeded instruction budget at{u.reg_read(UC_X86_REG_EIP):x}')
    def setup(b):
        u.mem_write(BSP,bytes(0x50000));put(COLL,len(b['nodes']),NODES,len(b['planes']),PLANES,len(b['leaves']),LEAVES);put(BSP+0x30,RENDER)
        for i,(p,a,z)in enumerate(b['nodes']):u.mem_write(NODES+i*8,struct.pack('<H',p)+(a&0xffffff).to_bytes(3,'little')+(z&0xffffff).to_bytes(3,'little'))
        for i,p in enumerate(b['planes']):put(PLANES+i*16,*map(word,p))
        for i,c in enumerate(b['leaves']):u.mem_write(RENDER+i*8,struct.pack('<h',c))
        put(0x4e033c,COLL);put(0x4e0340,COLL);put(0x4e0348,BSP)
    def original(selected,queries):
        put(0x4e0300,HEADER);put(HEADER+0x44,HANDLES);put(HANDLES+8,OBJECT);u.mem_write(0x4686c4,struct.pack('<h',selected));result=[]
        for q in queries:
            put(POINT,*map(word,q['center']))
            if q['kind']==0:
                run(0x11bed0,regs={UC_X86_REG_EDX:POINT,UC_X86_REG_ESI:OUTPUT});location=struct.unpack('<IHH',u.mem_read(OUTPUT,8))
            elif q['kind']==1:
                run(0x1dde10,[POINT,word(q['radius'])],{UC_X86_REG_EAX:struct.unpack('<I',u.mem_read(0x4e0340,4))[0],UC_X86_REG_ESI:PACKET});count=struct.unpack('<I',u.mem_read(PACKET+0xc0c,4))[0];result.extend([count,*struct.unpack('<'+'I'*count,u.mem_read(PACKET+0xc10,count*4))]);continue
            else:
                put(OBJECT+0x40,*map(word,q['center']),word(q.get('radius',1)));put(OBJECT+0x64,*map(word,q['root']))
                if q['kind']==2:
                    run(0xbf510,[0xe0000000],{UC_X86_REG_EDI:OUTPUT});location=struct.unpack('<IHH',u.mem_read(OUTPUT,8))
                else:
                    run(0xb8600,[0xe0000000,0],end=0xb8654);location=struct.unpack('<IHH',u.mem_read(u.reg_read(UC_X86_REG_EDI),8))
            result.extend(location)
        return result
    rng=random.Random(0x11bed0);cases=[];expected=[];rows=[];labels=[]
    for i in range(args.count):
        normal=[rng.uniform(-1,1)for _ in range(3)];planes=[normal+[rng.uniform(-3,3)],[1.,0,0,0],[0,0,1.,0]]
        nodes=[[0,1,2],[1,0x800000,0x800001],[2,0x800002,0xffffff]];leaves=[0,1,rng.choice([-1,2])]
        b=dict(nodes=nodes,planes=planes,leaves=leaves);center=[rng.uniform(-10,10)for _ in range(3)];radius=rng.choice([0,.01,1,2.5,30]);root=[rng.uniform(-10,10)for _ in range(3)]
        if i%16==0:center=[0,0,0];planes[0][3]=0
        selected=-1 if i%17==0 else i%4
        q=[dict(kind=0,center=center),dict(kind=3,center=center,root=root)]
        if selected!=-1:q+=[dict(kind=1,center=center,radius=radius),dict(kind=2,center=center,radius=radius,root=root)]
        setup(b);cases.append(serialize(b,selected,q));expected.append(original(selected,q));labels.append(f'synthetic-{i}')
    # Balanced leaf trees cover the literal independent256-leaf output cap.
    for count in [2,256,512]:
        nodes=[]
        def tree(first,count):
            if count==1:return 0x800000|first
            index=len(nodes);nodes.append(None);n=count//2;nodes[index]=[0,tree(first,n),tree(first+n,count-n)];return index
        tree(0,count);b=dict(nodes=nodes,planes=[[1,0,0,0]],leaves=[0]*count);q=[dict(kind=1,center=[0,0,0],radius=1)];setup(b);cases.append(serialize(b,0,q));expected.append(original(0,q));labels.append(f'leaf-cap-{count}')
    b=dict(nodes=[[0,i+1 if i<319 else 0x800000,0xffffff]for i in range(320)],planes=[[1,0,0,0]],leaves=[0]);q=[dict(kind=1,center=[-1,0,0],radius=.5)];setup(b);cases.append(serialize(b,0,q));expected.append(original(0,q));labels.append('deep-single-side-320')
    input='\n'.join(' '.join(f'{w:x}'for w in v)for v in cases)+'\n';actual=[list(int(w,16)for w in line.split())for line in subprocess.check_output([str(args.binary.resolve()),'--probe'],input=input,text=True).splitlines()];assert len(actual)==len(expected)
    failures=[dict(label=labels[i],original=a,native=b)for i,(a,b)in enumerate(zip(expected,actual))if a!=b]
    frozen=['synthetic '+' '.join(f'{w:x}'for w in a)+' | '+' '.join(f'{w:x}'for w in b)for i,(a,b)in enumerate(zip(cases,expected))if i%4==0 or i>=args.count]
    cold=[]
    for kind in ['scenery','machine','prop']:
        for r in json.loads((ROOT/f'analysis/multiplayer-shading/{kind}-cold-spatial-original.json').read_text())['records']:cold.append(dict(r,kind=kind))
    map_reports=[];cold_results=[]
    for name in dict.fromkeys(r['map']for r in cold):
        path=ROOT/'assets/disc/maps'/(name+'.map')
        if not path.exists():path=ROOT/'assets/map-pack/maps'/(name+'.map')
        c=Cache(path);e=c.block(c.scenario+0x210,68)[0];start,size,memory=c.u(e),c.u(e+4),c.u(e+8);root=c.u(start+4);magic=memory-start;coll=c.u(root-magic+0x18);rounded=(size+4095)//4096*4096
        u.mem_map(memory,rounded);u.mem_write(memory,bytes(c.data[start:start+size]));put(0x4e033c,coll);put(0x4e0340,coll);put(0x4e0348,root)
        map_rows=[];map_expected=[]
        for r in [r for r in cold if r['map']==name]:
            s=[scalar(w)for w in r['primitive_sphere_words']];position=[scalar(w)for w in r['root_position_words']]
            q=[dict(kind=0,center=s[:3]),dict(kind=2,center=s[:3],radius=s[3],root=position),dict(kind=3,center=s[:3],root=position)]
            wanted=original(0,q);row=serialize(None,0,q);map_rows.append(' '.join(f'{w:x}'for w in row));map_expected.append(wanted);cold_results.append(dict(kind=r['kind'],map=name,placement=r['placement'],definition=r['definition'],source_point_location=wanted[:3],source_initial_location=wanted[3:6],source_default_registration_location=wanted[6:9]))
        values=[list(int(w,16)for w in line.split())for line in subprocess.check_output([str(args.binary.resolve()),'--cache-probe',str(path)],input='\n'.join(map_rows)+'\n',text=True).splitlines()]
        assert len(values)==len(map_expected)
        failures.extend(dict(map=name,index=i,original=a,native=b)for i,(a,b)in enumerate(zip(map_expected,values))if a!=b)
        frozen.extend('asset '+str(path.relative_to(ROOT))+' '+a+' | '+' '.join(f'{w:x}'for w in b)for a,b in zip(map_rows,map_expected))
        map_reports.append(dict(map=name,owners=len(map_rows),cache_sha256=hashlib.sha256(c.data).hexdigest()));expected.extend(map_expected)
        print('original location',name,len(map_rows),flush=True);u.mem_unmap(memory,rounded);c.data.close();c.stream.close()
    args.write_fixture.write_text('# Untouched source registration location services; '+PIN+'; PC53/MXCSR1F80\n'+'\n'.join(frozen)+'\n')
    report=dict(xbe_sha256=PIN,native_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),cases=args.count+4+len(cold),words=sum(map(len,expected)),exact=args.count+4+len(cold)-len(failures),source_calls={f'{a:08x}':calls[a]for a in source},maps=map_reports,cold_initial_resolved=sum(r['source_initial_location'][1]!=65535 for r in cold_results),cold_final_default_resolved=sum(r['source_default_registration_location'][1]!=65535 for r in cold_results),failures=failures[:8],cold_results=cold_results,scope='Untouched11BED0/14A280, complete BF510 with actual1DDE10/1DDEA0 surface callees, andB8600 prefix throughB8654 before registration. Actual cold object+40/+4C/+64 words against authoredBSP0; each owner isolated. No registered-history/creation-order, source spatial allocator pressure or contact-output/status producer claim.')
    (ROOT/'analysis/multiplayer-shading/object-location-differential.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in ['maps','cold_results','scope']},indent=2));assert not failures
if __name__=='__main__':main()
