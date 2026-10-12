#!/usr/bin/env python3
"""Independent unchanged TU1.1 terrain endpoint retreat/relocalization oracle."""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_XMM0,UC_X86_REG_XMM3,UC_X86_REG_XMM4
from oracle_mp_pickup_visibility_1_1 import VisibilityTrace,ENTRIES as VISIBILITY_ENTRIES
from oracle_mp_inventory_spatial_1_1 import cache_record
from oracle_mp_inventory_1_1 import (ROOT,PIN,CACHE_PIN,UNIT,UI,SCRATCH,SEED,STOP,_trace,
    UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_ESP)

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/pickup-retreat'
VERTICES=(188,756,761,768,770,773,774,775,783,784,895,914)
EDGES=((399,0),(406,0),(887,0),(1101,1),(1198,0),(1234,1),
    (1391,0),(1412,0),(1416,1),(1421,1),(1433,0),(1481,1))
SITES=(0x16b8eb,0x16b920,0x16b96b,0x16b9b1,0x16b9d4,0x16ba07,0x16ba1f,
    0x16ba27,0x16ba30,0x16ba4d,0x16ba80,0x16ba85,0x16ba89,0x16ba8b,
    0x16ba90,0x16ba96,0x16baa1,0x16baa4,0x16baa6)

class RetreatTrace(VisibilityTrace):
    def finish_visibility_returns(self,a):
        completed=[row for ret,row,kind,_ in self.visibility_returns if ret==a and kind=='outer']
        super().finish_visibility_returns(a)
        for row in completed:
            contact=row.get('contact_cache')
            if contact and contact['material']==0xffff:
                # Original16B321 skips material lookup for signed indexNONE.
                contact.pop('material_record',None);contact['material_absent']=True

    def hook(self,g,a,size,user):
        if a in SITES and getattr(self,'active_segments',None):
            q=self.active_segments[-1];p=int(q['output_pointer'],16)
            sp=g.reg_read(UC_X86_REG_ESP)
            q.setdefault('retreat_trace',[]).append(dict(instruction=f'{a:08x}',
                caller=q['caller'],mask=q['mask'],fraction_bits=f'{self.u(p+4):08x}',
                point=self.vec(p+8),initial_location_hex=bytes(g.mem_read(p+0x14,8)).hex(),
                final_location_hex=bytes(g.mem_read(p+0x1c,8)).hex(),
                normal_bits=[f'{self.u(p+0x28+n*4):08x}'for n in range(4)],
                dot_or_step_stack_bits=f'{self.u(sp+0x1c):08x}',
                xmm0_bits=f'{g.reg_read(UC_X86_REG_XMM0)&0xffffffff:08x}',
                xmm3_bits=f'{g.reg_read(UC_X86_REG_XMM3)&0xffffffff:08x}',
                xmm4_bits=f'{g.reg_read(UC_X86_REG_XMM4)&0xffffffff:08x}',registers=self.registers()))
        super().hook(g,a,size,user)

    def located(self,point,name):
        self.action(name,0x11d4a0,registers=((UC_X86_REG_EDX,point),(UC_X86_REG_ESI,SCRATCH+0xd00)))
        return bytes(self.g.mem_read(SCRATCH+0xd00,8)).hex()

    def segment(self,start,end,name,provenance):
        before=len(self.outer_queries);call_begin=len(self.visibility_calls)
        origin_location=self.located(start,'original_probe_origin_location')
        endpoint_location=self.located(end,'original_probe_endpoint_location')
        self.action(name,0x16bac0,(0x24800001,0xffffffff,SCRATCH+0xc30),
            ((UC_X86_REG_EAX,start),(UC_X86_REG_ECX,end),(UC_X86_REG_EDX,UI)))
        assert len(self.outer_queries)==before+1
        q=self.outer_queries[-1]
        return dict(name=name,provenance=provenance,query_index=before,
            call_range=[call_begin,len(self.visibility_calls)],origin_location_hex=origin_location,
            endpoint_location_hex=endpoint_location,
            retreats=sum(s['instruction']=='0016ba30'for s in q.get('retreat_trace',[])))

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');args=p.parse_args()
    args.output=args.output.resolve();t=RetreatTrace();pose=t.pose(0)
    source_visibility=t.visibility(t.world_datums[0],'original_capsule_point_visibility_producer')
    origin=t.outer_queries[source_visibility['query_range'][0]]['start']
    t.g.mem_write(SCRATCH+0xc00,b''.join(struct.pack('<I',int(b,16))for b in origin['bits']))
    collision=t.u(0x4e790c);vertices=t.u(collision+0x3c);edges=t.u(collision+0x34)
    cases=[]
    for index in VERTICES:
        cases.append(t.segment(vertices+index*16,SCRATCH+0xc00,'original_cache_vertex_to_capsule_top',
            dict(vertex=index,vertex_record=cache_record(t,vertices+index*16,16),
                endpoint_source_query=source_visibility['query_range'][0],endpoint=origin)))
    for index,direction in EDGES:
        pair=[t.u(edges+index*12+n*2,2)for n in range(2)]
        cases.append(t.segment(vertices+pair[direction]*16,vertices+pair[1-direction]*16,
            'original_cache_edge_vertex_segment',dict(edge=index,direction=direction,
                edge_record=cache_record(t,edges+index*12,12),vertices=pair,
                vertex_records=[cache_record(t,vertices+v*16,16)for v in pair])))
    assert not t.visibility_returns and not t.active_segments
    assert t.u(SEED)==0x12345678
    directory=args.output.parent/'instructions';directory.mkdir(parents=True,exist_ok=True);instructions=[]
    for entry in VISIBILITY_ENTRIES:
        routine=_trace(t.code,entry,t.starts|set(VISIBILITY_ENTRIES));code=[t.code.at(a)for a in sorted(routine.code)]
        path=directory/f'{entry:08x}.asm';path.write_text('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'for i in code)+'\n')
        instructions.append(dict(entry=f'{entry:08x}',path=str(path.relative_to(ROOT)),sha256=sha(path),
            instructions=len(code),executed=sum(i.address in t.executed for i in code)))
    dependencies=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/pickup-visibility/report.json').read_text())['dependencies']
    dependencies=[dict(path='tools/oracle_mp_pickup_retreat_1_1.py',sha256=sha(Path(__file__)))]+dependencies
    report=dict(xbe_sha256=PIN,cache_sha256=CACHE_PIN,shared_cache_sha256=t.equipped_metadata['shared_cache_sha256'],
        dependencies=dependencies,initial=t.initial_equipped,initial_world=t.initial_world,
        initial_registry=t.initial_registry,pose=pose,source_visibility=source_visibility,
        collision_block=cache_record(t,collision,0x40),cases=cases,
        outer_queries=t.outer_queries,calls=t.visibility_calls,rows=t.inventory_rows,
        events=[{k:v for k,v in e.items()if k!='state'}for e in t.events],live_events=t.live_events,
        instructions=instructions,rng=f'{t.u(SEED):08x}',
        scope='Independent cache-derived mask24800001 terrain probes, original capsule producer, contact and endpoint retreat/relocalization. These are explicit probes, not claimed live C57A0 emissions. Heap/residency, full world/Havok and Xbox reciprocal boundaries remain explicit.')
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),retreat_iterations=sum(c['retreats']for c in cases),
        queries=len(t.outer_queries),report_sha256=sha(args.output))))

if __name__=='__main__':main()
