#!/usr/bin/env python3
"""Independent original outer pickup visibility/location/contact correction trace."""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_mp_weapon_attachment_1_1 import AttachmentTrace,authored_fields
from oracle_mp_inventory_equipped_1_1 import EquippedPickupTrace
from oracle_mp_inventory_spatial_1_1 import cache_record
from oracle_mp_inventory_1_1 import (ROOT,PIN,CACHE_PIN,UNIT,UI,HEADERS,SCRATCH,TIME,SEED,STOP,_trace,
    UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,
    UC_X86_REG_EDI,UC_X86_REG_ESP)
from unicorn.x86_const import UC_X86_REG_XMM0

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/pickup-visibility'
ENTRIES=(0xc57a0,0xe0c20,0xfbfa0,0xbacc0,0x16bac0,0x16b240,0x16a560,
    0x14b9d0,0x11d4a0,0x1e2020,0x1e20c0,0x1e23f0,0x29c7d0,0x1e25c0,0x14ba50,
    0xb9c20,0x1dc8f0,0x1dc940,0xbbf40,0x14bd00,0x14bac0,0xb9830,0xb9900,
    0xf9b10,0xf9ea0,0xbdf10,0xbdf80,0xbede0,0xb8320,0xb8040,0xbd360,0xbf7d0)
CORRECTION_SITES=(0x16b89e,0x16b8eb,0x16b920,0x16b96b,0x16b9b1,
    0x16b9d4,0x16ba1f,0x16ba27,0x16ba30,0x16ba85,0x16baa6)

class VisibilityTrace(AttachmentTrace):
    def __init__(self):
        self.visibility_calls=[];self.visibility_returns=[];self.outer_queries=[];self.active_segments=[]
        self.contact_writes=[]
        super().__init__(0,False)
        self.world_datums={0:self.target_datum}
        for row in (1,12):
            self.w(TIME+8,row)
            self.action('original_authored_additional_allocation',0x15c110,(self.entries,row))
            self.world_datums[row]=self.u(self.entries+row*8)
        self.initial_world={str(row):self.object_state(datum)for row,datum in self.world_datums.items()}
        self.initial_registry=self.registry()

    def finish_visibility_returns(self,a):
        for ret,row,kind,values in self.visibility_returns[:]:
            if ret!=a:continue
            row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
            if kind=='outer':
                out=values[0];row['result_hex']=bytes(self.g.mem_read(out,0x5c)).hex()
                row['collision_type']=self.u(out);row['fraction_bits']=f'{self.u(out+4):08x}'
                row['point']=self.vec(out+8)
                row['initial_location_hex']=bytes(self.g.mem_read(out+0x14,8)).hex()
                row['final_location_hex']=bytes(self.g.mem_read(out+0x1c,8)).hex()
                if self.u(out)==1:
                    collision=self.u(0x4e790c);bsp=self.u(0x4e7918)
                    surface=self.u(out+0x50);plane=self.u(out+0x54);material=self.u(out+0x5a,2)
                    row['contact_cache']=dict(surface=surface,plane=f'{plane:08x}',material=material,
                        surface_record=cache_record(self,self.u(collision+0x2c)+surface*8,8),
                        plane_record=cache_record(self,self.u(collision+0xc)+(plane&0x7fff)*16,16),
                        material_record=cache_record(self,self.u(bsp+0x10)+material*0x14,0x14))
                assert self.active_segments.pop()is row
            elif kind=='lower':
                descriptor,out=values;row['descriptor_after_hex']=bytes(self.g.mem_read(descriptor,0x28)).hex()
                row['result_hex']=bytes(self.g.mem_read(out,0x420)).hex()
                row['visited_leaves']=[self.u(out+0x1c+n*4)for n in range(min(self.u(out+0x18),256))]
            elif kind=='capsule':
                bottom,extent,radius=values
                row.update(bottom=self.vec(bottom),extent_bits=f'{self.u(extent):08x}',radius_bits=f'{self.u(radius):08x}')
            elif kind=='point':row['point']=self.vec(values[0])
            elif kind=='bytes':row['output_hex']=bytes(self.g.mem_read(values[0],values[1])).hex()
            elif kind=='visibility':
                row['visible']=bool(self.g.reg_read(UC_X86_REG_EAX)&255)
                row['query_range'][1]=len(self.outer_queries)
            elif kind=='clusters':
                count=self.g.reg_read(UC_X86_REG_EAX)&0xffff
                row['returned_clusters']=[self.u(values[0]+n*2,2)for n in range(min(count,values[1]))]
            elif kind=='enumeration':
                count=self.g.reg_read(UC_X86_REG_EAX)&0xffff
                row['returned_datums']=[f'{self.u(values[0]+n*4):08x}'for n in range(min(count,values[1]))]
            self.visibility_returns.remove((ret,row,kind,values))

    def run(self,a,argv=(),regs=()):
        super().run(a,argv,regs);self.finish_visibility_returns(STOP)

    def hook(self,g,a,size,user):
        if hasattr(self,'services'):
            for service in (0xfb560,0x1dc8f0,0x1dc940):self.services.pop(service,None)
        self.finish_visibility_returns(a)
        if hasattr(self,'source_calls'):
            sp=g.reg_read(UC_X86_REG_ESP)
            if a==0x16b240:
                row=dict(phase=self.active,caller=f'{self.u(sp):08x}',mask=f'{self.u(sp+4):08x}',
                    start=self.vec(self.u(sp+8)),delta=self.vec(self.u(sp+12)),
                    ignored=[f'{self.u(sp+16):08x}',f'{self.u(sp+20):08x}'],
                    output_pointer=f'{self.u(sp+24):08x}',correction_steps=[])
                self.outer_queries.append(row);self.active_segments.append(row)
                self.visibility_returns.append((self.u(sp),row,'outer',(self.u(sp+24),)))
            if a==0x1e2020 and self.active_segments:
                self.active_segments[-1]['lower_wrapper']=dict(flags=f'{g.reg_read(UC_X86_REG_ECX):08x}',
                    bsp=f'{g.reg_read(UC_X86_REG_EDX):08x}',breakable_count=self.u(sp+4,2),
                    breakable_pointer=f'{self.u(sp+8):08x}',initial_best_fraction=f'{g.reg_read(UC_X86_REG_XMM0)&0xffffffff:08x}')
            if a==0x1e20c0 and self.u(sp)==0x1e20ad:
                d=self.u(sp+4);out=self.u(d+0x18)
                lower=dict(descriptor_before_hex=bytes(g.mem_read(d,0x28)).hex(),
                    root=f'{self.u(sp+8):08x}',range_bits=[f'{self.u(sp+12):08x}',f'{self.u(sp+16):08x}'],
                    start=self.vec(self.u(d+0x10)),delta=self.vec(self.u(d+0x14)))
                self.active_segments[-1]['lower']=lower
                self.visibility_returns.append((self.u(sp),lower,'lower',(d,out)))
            if a in CORRECTION_SITES and self.active_segments:
                q=self.active_segments[-1];p=int(q['output_pointer'],16)
                q['correction_steps'].append(dict(instruction=f'{a:08x}',fraction_bits=f'{self.u(p+4):08x}',
                    point=self.vec(p+8),initial_location_hex=bytes(g.mem_read(p+0x14,8)).hex(),
                    final_location_hex=bytes(g.mem_read(p+0x1c,8)).hex(),registers=self.registers()))
            row=None;kind='';values=()
            if a in ENTRIES:
                row=dict(phase=self.active,entry=f'{a:08x}',caller=f'{self.u(sp):08x}',
                    registers=self.registers(),stack=[f'{self.u(sp+4+n*4):08x}'for n in range(6)])
                if a==0xc57a0:
                    row.update(unit=f'{g.reg_read(UC_X86_REG_EDI):08x}',target=f'{g.reg_read(UC_X86_REG_ESI):08x}',
                        query_range=[len(self.outer_queries),None]);kind='visibility'
                elif a==0xe0c20:
                    kind='capsule';values=(g.reg_read(UC_X86_REG_ECX),self.u(sp+4),self.u(sp+8))
                elif a==0xfbfa0:
                    target=g.reg_read(UC_X86_REG_ECX);row['object_before']=self.object_state(target)
                    kind='point';values=(g.reg_read(UC_X86_REG_EAX),)
                elif a==0xbacc0:kind='point';values=(g.reg_read(UC_X86_REG_ESI),)
                elif a==0x16bac0:
                    row.update(start=self.vec(g.reg_read(UC_X86_REG_EAX)),end=self.vec(g.reg_read(UC_X86_REG_ECX)))
                elif a==0x14b9d0:
                    row.update(root=f'{g.reg_read(UC_X86_REG_EAX):08x}',bsp=f'{g.reg_read(UC_X86_REG_ECX):08x}',
                        point=self.vec(g.reg_read(UC_X86_REG_EDX)))
                elif a==0x11d4a0:
                    row['point']=self.vec(g.reg_read(UC_X86_REG_EDX))
                    kind='bytes';values=(g.reg_read(UC_X86_REG_ESI),8)
                elif a==0x14ba50:
                    collision=self.u(sp+4);plane=g.reg_read(UC_X86_REG_EDX)&0xffff
                    row.update(collision_bsp=f'{collision:08x}',signed_plane=f'{plane:04x}',
                        plane_record=cache_record(self,self.u(collision+0xc)+(plane&0x7fff)*16,16))
                    kind='bytes';values=(g.reg_read(UC_X86_REG_EAX),16)
                elif a==0xb9c20:
                    row.update(object=f'{self.u(sp+4):08x}',marker=f'{self.u(sp+8):08x}')
                    kind='bytes';values=(g.reg_read(UC_X86_REG_EBX),0x70)
                elif a==0x1dc940:
                    row.update(mode=f'{g.reg_read(UC_X86_REG_ECX):08x}',group=f'{g.reg_read(UC_X86_REG_EAX):08x}')
                    kind='bytes';values=(self.u(sp+20),0x70)
                elif a==0x14bd00:
                    row.update(start_cluster=self.u(sp+4,2),center=self.vec(self.u(sp+8)),radius_bits=f'{self.u(sp+12):08x}')
                    kind='clusters';values=(self.u(sp+20),self.u(sp+16,2))
                elif a==0xbbf40:
                    row.update(registry_mask=f'{self.u(sp+4):08x}',type_mask=f'{self.u(sp+8):08x}',
                        location_hex=bytes(g.mem_read(self.u(sp+12),8)).hex(),center=self.vec(self.u(sp+16)),
                        radius_bits=f'{self.u(sp+20):08x}')
                    kind='enumeration';values=(self.u(sp+24),self.u(sp+28,2))
                elif a in (0xb9830,0xb9900):
                    cluster=self.u(sp+4,2);row['cluster']=cluster;row['stored_order']=self.cluster_chain(cluster,a==0xb9830)
                self.visibility_calls.append(row)
                self.visibility_returns.append((self.u(sp),row,kind,values))
        # Use the frozen equipped fixture's original call/resource hooks; this
        # independent suite deliberately does not duplicate attachment logs.
        EquippedPickupTrace.hook(self,g,a,size,user)

    def mem_write(self,g,access,a,size,value,user):
        if hasattr(self,'world_datums'):
            from oracle_mp_inventory_1_1 import UC_X86_REG_EIP
            for datum in self.world_datums.values():
                p=self.u(HEADERS+(datum&0xffff)*12+8)
                if p+0x12c<=a<p+0x140:
                    self.contact_writes.append(dict(phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
                        datum=f'{datum:08x}',offset=f'{a-p:03x}',size=size,before=bytes(g.mem_read(a,size)).hex(),value=f'{value:0{size*2}x}'))
        EquippedPickupTrace.mem_write(self,g,access,a,size,value,user)

    def pose(self,index=0,position=None):
        start=self.cache.block(self.cache.scenario+0x100,0x34)[index]
        root=start+self.cache.magic
        if position is not None:
            root=SCRATCH+0x780;self.g.mem_write(root,struct.pack('<3f',*position))
        self.g.mem_write(SCRATCH+0x700,struct.pack('<3f',self.cache.f(start+12),0,0))
        self.action('original_authored_facing',0x11f680,registers=((UC_X86_REG_EAX,SCRATCH+0x700),
            (UC_X86_REG_EDX,UNIT+0x70),(UC_X86_REG_ECX,UNIT+0x7c)))
        self.action('original_world_root_pose_store',0xb8320,(UI,root,UNIT+0x70,UNIT+0x7c,0,1,1,0,0))
        self.action('original_animated_unit_pose_bounds',0xbdf10,(UI,))
        self.action('original_unit_location',0x11d4a0,registers=((UC_X86_REG_EDX,UNIT+0x30),(UC_X86_REG_ESI,UNIT+0x28)))
        return dict(authored_start=index,authored_record=cache_record(self,start+self.cache.magic,0x34),
            supplied_proximity_root=position,source_root=self.vec(UNIT+0x64),bound_center=self.vec(UNIT+0x30),
            bound_radius_bits=f'{self.u(UNIT+0x3c):08x}',location_hex=bytes(self.g.mem_read(UNIT+0x28,8)).hex(),
            node_matrices_hex=bytes(self.g.mem_read(UNIT+self.u(UNIT+0x116,2),33*0x34)).hex(),manager=self.manager_state())

    def visibility(self,datum,name):
        self.action(name,0xc57a0,registers=((UC_X86_REG_EDI,UI),(UC_X86_REG_ESI,datum)))
        return next(c for c in reversed(self.visibility_calls)if c['entry']=='000c57a0')

    def candidate(self,name):
        self.action(name,0x1527c0,(0,SCRATCH+0xa00))
        return dict(raw_hex=bytes(self.g.mem_read(SCRATCH+0xa00,12)).hex(),
            selected=f'{self.u(SCRATCH+0xa04):08x}')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');args=p.parse_args()
    args.output=args.output.resolve();t=VisibilityTrace();authored=[];near=[]
    for index in range(len(t.cache.block(t.cache.scenario+0x100,0x34))):
        pose=t.pose(index)
        authored.append(dict(pose=pose,candidate=t.candidate('original_authored_start_candidate_scan'),
            visibility={str(row):t.visibility(datum,'original_authored_start_visibility')for row,datum in t.world_datums.items()}))
    t.pose(0)
    t.action('original_broad_unsorted_query',0xbbf40,(0,4,UNIT+0x28,UNIT+0x30,0x42c80000,SCRATCH+0xb00,64))
    broad=next(c for c in reversed(t.visibility_calls)if c['entry']=='000bbf40')
    for row,datum in t.world_datums.items():
        ptr=t.u(HEADERS+(datum&0xffff)*12+8)
        position=[t.fv(ptr+0x30+n*4)+(0.05 if n==0 else 0)for n in range(3)]
        near.append(dict(row=row,datum=f'{datum:08x}',pose=t.pose(0,position),
            candidate=t.candidate('original_nearby_candidate_scan'),visibility=t.visibility(datum,'original_nearby_visibility')))
    datum=t.world_datums[12]
    contact_before=t.object_state(datum);event_begin=len(t.events);query_begin=len(t.outer_queries)
    t.w(TIME+8,12);t.action('original_new_object_postpass_first_item_ticks',0xb8040)
    contact_after=t.object_state(datum)
    t.action('original_contact_smg_graph_pose_bounds',0xbdf10,(datum,))
    t.action('original_contact_interaction_point',0xfbfa0,registers=((UC_X86_REG_ECX,datum),(UC_X86_REG_EAX,SCRATCH+0x900)))
    contact_point=t.vec(SCRATCH+0x900)
    ptr=t.u(HEADERS+(datum&0xffff)*12+8)
    position=[t.fv(ptr+0x30+n*4)+(0.05 if n==0 else 0)for n in range(3)]
    contact=dict(before=contact_before,after=contact_after,after_pose=t.object_state(datum),point=contact_point,
        event_range=[event_begin,len(t.events)],query_range=[query_begin,len(t.outer_queries)],
        pose=t.pose(0,position),candidate=t.candidate('original_contact_item_candidate_scan'),
        visibility=t.visibility(datum,'original_contact_item_visibility'))
    # Independent clipping-mask probes reuse exact original visibility-emitted
    # origin/delta bit patterns. They are not claimed as C57A0-emitted masks.
    probes=[]
    source_segments=[(index,q)for index,q in enumerate(t.outer_queries)
        if q['mask']=='14800001'and q['collision_type']==1][:256]
    for index,q in source_segments:
        for at,field in ((SCRATCH+0xc00,'start'),(SCRATCH+0xc10,'delta')):
            t.g.mem_write(at,b''.join(struct.pack('<I',int(b,16))for b in q[field]['bits']))
        begin=len(t.outer_queries)
        t.action('original_outer_clipping_mask_replay_of_emitted_segment',0x16b240,
            (0x24800001,SCRATCH+0xc00,SCRATCH+0xc10,int(q['ignored'][0],16),int(q['ignored'][1],16),SCRATCH+0xc30))
        probes.append(dict(original_query_index=index,replay_query_index=begin))
    assert not t.visibility_returns and not t.active_segments
    assert t.u(SEED)==0x12345678
    directory=args.output.parent/'instructions';directory.mkdir(parents=True,exist_ok=True);instructions=[]
    for entry in ENTRIES:
        routine=_trace(t.code,entry,t.starts|set(ENTRIES));code=[t.code.at(a)for a in sorted(routine.code)]
        path=directory/f'{entry:08x}.asm';path.write_text('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'for i in code)+'\n')
        instructions.append(dict(entry=f'{entry:08x}',path=str(path.relative_to(ROOT)),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
            instructions=len(code),executed=sum(i.address in t.executed for i in code)))
    dependencies=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json').read_text())['dependencies']
    dependencies=[dict(path='tools/oracle_mp_pickup_visibility_1_1.py',sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())]+dependencies
    report=dict(xbe_sha256=PIN,cache_sha256=CACHE_PIN,shared_cache_sha256=t.equipped_metadata['shared_cache_sha256'],dependencies=dependencies,
        initial=t.initial_equipped,initial_world=t.initial_world,initial_registry=t.initial_registry,authored_fields=authored_fields(t),
        authored=authored,broad_query=broad,near=near,conditional_source_contact=contact,clipping_mask_replays=probes,
        outer_queries=t.outer_queries,calls=t.visibility_calls,contact_writes=t.contact_writes,rows=t.inventory_rows,
        events=[{k:v for k,v in e.items()if k!='state'}for e in t.events],live_events=t.live_events,instructions=instructions,
        rng=f'{t.u(SEED):08x}',scope='Original equipped poses, unsorted registry, outer terrain pickup visibility and conditional original item contact correction. Authored/proximity root inputs, heap/residency and unavailable Havok service remain explicit; no complete settling or Xbox reciprocal claim.')
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(authored=len(authored),outer_queries=len(t.outer_queries),calls=len(t.visibility_calls),
        contact_flags=contact_after['item_flags'],report_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())))

if __name__=='__main__':main()
