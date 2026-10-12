#!/usr/bin/env python3
"""Separate original carried weapon parent/marker attachment research."""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_mp_inventory_equipped_1_1 import EquippedPickupTrace
from oracle_mp_inventory_equipped_1_1 import continue_graph
from oracle_mp_inventory_1_1 import (
    ROOT,PIN,CACHE_PIN,UNIT,UI,POOL,HEADERS,STOP,SCRATCH,TIME,SEED,
    UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP)
from oracle_mp_inventory_1_1 import _trace

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/weapon-attachment'
ENTRIES=(0xfb560,0xfb490,0xb9dd0,0xb9c20,0xba0c0,0xba1c0,0xba2a0,
    0xba980,0xba780,0xba7d0,0xbf540,0xbacc0,0xb8320,0xb8490,0xbdf10,0xbdf80,
    0x1dc8f0,0x1dc940,0x143580,0x143860,0x142d00,0x1441d0,
    0xd1d20,0xfb3f0,0xfb120,0xcfd80,0xcfed0)

class AttachmentTrace(EquippedPickupTrace):
    def __init__(self,row=1,near=True):
        self.attachment_calls=[];self.attachment_returns=[];self.attachment_writes=[]
        self.marker_metadata={};self.reciprocal_sites=set()
        super().__init__(row,near)

    def object_state(self,datum):
        result=super().object_state(datum);p=int(result['address'],16)
        result.update(sibling=f'{self.u(p+0xc):08x}',first_child=f'{self.u(p+0x10):08x}',
            parent_node=self.u(p+0x18,1),attachment_flags=f'{self.u(p+0x19,1):02x}',
            node_matrix_hex=bytes(self.g.mem_read(p+self.u(p+0x116,2),self.u(p+0x114,2))).hex())
        return result

    def attachment_state(self):
        return dict(unit=dict(first_child=f'{self.u(UNIT+0x10):08x}',flags=f'{self.u(UNIT+4):08x}',
            root=self.vec(UNIT+0x64),current=list(self.g.mem_read(UNIT+0x212,2)),requested=list(self.g.mem_read(UNIT+0x216,2)),
            current_node_matrices_hex=bytes(self.g.mem_read(UNIT+self.u(UNIT+0x116,2),33*0x34)).hex()),
            items=[self.object_state(self.u(HEADERS+n*12,2)<<16|n)
                for n in range(2,self.u(POOL+0x38))if self.u(HEADERS+n*12+3,1)==2 and self.u(HEADERS+n*12+8)])

    def complete_attachment_returns(self,a):
        for ret,row in self.attachment_returns[:]:
            if ret==a:
                row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
                row['after']=self.attachment_state()
                if '_out'in row:
                    out,length=row.pop('_out');row['output_hex']=bytes(self.g.mem_read(out,length)).hex()
                self.attachment_returns.remove((ret,row))

    def run(self,a,argv=(),regs=()):
        super().run(a,argv,regs);self.complete_attachment_returns(STOP)

    def hook(self,g,a,size,user):
        if hasattr(self,'services'):
            for service in (0xfb560,0x1dc8f0,0x1dc940):self.services.pop(service,None)
        self.complete_attachment_returns(a)
        if hasattr(self,'source_calls')and a in ENTRIES:
            sp=g.reg_read(UC_X86_REG_ESP)
            row=dict(phase=self.active,entry=f'{a:08x}',registers=self.registers(),
                stack=[f'{self.u(sp+4+n*4):08x}'for n in range(7)],before=self.attachment_state())
            if a==0x1dc8f0:
                row['render_model']=f'{g.reg_read(UC_X86_REG_ECX):08x}'
                row['marker_string_id']=f'{g.reg_read(UC_X86_REG_EDI):08x}'
            if a==0x1dc940:
                group=g.reg_read(UC_X86_REG_EAX);model=g.reg_read(UC_X86_REG_ECX)
                row.update(render_model=f'{model:08x}',group_index=f'{group:08x}',
                    region_pointer=f'{self.u(sp+4):08x}',node_mapping_pointer=f'{self.u(sp+8):08x}',
                    node_matrices_pointer=f'{self.u(sp+12):08x}',mirror=self.u(sp+16,1),maximum=self.u(sp+24))
                row['_out']=(self.u(sp+20),self.u(sp+24)*0x70)
                if group!=0xffffffff:
                    tag=self.cache.tags[model];base=tag['offset']+self.cache.magic
                    gp=self.u(base+0x5c)+group*12
                    markers=[self.u(gp+8)+n*0x24 for n in range(self.u(gp+4))]
                    row['marker_records']=[self.cache_record(p,0x24)for p in markers]
                    row['source_node_matrices']=[dict(node=self.u(p+2,1),
                        matrix_hex=bytes(g.mem_read(self.u(sp+12)+self.u(p+2,1)*0x34,0x34)).hex())for p in markers]
                    self.marker_metadata[f'{model:08x}:{group}']=dict(tag=tag,group=group,
                        group_record=self.cache_record(gp,12),markers=row['marker_records'],
                        nodes=[self.cache_record(self.u(base+0x4c)+self.u(p+2,1)*0x60,0x60)for p in markers])
            if a==0xb9c20:
                row.update(object=f'{self.u(sp+4):08x}',marker=f'{self.u(sp+8):08x}',
                    maximum=self.u(sp+12,2),direct_object=bool(g.reg_read(UC_X86_REG_ECX)&255))
                row['_out']=(g.reg_read(UC_X86_REG_EBX),self.u(sp+12,2)*0x70)
            if a==0xba1c0:
                row['parent_marker_hex']=bytes(g.mem_read(self.u(sp+4),0x70)).hex()
                row['child_marker_hex']=bytes(g.mem_read(self.u(sp+8),0x70)).hex()
            if a==0xbacc0:row['_out']=(g.reg_read(UC_X86_REG_ESI),12)
            self.attachment_calls.append(row);self.attachment_returns.append((self.u(sp),row))
        if hasattr(self,'source_calls'):
            ins=self.code.at(a)
            if ins is not None and ins.mnemonic in('rcpss','rsqrtss','rcpps','rsqrtps'):self.reciprocal_sites.add(a)
        super().hook(g,a,size,user)

    def cache_record(self,address,size):
        offset=address-self.cache.magic
        raw=bytes(self.g.mem_read(address,size));assert raw==bytes(self.cache.data[offset:offset+size])
        return dict(address=f'{address:08x}',file_offset=offset,raw_hex=raw.hex())

    def mem_write(self,g,access,a,size,value,user):
        if hasattr(self,'source_calls'):
            for n in range(self.u(POOL+0x38)):
                p=self.u(HEADERS+n*12+8)
                if p and (p+0xc<=a<p+0x1c):
                    self.attachment_writes.append(dict(phase=self.active,
                        instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',datum=f'{self.u(HEADERS+n*12,2)<<16|n:08x}',
                        offset=f'{a-p:03x}',size=size,before=bytes(g.mem_read(a,size)).hex(),value=f'{value:0{size*2}x}'))
                    break
        super().mem_write(g,access,a,size,value,user)

def observe_pose(t,name):
    t.action(name+'_original_parent_pose_bounds',0xbdf10,(UI,))
    points=[]
    for item in t.attachment_state()['items']:
        datum=int(item['datum'],16)
        t.action(name+'_original_object_world_point',0xbacc0,registers=((UC_X86_REG_EAX,datum),(UC_X86_REG_ESI,SCRATCH+0x300)))
        points.append(dict(datum=item['datum'],point=t.vec(SCRATCH+0x300)))
    return dict(name=name,state=t.attachment_state(),source_world_points=points,manager=t.manager_state(),registry=t.registry())

def authored_fields(t):
    c=t.cache
    def sid(value):return c.text(c.u(368)+c.u(c.u(364)+(value&0xffff)*4)) if value else 'NONE'
    result=[]
    for datum,fields in ((0xe9f2086d,(0x158,0x15c,0x160)),(0xe57b0406,(0x284,)),(0xe192001d,(0x284,))):
        tag=c.tags[datum]
        model=c.tags[c.u(tag['offset']+0x38)]
        render=c.tags[c.u(model['offset']+4)]
        result.append(dict(tag=tag,fields=[dict(offset=f'{f:03x}',name=sid(c.u(tag['offset']+f)),
            bits=f'{c.u(tag["offset"]+f):08x}',record=t.cache_record(tag['offset']+f+c.magic,4))for f in fields],
            model=model,render_model=render,
            flags_bc=dict(bits=f'{c.u(tag["offset"]+0xbc):08x}',record=t.cache_record(tag['offset']+0xbc+c.magic,4)),
            model_link=t.cache_record(tag['offset']+0x38+c.magic,4),
            render_model_link=t.cache_record(model['offset']+4+c.magic,4),
            animation_graph_link=t.cache_record(model['offset']+0x14+c.magic,4),
            node_block=t.cache_record(render['offset']+0x48+c.magic,8),
            marker_group_block=t.cache_record(render['offset']+0x58+c.magic,8),
            nodes=[dict(index=i,name=sid(c.u(at)),record=t.cache_record(at+c.magic,0x60))
                for i,at in enumerate(c.block(render['offset']+0x48,0x60))],
            marker_groups=[dict(index=i,name=sid(c.u(at)),record=t.cache_record(at+c.magic,12),
                markers=[t.cache_record(m+c.magic,0x24)for m in c.block(at+4,0x24)])
                for i,at in enumerate(c.block(render['offset']+0x58,12))]))
    return result

def save(t,name,observations,**fields):
    assert not t.attachment_returns
    used={e['service']for e in t.events if 'service'in e}
    assert not used&{'000fb560','001dc8f0','001dc940'}
    indices=set()
    for state in [t.initial_equipped['equipped']]+[o['manager']for o in observations]+[
            c[key]for c in t.source_calls for key in ('before','after')if key in c]:
        for field in ('clip','secondary_clip'):
            if state.get(field):indices.add(state[field]['index'])
    return dict(name=name,**fields,initial=t.initial_equipped,pose_fixture=t.player_pose,
        authored_row=t.row,authored_row_record=t.cache_record(t.record,0x90),
        map_row_entry_hex=bytes(t.g.mem_read(t.entries+t.row*8,8)).hex(),
        source_graph=t.live_metadata['graph'],
        authored_animation_records=[t.live_metadata['animations'][i]for i in sorted(indices)],
        initial_weapon_resources=t.weapon_resources,
        authored_fields=authored_fields(t),observations=observations,final=t.attachment_state(),
        calls=t.attachment_calls,stores=t.attachment_writes,marker_metadata=t.marker_metadata,
        source_calls=t.source_calls,rows=t.inventory_rows,samples=t.samples,
        collision_queries=t.collision_queries,
        events=[{k:v for k,v in e.items()if k!='state'}for e in t.events],live_events=t.live_events,
        reciprocal_sites=[dict(address=f'{a:08x}',bytes=t.code.at(a).bytes.hex(),
            mnemonic=t.code.at(a).mnemonic,operands=t.code.at(a).op_str)for a in sorted(t.reciprocal_sites)],
        shared_rng=f'{t.u(SEED):08x}')

def listings(traces,directory):
    directory.mkdir(parents=True,exist_ok=True);t=traces[0]
    executed=set().union(*(x.executed for x in traces));result=[]
    for entry in ENTRIES:
        routine=_trace(t.code,entry,t.starts|set(ENTRIES))
        ins=[t.code.at(a)for a in sorted(routine.code)];path=directory/f'{entry:08x}.asm'
        path.write_text('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'for i in ins)+'\n')
        result.append(dict(entry=f'{entry:08x}',path=str(path.relative_to(ROOT)),
            sha256=hashlib.sha256(path.read_bytes()).hexdigest(),instructions=len(ins),
            executed_instruction_rows=sum(i.address in executed for i in ins)))
    return result

def run_cases():
    traces=[];cases=[]
    for kind in ('starting_smg','primary_br','secondary_smg','switch_back_to_smg','drop_br'):
        row=12 if kind=='secondary_smg' else 1
        t=AttachmentTrace(row);observations=[observe_pose(t,'starting_equipped_smg')]
        initial=next(i for i in observations[0]['state']['items']if i['datum']=='00010002')
        assert initial['parent']==f'{UI:08x}' and initial['parent_node']==20
        if kind!='starting_smg':
            button=3 if kind=='secondary_smg' else 2
            t.seeded_duration(button,232);t.poll('original_233ms_pickup',{button:255},milliseconds=1)
            observations.append(observe_pose(t,'after_original_pickup'))
            target=next(i for i in observations[-1]['state']['items']if i['datum']=='00020003')
            assert target['parent']==f'{UI:08x}' and target['parent_node']==(19 if button==3 else 20)
            if button==3:
                assert target['sibling']=='00010002' and observations[-1]['state']['unit']['first_child']=='00020003'
            else:
                original=next(i for i in observations[-1]['state']['items']if i['datum']=='00010002')
                assert original['parent']=='ffffffff' and original['parent_node']==255 and original['owner']==f'{UI:08x}'
        continuation=None
        if kind=='switch_back_to_smg':
            for name,buttons in (('primary_release_1',{}),('primary_release_2',{}),
                ('short_other_press_1',{3:255}),('short_other_press_2',{3:255}),
                ('short_other_release_1',{}),('short_other_release_2',{})):
                t.poll(name,buttons,milliseconds=33)
            continuation=continue_graph(t,10,True)
            observations.append(observe_pose(t,'after_original_delayed_switch'))
            items={i['datum']:i for i in observations[-1]['state']['items']}
            assert items['00010002']['parent']==f'{UI:08x}' and items['00010002']['parent_node']==20
            assert items['00020003']['parent']=='ffffffff' and items['00020003']['owner']==f'{UI:08x}'
        if kind=='drop_br':
            t.action('original_drop_primary_br',0xcfd80,(1,0,0),((UC_X86_REG_EBX,UI),))
            observations.append(observe_pose(t,'after_original_drop'))
            target=next(i for i in observations[-1]['state']['items']if i['datum']=='00020003')
            assert target['parent']==target['owner']=='ffffffff' and target['parent_node']==255
            assert t.u(SEED)==0x4b5c952c
        else:assert t.u(SEED)==0x12345678
        cases.append(save(t,kind,observations,continuation=continuation));traces.append(t)
    return traces,cases

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');args=p.parse_args()
    args.output=args.output.resolve()
    traces,cases=run_cases()
    names=('oracle_mp_weapon_attachment_1_1.py','oracle_mp_inventory_equipped_1_1.py',
        'oracle_mp_inventory_1_1.py','oracle_mp_inventory_controls_1_1.py',
        'oracle_mp_inventory_world_1_1.py','oracle_mp_inventory_spatial_1_1.py',
        'oracle_mp_inventory_netgame_1_1.py','oracle_mp_inventory_target_1_1.py',
        'oracle_spartan_equipped_animation_1_1.py','oracle_spartan_live_pose_1_1.py',
        'oracle_spartan_default_pose_1_1.py','oracle_spartan_collision_1_1.py',
        'oracle_smg_firing_1_1.py','inspect_multiplayer.py')
    dependencies=['tools/'+n for n in names]+[
        'tools/reference/halo2-decompiled/tools/'+n for n in ('xbe.py','functions.py')]
    report=dict(xbe_sha256=PIN,cache_sha256=CACHE_PIN,shared_cache_sha256=traces[0].equipped_metadata['shared_cache_sha256'],
        dependencies=[dict(path=n,sha256=hashlib.sha256((ROOT/n).read_bytes()).hexdigest())for n in dependencies],
        cases=cases,instructions=listings(traces,args.output.parent/'instructions'),
        scope='Original source allocator/equipped pose fixture plus actual FB560/marker/parent attach-detach transforms. Exact Xbox reciprocal, complete physics and normal engine scheduler remain unresolved.')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),calls=sum(len(c['calls'])for c in cases),stores=sum(len(c['stores'])for c in cases),
        instruction_listings=len(report['instructions']),instruction_rows=sum(i['instructions']for i in report['instructions']),
        report_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())))

if __name__=='__main__':main()
