#!/usr/bin/env python3
"""Execute untouched134390 and full165010/181800/134300/16E210 callees.

Explicit main-mode0/auxNULL or --sphere-mode with prior queues, source capacities, ordered
cluster membership and authored spheres. Cache cases first execute original
441B0+173130 to obtain the actual bounded constructed frustum bank.
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
BASE,STACK,STOP=0x2100000,0x28fff00,0x3000000
BSP,CLUSTERS,INSTANCES,DEFS,SET,CONTEXT,QUEUES,STREAM,CAMERA,VIEW,RAW,PORTALS,PLANES,MASK,SCENARIO=[BASE+x for x in (0,0x10000,0x40000,0x80000,0xb0000,0x140000,0x150000,0x180000,0x300000,0x310000,0x380000,0x600000,0x610000,0x620000,0x630000)]
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def scalar(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def packed(v):return struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v))
def flat(v):return [word(x)for p in v for x in p]
def fnv(data):
    h=14695981039346656037
    for x in data:h=((h^x)*1099511628211)&0xffffffffffffffff
    return h
def serialize(case):
    out=[case['ccap'],case['icap'],case['flags'],case['context'],*map(word,case['plane']),case['groups'],len(case['entries'])]
    for e in case['entries']:out.extend(x&65535 for x in e)
    out.append(len(case['frustums']));out.extend(flat(case['frustums']));out.extend([len(case['defs']),*case['defs'],len(case['instances'])])
    for i in case['instances']:out.extend([i[0]&65535,*map(word,i[1:])])
    out.append(len(case['clusters']))
    for c in case['clusters']:out.extend([c['geometry'],*map(word,c['bounds']),len(c['ids']),*c['ids'],c['fog']])
    if 'mode' in case:
        out.extend([0x314c5053,case['mode'],*map(word,case['sphere']),int(case['auxiliary'] is not None)])
        if case['auxiliary'] is not None:
            for ids in case['auxiliary']:out.extend([len(ids),*ids])
    return out
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer-shading/source_bsp_admission-standalone');parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/bsp-admission-differential.json');parser.add_argument('--write-fixture',type=Path);parser.add_argument('--write-assets',type=Path);parser.add_argument('--count',type=int,default=2048);parser.add_argument('--skip-maps',action='store_true')
    parser.add_argument('--sphere-mode',action='store_true',help='Exercise132E60 sphere volume and actual prior BSP queue membership')
    args=parser.parse_args()
    image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in image.sections:u.mem_write(s.va,image.section_bytes(s))
    u.mem_map(BASE,0x800000);u.mem_map(STOP,0x1000)
    entries=[0x131ff0,0x134980,0x376f00,0x441b0,0x173130,0x134390,0x1348e0,0x132e60,0x165010,0x181800,0x134300,0x16e210]
    calls={e:0 for e in entries}
    allocator_active=False;allocation_cursor=BASE+0x690000;allocation_calls=0
    def hook(_u,at,_size,_data):
        nonlocal allocation_cursor,allocation_calls
        if at in calls:calls[at]+=1
        if allocator_active and at in [0x3312cc,0x3202bb]:
            # Only external allocation services are supplied for131FF0;
            # untouched134980 builds every actual queue/header itself.
            sp=u.reg_read(UC_X86_REG_ESP);ret,size=struct.unpack('<II',u.mem_read(sp,8));allocation_calls+=1
            result=BASE+0x640000 if at==0x3312cc else allocation_cursor
            if at==0x3202bb:allocation_cursor=(allocation_cursor+size+15)&~15
            if result+size>BASE+0x800000:raise RuntimeError('initializer allocation bank exhausted')
            u.mem_write(result,bytes(size));u.reg_write(UC_X86_REG_EAX,result);u.reg_write(UC_X86_REG_ESP,sp+(20 if at==0x3312cc else 4));u.reg_write(UC_X86_REG_EIP,ret)
    u.hook_add(UC_HOOK_CODE,hook)
    def put(at,*v):u.mem_write(at,packed(v))
    def get(at,n=1):return list(struct.unpack('<'+'I'*n,u.mem_read(at,n*4)))
    def run(at,args=(),regs={}):
        put(STACK,STOP,*args);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        for r,v in regs.items():u.reg_write(r,v)
        u.emu_start(at,STOP,count=10000000)
        if u.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'original{at:x} did not return')
    allocator_active=True;run(0x131ff0);allocator_active=False
    queue_initializers=[]
    for at in [0x547f88,0x54aa68]:
        tuple_values=list(struct.unpack('<6H',u.mem_read(at+0x2ad4,12)))
        queue_initializers.append({'context':f'{at:08x}','tuple':tuple_values,'cluster_capacity':get(get(at+0xc)[0])[0],'instance_capacity':get(get(at+0x18)[0])[0]})
    native_caps=subprocess.run([str(args.binary.resolve()),'--capacities'],capture_output=True,text=True,check=True)
    if [int(v,16)for v in native_caps.stdout.split()]!=[queue_initializers[0]['cluster_capacity'],queue_initializers[0]['instance_capacity']]:raise RuntimeError('native main capacities differ from original131FF0/134980')
    run(0x376f00)
    def setup_bsp(case):
        u.mem_write(BSP,bytes(0x140000));cursor=RAW
        put(BSP+0x9c,len(case['clusters']),CLUSTERS);put(BSP+0x138,len(case['defs']),DEFS);put(BSP+0x140,len(case['instances']),INSTANCES)
        for j,c in enumerate(case['clusters']):
            at=CLUSTERS+j*0xb0;u.mem_write(at+0x24,struct.pack('<H',c['geometry']));put(at+0x54,*map(word,c['bounds']));put(at+0x98,len(c['ids']),cursor)
            if c['ids']:u.mem_write(cursor,struct.pack('<'+'H'*len(c['ids']),*c['ids']))
            cursor=(cursor+len(c['ids'])*2+3)&~3
            # Exact16E210 service with explicit positive palette0 or nofogFF.
            u.mem_write(at+0x70,bytes([0 if c['fog'] else 255]))
        for j,i in enumerate(case['instances']):
            at=INSTANCES+j*0x58;u.mem_write(at+0x34,struct.pack('<h',i[0]));put(at+0x3c,*map(word,i[1:]))
        for j,g in enumerate(case['defs']):u.mem_write(DEFS+j*0xc8+0x24,struct.pack('<H',g))
        put(0x4e0348,BSP);put(0x4e0350,SCENARIO);put(SCENARIO+0x34c,SCENARIO+0x800);put(SCENARIO+0x808,0x1234);put(0x4b9f8c,0x1234);put(0x4e64c8,0)
        return cursor
    def original(case):
        setup_bsp(case);u.mem_write(SET,bytes(0x24000));u.mem_write(CONTEXT,bytes(0x2b00));u.mem_write(QUEUES,bytes(0x1000));u.mem_write(STREAM,bytes(0x180000))
        u.mem_write(SET,struct.pack('<h',case['groups']));u.mem_write(SET+0xa6c,struct.pack('<h',len(case['entries'])))
        for i,e in enumerate(case['entries']):u.mem_write(SET+0xa6e+i*0x1a,struct.pack('<13H',*(v&65535 for v in e)))
        for i,f in enumerate(case['frustums']):put(SET+0x1974+i*0x108+0x60,*map(word,f))
        put(CONTEXT,SET);put(CONTEXT+0x2a60,case['flags']);put(CONTEXT+0x2a78,case['context'],0);put(CONTEXT+0x2a88,*map(word,case['plane']))
        if 'mode' in case:
            put(CONTEXT+0x2a7c,case['mode']);put(CONTEXT+0x2a68,*map(word,case['sphere']))
            auxiliary=case['auxiliary'];aux=BASE+0x740000
            u.mem_write(aux,bytes(0x10000));put(CONTEXT+0x2ad0,aux if auxiliary is not None else 0)
            if auxiliary is not None:
                for q,(offset,ids) in enumerate(zip((0xc,0x18),auxiliary)):
                    header=aux+0x3000+q*0x100;stream=aux+0x4000+q*0x4000
                    put(aux+offset,header);put(header,1024,len(ids),0,stream)
                    if ids:put(stream,*ids)
        for q,offset,capacity in [(0,0xc,case['ccap']),(1,0x18,case['icap']),(2,0x10,1),(3,0x14,1)]:
            at=QUEUES+q*0x100;storage=STREAM+q*0x60000;put(at,capacity,0,storage,storage+0x10000,storage+0x30000,storage+0x50000);put(CONTEXT+offset,at)
        put(0x4e7c1c,1);u.mem_write(0x4e7c20,bytes(len(case['instances'])*4))
        run(0x1348e0,regs={UC_X86_REG_EDX:CONTEXT});run(0x134390,[CONTEXT]);out=get(CONTEXT+0x1c,128)+get(CONTEXT+0x21c,16)
        for q in [0,1]:
            at=QUEUES+q*0x100;n=struct.unpack('<H',u.mem_read(at+4,2))[0];out.append(n)
            flags,ids,other,assoc=get(at+8,4)
            for i in range(n):out.extend([get(ids+i*4)[0],struct.unpack('<H',u.mem_read(flags+i*2,2))[0],get(other+i*4)[0],struct.unpack('<H',u.mem_read(assoc+i*2,2))[0]])
        n=struct.unpack('<H',u.mem_read(CONTEXT+0xa5e,2))[0];out.extend([n,*get(CONTEXT+0xa60,n*4)]);return out
    rng=random.Random(0x134390);cases=[];labels=[];expected=[];mapreport=[];assetrows=[]
    def add(case,label):
        if args.sphere_mode:
            case=dict(case);case['mode']=1;case['sphere']=[rng.uniform(-5,5),rng.uniform(-5,5),rng.uniform(-5,5),rng.choice([0,.01,.371,2.5,3,10,32])]
            case['auxiliary']=None if len(cases)%4==0 else [[i for i in range(len(case['clusters']))if rng.random()<.5],[i for i in range(len(case['instances']))if rng.random()<.5]]
            case['flags']|=int(case['auxiliary'] is not None)
        cases.append(case);labels.append(label);expected.append(original(case))
    box=[-10,10,-10,10,-10,10];planes=[[1,0,0,10],[-1,0,0,10],[0,1,0,10],[0,-1,0,10],[0,0,1,10],[0,0,-1,10]];frustum=box+[x for p in planes for x in p]
    for j in range(args.count):
        ng=1+j%6;nc=1+j%12;ni=1+j%30;nf=nc;entries=[]
        order=list(range(nc));rng.shuffle(order)
        for k in order:
            counts=[0]*6;first=[-1]*6;counts[j%ng]=1;first[j%ng]=k;entries.append([k,*counts,*first])
        spheres=[]
        radii=[0,.01,2.5,scalar(word(2.5)-1),scalar(word(2.5)+1),3,10]
        for k in range(ni):spheres.append([k%3,rng.choice([-15,-10,-8,0,8,10,15]),rng.uniform(-15,15),rng.uniform(-15,15),radii[(j+k)%len(radii)]])
        clusters=[{'geometry':rng.choice([0,1,2,65535]),'bounds':box,'ids':[k for k in range(ni)if rng.random()<.6], 'fog':int(rng.random()<.3)}for k in range(nc)]
        case={'ccap':rng.choice([1,2,5,513]),'icap':rng.choice([1,2,5,1025]),'flags':rng.choice([0,0,0x80,0x200,0x280,0x20]),'context':j%2,'plane':[rng.uniform(-1,1),rng.uniform(-1,1),rng.uniform(-1,1),rng.uniform(-20,20)],'groups':ng,'entries':entries,'frustums':[frustum]*nf,'defs':[0,1,65535],'instances':spheres,'clusters':clusters}
        add(case,f'synthetic{j}')
    # Exact inclusive2.5 promotion, clipping sides and retained repeated masks.
    for radius in [scalar(word(2.5)-1),2.5,scalar(word(2.5)+1),3,0]:
        for x in [-13,-10,-7,0,7,10,13]:
            for context in [0,1]:
                add({'ccap':3,'icap':3,'flags':0x200,'context':context,'plane':[1,0,0,0],'groups':1,'entries':[[1,1,0,0,0,0,0,0,65535,65535,65535,65535,65535],[0,1,0,0,0,0,0,0,65535,65535,65535,65535,65535]],'frustums':[frustum],'defs':[1],'instances':[[0,x,0,0,radius]],'clusters':[{'geometry':1,'bounds':box,'ids':[0,0],'fog':1},{'geometry':0,'bounds':box,'ids':[0],'fog':0}]},f'endpoint{radius}:{x}:{context}')
    # Group allocation precedes capacity admission, including duplicatefailed
    # appends,512 overflow returning0, and successful repeatOR into group0.
    for ni in [511,512,513,1024]:
        for cap in [1,2,513,1025]:
            ids=list(range(ni));clusters=[{'geometry':1,'bounds':box,'ids':ids,'fog':0},{'geometry':0,'bounds':box,'ids':ids,'fog':1}]
            add({'ccap':1,'icap':cap,'flags':0,'context':1,'plane':[0,0,1,0],'groups':1,'entries':[[k,1,0,0,0,0,0,0,65535,65535,65535,65535,65535]for k in range(2)],'frustums':[frustum],'defs':[1],'instances':[[0,10,0,0,3]for k in ids],'clusters':clusters},f'capacity{ni}:{cap}')
    for cap in [1,128,129]:
        for context in [0,1]:
            order=list(range(512));rng.shuffle(order);order=order[:128];clusters=[{'geometry':1,'bounds':box,'ids':[0],'fog':int(k%7==0)}for k in range(512)]
            add({'ccap':cap,'icap':2,'flags':0,'context':context,'plane':[0,0,0,0],'groups':1,'entries':[[k,1,0,0,0,0,0,0,65535,65535,65535,65535,65535]for k in order],'frustums':[frustum],'defs':[1],'instances':[[0,10,0,0,2.5]],'clusters':clusters},f'128entrymask:{cap}:{context}')
    for groups,entries,flags in [(0,[],0),(1,[],0),(0,[[0,0,0,0,0,0,0,65535,65535,65535,65535,65535,65535]],0),(0,[[0,0,0,0,0,0,0,65535,65535,65535,65535,65535,65535]],0x80)]:
        add({'ccap':128,'icap':384,'flags':flags,'context':0,'plane':[0,0,0,0],'groups':groups,'entries':entries,'frustums':[],'defs':[1],'instances':[[0,0,0,0,1]],'clusters':[{'geometry':1,'bounds':box,'ids':[0],'fog':1}]},f'emptyrange:{groups}:{len(entries)}:{flags}')
    if not args.skip_maps:
        for group,names in [('disc',['ascension','beavercreek','burial_mounds','coagulation','colossus','cyclotron','foundation','headlong','lockout','midship','waterworks','zanzibar']),('map-pack',['containment','warlock','deltatap','turf','backwash','dune','elongation','gemini','triplicate'])]:
            for name in names:
                path=ROOT/'assets'/group/'maps'/(name+'.map');c=Cache(path);_,s=c.tag(c.scenario,'scnr');ref=c.block(s+0x210,68)[0];at,size,mem=struct.unpack_from('<III',c.data,ref);b=at+16;magic=mem-at
                clusterptrs=c.block(b+0x9c,0xb0,magic);clusters=[];clusterportals=[]
                for p in clusterptrs:
                    ids=[c.u16(q)for q in c.block(p+0x98,2,magic)];bounds=list(struct.unpack_from('<6f',c.data,p+0x54));clusters.append({'geometry':c.u16(p+0x24),'bounds':bounds,'ids':ids,'fog':int(struct.unpack_from('<b',c.data,p+0x70)[0]!=-1)});clusterportals.append([c.u16(q)for q in c.block(p+0x8c,2,magic)])
                defs=[c.u16(p+0x24)for p in c.block(b+0x138,0xc8,magic)];instances=[[struct.unpack_from('<h',c.data,p+0x34)[0],*struct.unpack_from('<4f',c.data,p+0x3c)]for p in c.block(b+0x140,0x58,magic)]
                # Radius is+48; center occupies3C/40/44, contiguous fourthfloat.
                collision=c.block(b+0x14,64,magic)[0];sourceplanes=[struct.unpack_from('<4f',c.data,p)for p in c.block(collision+8,16,magic)];portals=[]
                for p in c.block(b+0x5c,36,magic):portals.append({'back':c.u16(p),'front':c.u16(p+2),'plane':c.u32(p+4),'flags':c.u32(p+24),'vertices':[struct.unpack_from('<3f',c.data,v)for v in c.block(p+28,12,magic)]})
                template={'ccap':queue_initializers[0]['cluster_capacity'],'icap':queue_initializers[0]['instance_capacity'],'flags':0,'context':0,'plane':[0,0,0,0],'groups':1,'entries':[],'frustums':[],'defs':defs,'instances':instances,'clusters':clusters};start=len(cases)
                cb=b''.join(struct.pack('<H6fI',q['geometry'],*q['bounds'],len(q['ids']))+struct.pack('<'+'H'*len(q['ids']),*q['ids'])for q in clusters);ib=b''.join(struct.pack('<h4f',*i)for i in instances);db=struct.pack('<'+'H'*len(defs),*defs);hashes=[fnv(x)for x in [cb,ib,db]];values=[len(clusters),len(instances),len(defs),sum(len(q['ids'])for q in clusters),*[x for h in hashes for x in [h&0xffffffff,h>>32]]]
                assetrows.append(str(path.relative_to(ROOT))+' | '+' '.join(f'{v:x}'for v in values))
                for j,cluster in enumerate(clusters):
                    setup_bsp(template);cursor=RAW+0x10000
                    put(BSP+0x18,BSP+0x1000);put(BSP+0x1000+0xc,PLANES);put(PLANES,*flat(sourceplanes));put(BSP+0x5c,len(portals),PORTALS)
                    for k,p in enumerate(portals):
                        put(PORTALS+k*36,(p['back']&65535)|((p['front']&65535)<<16),p['plane']);put(PORTALS+k*36+24,p['flags'],len(p['vertices']),cursor);put(cursor,*flat(p['vertices']));cursor+=len(p['vertices'])*12
                    for k,ids in enumerate(clusterportals):
                        put(CLUSTERS+k*0xb0+0x8c,len(ids),cursor)
                        if ids:u.mem_write(cursor,struct.pack('<'+'H'*len(ids),*ids))
                        cursor=(cursor+len(ids)*2+3)&~3
                    put(MASK,*([0xffffffff]*16));put(0x4f93a4,MASK);eye=[(cluster['bounds'][k]+cluster['bounds'][k+1])*.5 for k in [0,2,4]];yaw=(j%16)*math.pi/8;put(VIEW+8,*flat([eye,[math.cos(yaw),math.sin(yaw),-.1],[0,0,1]]));put(VIEW+0x48,word(.015625),word(1024));put(VIEW+0x68,*map(word,[-.532049,.532049,-.399037,.399037]));u.mem_write(CAMERA,bytes(0x1bc));run(0x441b0,[VIEW],{UC_X86_REG_EBX:VIEW+8,UC_X86_REG_ESI:CAMERA});u.mem_write(SET,bytes(0x24000));run(0x173130,[1,CAMERA,j,SET]);nc=struct.unpack('<H',u.mem_read(SET+0xa6c,2))[0];nf=struct.unpack('<H',u.mem_read(SET+0x1970,2))[0]
                    case=dict(template);case['entries']=[list(struct.unpack('<13H',u.mem_read(SET+0xa6e+k*0x1a,0x1a)))for k in range(nc)];case['frustums']=[list(struct.unpack('<30f',u.mem_read(SET+0x1974+k*0x108+0x60,120)))for k in range(nf)];add(case,f'{name}:cluster{j}:original173130:explicitallopen')
                mapreport.append({'map':name,'clusters':len(clusters),'instances':len(instances),'definitions':len(defs),'memberships':values[3],'cases':len(cases)-start,'cache_sha256':hashlib.sha256(path.read_bytes()).hexdigest()});c.close()
    encoded=[' '.join(f'{v:x}'for v in serialize(case))for case in cases];native=subprocess.run([str(args.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',capture_output=True,text=True,check=True);actual=[[int(v,16)for v in line.split()]for line in native.stdout.splitlines()]
    if len(actual)!=len(expected):raise RuntimeError('native admission probe lost cases')
    failures=[{'case':i,'label':labels[i],'differences':[{'word':j,'source':s,'native':n}for j,(s,n)in enumerate(zip(source,result))if s!=n][:32],'source_words':len(source),'native_words':len(result),'input':serialize(cases[i])}for i,(source,result)in enumerate(zip(expected,actual))if source!=result]
    if args.write_fixture and not failures:
        selected=sorted(set([*range(min(128,args.count)),*range(args.count,args.count+96),*range(args.count+96,len(cases),max(1,(len(cases)-args.count-96)//128))]));args.write_fixture.write_text('# Untouched134390 full pure services; source '+PIN+'\n'+'\n'.join(encoded[i]+' | '+' '.join(f'{v:x}'for v in expected[i])for i in selected)+'\n')
    if args.write_assets and not failures:args.write_assets.write_text('# All21 literal134390 BSP fields; FNV1a64 low/high halves\n'+'\n'.join(assetrows)+'\n')
    context_scope=('sphere mode1 with explicit sphere and optional prior cluster/instance queues; exact sphere overlap/containment and163D20 membership' if args.sphere_mode else 'main mode0/auxNULL; exact165010/181800 frustum admission')
    report={'source_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'sphere_mode':args.sphere_mode,'cases':len(cases),'bit_exact_cases':len(cases)-len(failures),'compared_words':sum(map(len,expected)),'original_calls':{f'{at:08x}':n for at,n in calls.items()},'queue_initializers':queue_initializers,'external_allocation_calls':allocation_calls,'maps':mapreport,'failures':failures[:8],'scope':'Untouched134390 after reset and fresh epoch: '+context_scope+'. Exact16E210 services with no hooks. Queue capacities independently execute131FF0/134980 with only external allocation services. Map spheres/membership/geometry counts are literal cache fields; supplied visibility banks are original441B0+173130 with explicit fixture camera and all-open mask when maps are enabled. No live camera/door, sphere-constructor or dynamic132220 admission claim.'}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='failures'},indent=2));print('failures',json.dumps(failures[:8])[:10000]);return bool(failures)
if __name__=='__main__':raise SystemExit(main())
