#!/usr/bin/env python3
"""Execute untouched132220/133050 and all ordinary admission callees.

Ordered lists are built by original1CAC60 with explicit successful allocator
services; actual spheres and full handles are fixture inputs. Auxiliary/light
volume banks are absent. No owner list, queue, sphere or fog helper is hooked.
"""
import argparse, collections, hashlib, json, random, struct, subprocess, sys
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x28fff00,0x3000000
BSP,CLUSTERS,SET,CONTEXT,QUEUES,STREAM,ROOTS,HEADS,POOLS,HANDLES,OBJECTS,CONTACTS,MEMBERS,BITS,PRIMITIVE,LOCATION,TRUNCATED,SCENARIO=[BASE+x for x in (0,0x10000,0x40000,0x80000,0x90000,0xa0000,0x100000,0x110000,0x120000,0x130000,0x180000,0x280000,0x380000,0x480000,0x490000,0x4a0000,0x4b0000,0x4c0000)]
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def scalar(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def packed(v):return struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v))
def serialize(c):
    out=[c['capacity'],c['flags'],*map(word,c['plane']),c['sky']&65535,c['groups'],len(c['entries'])]
    for e in c['entries']:out.extend(x&65535 for x in e)
    out.append(len(c['frustums']))
    for f in c['frustums']:out.extend(map(word,f))
    out.extend([len(c['fog']),*c['fog'],len(c['objects'])])
    for o in c['objects']:out.extend([o['id'],o['flags'],o['type']&255,*map(word,o['sphere']),o['collidable'],len(o['members']),*o['members']])
    out.append(len(c['prior']))
    for mask in c['prior']:out.extend(mask)
    return out
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,required=True);p.add_argument('--count',type=int,default=2048);p.add_argument('--write-fixture',type=Path);p.add_argument('--write-cold',type=Path);p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/object-admission-differential.json');args=p.parse_args()
    image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in image.sections:u.mem_write(s.va,image.section_bytes(s))
    u.mem_map(BASE,0x800000);u.mem_map(STOP,0x1000)
    source_entries=[0x132220,0x133050,0x132e60,0x165010,0x181800,0x132fd0,0x132b10,0x134300,0x163c80,0xbec70,0x1cac60,0x14a550,0x16e210]
    calls=collections.Counter();allocations={};allocator_calls=0
    def read(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    def put(at,*v):u.mem_write(at,packed(v))
    def hook(_u,at,_size,_data):
        nonlocal allocator_calls
        if at in source_entries:calls[at]+=1
        if at==0x16b840:
            header=u.reg_read(UC_X86_REG_EDX);index=allocations[header];allocations[header]+=1;allocator_calls+=1
            if index>=4096:raise RuntimeError('explicit successful fixture allocator exhausted')
            datum=0xe5000000|index;sp=u.reg_read(UC_X86_REG_ESP)
            u.reg_write(UC_X86_REG_EAX,datum);u.reg_write(UC_X86_REG_ESP,sp+4);u.reg_write(UC_X86_REG_EIP,read(sp))
    u.hook_add(UC_HOOK_CODE,hook)
    def run(at,args=(),regs={}):
        for r in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(r,regs.get(r,0))
        put(STACK,STOP,*args);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.emu_start(at,STOP,count=10000000)
        if u.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'original{at:x} did not return')
    def original(c):
        u.mem_write(BSP,bytes(0x480000));allocations.clear()
        put(BSP+0x9c,len(c['fog']),CLUSTERS);put(0x4e0348,BSP);put(0x4e0350,SCENARIO)
        put(SCENARIO+0x34c,SCENARIO+0x100);put(SCENARIO+0x108,0x1234);put(0x4b9f8c,0x1234);put(0x4e64c8,0)
        for i,f in enumerate(c['fog']):u.mem_write(CLUSTERS+i*0xb0+0x70,bytes([0 if f else 255]))
        u.mem_write(SET,struct.pack('<h',c['groups']));u.mem_write(SET+0xa6c,struct.pack('<h',len(c['entries'])))
        for i,e in enumerate(c['entries']):u.mem_write(SET+0xa6e+i*0x1a,struct.pack('<13H',*(v&65535 for v in e)))
        for i,f in enumerate(c['frustums']):put(SET+0x1974+i*0x108+0x60,*map(word,f))
        put(CONTEXT,SET);u.mem_write(CONTEXT+8,struct.pack('<h',-1));put(CONTEXT+0x2a60,c['flags']);put(CONTEXT+0x2a88,*map(word,c['plane']))
        for q,offset in enumerate([0xc,0x10,0x14,0x18]):
            at=QUEUES+q*0x100;storage=STREAM+q*0x10000;put(at,c['capacity']if q==2 else 1,0,storage,storage+0x2000,storage+0x4000,storage+0x6000);put(CONTEXT+offset,at)
        run(0x1348e0,regs={UC_X86_REG_EDX:CONTEXT})
        u.mem_write(CONTEXT+0xa5e,struct.pack('<H',len(c['prior'])))
        for i,mask in enumerate(c['prior']):put(CONTEXT+0xa60+i*16,*mask)
        u.mem_write(0x4de300,bytes(0x2000));put(0x4de2fc,1);put(0x4e0300,POOLS+0x800);put(POOLS+0x800+0x44,HANDLES)
        for side in [0,1]:
            root=ROOTS+side*0x100;heads=HEADS+side*0x1000;contact=POOLS+side*0x100;member=POOLS+0x400+side*0x100
            put(root,heads,contact,member);put(contact+0x24,32);put(contact+0x44,CONTACTS+side*0x80000);put(member+0x24,12);put(member+0x44,MEMBERS+side*0x80000)
            allocations[contact]=allocations[member]=0;u.mem_write(heads,bytes([255])*len(c['fog'])*4)
        put(0x4de2e0,HEADS+0x1000);put(0x4de2e4,POOLS+0x100);put(0x4de2d4,HEADS);put(0x4de2d8,POOLS)
        put(0x4e0310,HEADS+0x2000);u.mem_write(HEADS+0x2000,bytes([255])*len(c['fog'])*4)
        for i,o in enumerate(c['objects']):
            slot=o['id']&65535;obj=OBJECTS+i*0x400;put(HANDLES+slot*12+8,obj);put(obj+4,o['flags']);u.mem_write(obj+0xaa,bytes([o['type']&255]));put(obj+0x30,*map(word,o['sphere']));put(obj+0x60,0xffffffff)
            u.mem_write(PRIMITIVE,struct.pack('<hH4I',o['type'],0,*map(word,o['sphere'])))
            forced=[sum(1<<(j&31)for j in o['members']if j//32==w)for w in range((len(c['fog'])+31)//32)]
            put(BITS,*forced);u.mem_write(LOCATION+4,struct.pack('<h',-1))
            run(0x1cac60,[ROOTS+o['collidable']*0x100,o['id'],obj+0x60,PRIMITIVE+4,word(o['sphere'][3]),LOCATION,20,PRIMITIVE,TRUNCATED],{UC_X86_REG_EAX:BITS})
        put(0x4b9ee9,0);put(0x4b9eec,0xffffffff);u.mem_write(0x4686c4,struct.pack('<h',-1))
        if c['sky']!=-1:
            u.mem_write(0x4b9ee9,bytes([1]));put(0x4b9eec,0);u.mem_write(0x4686c4,struct.pack('<h',0));put(BSP+0xac,1,BSP+0x800);u.mem_write(BSP+0x800,struct.pack('<h',c['sky']))
        run(0x132220,[CONTEXT]);queue=QUEUES+0x200;n=struct.unpack('<H',u.mem_read(queue+4,2))[0];flags,ids,other,assoc=struct.unpack('<4I',u.mem_read(queue+8,16));out=[n]
        for i in range(n):out.extend([read(ids+i*4),struct.unpack('<H',u.mem_read(flags+i*2,2))[0],read(other+i*4),struct.unpack('<H',u.mem_read(assoc+i*2,2))[0]])
        n=struct.unpack('<H',u.mem_read(CONTEXT+0xa5e,2))[0];out.append(n);out.extend(struct.unpack('<'+'I'*(n*4),u.mem_read(CONTEXT+0xa60,n*16)))
        return out
    rng=random.Random(0x132220);cases=[];labels=[];expected=[];map_cases=[]
    def add(c,label):cases.append(c);labels.append(label);expected.append(original(c))
    def cube(center=(0,0,0),half=10):
        x,y,z=center;return [x-half,x+half,y-half,y+half,z-half,z+half,1,0,0,x+half,-1,0,0,half-x,0,1,0,y+half,0,-1,0,half-y,0,0,1,z+half,0,0,-1,half-z]
    for i in range(args.count):
        count=1+i%12;groups=1+i%6;order=list(range(count));rng.shuffle(order);frusta=[cube(half=rng.choice([1,5,10,30]))for _ in order]
        entries=[]
        for j,cluster in enumerate(order):
            counts=[0]*6;first=[-1]*6;counts[i%groups]=1;first[i%groups]=j;entries.append([cluster,*counts,*first])
        flags=rng.choice([0,0,0,0x10,0x40,0x80,0x200,0x400,0x600,0x680]);sky=rng.randrange(count)if i%19==0 else -1
        owners=[]
        for j in range(1+i%32):
            o=dict(id=((0xe000+j)<<16)|j,flags=rng.choice([0,0x80000000]),type=rng.choice([0,6,7,11]),sphere=[rng.choice([-35,-10,-8,0,8,10,35]),rng.uniform(-15,15),rng.uniform(-15,15),rng.choice([0,.01,2.5,scalar(word(2.5)+1),3,10])],collidable=rng.randrange(2),members=rng.sample(range(count),rng.randrange(count+1)))
            # The original sky branch has a negative association index. The
            # bounded public seam rejects that unsafe bank write explicitly.
            if sky!=-1:o['flags']=0
            owners.append(o)
        prior_count=512 if i%37==0 else rng.randrange(5)
        c=dict(capacity=rng.choice([1,2,4,8,256]),flags=flags,plane=[rng.uniform(-1,1)for _ in range(4)],sky=sky,groups=groups,entries=entries,frustums=frusta,fog=[rng.randrange(2)for _ in order],objects=owners,prior=[[rng.getrandbits(32)for _ in range(4)]for _ in range(prior_count)])
        add(c,f'synthetic-{i}')
    # Original cold spheres provide real world magnitudes/flags/type inputs;
    # cluster locations/order remain explicit synthetic services in these cases.
    for kind in ['scenery','machine','prop']:
        records=json.loads((ROOT/f'analysis/multiplayer-shading/{kind}-cold-spatial-original.json').read_text())['records']
        by_map=collections.defaultdict(list)
        for r in records:by_map[r['map']].append(r)
        for name,records in by_map.items():
            for first in range(0,len(records),32):
                rows=records[first:first+32];sphere=[scalar(w)for w in rows[0]['initial_sphere_words']];frusta=[cube(sphere[:3],30)]
                objects=[dict(id=0xe0000000|i,flags=r['before_registration_flags'],type={'scenery':6,'machine':7,'prop':11}[kind],sphere=[scalar(w)for w in r['initial_sphere_words']],collidable=int(bool(r['before_registration_flags']&0x200)),members=[0])for i,r in enumerate(rows)]
                c=dict(capacity=256,flags=0,plane=[0,0,0,0],sky=-1,groups=1,entries=[[0,1,0,0,0,0,0,0,-1,-1,-1,-1,-1]],frustums=frusta,fog=[0],objects=objects,prior=[])
                add(c,f'cold-{kind}-{name}-{first}');map_cases.append(dict(kind=kind,map=name,owners=len(rows)))
    rows=[' '.join(f'{w:x}'for w in serialize(c))for c in cases]
    actual=[list(int(w,16)for w in line.split())for line in subprocess.check_output([str(args.binary.resolve()),'--probe'],input='\n'.join(rows)+'\n',text=True).splitlines()]
    assert len(actual)==len(expected)
    failures=[dict(label=labels[i],input=rows[i],original=a,native=b)for i,(a,b)in enumerate(zip(expected,actual))if a!=b]
    if args.write_fixture:
        chosen=[i for i in range(len(cases))if i<20 or i%8==0 or labels[i].startswith('cold-')]
        args.write_fixture.write_text('# Untouched132220/133050; source '+PIN+'; PC53/MXCSR1F80\n'+''.join(rows[i]+' | '+' '.join(f'{w:x}'for w in expected[i])+'\n'for i in chosen))
    if args.write_cold:
        cold=[]
        for kind in ['scenery','machine','prop']:
            for r in json.loads((ROOT/f'analysis/multiplayer-shading/{kind}-cold-spatial-original.json').read_text())['records']:
                input=[*r['spatial_definition_words'],*r['initial_sphere_words'],r['object_scale_word'],0,0,0,0]
                output=[*r['dynamic_sphere_words'],*r['primitive_sphere_words'],1]
                cold.append(' '.join(f'{w:x}'for w in input)+' | '+' '.join(f'{w:x}'for w in output))
        args.write_cold.write_text('# Untouched cold BDEF0 observations; source '+PIN+'; SCEN278/MACH49/BLOC415\n'+'\n'.join(cold)+'\n')
    report=dict(xbe_sha256=PIN,native_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),fpcw='027F',mxcsr='1F80',cases=len(cases),exact=len(cases)-len(failures),words=sum(map(len,expected)),original_calls={f'{a:08x}':calls[a]for a in source_entries},successful_allocator_calls=allocator_calls,cold_map_cases=map_cases,failures=failures[:4],scope='Untouched ordinary main-mode0 auxNULL queue with unchanged frustum/fog/association services and original successful head registration. Explicit synthetic full-handle chronology and source cold-sphere cases. No original initial location/creation-order/allocator-pressure, light-volume, aux163D20 or negative-index sky association producer claim.')
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in ['cold_map_cases','scope','failures']},indent=2));print(json.dumps(failures[:1],indent=2));assert not failures
if __name__=='__main__':main()
