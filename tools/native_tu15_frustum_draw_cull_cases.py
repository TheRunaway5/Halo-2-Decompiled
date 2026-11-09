"""Declared raw/typed44E80 and44710 inputs; original execution supplies outcomes."""
import copy
import math
import random
import struct

D=0x1000000
POOL,HEADERS,TAGS,BSP,MODE,SECTIONS,INVERSE,BODY=D,D+0x800,D+0x1000,D+0x1800,D+0x2000,D+0x3000,D+0x10000,D+0x18000
COLLECT,CONTEXT,MASK,GEOMETRY=D+0x30000,D+0x40000,D+0x50000,D+0x60000
SP,STOP=0x62018000,0x63000000
SECTIONS_MAP={'constant':(0x46a000,4096),'units':(0x4c6000,8192),'context':(0x4cd000,4096),
    'objects':(0x4ec000,4096),'tags':(0x4f0000,4096),'data':(D,0x80000),'literal_bounds':(0x2000000,20*65536),
    'stack':(0x62000000,0x20000)}
NONE=0xffffffff


def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def real(value):return struct.unpack('<f',struct.pack('<I',value))[0]
def matrix(position=(0,0,0),angle=0,scale=1,tilt=0,roll=0):
    c,s=math.cos(angle),math.sin(angle);ct,st=math.cos(tilt),math.sin(tilt);cr,sr=math.cos(roll),math.sin(roll)
    return list(map(bits,[scale,c*ct,s*ct,-st,c*st*sr-s*cr,s*st*sr+c*cr,ct*sr,
        c*st*cr+s*sr,s*st*cr-c*sr,ct*cr,*position]))
def model(parts=(0,),bounds=((0,0,-2,.5,0),),mask_offset=0,nodes=None,inverse=None,flags=0x40,association=0):
    return dict(parts=list(parts),bounds=[[*map(bits,b[:4]),b[4]]for b in bounds],count=len(parts),mask_offset=mask_offset,
        nodes=nodes if nodes is not None else[matrix()],inverse=inverse if inverse is not None else[matrix()],flags=flags,association=association)


def cases(image,genuine=()):
    rows=[]
    def add(name,items,*,initial=None,associations=None,entries=1,frustums=None,walk=False,prefix=0,total=None,mode=1,entry=0,fp=None,visibility_ranges=None,groups=1):
        items=copy.deepcopy(items);mask=[0]*512 if initial is None else initial[:]
        frustums=frustums or[dict(bounds=list(map(bits,[-5,5,-5,5,-5,5])),planes=[list(map(bits,p))for p in
            ((1,0,0,5),(-1,0,0,5),(0,1,0,5),(0,-1,0,5),(0,0,1,5),(0,0,-1,5))])]
        banks=[dict(address=at,before=bytearray(size))for name,(at,size)in SECTIONS_MAP.items()if name!='literal_bounds']
        def put(at,value,size=4):
            for bank in banks:
                if bank['address']<=at and at+size<=bank['address']+len(bank['before']):
                    bank['before'][at-bank['address']:at-bank['address']+size]=(value&((1<<(8*size))-1)).to_bytes(size,'little');return
            raise ValueError('Unmapped declared frustum input '+hex(at))
        def words(at,values):
            for i,value in enumerate(values):put(at+4*i,value)
        banks[0]['before'][:]=image.read(0x46a000,4096)
        put(0x4ecbd0,POOL);put(POOL+0x44,HEADERS);put(0x4f0414,TAGS);put(0x4ecc18,BSP)
        put(0x4cd3a0,CONTEXT);put(CONTEXT,COLLECT);put(CONTEXT+0x2a78,mode);put(CONTEXT+0x2a7c,0)
        put(0x4cd39c,prefix);put(0x4cd398,len(items)if total is None else total)
        put(COLLECT,groups,2);put(COLLECT+0xa6c,entries,2)
        ranges=[]
        for i in range(entries):
            first=i%len(frustums);put(COLLECT+0xa6e+i*26,i,2);put(COLLECT+0xa70+i*26,1,2);put(COLLECT+0xa7c+i*26,first,2)
            ranges.append(dict(cluster=i,counts=[1,0,0,0,0,0],first=[first,0,0,0,0,0]))
        if visibility_ranges is not None:
            ranges=copy.deepcopy(visibility_ranges)
            assert len(ranges)==entries
            for i,r in enumerate(ranges):
                put(COLLECT+0xa6e+i*26,r['cluster'],2)
                for j in range(6):
                    put(COLLECT+0xa70+i*26+j*2,r['counts'][j],2)
                    put(COLLECT+0xa7c+i*26+j*2,r['first'][j],2)
        for i,f in enumerate(frustums):
            words(COLLECT+0x1974+i*0x108+0x60,f['bounds'])
            for j,plane in enumerate(f['planes']):words(COLLECT+0x1974+i*0x108+0x78+j*16,plane)
        associations=associations or[]
        for i,association in enumerate(associations):words(CONTEXT+0xa60+i*16,association)
        words(MASK,mask)
        for i,item in enumerate(items):
            slot=GEOMETRY+i*0x1000;geometry,parts,bounds,context=slot,slot+0x100,slot+0x400,slot+0xc00
            w=[item['flags'],0xaabb0000+i,0x12340000,0,context,MASK+item['mask_offset']*4,item['association'],0]
            if item.get('absent'):
                item['words']=w;words(0x4c6958+i*32,w);continue
            mode_at,section=MODE+i*0x100,SECTIONS+i*0x100
            owner=0xbeef0000+i;body=BODY+i*0x1000;inverse=INVERSE+i*0x1000
            put(TAGS+i*16+8,mode_at);put(mode_at+0x28,section);put(mode_at+0x4c,inverse);put(section+0x34,geometry)
            put(context,owner);put(HEADERS+i*12+8,body);put(body+0x114,len(item['nodes'])*52,2);put(body+0x116,0x200,2)
            for j,node in enumerate(item['nodes']):words(body+0x200+j*52,node)
            for j,node in enumerate(item['inverse']):words(inverse+0x28+j*96,node)
            put(geometry+8,item['count']);put(geometry+0xc,parts);put(geometry+0x14,bounds)
            for j,index in enumerate(item['parts']):put(parts+j*8+4,index,2)
            if item.get('literal_ffff'):
                banks.append(dict(address=0x2000000,before=bytearray(20*65536)));bounds=0x2000000;put(geometry+0x14,bounds)
            for j,bound in enumerate(item['bounds']):words(bounds+j*20,bound)
            if item.get('literal_ffff'):words(bounds+65535*20,item['literal_ffff'])
            if item.get('transform'):
                w[7]=context+0x20;words(w[7],item['transform'])
            item.update(words=w,owner=owner);words(0x4c6958+i*32,w)
        spec=dict(units=items,arena=mask,arena_address=MASK,arena_used=511,groups=groups,ranges=ranges,frustums=frustums,
            associations=associations,mode=mode,prefix=prefix,total=len(items)if total is None else total,entry=entry,walk=walk)
        row=dict(id=len(rows),name=name,entry=0x44710 if walk else 0x44e80,stop_pc=STOP,max_steps=100000,
            gp=[0,0,0,0,SP,0,0,0],matrix=0,source=entry,before_fp=fp or[0x27f,0,0x1f80],live_x87=[],spec=spec,
            banks=[dict(address=b['address'],before=b['before'].hex())for b in banks])
        rows.append(row)
        return row
    for selected in (False,True):
        add('original shared-mask flag chronology '+str(selected),[model()],initial=[int(selected)]+[0]*511)
    for scale in (0,.5,1,2,-1):
        for angle in (0,.625,-.375):
            for delta in (-1,0,1):
                x=real(bits(5.5)+delta)
                add('WORLD point order, scale and inclusive adjacent plane',
                    [model(bounds=((x,.25,-1,.5,0),),nodes=[matrix((.125,-.25,.5),angle,scale)],inverse=[matrix((-.25,.125,0),-.125)])])
    for tilt in (-.5,.25,1.1):
        for roll in (.3,-.7):
            for scale in (.5,1,2):
                add('three-axis WORLD/inverse composition and point sum order',[model(bounds=((.3333333,-2.71828,3.14159,.5,0),),
                    nodes=[matrix((.125,-.25,.5),.625,scale,tilt,roll)],inverse=[matrix((-.25,.125,0),-.125,1,-.2,.4)])])
    for count in (-2,0,1,33,65):
        item=model(parts=[0]*max(1,count),nodes=[matrix(),matrix(scale=1e-30)],inverse=[matrix(),matrix(scale=1e-30)])
        item['count']=count
        add('signed count with eager unused underflowing node composition '+str(count),[item])
    for node in (0,1,2,254,255):
        n=node+1
        row=add('literal current/inverse node '+str(node),[model(bounds=((.25,0,-1,.25,node),),nodes=[matrix((i*.01,0,0))for i in range(n)],inverse=[matrix()for _ in range(n)])])
        if node==255:
            # Whole original256-node composition overwrites its caller's
            # return word. Observe the unchanged prefix before its epilogue;
            # the native typed boundary rejects this invalid matrix count.
            row.update(stop_before_entries=[0x45545],native_rejection=True)
    item=model(parts=[0xffff],bounds=());item['literal_ffff']=[*map(bits,[20,0,0,.5]),0]
    add('FFFF is a literal outside bound65535, never automatic admission',[item])
    add('masked FFFF does not consume an unavailable literal bound',[model(parts=[0xffff],bounds=())],initial=[1]+[0]*511)
    add('ignored unit transform in linear WORLD branch',[dict(model(),transform=matrix((100,100,100),.5,3))])
    add('shared mask publishes before next model',[model(),model()],walk=True)
    add('partially overlapping shared mask publishes before next model',[model(parts=[0]*33),model(mask_offset=1)],walk=True)
    for total,prefix,mode in ((2,0,1),(0x10002,0x10001,1),(2,2,0)):
        add('low16 draw endpoint and pass mode',[model(),model(mask_offset=1)],walk=True,total=total,prefix=prefix,mode=mode)
    add('all128 association bits ascend without flag1 retest',[model(flags=0x48)],walk=True,entries=128,associations=[[0xffffffff]*4])
    empty=dict(model(flags=0x48),absent=True)
    add('empty association leaves unavailable geometry unread',[empty],walk=True,associations=[[0]*4])
    for flag in (0,1,0x41):add('ineligible unit leaves unavailable geometry unread '+str(flag),[dict(model(flags=flag),absent=True)],walk=True)
    rng=random.Random(0x4471044e80)
    for n in range(48):
        nodes=[matrix((rng.uniform(-2,2),rng.uniform(-2,2),rng.uniform(-2,2)),rng.uniform(-1,1),rng.choice([0,.5,1,2,-1]),rng.uniform(-1,1),rng.uniform(-1,1))for _ in range(3)]
        inverse=[matrix((rng.uniform(-1,1),0,0),rng.uniform(-1,1),1,rng.uniform(-1,1),rng.uniform(-1,1))for _ in range(3)]
        bounds=[(rng.uniform(-8,8),rng.uniform(-8,8),rng.uniform(-8,8),rng.choice([0,.25,1,-1]),rng.randrange(3))for _ in range(4)]
        item=model(parts=[rng.randrange(4)for _ in range(1+n%65)],bounds=bounds,nodes=nodes,inverse=inverse)
        initial=[rng.getrandbits(32)&rng.getrandbits(32)for _ in range(512)]
        add('controlled literal model/frustum bank '+str(n),[item],initial=initial,fp=[0x27f|((n%4)<<10),0,0x1f80|((n%4)<<13)])
    for fixture in genuine:
        row=add(fixture['name'],fixture['units'],initial=fixture['arena'],associations=fixture['associations'],
            entries=len(fixture['ranges']),frustums=fixture['frustums'],visibility_ranges=fixture['ranges'],
            groups=fixture['groups'],walk=True,mode=fixture.get('mode',0),
            prefix=0 if fixture.get('mode',0)else len(fixture['units']),fp=fixture.get('fp'))
        row['genuine_expected']=fixture['expected']
    return rows
