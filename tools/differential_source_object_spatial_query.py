#!/usr/bin/env python3
"""Compare BB050 and its complete spatial traversal against unchanged Xbox code.

The declared BSP/pool banks are supplied test inputs. Registration chronology,
unit location, pose/primitive production and player dispatch are not executed.
BB050, B8940, B8A10, 14A5B0, 14A370, 120850 and 23A160 run without substituted
services or patched instructions. Returned full handles, epochs and complete
visit-bank FNV hashes are compared; all supplied spatial banks stay unchanged.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BANK,STACK,STOP=0x2100000,0x22f0000,0x3000000
META,COLLISION,PLANES,CLUSTERS,LINKS,PORTALS,VERTICES=[BANK+x for x in (0,0x1000,0x2000,0x4000,0x1c000,0x20000,0x40000)]
HEADERS=[BANK+0x80000,BANK+0x80100]
RECORDS=[BANK+0x90000,BANK+0xa0000]
HEADS=[BANK+0xb0000,BANK+0xb1000]
POINT,LOCATION,OUTPUT=BANK+0xb2000,BANK+0xb2100,BANK+0xb3000

def packed(words):return struct.pack('<'+'I'*len(words),*[w&0xffffffff for w in words])
def fw(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def fnv(raw):
    value=2166136261
    for b in raw:value=((value^b)*16777619)&0xffffffff
    return value
def base(**changes):
    planes=[[1.,0.,0.,1.],[1.,0.,0.,2.]]
    portals=[dict(back=i,front=i+1,plane=i,center=[i+1.,0.,0.],radius=3.,
                  vertices=[[i+1.,-2.,-2.],[i+1.,-2.,2.],[i+1.,2.,2.],[i+1.,2.,-2.]]) for i in range(2)]
    row=dict(lists=0,types=0xffffebcf,cluster=0,center=[0.,0.,0.],radius=3.,capacity=64,
             object_epoch=0,cluster_epoch=0,object_fill=0,cluster_fill=0,
             planes=planes,portals=portals,links=[[0],[0,1],[1]],
             pools=[[[[0x10001,8,fw(.5),0,0,fw(.2)]],[[0x10002,8,fw(1.5),0,0,fw(.2)]],[]],
                    [[],[],[[0x10003,2,fw(2.5),0,0,fw(.2)]]]],object_stamps={},cluster_stamps={})
    row.update(changes);return row
def encode(r):
    w=[r['lists'],r['types'],r['cluster'],*map(fw,r['center']),fw(r['radius']),r['capacity'],
       r['object_epoch'],r['cluster_epoch'],r['object_fill'],r['cluster_fill'],
       len(r['links']),len(r['planes']),len(r['portals'])]
    for p in r['planes']:w.extend(map(fw,p))
    for p in r['portals']:
        w.extend([(p['back']&0xffff)|((p['front']&0xffff)<<16),p['plane'],*map(fw,p['center']),
                  fw(p['radius']),len(p['vertices'])])
        for v in p['vertices']:w.extend(map(fw,v))
    for c in r['links']:w.extend([len(c),*c])
    for pool in r['pools']:
        for c in pool:
            w.append(len(c))
            for record in c:w.extend(record)
    for key in ('object_stamps','cluster_stamps'):
        w.append(len(r[key]))
        for slot,stamp in r[key].items():w.extend([slot,stamp])
    return [v&0xffffffff for v in w]

class Oracle:
    def __init__(self):
        self.image=Xbe(ROOT/'assets/disc/default.xbe')
        assert hashlib.sha256(self.image.data).hexdigest()==PIN
        rows={int(r['va'],16):r for r in csv.DictReader(open(ROOT/'tools/reference/halo2-decompiled/config/functions.csv'))}
        self.ranges=[(a,a+int(rows[a]['size'])) for a in (0xbb050,0xb8940,0xb8a10,0x14a5b0,0x14a370,0x120850,0x23a160)]
        self.g=Uc(UC_ARCH_X86,UC_MODE_32);self.g.mem_map(0x10000,0x600000)
        for section in self.image.sections:self.g.mem_write(section.va,self.image.section_bytes(section))
        self.g.mem_map(BANK,0x200000);self.g.mem_map(STOP,0x1000)
        self.visited=set();self.calls={};self.g.hook_add(UC_HOOK_CODE,self.hook)
    def put(self,a,w):self.g.mem_write(a,packed([w]))
    def word(self,a):return struct.unpack('<I',self.g.mem_read(a,4))[0]
    def hook(self,g,pc,size,user):
        if not any(a<=pc<b for a,b in self.ranges):raise RuntimeError(f'unexpected original instruction {pc:08x}')
        if bytes(g.mem_read(pc,size))!=self.image.read(pc,size):raise RuntimeError('original instruction changed')
        self.visited.add(pc)
        if any(pc==a for a,_ in self.ranges):self.calls[f'{pc:08x}']=self.calls.get(f'{pc:08x}',0)+1
    def original(self,r):
        g=self.g
        g.mem_write(META,b'\0'*0x80000)
        self.put(0x4e0348,META);self.put(0x4e033c,COLLISION)
        self.put(META+0x18,COLLISION);self.put(META+0x60,PORTALS);self.put(META+0xa0,CLUSTERS)
        self.put(COLLISION+0xc,PLANES)
        g.mem_write(PLANES,packed([fw(v) for p in r['planes'] for v in p]))
        vertex_at=VERTICES
        for i,p in enumerate(r['portals']):
            data=[(p['back']&0xffff)|((p['front']&0xffff)<<16),p['plane'],*map(fw,p['center']),
                  fw(p['radius']),0,len(p['vertices']),vertex_at]
            g.mem_write(PORTALS+36*i,packed(data))
            if p['vertices']:g.mem_write(vertex_at,packed([fw(v) for xyz in p['vertices'] for v in xyz]))
            vertex_at+=12*len(p['vertices'])
        link_at=LINKS
        for i,links in enumerate(r['links']):
            self.put(CLUSTERS+0xb0*i+0x8c,len(links));self.put(CLUSTERS+0xb0*i+0x90,link_at)
            if links:g.mem_write(link_at,struct.pack('<'+'H'*len(links),*links))
            link_at+=2*len(links)
        for kind,pool in enumerate(r['pools']):
            g.mem_write(RECORDS[kind],b'\0'*0x10000);g.mem_write(HEADS[kind],b'\xff'*0x800)
            self.put(HEADERS[kind]+0x24,32);self.put(HEADERS[kind]+0x44,RECORDS[kind]);slot=0
            for c,records in enumerate(pool):
                if records:self.put(HEADS[kind]+4*c,0x80000000|slot)
                for n,record in enumerate(records):
                    tail=0x80000000|(slot+1) if n+1<len(records) else 0xffffffff
                    g.mem_write(RECORDS[kind]+32*slot,packed([0x8000,record[0],tail,*record[1:]]));slot+=1
        for address,value in zip((0x4de2e0,0x4de2e4,0x4de2d4,0x4de2d8),(HEADS[0],HEADERS[0],HEADS[1],HEADERS[1])):self.put(address,value)
        self.put(0x4de2fc,r['object_epoch']);self.put(0x4e7414,r['cluster_epoch'])
        g.mem_write(0x4de300,packed([r['object_fill']]*2048));g.mem_write(0x4e7418,packed([r['cluster_fill']]*512))
        for slot,value in r['object_stamps'].items():self.put(0x4de300+4*slot,value)
        for slot,value in r['cluster_stamps'].items():self.put(0x4e7418+4*slot,value)
        g.mem_write(0x4de2f8,b'\0');g.mem_write(0x4e7411,b'\0')
        g.mem_write(POINT,packed(list(map(fw,r['center']))));g.mem_write(LOCATION,packed([0xffffffff,r['cluster']&0xffff]))
        g.mem_write(OUTPUT,b'\xcd'*0x1000)
        protected=[(META,0x80200),(RECORDS[0],0x20000),(HEADS[0],0x2000)]
        before=[bytes(g.mem_read(a,n)) for a,n in protected]
        g.mem_write(STACK,packed([STOP,r['lists'],r['types'],LOCATION,POINT,fw(r['radius']),OUTPUT,r['capacity']]))
        g.reg_write(R.UC_X86_REG_ESP,STACK);g.reg_write(R.UC_X86_REG_FPCW,0x027f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        g.emu_start(0xbb050,STOP,count=2000000)
        assert g.reg_read(R.UC_X86_REG_EIP)==STOP
        assert [bytes(g.mem_read(a,n)) for a,n in protected]==before,'spatial input bank mutated'
        assert bytes(g.mem_read(0x4de2f8,1))==b'\0' and bytes(g.mem_read(0x4e7411,1))==b'\0'
        count=g.reg_read(R.UC_X86_REG_EAX)&0xffff
        ids=list(struct.unpack('<'+'I'*count,g.mem_read(OUTPUT,count*4))) if count else []
        assert bytes(g.mem_read(OUTPUT+count*4,0x1000-count*4))==b'\xcd'*(0x1000-count*4),'query wrote beyond returned prefix'
        return [count,*ids,self.word(0x4de2fc),self.word(0x4e7414),fnv(g.mem_read(0x4de300,2048*4)),fnv(g.mem_read(0x4e7418,512*4))]

def cases():
    rows=[]
    for lists in (0,1,2,3,4,0xffffffff):
        for types in (0,1,4,0x100,0xffffebcf):
            for radius in (-1.,0.,1.,3.):rows.append(base(lists=lists,types=types,radius=radius))
    for capacity in (0,1,2,3,4,64,32767):rows.append(base(capacity=capacity))
    for cluster in (-1,0,1,2):rows.append(base(cluster=cluster))
    for object_epoch in (0,7,0xfffffffe,0xffffffff):
        for cluster_epoch in (0,7,0xfffffffe,0xffffffff):
            rows.append(base(object_epoch=object_epoch,cluster_epoch=cluster_epoch,
                             object_stamps={1:0,2:(object_epoch+1)&0xffffffff},cluster_stamps={1:0,2:7}))
    # One record seen outside the radius stamps its slot before a nearer
    # duplicate in the next cluster/list. Type-rejected records do not stamp.
    for first_type in (2,8,40,255):
        pools=[[[[0x10001,first_type,fw(99),0,0,0]],[[0x10001,8,fw(1.5),0,0,0]],[]],
               [[[0x20001,8,fw(.5),0,0,0]],[],[]]]
        rows.append(base(pools=pools,types=0x100))
    crowded=[[],[],[]]
    crowded[0]=[[0x10000+i,8,fw(.5),0,0,fw(.2)] for i in range(1,71)]
    for capacity in (0,1,17,64,70):rows.append(base(pools=[crowded,[[],[],[]]],radius=1.,capacity=capacity))
    # Explicit squared-distance boundaries distinguish the list kinds' axis
    # accumulation order. Ordinary random positions rarely exercise this.
    rounding=random.Random(0xbb050);found=0
    while found<8:
        x,y,z=[f32(rounding.uniform(-10,10)) for _ in range(3)]
        xx,yy,zz=[f32(v*v) for v in (x,y,z)]
        a,b=f32(f32(zz+yy)+xx),f32(f32(zz+xx)+yy)
        radius=f32(a**.5);square=f32(radius*radius)
        if (square>=a)==(square>=b):continue
        for kind in (0,1):
            pools=[[[],[],[]],[[],[],[]]]
            pools[kind][0]=[[0x10001,8,fw(x),fw(y),fw(z),fw(radius)]]
            rows.append(base(pools=pools,radius=0.))
        found+=1
    rng=random.Random(0x151110)
    for _ in range(640):
        pools=[[[] for _ in range(3)] for _ in range(2)]
        for n in range(rng.randrange(1,110)):
            c=rng.randrange(3);kind=rng.randrange(2);slot=rng.randrange(1,100)
            record=[(rng.choice([1,2,0x8001])<<16)|slot,
                    rng.choice([0,2,3,4,5,7,8,9,10,11,31,32,40,255]),
                    *map(fw,[rng.uniform(-5,5) for _ in range(3)]),fw(rng.choice([0.,.2,1.,4.]))]
            pools[kind][c].append(record)
        rows.append(base(pools=pools,lists=rng.choice([0,1,2,3,4,0xffffffff]),types=rng.getrandbits(32),
                         cluster=rng.choice([-1,0,1,2]),center=[rng.uniform(-2,3) for _ in range(3)],
                         radius=rng.choice([-1.,0.,.1,1.,3.,8.]),capacity=rng.choice([0,1,2,5,64]),
                         object_epoch=rng.choice([0,7,0xfffffffe,0xffffffff]),
                         cluster_epoch=rng.choice([0,7,0xfffffffe,0xffffffff]),
                         object_fill=rng.choice([0,8,0xffffffff]),cluster_fill=rng.choice([0,8,0xffffffff])))
    return rows

def snapshot_cases(directory):
    # Replay queries with retained Zanzibar geometry and independent forward
    # lists. Backing addresses are relocated, and portal plane indices are
    # remapped to their unchanged consumed planes. These are declared queries,
    # not captured input scheduling or an invented nearby player pose.
    from differential_source_object_admission_snapshot import Snapshot,uint,words
    s=Snapshot(directory);digest=hashlib.sha256(s.ram).hexdigest()
    assert digest=='ab7e518eda315ecd55e681d440c988d2814260c292bbbd69972754ac7d32bf55'
    bsp=uint(s.read(0x4e0348,4));header=s.read(bsp,0xa8)
    count,portal_count=uint(header,0x9c),uint(header,0x5c)
    collision=uint(header,0x18);plane_address=uint(s.read(collision,16),12)
    planes=[];plane_map={};portals=[]
    for i in range(portal_count):
        raw=s.read(uint(header,0x60)+36*i,36);back,front=struct.unpack_from('<hh',raw)
        index=uint(raw,4)
        if index not in plane_map:
            plane_map[index]=len(planes);planes.append(list(struct.unpack('<4f',s.read(plane_address+16*index,16))))
        vertices=uint(raw,28);address=uint(raw,32)
        portals.append(dict(back=back,front=front,plane=plane_map[index],
            center=list(struct.unpack_from('<3f',raw,8)),radius=struct.unpack_from('<f',raw,20)[0],
            vertices=[list(struct.unpack('<3f',s.read(address+12*j,12))) for j in range(vertices)]))
    links=[]
    for i in range(count):
        raw=s.read(uint(header,0xa0)+0xb0*i,0xb0);n=uint(raw,0x8c)
        links.append(list(struct.unpack('<'+'H'*n,s.read(uint(raw,0x90),n*2))) if n else [])
    pools=[]
    for side in ('collidable','noncollidable'):
        heads=words(s.file(side+'-heads.bin'));records=s.file(side+'-entries-records.bin');pool=[]
        for c in range(count):
            token=heads[c];seen=set();rows=[]
            while token!=0xffffffff:
                slot=token&0xffff;assert slot<2048 and slot not in seen;seen.add(slot)
                r=words(records[slot*32:slot*32+32]);rows.append([r[1],*r[3:]]);token=r[2]
            pool.append(rows)
        pools.append(pool)
    stamps=words(s.read(0x4de300,2048*4));clusters=words(s.read(0x4e7418,512*4))
    common=dict(planes=planes,portals=portals,links=links,pools=pools,
                object_epoch=uint(s.read(0x4de2fc,4)),cluster_epoch=uint(s.read(0x4e7414,4)),
                object_stamps={i:v for i,v in enumerate(stamps) if v},
                cluster_stamps={i:v for i,v in enumerate(clusters) if v})
    rows=[];queries=[]
    for owner in s.report['objects']:
        raw=s.file('object-'+owner['handle']+'.bin')
        if raw[0xaa] not in (0,8):continue
        center=list(struct.unpack_from('<3f',raw,0x30));bound=struct.unpack_from('<f',raw,0x3c)[0]
        cluster=struct.unpack_from('<h',raw,0x2c)[0]
        radius=struct.unpack('<f',struct.pack('<f',bound+.4))[0]+3.
        for cap in (0,1,64):
            rows.append(base(**common,center=center,cluster=cluster,radius=radius,capacity=cap))
            queries.append(dict(owner=owner['handle'],type=raw[0xaa],capacity=cap))
    provenance=dict(snapshot_sha256=digest,clusters=count,portals=portal_count,consumed_planes=len(planes),queries=queries,
                    boundary='Retained geometry/forward-list/stamp fields in relocated backing; diagnostic query centers at actual unit/control bounds, not captured input calls.')
    return rows,provenance

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--binary',type=Path,required=True)
    ap.add_argument('--output',type=Path,default=ROOT/'analysis/map-world-features/control-spatial-query-differential.json')
    ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/retail_source_object_spatial_query.tsv');args=ap.parse_args()
    oracle=Oracle();rows=cases();declared=len(rows)
    retained,provenance=snapshot_cases(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    rows.extend(retained);inputs=[encode(r) for r in rows];expected=[oracle.original(r) for r in rows]
    payload=''.join(' '.join(f'{v:x}' for v in w)+'\n' for w in inputs)
    p=subprocess.run([str(args.binary.resolve()),'--probe'],input=payload,text=True,capture_output=True)
    if p.returncode:raise RuntimeError(p.stderr)
    actual=[[int(v,16) for v in line.split()] for line in p.stdout.splitlines()]
    mismatches=[i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
    if len(actual)!=len(expected) or mismatches:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(dict(cases=len(rows),actual_cases=len(actual),mismatches=mismatches,
            examples=[dict(index=i,input=rows[i],expected=expected[i],actual=actual[i]) for i in mismatches[:5]]),indent=2)+'\n')
        raise RuntimeError(f'{len(mismatches)} spatial-query mismatches; see {args.output}')
    fixture='# Declared source banks; unchanged BB050 and complete callees. Output includes full visit-bank FNV hashes.\n'
    fixture+=''.join(' '.join(f'{v:x}' for v in a)+' | '+' '.join(f'{v:x}' for v in b)+'\n' for a,b in zip(inputs,expected))
    args.fixture.write_text(fixture)
    report=dict(boundary=__doc__,original_sha256=PIN,cases=len(rows),compared_output_words=sum(map(len,expected)),
                declared_cases=declared,retained_snapshot_cases=len(retained),snapshot=provenance,
                distinct_original_instructions=len(oracle.visited),calls=oracle.calls,mismatches=0,
                source_services_replaced=0,protected_spatial_banks_per_case=3,
                fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
                tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest())
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
