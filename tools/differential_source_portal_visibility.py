#!/usr/bin/env python3
"""Execute original173130 and its complete pure portal traversal callees.

Camera1BC banks are first constructed by untouched441B0 from explicit camera
fields. Actual cache portal vertices/planes/ordered cluster lists are supplied
without inferred adjacency. Open-portal mask is explicit, not a live door claim.
"""
import argparse,hashlib,json,math,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from audit_rendering import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x24fff00,0x3000000
BSP,COLLISION,CLUSTERS,PORTALS,PLANES,MASK,BANKS,OUTPUT,INPUTS,RAW,CONTAINED=[BASE+x for x in (0,0x1000,0x2000,0x20000,0x30000,0x110000,0x111000,0x120000,0x160000,0x170000,0x1f0000)]
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def scalar(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def packed(values):return struct.pack('<'+'I'*len(values),*(x&0xffffffff for x in values))
def flat(points):return [word(v) for p in points for v in p]
def fnv(data):
    h=14695981039346656037
    for x in data:h=((h^x)*1099511628211)&0xffffffffffffffff
    return h
def serialize(bsp):
    clusters,planes,portals=bsp;out=[len(clusters)]
    for p in clusters:out.extend([len(p),*p])
    out.append(len(planes));out.extend(flat(planes));out.append(len(portals))
    for p in portals:out.extend([p['back']&65535,p['front']&65535,p['plane'],p['flags'],len(p['vertices']),*flat(p['vertices'])])
    return out

def main():
    a=argparse.ArgumentParser(description=__doc__);a.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer-shading/source_portal_visibility-standalone')
    a.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/portal-visibility-differential.json')
    a.add_argument('--write-fixture',type=Path);a.add_argument('--write-assets',type=Path);a.add_argument('--skip-maps',action='store_true');a.add_argument('--count',type=int,default=256)
    args=a.parse_args();image=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(image.data).hexdigest()!=PIN:raise ValueError('unsupported original executable')
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in image.sections:u.mem_write(s.va,image.section_bytes(s))
    u.mem_map(BASE,0x400000);u.mem_map(STOP,0x1000)
    entries=[0x376f00,0x441b0,0x173130,0x172bb0,0x173b00,0x172840,0x173520,0x1429d0,0x11fc80,0x173890,0x173350,0x173380,0x23a2b0,0x23a430,0x11f5f0,0x173b60,0x172c70,0x1730a0,0x172ee0,0x163db0,0x13de30,0x134950,0x165010,0x181800]
    calls={at:0 for at in entries}
    def hook(_u,at,_size,_data):
        if at in calls:calls[at]+=1
    u.hook_add(UC_HOOK_CODE,hook)
    def put(at,*v):u.mem_write(at,packed(v))
    def get(at,n=1):return list(struct.unpack('<'+'I'*n,u.mem_read(at,n*4)))
    def run(at,args=(),regs={}):
        put(STACK,STOP,*args);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        for reg,value in regs.items():u.reg_write(reg,value)
        u.emu_start(at,STOP,count=10000000)
        if u.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'original{at:x} did not return')
    run(0x376f00)
    def camera(pos,forward,up,rectangle=(-1,1,-1,1),near=.015625,far=1024):
        put(INPUTS+8,*flat([pos,forward,up]));put(INPUTS+0x48,word(near),word(far));put(INPUTS+0x68,*map(word,rectangle))
        run(0x441b0,[INPUTS],{UC_X86_REG_EBX:INPUTS+8,UC_X86_REG_ESI:BANKS})
        return get(BANKS,111)
    def original(groups,containing,bsp,mask):
        clusters,planes,portals=bsp
        u.mem_write(BSP,bytes(0x100000));u.mem_write(OUTPUT,bytes(0x24000));u.mem_write(RAW,bytes(0x80000))
        put(BSP+0x18,COLLISION);put(COLLISION+0xc,PLANES);put(BSP+0x5c,len(portals),PORTALS);put(BSP+0x9c,len(clusters),CLUSTERS)
        put(PLANES,*flat(planes));cursor=RAW
        for i,p in enumerate(portals):
            put(PORTALS+i*36,(p['back']&65535)|((p['front']&65535)<<16),p['plane']);put(PORTALS+i*36+0x18,p['flags'],len(p['vertices']),cursor)
            put(cursor,*flat(p['vertices']));cursor+=len(p['vertices'])*12
        for i,cluster in enumerate(clusters):
            put(CLUSTERS+i*0xb0+0x8c,len(cluster),cursor)
            if cluster:u.mem_write(cursor,struct.pack('<'+'H'*len(cluster),*cluster))
            cursor=(cursor+len(cluster)*2+3)&~3
        if cursor>RAW+0x80000:raise RuntimeError('oracle source geometry bank exhausted')
        put(MASK,*mask);put(0x4f93a4,MASK);put(0x4e0348,BSP)
        for i,g in enumerate(groups):put(BANKS+i*0x1bc,*g)
        run(0x173130,[len(groups),BANKS,containing&0xffffffff,OUTPUT])
        n=struct.unpack('<H',u.mem_read(OUTPUT+0xa6c,2))[0];nf=struct.unpack('<H',u.mem_read(OUTPUT+0x1970,2))[0]
        out=[n]
        for i in range(n):out.extend(struct.unpack('<13H',u.mem_read(OUTPUT+0xa6e+i*0x1a,0x1a)))
        out.extend(get(OUTPUT+0x1770,n));out.append(nf)
        for i in range(nf):
            w=get(OUTPUT+0x1974+i*0x108,66);out.extend(w[:3]+w[4:])
        for i in range(n):
            eye=groups[0][0x60//4:0x60//4+3];put(INPUTS,*eye)
            run(0x165010,[INPUTS,CONTAINED],{UC_X86_REG_EAX:i,UC_X86_REG_EDI:OUTPUT,UC_X86_REG_XMM7:word(.01)})
            out.extend([u.reg_read(UC_X86_REG_EAX)&255,u.mem_read(CONTAINED,1)[0]])
        return out
    rng=random.Random(0x173130);rows=[];expected=[];labels=[];assetrows=[];mapreport=[]
    def add(groups,containing,bsp,mask,label):
        rows.append([len(groups),*[v for g in groups for v in g],containing&0xffffffff,*mask,*serialize(bsp)])
        expected.append(original(groups,containing,bsp,mask));labels.append(label)
    masks=[[0xffffffff]*16,[0]*16]+[[rng.getrandbits(32)for j in range(16)]for i in range(8)]
    for i in range(args.count):
        n=1+i%12;portals=[];clusters=[[]for j in range(n)];planes=[]
        edges=[]
        for j in range(n-1):edges.append((j,j+1))
        if i%4==0 and n>2:edges.append((0,n-1))
        if i%5==0 and n>2:edges.extend((0,j)for j in range(2,n))
        for j,(back,front) in enumerate(edges):
            z=-float(j+1);r=rng.uniform(.2,2);verts=[[-r,-r,z],[r,-r,z],[r,r,z],[-r,r,z]]
            planes.append([0,0,-1,-z]);portals.append({'back':back,'front':front,'plane':j,'flags':rng.choice([0,0,0,2,8,16]),'vertices':verts})
            clusters[back].append(j);clusters[front].append(j)
        groups=[camera([0,0,0],[0,0,-1],[0,1,0],far=rng.choice([1,2,1024]))for j in range(1+i%6)]
        for j,g in enumerate(groups):
            if j:g[0x84//4]=0 # explicit source far-disabled rectangle branch
        add(groups,-1 if i%13==0 else rng.randrange(n),(clusters,planes,portals),masks[i%len(masks)],f'synthetic{i}')
    # Explicit output/node capacity bounds, with independently admitted polygons.
    for n in [127,128,129,255,256,257,511,512]:
        portals=[];clusters=[[]for i in range(n)];planes=[]
        for j in range(n-1):
            planes.append([0,0,-1,1]);portals.append({'back':0,'front':j+1,'plane':j,'flags':0,'vertices':[[-1,-1,-1],[1,-1,-1],[1,1,-1],[-1,1,-1]]});clusters[0].append(j);clusters[j+1].append(j)
        add([camera([0,0,0],[0,0,-1],[0,1,0])for j in range(1 if n%2 else 6)],0,(clusters,planes,portals),masks[0],f'capacity{n}')
    for inside in [False,True]:
        for side in [0.,.015625,-.015625,scalar(word(.015625)+1),scalar(word(-.015625)+1)]:
            if inside:
                plane=[0,0,1,0];vertices=[[-1,-1,0],[1,-1,0],[1,1,0],[-1,1,0]];eye=[0,0,side]
            else:
                plane=[1,0,0,0];vertices=[[0,-1,-2],[0,1,-2],[0,1,-1],[0,-1,-1]];eye=[side,0,0]
            portal={'back':0,'front':1,'plane':0,'flags':0,'vertices':vertices}
            add([camera(eye,[0,0,-1],[0,1,0])],0,([[0],[0]],[plane],[portal]),masks[0],f'nearplane:{inside}:{side}')
    if not args.skip_maps:
        for group,names in [('disc',['ascension','beavercreek','burial_mounds','coagulation','colossus','cyclotron','foundation','headlong','lockout','midship','waterworks','zanzibar']),('map-pack',['containment','warlock','deltatap','turf','backwash','dune','elongation','gemini','triplicate'])]:
            for name in names:
                path=ROOT/'assets'/group/'maps'/(name+'.map');c=Cache(path);_,s=c.tag(c.scenario,'scnr');ref=c.block(s+0x210,68)[0];at,size,mem=struct.unpack_from('<III',c.data,ref);b=at+16;magic=mem-at
                collision=c.block(b+0x14,64,magic)[0];planes=[struct.unpack_from('<ffff',c.data,p)for p in c.block(collision+8,16,magic)];portals=[]
                for p in c.block(b+0x5c,36,magic):portals.append({'back':c.u16(p),'front':c.u16(p+2),'plane':c.u32(p+4),'flags':c.u32(p+24),'vertices':[struct.unpack_from('<fff',c.data,v)for v in c.block(p+28,12,magic)],'center':struct.unpack_from('<fff',c.data,p+8)})
                clusters=[[c.u16(v)for v in c.block(p+0x8c,2,magic)]for p in c.block(b+0x9c,0xb0,magic)];bsp=(clusters,planes,portals);start=len(rows)
                for j,cluster in enumerate(clusters):
                    if not cluster:eye=[0.,0.,0.]
                    else:
                        p=portals[cluster[0]];normal=planes[p['plane']][:3];side=-1 if p['back']==j else 1;eye=[p['center'][k]+side*normal[k]*.1 for k in range(3)]
                    yaw=(j%16)*math.pi/8;forward=[math.cos(yaw),math.sin(yaw),-.1]
                    add([camera(eye,forward,[0,0,1],rectangle=(-.532049,.532049,-.399037,.399037))],j,bsp,masks[j%len(masks)],f'{name}:cluster{j}:explicitmask')
                pbytes=b''.join(struct.pack('<hhIII',p['back'],p['front'],p['plane'],p['flags'],len(p['vertices']))+b''.join(struct.pack('<fff',*v)for v in p['vertices'])for p in portals)
                cbytes=b''.join(struct.pack('<I',len(v))+struct.pack('<'+'H'*len(v),*v)for v in clusters)
                hashes=[fnv(b''.join(struct.pack('<ffff',*p)for p in planes)),fnv(pbytes),fnv(cbytes)];values=[len(planes),len(portals),len(clusters),*[x for h in hashes for x in [h&0xffffffff,h>>32]]]
                assetrows.append(str(path.relative_to(ROOT))+' | '+' '.join(f'{v:x}'for v in values));mapreport.append({'map':name,'clusters':len(clusters),'portals':len(portals),'cases':len(rows)-start,'cache_sha256':hashlib.sha256(path.read_bytes()).hexdigest()});c.close()
    encoded=[' '.join(f'{v:x}'for v in row)for row in rows]
    native=subprocess.run([str(args.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(v,16)for v in line.split()]for line in native.stdout.splitlines()]
    if len(actual)!=len(rows):raise RuntimeError('native full portal probe lost cases')
    failures=[{'case':i,'label':labels[i],'input':row,'source':source,'native':result}for i,(row,source,result)in enumerate(zip(rows,expected,actual))if source!=result]
    if args.write_fixture and not failures:
        selected=sorted(set([*range(min(128,len(rows))),*range(args.count,args.count+18),*range(args.count+18,len(rows),max(1,(len(rows)-args.count-18)//128))]))
        args.write_fixture.write_text('# Untouched173130 complete pure traversal plus165010; source '+PIN+'\n'+'\n'.join(encoded[i]+' | '+' '.join(f'{v:x}'for v in expected[i])for i in selected)+'\n')
    if args.write_assets and not failures:args.write_assets.write_text('# Original all21 literal portal/plane/ordered-cluster bank words. FNV1a64 low,high halves\n'+'\n'.join(assetrows)+'\n')
    report={'source_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),'cases':len(rows),'bit_exact_cases':len(rows)-len(failures),'compared_words':sum(map(len,expected)),'original_calls':{f'{at:08x}':n for at,n in calls.items()},'maps':mapreport,'failures':[{'case':f['case'],'label':f['label'],'differences':[{'word':j,'source':s,'native':n}for j,(s,n)in enumerate(zip(f['source'],f['native']))if s!=n][:20],'source_words':len(f['source']),'native_words':len(f['native']),'input':f['input']}for f in failures[:8]],'scope':'Original complete173130 pure constructor and165010 consumer, explicit original441B0 camera fields and4F93A4 masks. Word+0xC of108frustum is unwritten/unused and excluded. No live containing-cluster/door producers or runtime culling integration.'}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='failures'},indent=2));print('failures',json.dumps(report['failures'])[:8000]);return bool(failures)
if __name__=='__main__':raise SystemExit(main())
