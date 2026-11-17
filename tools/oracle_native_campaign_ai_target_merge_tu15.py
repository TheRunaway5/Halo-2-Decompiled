#!/usr/bin/env python3
"""Unchanged physical273290 target merge with real clone/property allocation."""
import argparse,copy,gzip,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical
import oracle_native_campaign_ai_perception_group_clone_tu15 as clone
import oracle_native_campaign_ai_perception_group_link_tu15 as link
from oracle_native_campaign_ai_perception_group_actor_tu15 import BASE,ACTORS,AROWS,PROPERTIES,PROWS,GROUPS,GROWS,OBJECTS,HEADERS,CLUMP,BITS
NONE=0xffffffff;BODIES=BASE+0x20000;MEMBERS=BASE+0x30000
ENTRIES=tuple(dict.fromkeys((0x273290,0xbcdf0,0x274670,0x274750,0x2745e0,0xbbe00,*clone.ENTRIES)))

def make(image,seed,name,*,destinations=(),sources=(1,),actors=0,duplicate=False,
         special=0,invalid='',generation=7,allocation='ok',alias=False,source_float=0x40000000,
         dest_float=0x3f800000,source_rank=2,dest_rank=1,source_priority=2,dest_priority=1,
         profile=physical.FP_PROFILES[0]):
    row=copy.deepcopy(seed);banks={b['address']:bytearray.fromhex(b['before'])for b in row['banks']}
    banks[BODIES]=bytearray(0x8000);banks[MEMBERS]=bytearray(0x1000);banks[0x510fa8]=bytearray(4)
    def put(at,value,width=4):
        for first,data in banks.items():
            if first<=at and at+width<=first+len(data):data[at-first:at-first+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError(hex(at))
    def get(at,width=4):
        for first,data in banks.items():
            if first<=at and at+width<=first+len(data):return int.from_bytes(data[at-first:at-first+width],'little')
        raise ValueError(hex(at))
    # All target/actor/property and allocator inputs are real retained records.
    for p,v in((0x50eef8,link.CLUMPS),(link.CLUMPS+0x44,CLUMP),(CLUMP+0x18,0xe1000000 if actors else NONE),
              (CLUMP+0x14,0xe3000008 if destinations else NONE),(CLUMP+0x50+0x14,0xe3000000 if sources else NONE),
              (OBJECTS+0x24,12),(OBJECTS+0x38,16),(OBJECTS+0x44,HEADERS),(GROUPS+0x24,0xc4),
              (GROUPS+0x20,16),(GROUPS+0x34,0),(GROUPS+0x38,12),(GROUPS+0x3c,len(sources)+len(destinations)),
              (link.GROUP_BITS,sum(1<<i for i in range(len(sources)))+sum(1<<(8+i)for i in range(len(destinations)))),
              (0x4eabcc,generation),(0x4eabc8,0x44332200),(0x510fa8,MEMBERS),(MEMBERS+0x44,MEMBERS+0x100),
              (PROPERTIES+0x20,16),(PROPERTIES+0x34,0),(PROPERTIES+0x38,0),(PROPERTIES+0x3c,0),(BITS,0)):
        put(p,v)
    put(CLUMP+0x24,4 if allocation=='threshold'else 0,2)
    if allocation=='full':put(GROUPS+0x20,12);put(GROUPS+0x34,12);put(GROUPS+0x3c,12);put(link.GROUP_BITS,0xfff)
    for n in range(16):
        put(0x4eabd0+n*4,0x12345678)
        put(HEADERS+n*12,0xea00,2);put(HEADERS+n*12+3,0,1);put(HEADERS+n*12+8,BODIES+n*0x400)
        body=BODIES+n*0x400
        put(body+0x14,NONE);put(body+0xaa,0,1);put(body+0x12c,0xe1000000);put(body+0x13c,NONE)
        put(body+0x12e,2,2);put(body+0x138,2,2);put(body+0x13a,0x200,2)
        put(body+0x204,NONE);put(body+0x208,NONE);put(body+0x20c,NONE)
        for lane,value in enumerate((0x3f800000,0x40000000,0x40400000)):put(body+0x64+lane*4,value)
    for n in range(max(actors,1)):
        put(AROWS+n*0x888+0x80,0xe1000000+n+1 if n+1<actors else NONE);put(AROWS+n*0x888+0x58,NONE)
    for values,start in((sources,0),(destinations,8)):
        for n,target in enumerate(values):
            g=GROWS+(start+n)*0xc4
            for off in range(0xc4):put(g+off,(off*13+7)&255,1)
            put(g,0xe300,2);put(g+2,2,2);put(g+8,NONE if target==NONE else 0xea000000|target)
            put(g+0xc,3,2);put(g+0x14,0xe3000000+start+n+1 if n+1<len(values)else NONE);put(g+0x18,NONE)
    if special:
        body=BODIES+destinations[0]*0x400;put(body+0xaa,12,1)
        if special==1:put(body+0x134,1)
        elif special>=3:
            put(body+0x208,0xe4000000);put(MEMBERS+0x100+0x18,0xea000005 if special!=6 else NONE)
            put(BODIES+5*0x400+0x20c,0xea000006)
            if special==4:put(BODIES+5*0x400+0x134,1)
            if special==5:put(0x4eabd0+5*4,(generation+1)&0xffffffff)
    for label,group_index in(('source',0),('destination',8)):
        if invalid==label+'_none':put(GROWS+group_index*0xc4+8,NONE)
        if invalid==label+'_limit':put(OBJECTS+0x38,0)
        if invalid==label+'_negative_limit':put(OBJECTS+0x38,0x80000000)
        if invalid==label+'_zero_salt':put(HEADERS+get(GROWS+group_index*0xc4+8,2)*12,0,2)
        if invalid==label+'_stale_salt':put(HEADERS+get(GROWS+group_index*0xc4+8,2)*12,0xea01,2)
        if invalid==label+'_null_body':put(HEADERS+get(GROWS+group_index*0xc4+8,2)*12+8,0)
    if invalid=='group_limit':put(GROUPS+0x38,0)
    if invalid=='group_zero_salt':put(GROWS,0,2)
    if invalid=='group_stale_salt':put(GROWS,0xe301,2)
    if duplicate:put(0x4eabd0+sources[0]*4,(generation+1)&0xffffffff)
    for off,a,b,width in((0x2c,dest_float,source_float,4),(0x30,0x51,0xa2,1),(0x34,dest_priority,source_priority,4),
                          (0x26,dest_rank,source_rank,2),(0x28,0x24681357,0xdeadbeef,4)):
        put(CLUMP+off,a,width);put(CLUMP+0x50+off,b,width)
    gp=[0x11335577+k*0x10101 for k in range(8)];gp[0]=0xec630000;gp[1]=0xec640000 if alias else 0xec640001;gp[4]=ref.SP
    row.update(name=name,entry=0x273290,gp=gp,matrix=0x14,source=0x830694f8,stack_arguments=[],
        banks=[dict(address=p,before=bytes(v).hex())for p,v in sorted(banks.items())],before_fp=list(profile),live_x87=[],
        xmm=[[0x6b000000+r*4+k for k in range(4)]for r in range(8)],max_steps=50000,strict_writes=False,stop_before_entries=[])
    return row

def inputs(image):
    seed=next(r for r in clone.pressure.inputs(image)if r['entry']==0x274200);rows=[]
    for p,profile in enumerate(physical.FP_PROFILES):
        def add(name,**kwargs):rows.append(make(image,seed,name+f'_fp{p}',profile=profile,**kwargs))
        for destinations in((),(2,),(1,),(2,3)):
            for sources in((),(1,),(1,1),(1,3,4)):
                for actors in(0,1,2):add(f'dest{destinations}_source{sources}_actors{actors}',destinations=destinations,sources=sources,actors=actors)
        for invalid in('source_none','source_limit','source_negative_limit','source_zero_salt','source_stale_salt','source_null_body',
                        'destination_none','destination_limit','destination_negative_limit','destination_zero_salt','destination_stale_salt','destination_null_body',
                        'group_limit','group_zero_salt','group_stale_salt'):
            add(invalid,invalid=invalid,destinations=(2,))
        for special in range(1,7):add(f'special{special}',destinations=(2,),sources=(5,6,1),special=special)
        for allocation in('threshold','full'):add(allocation,allocation=allocation)
        add('already_stamped',duplicate=True);add('generation_wrap',generation=NONE);add('clump_alias',destinations=(2,),alias=True)
        for a,b in((0,0x80000000),(0x40000000,0x3f800000),(0x3f800000,0x40000000),
                    (0x7fc12345,0x3f800000),(0x7f812345,0x3f800000),(0x3f800000,0x7f812345),(1,0),(0xff800000,0x7f800000)):
            add(f'numeric_{a:x}_{b:x}',sources=(),source_float=a,dest_float=b)
        for a,b in((-32768,32767),(32767,-32768),(1,1),(-1,0),(0,-1)):
            add(f'rank_{a}_{b}',sources=(),source_rank=a,dest_rank=b,source_priority=a,dest_priority=b)
        for a,b in((0x80000000,0x7fffffff),(0x7fffffff,0x80000000)):
            add(f'priority_{a:x}_{b:x}',sources=(),source_priority=a,dest_priority=b)
    for n,row in enumerate(rows):row['id']=n
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);p.add_argument('--select');a=p.parse_args()
    assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=ref.metadata(image,ENTRIES)
    owned=ref.prepare(image,meta,scratch);rows=inputs(image)
    if a.select:
        import re
        rows=[r for r in rows if re.search(a.select,r['name'])]
        for n,r in enumerate(rows):r['id']=n
    rows=physical.physical(owned,rows,'whole');assert all(r['completed']for r in rows)
    reached=set().union(*(set(r['executed_sites'])for r in rows));code=_Code(image,'.text');own=set(complete_body(code,0x273290).code)
    coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code),missing=[hex(pc)for pc in sorted(set(complete_body(code,e).code)-reached)])for e in ENTRIES}
    child_pcs=sorted(s['instruction']for s in meta['sites']if s['instruction']not in own and any(o['write']for o in s['operands']))
    count=len(rows);stores=sum(len(r['writes'])for r in rows)
    fixture=dict(fixture_kind='native-target-merge-v1',image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,unattributed_child_store_pcs=child_pcs,bank_encoding='sparse-runs-v1',cases=ref.compact(rows))
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert encoded==a.verify.read_bytes(),'Independent original repeat differs'
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=not a.select,cases=count,coverage=coverage,ordered_stores=stores,fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=ref.sha(a.output),original_sha256=NEW_PIN,owned_scratch=str(scratch),repeat_identical=bool(a.verify))
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
