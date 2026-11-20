#!/usr/bin/env python3
"""Private full134D30 cluster contents proof against the physical original.

Unchanged TU1.5 instructions of 134D30, the queue append 166B60 (inlined by
the native), and the genuine children 171070 and the 135B60 tree
(135970/167EF0/184690/166C00/135AE0/135620/136E10/166B60/C0CA0) run
single-stepped on physical i386. Each run stops before the four separately
owned externals (135540, C5980, C5170, C5140); a deterministic stub
(identical to Stub in tests/native_tu15_cluster_contents_tests.cpp) supplies
that callee's ABI and storage effects and the original resumes at the return
address. The native route is compared at every ordered read and store
(complete GP/EFLAGS/XMM/MXCSR/x87 packet), at every required entry, at the
final ABI and over every backed byte.

PREFETCHT0 performs no architectural memory access (no value, no fault); its
operand is removed from the observer metadata, as the literal-data exporters
skip it.
"""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from validate_native_tu15_prt_helpers import PIN
from native_bsp_admission_original import observe as physical_original
UNIT='src/rendering/shadows/source_shadow_frame_tu15_generated.cpp'
BODY_PIN='ccf8da58ec7c68ce19cc5d6a3c0860ee6a664f3fbca031d6ed3c50a7697f4c54'
REGIONS=((0x46a000,0x1000),(0x474000,0x1000),(0x4c6000,0x1000),(0x4ea000,0x3000),(0x4f2000,0x1000),(0x61000000,0x20000),(0x62000000,0x4000))
OWNED={0x134d30:0x13528c,0x166b60:0x166bb0}
# The padding LEA after 134DE8's JMP is never executed.
UNREACHABLE=(0x134dea,)
ENTRIES=(0x134d30,0x171070,0x135b60,0x135970,0x167ef0,0x184690,0x166c00,0x135ae0,0x135620,0x136e10,0x166b60,0xc0ca0)
# Separately owned externals: entry -> (call site, argument bytes).
CALLEES={0x135540:(0x134fb3,8),0xc5980:(0x1350b0,8),0xc5170:(0x1350e7,0),0xc5140:(0x135113,0)}
ORDER=(0x135540,0xc5980,0xc5170,0xc5140)
INPUTS=(UNIT,'src/rendering/native_tu15_cluster_contents_systems.hpp','src/rendering/native_tu15_cluster_contents.hpp','tests/native_tu15_cluster_contents_tests.cpp','tests/native_tu15_cluster_contents_guards.inc','tools/validate_native_cluster_contents_tu15.py','tools/native_cluster_contents_replacements.py',
 'src/physics/runtime/native_tu15_object_tick_systems.hpp','src/rendering/native_tu15_object_admission.hpp','include/halo2/native_tu15_object_admission_kernel.hpp','src/rendering/native_tu15_fog_membership.hpp','include/halo2/native_tu15_fog_membership_kernel.hpp',
 'src/rendering/native_tu15_bsp_admission.hpp','src/rendering/native_tu15_geometry_visibility.hpp','include/halo2/native_tu15_geometry_visibility_kernel.hpp','src/rendering/native_tu15_cluster_frustum_abi.hpp','src/rendering/native_tu15_sphere_frustum_abi.hpp','src/rendering/native_tu15_bsp_queue_find.hpp','src/rendering/native_tu15_visibility_association.hpp',
 'src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/native_tu15_required_owner_policy.hpp','src/physics/runtime/native_tu15_census_admission.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_step_internal.hpp',
 'tools/native_bsp_admission_original.py','tools/native_geometry_visibility_original.py','tools/validate_native_tu15_prt_helpers.py')
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def prepare(scratch,require_binding=False):
    from generated_routine_ownership import function_bodies
    from audit_native_execution_ownership import unique_function
    import native_cluster_contents_replacements as binding
    from specialize_native_operands import specialize
    source=(ROOT/UNIT).read_text();match,body=binding.authenticate(source)
    if digest(body)!=BODY_PIN:raise ValueError('Complete134D30 owner drift')
    bound=binding.replace_cluster_contents(source)
    if require_binding and bound!=source:raise ValueError('Production134D30 default missing')
    if binding.replace_cluster_contents(bound)!=bound:raise ValueError('134D30 default installer is not idempotent')
    prepared,_=specialize(bound);resolver=unique_function(source,'selected')
    # The genuine children run on their own native bridges; the externals go
    # through the State's required boundary, as the unit's dispatcher does.
    helpers='''
static void run_171070(WorldState&s){::genuine_fog(s);}
static void run_135b60(WorldState&s){::genuine_admission(s);}
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){if(target==0x171070u)run_171070(s);else if(target==0x135b60u)run_135b60(s);else s.required(site,target,tail);}
'''
    text=unique_function(source,'sign_extend')+'\n';receipts=dict(body_sha256=BODY_PIN,resolver_sha256=digest(resolver))
    pick=lambda value,entry:next(item for sig,item in function_bodies(value)if sig[1]==entry)
    for name,unit in(('retained',source),('bound',bound),('prepared',prepared)):
        owner=body if name=='retained'else pick(unit,'134d30')
        leaf=pick(unit,'166b60')
        instrumented,count=re.subn(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::retained_statement();',owner)
        if count!=binding.SITES:raise ValueError('Incomplete415site cluster contents body '+name)
        text+='namespace '+name+' {\n'+resolver+'\nstatic void run_166b60(WorldState&);\n'+helpers+instrumented+'\n'+leaf+'\n}\n'
        receipts[name+'_body_sha256']=digest(owner);receipts[name+'_leaf_sha256']=digest(leaf)
    (scratch/'cluster_contents_bodies.inc').write_text(text);return receipts

# ------------------------------------------------------------------- corpus
CONTEXT=0x61001000;DESCRIPTOR=0x61004000;OBJECT_QUEUE=0x61006000;LIGHT_QUEUE=0x61006800
OBJECT_TABLE=0x61007000;RECORDS=0x61008000;AUX=0x61009000
HEADS=(0x6100a010,0x6100a110,0x6100a210);POOLS=(0x6100b000,0x6100c000,0x6100d000)
LIGHTS=0x6100e000;BSP=0x6100f000;FOG=0x61013000;STACK=0x62002000
FAMILIES=21
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
class World:
    def __init__(self,image,rng):
        self.rng=rng;self.banks=[bytearray(image.read(base,size))if base==0x46a000 else bytearray(rng.randbytes(size))for base,size in REGIONS]
    def put(self,at,value,width=4):
        for(base,size),bank in zip(REGIONS,self.banks):
            if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError('Unbacked cluster contents fixture '+hex(at))
    def get(self,at,width=4):
        for(base,size),bank in zip(REGIONS,self.banks):
            if base<=at<=base+size-width:return int.from_bytes(bank[at-base:at-base+width],'little')
        raise ValueError('Unbacked cluster contents input '+hex(at))
def specification(family,v,rng):
    """The case's world, as plain parameters."""
    p=dict(clusters=[1+v%3,4+v%2],count=None,coll={},noncoll={},lights={},flags=0,kind=0,objects={},sky=None,fog=v%3,
           direct=None,stub=v%5,sizes=(12,16,12),stamped=True,stamp_bits=0)
    for n,cluster in enumerate(p['clusters']):
        p['coll'][cluster]=[1+(n*3+k)%10 for k in range((v+n)%3)]
        p['noncoll'][cluster]=[11+(n+k)%4 for k in range((v+2*n)%3)]
        p['lights'][cluster]=[0xa0000000|(n*4+k)for k in range((v+n+1)%3)]
    if family==1:
        p['count']=(0,0x8000,0xffff,0,0x8000,0,0xffff,0)[v];p['direct']=dict(owner=0x10,listed=[0x20001,0x20002][:v%3],count=v%3,links=(0xffff,0xffff),stamps=(0,0))
    if family==2:p['flags']=0x40
    if family==3:p['flags']=0x10
    if family==4:p['kind']=(1,2,0xffffffff,1)[v%4];p['sky']=dict(visible=1,index=v%2,selection=0,count=2,owners=[3,5])
    if family==5:
        p['flags']=0x400|(0x10 if v%2 else 0)
        for idx in range(1,11):p['objects'][idx]=dict(type=7 if(idx+v)%3==0 else(idx%7 if idx%7!=0 else 1))
    if family==6:
        p['clusters']=[2,3,6][:1+v%3]
        for cluster in p['clusters']:
            p['coll'][cluster]=[]if(v+cluster)%2 else[2]
            p['noncoll'][cluster]=[]
            p['lights'][cluster]=[]if v%2 else[0xa0000001]
    if family==7:
        p['clusters']=[1,2,3,5][:2+v%3];p['sizes']=(16,12,20)
        for n,cluster in enumerate(p['clusters']):
            p['coll'][cluster]=[1+(n*2+k)%10 for k in range(2+(v+n)%3)]
            p['noncoll'][cluster]=[11+(n+k)%4 for k in range(1+(v+n)%3)]
            p['lights'][cluster]=[0xa0000000|(n*4+k)for k in range(1+(v+n)%3)]
    if family==8:
        # Sky cluster 7: an empty collideable list (v 0) and up to three
        # non-collideable objects (v 3).
        p['sky']=dict(visible=1,index=v%3,selection=v,count=3,owners=[7,6,7]);p['coll'][7]=[]if v%4==0 else[3,4,5][:1+v%3];p['noncoll'][7]=[12,13,14][:v%4];p['coll'][6]=[2];p['noncoll'][6]=[]
    if family==9:
        sky=dict(visible=1,index=1,selection=0,count=2,owners=[6,7]);p['coll'][7]=[3];p['noncoll'][7]=[12]
        if v==0:sky['visible']=0
        if v==1:sky['index']=0xffffffff
        if v==2:sky['selection']=0xffff
        if v==3:sky['count']=0
        if v==4:sky['count']=0x80000000
        if v==5:p['kind']=1
        if v==6:sky['owners']=[6,0xffff]
        if v==7:p['flags']=0x40
        p['sky']=sky
    if family==10:
        listed=[0x20001,0x10002,0x20003,0x20004][:1+v%4]
        p['direct']=dict(owner=v,listed=listed,count=len(listed),links=(0xffff,0xffff,0x0003,0xffff),stamps=(0,1,0,0),capacity=16,used=v%3)
    if family==11:
        listed=[0x20001,0x20002,0x20003]
        p['direct']=dict(owner=1,listed=listed,count=(3,0,0xffffffff,2,3,1,3,3)[v],links=(0xffff,0xffff,0xffff),stamps=(0,0,0),capacity=(2,3,4,8,1,0,0x80000000,5)[v],used=(1,1,2,1,0,0,0,4)[v])
        if v==6:p['direct']['capacity']=0
    if family==12:
        listed=[0x20002,0x20002,0x30001,0x20001]
        p['direct']=dict(owner=2,listed=listed,count=4,links=(0xffff,0xffff,0xffff,0xffff),stamps=(0,0,0,1),capacity=16,used=0)
        if v%2:p['flags']=0x10
    if family==13:p['stub']=(2,3,2,3,2,3,2,3)[v];p['clusters']=[1,2,4]
    if family==14:p['stamped']=False
    if family==15:
        p['fog']=1;p['sky']=dict(visible=1,index=0,selection=0,count=1,owners=[1]);p['coll'][1]=[2,3];p['noncoll'][1]=[11]
        # v 0 and 3: the object queue holds only object 2 and its flags
        # stream points at the collideable pool pointer, so 135B60's
        # mark_primary (OR BYTE [flags+row*2+1],4) moves [4EABB4] by 4 bytes
        # to an alternative pool whose entries all end the list. The original
        # reads the pool again after each admission.
        p['pool_alias']=v in(0,3)
    if family==16:
        p['fog']=2;p['clusters']=[0xffff,1,2] if v%2 else[1,0xffff]
        for cluster in p['clusters']:
            p['coll'].setdefault(cluster,[1]);p['noncoll'].setdefault(cluster,[]);p['lights'].setdefault(cluster,[0xa0000003])
    if family==17:
        # The light queue's value stream lands in the caller's listed light
        # buffer: the original reads each listed light again from the frame.
        listed=[0x20001,0x20002,0x20003,0x20004]
        p['direct']=dict(owner=3,listed=listed,count=4,links=(0xffff,0xffff,0xffff,0xffff),stamps=(0,0,0,0),capacity=16,used=0,alias=True)
    if family==18:
        p['sizes']=((20,0x10c,12),(12,12,0x110),(0x24,16,20),(12,20,16))[v%4]
        for cluster in p['clusters']:p['coll'][cluster]=[1,2];p['lights'][cluster]=[0xa0000001,0xa0000002]
    if family==19:
        p['clusters']=[3];p['coll'][3]=[1];p['noncoll'][3]=[11];p['lights'][3]=[0xa0000001]
        p['direct']=dict(owner=1,listed=[0x20001],count=1,links=(0xffff,),stamps=(0,),capacity=4,used=0)
        p['sky']=dict(visible=1,index=0,selection=0,count=1,owners=[3])
    if family==20:
        if v<4:
            # A sky owner word with bit 15 set: MOVSX ESI,AX makes the head
            # offset negative, so the sky lists come from heads[-2..-7].
            owner=(0xfffe,0xfffc,0xfff9,0xfffe)[v]
            p['sky']=dict(visible=1,index=v%2,selection=0,count=2,owners=[owner,owner])
            p['coll'][owner]=([],[3],[4,5],[2])[v];p['noncoll'][owner]=([12],[],[13],[11,14])[v]
        elif v<6:
            # The object queue holds only object 2 and its flags stream ends at
            # 135B60's saved EBP: its mark_primary (OR BYTE [flags+row*2+1],4,
            # the cluster in fog) changes the fog word 135B60 hands back in EBP.
            # The original passes that EBP to the following admissions.
            p['fog']=1;p['clusters']=[1];p['coll'][1]=[2,3]if v==4 else[2];p['noncoll'][1]=[11]if v==4 else[11,12]
            p['fog_alias']=True
        else:
            # Listed light indices whose low byte and the IMUL product's low
            # byte differ in parity (0x10, 0x13, 0x70): the flags 1350D4 leaves
            # reach the compared packet of the next read.
            listed=[0x20010,0x20013,0x10070,0x20001]if v==6 else[0x20070,0x20005,0x20010]
            p['direct']=dict(owner=v,listed=listed,count=len(listed),links=(0xffff,)*len(listed),stamps=(0,)*len(listed),capacity=16,used=0)
    if p['count']is None:p['count']=len(p['clusters'])
    return p
def build(image,rng,family,v):
    w=World(image,rng);p=specification(family,v,rng);put=w.put
    context=CONTEXT+v;descriptor=DESCRIPTOR+v;sp=STACK+4*(v%4)
    # The view: descriptor, owner, queues, flags (134D30's and 135B60's bits)
    # and kind (135B60's sphere selector).
    stamped=p['stamped'];clip=0x200 if(not stamped and v%2)else 0
    put(context,descriptor);put(context+8,(p['direct']or{}).get('owner',0xffff),2);put(context+0x10,LIGHT_QUEUE+v);put(context+0x14,OBJECT_QUEUE+v)
    put(context+0x2a60,p['flags']|clip|(0x100 if v%3==0 else 0));put(context+0x2a78,p['kind'])
    put(context+0x2a7c,(2,0,1,7)[v%4]if not stamped else 2);put(context+0xa5e,(0,1,3,511)[v%4],2)
    for axis in range(3):put(context+0x2a68+axis*4,bits(0.));put(context+0x2a88+axis*4,bits(1. if axis==v%3 else 0.))
    put(context+0x2a74,bits((10.,1.,100.,0.5)[v%4]));put(context+0x2a94,bits((-4.,0.,4.,0.5,-0.5,-1.5,1.5,9.)[v]));put(context+0x2ad0,0)
    # The visible cluster list. Every entry's frustum groups are those of the
    # 135970 proof (one open group); its first word is the cluster.
    put(descriptor,1,2);put(descriptor+0xa6c,p['count'],2)
    for n in range(8):
        entry=descriptor+0xa6e+n*26
        for offset in range(2,26,2):put(entry+offset,1 if offset==2 else 0,2)
        put(entry,p['clusters'][n]if n<len(p['clusters'])else rng.choice((1,2)),2)
    frustum=descriptor+0x1974
    for axis in range(3):put(frustum+0x60+axis*8,bits(-10.));put(frustum+0x64+axis*8,bits(10.))
    for plane in range(6):
        values=[0.,0.,0.,(9.,0.,-5.)[v%3]];values[plane//2]=1. if plane%2==0 else -1.
        for lane,value in enumerate(values):put(frustum+0x78+plane*16+lane*4,bits(value))
    # Objects: header table, records (two bounding spheres), types, stamps.
    stamp=0x5a000000+family;put(0x4eabcc,stamp);put(0x4ecbd0,OBJECT_TABLE);put(OBJECT_TABLE+0x44,OBJECT_TABLE+0x100)
    handles={}
    for idx in range(1,16):
        record=RECORDS+idx*0x100;handle=((0x1230+idx)<<16)|idx;handles[idx]=handle
        put(OBJECT_TABLE+0x100+idx*12+8,record);put(record,handle)
        info=p['objects'].get(idx,{})
        put(record+0xaa,info.get('type',(1,2,3,7)[idx%4]),1)
        put(record+4,(0x80000000 if(idx+v)%2 else 0)|rng.getrandbits(31));put(record+6,v%2,1)
        for sphere in(0x30,0x50):
            for axis in range(3):put(record+sphere+axis*4,bits((0.,1.5,-2.)[(v+axis+idx)%3]*(1 if sphere==0x30 else 0.5)))
            put(record+sphere+12,bits((1.,2.,0.5,4.)[(v+idx)%4]))
        put(0x4eabd0+idx*4,stamp if stamped else stamp^(1<<((v+idx)%31)))
    # The object queue holds most objects (stamped admission finds its row).
    queue=OBJECT_QUEUE+v;rows_count=(10,6,12,3,8,0,11,5)[v]if stamped else(0,2,7,3)[v%4]
    alias=p.get('pool_alias');stack_alias=p.get('fog_alias')
    if alias or stack_alias:rows_count=1
    # 135B60's saved EBP: 134D30's ESP is sp-0x34 after its prologue, four
    # arguments and the return address put 135B60's entry ESP at sp-0x48, and
    # its SUB ESP,14; PUSH EBX; PUSH EBP saves EBP at sp-0x64 (row 0, +1).
    flags_stream=0x4eabb3 if alias else sp-0x65 if stack_alias else queue+0x200
    put(queue,16);put(queue+4,rows_count,2);put(queue+8,flags_stream);put(queue+12,queue+0x100);put(queue+16,queue+0x300);put(queue+20,queue+0x400)
    for row in range(16):
        idx=1+(row*7+v)%15;put(queue+0x100+row*4,handles[idx]if row%5!=4 else handles[idx]^0x10000)
        put(queue+0x200+row*2,rng.choice((0,0x100,0x400,0x800)),2);put(queue+0x400+row*2,0xffff if row%4==3 else(row*3)%11,2)
    put(queue+0x1fe,rng.getrandbits(16),2);put(queue+0x3fe,(v*5)%13,2)
    if alias or stack_alias:put(queue+0x100,handles[2])
    # Cluster partitions: heads per cluster, entries pools of ClusterEntry.
    owners=(lambda idx:handles[idx],lambda idx:handles[idx],lambda handle:handle)
    signed=lambda cluster:cluster-0x10000 if cluster>=0x8000 else cluster
    low=min([-1]+[signed(cluster)for lists in(p['coll'],p['noncoll'],p['lights'])for cluster in lists])
    for which,lists in enumerate((p['coll'],p['noncoll'],p['lights'])):
        heads=HEADS[which];pool=POOLS[which];size=p['sizes'][which];data=pool+0x100;put(0x4eabb0 if which==0 else 0x4eaba4 if which==1 else 0x4ecbe0,heads)
        put((0x4eabb4,0x4eaba8,0x4ecbe4)[which],pool);put(pool+0x24,size);put(pool+0x44,data)
        for cluster in range(low,16):put(heads+cluster*4,0xffffffff)
        allocated=1
        for cluster,items in lists.items():
            cluster=signed(cluster)
            previous=None
            for item in items:
                entry=((0xe000+allocated)<<16)|allocated;datum=data+allocated*size;allocated+=1
                put(datum+4,owners[which](item));put(datum+8,0xffffffff)
                if previous is None:put(heads+cluster*4,entry)
                else:put(previous+8,entry)
                previous=datum
        if which==0 and p.get('pool_alias'):
            # The pool seen at [4EABB4]+4: the same datum size, and entries
            # that all end the list.
            put(pool+0x28,size);put(pool+0x48,pool+0x800)
            for n in range(24):put(pool+0x800+n*size+4,0xffffffff);put(pool+0x800+n*size+8,0xffffffff)
    # The sky: the BSP's owner table and the sky globals.
    sky=p['sky'];put(0x4ecc18,BSP);put(0x4c6709,0,1);put(0x4c670c,0xffffffff);put(0x474c24,0xffff,2)
    if sky:
        put(0x4c6709,sky['visible'],1);put(0x4c670c,sky['index']);put(0x474c24,sky['selection'],2);put(BSP+0xac,sky['count']);put(BSP+0xb0,BSP+0x100)
        for n,owner in enumerate(sky['owners']):put(BSP+0x100+n*2,owner,2)
    # 171070: no fog data, the override, or the full designator path.
    put(0x4c67ac,(0xffffffff,7,0x31,0x32)[v%4]);put(0x4f2d98,0x10 if p['fog']==1 else 0);put(0x4ecc20,FOG if p['fog']else 0)
    put(BSP+0xa0,BSP+0x1000);put(BSP+0x68,BSP+0x3000);put(FOG+0x34c,FOG+0x400)
    for cluster in range(16):
        put(BSP+0x1000+cluster*0xb0+0x70,(0xff,0x02,0x81,0x05)[(cluster+v)%4],1)
    for plane in range(8):put(BSP+0x3000+plane*24,(3,0xffff,2,1)[plane%4],2)
    for palette in range(8):put(FOG+0x400+palette*16+8,(0x31,0x32,7,0xffffffff)[(palette+v)%4])
    # Lights: the light array, the direct pass records, the light queue.
    put(0x4ecbdc,LIGHTS);put(LIGHTS+0x44,LIGHTS+0x100);frame=0x77000000+v;put(0x4ecbd8,frame)
    direct=p['direct']or dict(owner=0xffff,listed=[],count=0,links=(),stamps=())
    for n,handle in enumerate(direct['listed']):
        light=LIGHTS+0x100+(handle&0xffff)*0x110
        put(light+0x48,direct['links'][n],2);put(light+0xc,frame if direct['stamps'][n]else frame^0x100)
    lq=LIGHT_QUEUE+v;put(lq,direct.get('capacity',8));put(lq+4,direct.get('used',0),2)
    put(lq+8,lq+0x100);put(lq+12,lq+0x140);put(lq+16,lq+0x180);put(lq+20,lq+0x1c0)
    if direct.get('alias'):
        # The values stream's slot for the first appended entry is the frame
        # slot of the second listed light (ESP+0x28 inside 134D30's frame).
        frame_esp=sp-0x34
        put(lq+16,frame_esp+0x28)
        light=LIGHTS+0x100;put(light+0x48,0xffff,2);put(light+0xc,frame^0x100)
    put(sp,0x63000000);put(sp+4,context)
    gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
    depth=v%4;cw=(0x7f,0x27f,0x37f)[v%3]|((v%4)<<10);status=((v%8)<<11)|0x4500
    live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f')
    listed=[h for h in direct['listed']];count=direct['count']
    return dict(name=f'family{family}-variant{v}',entry=0x134d30,gp=gp,eflags=0x202|(0x400 if v%2 else 0)|(rng.getrandbits(12)&0x8d5),matrix=w.get(sp+4),source=w.get(sp+8),max_steps=100000,
        xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(live[:depth]),
        stub=p['stub'],listed=listed,listed_count=count if count<0x80000000 else count-0x100000000,banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,w.banks)])
def corpus(image):
    rng=random.Random(0x134d30);rows=[]
    for family in range(FAMILIES):
        for v in range(8):row=build(image,rng,family,v);row['id']=len(rows);rows.append(row)
    return rows

# --------------------------------------------------------------------- stubs
def stub(case,observed,row,call):
    """Return the continuation after the deterministic stub of the external at
    which the original stopped."""
    target=observed['stopped_at'];site,argument_bytes=CALLEES[target];which=ORDER.index(target);mode=row['stub']
    banks=[dict(address=bank['address'],before=after['after'])for bank,after in zip(case['banks'],observed['banks'],strict=True)]
    store={bank['address']:bytearray.fromhex(bank['before'])for bank in banks}
    def locate(at):
        for base,bank in store.items():
            if base<=at<=base+len(bank)-4:return bank,at-base
        raise ValueError('Stub storage is unbacked '+hex(at))
    def get(at):bank,offset=locate(at);return int.from_bytes(bank[offset:offset+4],'little')
    def put(at,value):bank,offset=locate(at);bank[offset:offset+4]=(value&0xffffffff).to_bytes(4,'little')
    gp=list(observed['boundary_gp']);sp=gp[4]
    if get(sp)!=site+5:raise ValueError('Required return address differs')
    seed=(0x51000000+0x101*call+mode+0x10000*which)&0xffffffff;handle=gp[0]
    gp[4]=sp+4+argument_bytes;gp[0]=seed;gp[1]=~seed&0xffffffff;gp[2]=seed^0x00ff00ff
    eflags=(observed['boundary_eflags']&0x400)|0x202|((0x11*call+0x41*mode)&0x8d5)
    xmm=[[(lane^(0x9e3779b9*(call+1)+r*4+k))&0xffffffff for k,lane in enumerate(item)]for r,item in enumerate(observed['boundary_xmm'])]
    fp=list(observed['after_fp'])
    if mode&1:fp[2]|=0x20
    if which==0:
        if mode==2 and call==0:gp[7]=0xffffffff
        if mode==3 and call==0:put(gp[4]+0x10,get(gp[4]+0x10)+1)
    elif which==1:
        buffer=get(sp+4)
        for k,item in enumerate(row['listed'][:4]):put(buffer+4*k,item)
        gp[0]=row['listed_count']&0xffffffff
    elif which==2:
        active=not((handle>>16)&1);gp[0]=(seed&0xffffff00)|((0x80 if mode==4 else 1)if active else 0)
    else:
        light=get(get(0x4ecbdc)+0x44)+(handle&0xffff)*0x110;put(light+0xc,get(0x4ecbd8));gp[0]=(seed&0xffffff00)|1
    for bank in banks:bank['before']=bytes(store[bank['address']]).hex()
    esp=gp[4]
    return dict(case,entry=site+5,gp=gp,eflags=eflags,xmm=xmm,before_fp=fp,live_x87=list(observed['boundary_live_x87']),banks=banks,return_pc=get(esp),matrix=get(esp+4),source=get(esp+8))
def entry_packet(observed):
    return dict(entry=observed['stopped_at'],gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'],live_x87=observed['boundary_live_x87'])
def segmented(image,rows,scratch,calls,label):
    """Run every row to completion (or its fault) through stub resumes."""
    state=[dict(segments=[],inputs=[],stops=[])for _ in rows];active=[]
    for index,row in enumerate(rows):
        case={key:row[key]for key in('name','entry','gp','eflags','xmm','before_fp','live_x87','banks','matrix','source','max_steps')}
        case['banks']=[dict(address=bank['address'],before=bank['before'])for bank in row['banks']];case['stop_before_entries']=list(ORDER)
        if 'segment_input' in row:case=copy.deepcopy(row['segment_input'])
        if 'fault' in row:case['fault']=row['segment_fault']
        active.append((index,case))
    rounds=0;authority=None;instructions=0
    while active:
        directory=scratch/f'{label}-segment{rounds}';directory.mkdir()
        cases=[]
        for n,(_,case) in enumerate(active):case=dict(case,id=n);cases.append(case)
        result,authority=physical_original(image,cases,directory,entries=ENTRIES,regions=REGIONS,trace_calls=calls,trace_extended=True)
        instructions+=result.get('original_instructions',0);following=[]
        for(index,case),observed in zip(active,result['cases'],strict=True):
            item=state[index];item['segments'].append(observed);item['inputs'].append(case)
            if observed.get('completed')and observed.get('stopped_at')in CALLEES:
                if 'fault' in case:raise ValueError('Prefix segment reached a required callee before its fault')
                item['stops'].append(entry_packet(observed))
                following.append((index,stub(case,observed,rows[index],len(item['stops'])-1)))
        active=following;rounds+=1
        if rounds>48:raise ValueError('Cluster contents corpus exceeds its required call budget')
    return state,authority,rounds,instructions

# Two earlier mutants are equivalent on every reachable input and are replaced
# by the nearby observable ones beside them:
#  - software-light-imul (IMUL ECX,ECX,110 at 1350D4 through the software
#    product, which defines CF/OF and keeps SF/ZF/AF/PF): the hardware block
#    differs only where the CPU redefines SF/ZF/AF/PF. The physical i386 host
#    (AMD Zen 5) keeps them from the preceding AND; family 20 lists light
#    indices 0x10/0x13/0x70, whose AND and product low bytes differ in parity,
#    and the software product still matched. The index is at most 0xFFFF, so
#    the product never overflows and CF/OF agree. Replaced by
#    unseeded-light-imul (the hardware block without the AND's flags).
#  - missing-end-or (OR reg,-1 at the end of an empty list as a plain move):
#    each of the ten ORs is immediately followed by CMP reg,-1, with no access
#    between them, and CMP defines all six status flags, so OR's flags are
#    never observed. Replaced by empty-list-not-ended (OR's flags without its
#    value), which the next packet sees in the register and CMP's flags.
DEFINITIONS=(
 ('skip-machine-filter','src/rendering/native_tu15_cluster_contents_systems.hpp','admit=memory.load8(object_type,at(object,offsetof(ObjectData,type)))==machine_type;','admit=memory.load8(object_type,at(object,offsetof(ObjectData,type)))!=0xffu;'),
 ('reuse-pool-after-admission','src/rendering/native_tu15_cluster_contents_systems.hpp','            k.pool=memory.load32(collideable_pool_again,at(globals::collideable,offsetof(ClusterPartition,entries)));\n','            memory.load32(collideable_pool_again,at(globals::collideable,offsetof(ClusterPartition,entries)));\n'),
 ('owner-before-next','src/rendering/native_tu15_cluster_contents_systems.hpp','    const auto next=memory.load32(walk_access(walk,LinkField::next),at(datum,offsetof(ClusterEntry,next)));\n    const auto owner=memory.load32(walk_access(walk,LinkField::owner),at(datum,offsetof(ClusterEntry,owner)));','    const auto owner=memory.load32(walk_access(walk,LinkField::owner),at(datum,offsetof(ClusterEntry,owner)));\n    const auto next=memory.load32(walk_access(walk,LinkField::next),at(datum,offsetof(ClusterEntry,next)));'),
 ('lights-for-light-views','src/rendering/native_tu15_cluster_contents_systems.hpp','    if(memory.load32(light_kind,at(view,offsetof(View,kind)))!=0)return;','    memory.load32(light_kind,at(view,offsetof(View,kind)));'),
 ('sky-signed-index','src/rendering/native_tu15_cluster_contents_systems.hpp','    k.sky_offset=widen16(sky)*4u;','    k.sky_offset=(sky&0xffffu)*4u;'),
 ('queue-fills-last-slot','src/rendering/native_tu15_cluster_contents_systems.hpp','    if(signed_value(count)>=signed_value(last))return frame.end_append(false);','    if(signed_value(count)>signed_value(last))return frame.end_append(false);'),
 ('kept-fog-from-logic','src/rendering/native_tu15_cluster_contents.hpp','            k.next=r(edi);k.inside_fog=r(ebp);\n            break;','            k.next=r(edi);\n            break;'),
 ('kept-next-ignored','src/rendering/native_tu15_cluster_contents.hpp','        k.next=r(edi);k.view=r(esi);k.position=r(ebp);','        k.view=r(esi);k.position=r(ebp);'),
 ('software-sky-shift','src/rendering/native_tu15_cluster_contents.hpp','r(edx)=v;const auto offset=sky_head_offset(widen(r(eax)),f.bits());r(esi)=offset.value;f.load(offset.flags);','r(edx)=v;r(esi)=f.shift(widen(r(eax)),2u,4);'),
 ('unseeded-light-imul','src/rendering/native_tu15_cluster_contents.hpp','light_datum_offset(r(ecx),f.bits())','light_datum_offset(r(ecx),0u)'),
 ('empty-list-not-ended','src/rendering/native_tu15_cluster_contents.hpp','        if(!walked_)r(site.owner_register)=s_.flags.logic(r(site.owner_register)|0xffffffffu,4);','        if(!walked_)s_.flags.logic(r(site.owner_register)|0xffffffffu,4);'),
 ('uncertified-required-owner','src/rendering/native_tu15_cluster_contents.hpp','    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','    if(!s.services.required_owner)return false;'),
 ('census-without-switch','src/rendering/native_tu15_cluster_contents.hpp','    if(native_census_detail::uncursored(s,0x134d30u,0x166bafu)){','    if(!s.instruction_cursor&&s.services.instruction){'),
)
GUARD_MUTANTS=('uncertified-required-owner','census-without-switch')
def mutants(scratch,command,rows,metadata):
    outcomes=[]
    for name,path,old,new in DEFINITIONS:
        text=(ROOT/path).read_text()
        if text.count(old)!=1:raise ValueError('Unique cluster contents mutant witness missing '+name)
        overlay=scratch/('mutant-'+name);target=overlay/Path(path).relative_to(Path(path).parts[0]);target.parent.mkdir(parents=True);target.write_text(text.replace(old,new,1))
        binary=overlay/'tests';mutated=list(command);mutated[mutated.index('-o')+1]=str(binary);mutated.insert(1,'-I'+str(overlay))
        if name not in GUARD_MUTANTS:mutated.insert(1,'-DHALO2_CLUSTER_CONTENTS_MUTATION_CORE_ONLY=1')
        compiled=subprocess.run(mutated,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Cluster contents mutant did not compile '+name+': '+compiled.stderr[-4000:])
        tests=['--guards']if name in GUARD_MUTANTS else[]
        if not tests:
            ordered=[row for row in rows if'fault'not in row]+[row for row in rows if'fault'in row]
            for index in range(0,len(ordered),64):
                fixture=overlay/(str(index//64)+'.json');fixture.write_text(json.dumps(dict(metadata,cases=ordered[index:index+64]),separators=(',',':')));tests.append(str(fixture))
        rejected=False;diagnostic=''
        for test in tests:
            done=subprocess.run([str(binary),test],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
            if done.returncode:rejected=True;diagnostic=(done.stdout+done.stderr)[-1500:];break
        outcomes.append(dict(name=name,rejected=rejected,diagnostic=diagnostic))
    survivors=[item['name']for item in outcomes if not item['rejected']]
    if survivors:raise ValueError('Compiled cluster contents mutants survived '+', '.join(survivors))
    return outcomes
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--require-binding',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');parser.add_argument('--limit',type=int);parser.add_argument('--families',type=str);args=parser.parse_args()
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS};source=prepare(scratch,args.require_binding)
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);assert hashlib.sha256(image.data).hexdigest()==PIN
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    original_metadata=common.metadata
    def metadata(image):
        result=original_metadata(image);code=common._Code(image,'.text')
        for site in result['sites']:
            mnemonic=code.at(site['instruction']).mnemonic
            if mnemonic in('ret','pop'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            if mnemonic.startswith('prefetch'):site['operands']=[]
        return result
    common.metadata=metadata;common.ENTRIES=ENTRIES
    sites=metadata(image)['sites'];code=common._Code(image,'.text')
    io=sorted({site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic in('push','call')})
    write_sites=[site['instruction']for site in sites if any(o['write']for o in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')]
    rmw_sites=[site['instruction']for site in sites if code.at(site['instruction']).mnemonic not in('push','call')and any(o['read']for o in site['operands'])and any(o['write']for o in site['operands'])]
    if 0x166ba6 not in rmw_sites:raise ValueError('Queue count RMW site metadata changed')
    deltas={0x135b46:1,0x135a87:1,0x135a89:1,0x135a8f:2,0x135a92:2,0x135a98:3,0x135ab3:1,0x135ab7:1}
    calls=[dict(entry=pc,gp=True,xmm=True,x87_delta=deltas.get(pc,0))for pc in io]
    rows=corpus(image)
    if args.families:
        chosen={int(item)for item in args.families.split(',')};rows=[row for row in rows if int(row['name'].split('-')[0][6:])in chosen]
    if args.limit:rows=rows[:args.limit]
    normal,authority,rounds,instructions=segmented(image,rows,scratch,calls,'normal')
    prefix_authority=None;witnesses=set();prefixes=[];prefix_state=[];prefix_rounds=0
    def flatten(item):
        return dict(reads=[r for segment in item['segments']for r in segment['reads']],writes=[w for segment in item['segments']for w in segment['writes']],calls=[c for segment in item['segments']for c in segment['calls']])
    if args.prefixes:
        for row,item in zip(rows,normal,strict=True):
            depth=len(row['live_x87']);family=int(row['name'].split('-')[0][6:])
            if depth not in(0,3):continue
            domain=family if family in(13,14,17)else'ordinary'
            reads=writes=0
            for number,(segment,case) in enumerate(zip(item['segments'],item['inputs'],strict=True)):
                for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(segment[key]):
                        pc=event['instruction']
                        if not any(start<=pc<end for start,end in OWNED.items()):continue
                        actual='before_instruction'if phase=='before_write'and pc in rmw_sites else phase
                        witness=(actual,pc,depth,domain)
                        if witness in witnesses:continue
                        witnesses.add(witness)
                        if actual=='before_instruction':
                            prior=sum(1 for earlier in item['segments'][:number]for c in earlier['calls']if c['entry']==pc)
                            step=sum(1 for earlier in segment[key][:ordinal]if earlier['instruction']==pc)
                            fault=dict(phase=actual,instruction=pc,ordinal=prior+step);segment_fault=dict(phase=actual,instruction=pc,ordinal=step)
                        else:
                            base=reads if phase=='before_read'else writes
                            fault=dict(phase=actual,ordinal=base+ordinal);segment_fault=dict(phase=actual,ordinal=ordinal)
                        prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'-{actual}-{number}-{ordinal}',fault=fault,segment_fault=segment_fault,segment_number=number,segment_input=copy.deepcopy(case),parent=row['name'])
                        prefixes.append(prefix)
                reads+=len(segment['reads']);writes+=len(segment['writes'])
        prefix_state,prefix_authority,prefix_rounds,prefix_instructions=segmented(image,prefixes,scratch,calls,'prefix');instructions+=prefix_instructions
    cases=[];by_name={row['name']:index for index,row in enumerate(rows)}
    for row,item in list(zip(rows,normal,strict=True))+list(zip(prefixes,prefix_state,strict=True)):
        if 'fault' in row:
            # Earlier segments come from the normal run of the same row.
            parent=normal[by_name[row['parent']]];number=row['segment_number']
            item=dict(segments=parent['segments'][:number]+item['segments'],stops=parent['stops'][:number],inputs=parent['inputs'][:number]+item['inputs'])
        final=item['segments'][-1];flat=flatten(item);fault=row.get('fault',{})
        expected=dict(row);[expected.pop(key,None)for key in('segment_input','segment_fault','segment_number','parent')]
        expected.update(boundary_gp=final['boundary_gp'],boundary_eflags=final['boundary_eflags'],boundary_xmm=final['boundary_xmm'],after_fp=final['after_fp'],reads=flat['reads'],writes=flat['writes'],after_live_x87=final['boundary_live_x87'],callee_calls=item['stops'])
        expected['expected_pc']=final['boundary']['instruction']if fault else flat['calls'][-1]['entry'];expected['calls']=[]
        for call_index,call in enumerate(flat['calls']):
            call=dict(call);assert call.pop('arguments',[])==[]
            last=call_index+1==len(flat['calls'])
            if fault.get('phase')=='before_instruction'and last:continue
            expected['calls'].append(call)
            if call['entry']in rmw_sites and not(fault.get('phase')=='before_read'and last):expected['calls'].append(call.copy())
        expected['banks']=[dict(address=bank['address'],before=bank['before'],after=after['after'])for bank,after in zip(row['banks'],final['banks'],strict=True)]
        cases.append(expected)
    executed={pc for item in normal for segment in item['segments']for pc in segment['executed_sites']}
    owned_sites={site['instruction']for site in sites if any(start<=site['instruction']<end for start,end in OWNED.items())}
    unexecuted=sorted(owned_sites-executed-set(UNREACHABLE))
    binary=scratch/'cluster-contents-tests';command=[args.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(ROOT/'tests/native_tu15_cluster_contents_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Cluster contents compile failed: '+compiled.stderr[-6000:])
    meta=dict(write_sites=write_sites,rmw_sites=rmw_sites);fixture=scratch/'fixture.json'
    def run_all(selected,env):
        outcomes=[];done=None
        for index in range(0,len(selected),64):
            fixture.write_text(json.dumps(dict(meta,cases=selected[index:index+64]),separators=(',',':')))
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env))
            if done.returncode:return False,(done.stdout+done.stderr)[-12000:],outcomes
            outcomes.append(json.loads(done.stdout))
        return True,'',outcomes
    passed,diagnostic,outcomes=run_all(cases,{})
    checks=[]
    if passed:
        for option,env in(('--guards',{}),('--reference-guards',{'HALO2_REFERENCE_CLUSTER_CONTENTS':'1'})):
            done=subprocess.run([str(binary),option],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env));checks.append(dict(option=option,passed=done.returncode==0,diagnostic=(done.stdout+done.stderr)[-4000:]))
        ok,text,forced=run_all([row for row in cases if'fault'not in row],{'HALO2_REFERENCE_CLUSTER_CONTENTS':'1','HALO2_CLUSTER_CONTENTS_REFERENCE_TEST':'1'})
        checks.append(dict(option='positive_forced_reference',passed=ok,diagnostic=text,counts={key:sum(item[key]for item in forced)for key in forced[0]if key!='passed'}if forced else{}))
        done=subprocess.run([str(binary),'--census-guards'],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));checks.append(dict(option='--census-guards',passed=done.returncode==0,diagnostic=(done.stdout+done.stderr)[-4000:]))
        ok,text,census=run_all([row for row in cases if'fault'not in row],{'HALO2_CLUSTER_CONTENTS_CENSUS_TEST':'1'})
        checks.append(dict(option='census_route',passed=ok and bool(census)and sum(item['census_route_comparisons']for item in census)==sum('fault'not in row for row in cases),diagnostic=text,counts={key:sum(item[key]for item in census)for key in census[0]if key!='passed'}if census else{}))
    mutated=mutants(scratch,command,cases,meta)if args.semantic_mutants and passed and all(item['passed']for item in checks)else[]
    changed=[name for name,pin in pins.items()if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=pin]
    stubbed={hex(entry):sum(1 for row in cases if'fault'not in row for call in row['callee_calls']if call['entry']==entry)for entry in ORDER}
    report=dict(passed=passed and not changed and all(item['passed']for item in checks)and not unexecuted,private_unbound=not args.require_binding,owned_scratch=str(scratch),source_sha256=pins,changed_inputs=changed,source_authority=source,physical_cases=len(cases),normal_cases=sum('fault'not in row for row in cases),prefix_cases=sum('fault'in row for row in cases),prefix_witnesses=len(witnesses),
        segment_rounds=rounds,prefix_segment_rounds=prefix_rounds,stubbed_required_calls=stubbed,original_instructions=instructions,
        owned_sites=len(owned_sites),declared_unreachable=[hex(pc)for pc in UNREACHABLE],unexecuted_owned_sites=[hex(pc)for pc in unexecuted],physical_original=authority,physical_prefix_original=prefix_authority,additional_source_checks=checks,compiled_semantic_mutants=mutated,compiler=args.compiler,sanitizers=args.sanitizers,diagnostic=diagnostic,retained_large_artifacts=[])
    if passed:report['native']={key:sum(item[key]for item in outcomes)for key in outcomes[0]if key!='passed'}
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','physical_cases','normal_cases','prefix_cases','stubbed_required_calls','owned_sites','unexecuted_owned_sites','changed_inputs')}));print(diagnostic[-3000:])
    return 0 if report['passed']else 1
if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            report=Path(sys.argv[sys.argv.index('--report')+1]);report.parent.mkdir(parents=True,exist_ok=True);report.write_text(json.dumps(dict(passed=False,owned_scratch=os.environ.get('HALO2_TEST_SCRATCH'),error=str(error)[-10000:],retained_large_artifacts=[]),indent=2)+'\n')
        raise
