#!/usr/bin/env python3
"""Physical vision timestamp/rescore, attachment selection and facing helpers."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_score_tu15 as score
import oracle_native_campaign_ai_perception_group_link_tu15 as transport
import validate_native_geometry_tu15 as geometry
BASE=score.BASE;OWNERS,OWNERROWS,DIRECTION,FACING,OUTPUT=BASE+0xe500,BASE+0xe580,BASE+0xf620,BASE+0xf600,BASE+0xf000
ENTRIES=(0x26f940,0x299860,0xcdcc0,0x111fd0,0x26df70)+score.ENTRIES
sha=common.sha
def prepare(image,meta,scratch):
    owned=transport.prepare(image,meta,scratch)
    result=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_vision_support_tu15.cpp'),'-o',str(owned/'observer')],capture_output=True,text=True);assert not result.returncode,result.stderr
    return owned
def inputs(image):
    rows=[];seeds=score.inputs(image);seed=next(s for s in seeds if s['name'].startswith('fault_seed_plain'))
    def make(entry=0x26f940,source=seed,name='vision_support',tracked=0,objects=3,preferred=0xe5000005,
             direction=(0,0,0xbf000000),facing=(0,0,0x3f800000),distance=0x40000000,
             private_facing=False,private_output=False,destination=OUTPUT,offset=0x300,
             unit_offset=0x400,unit_identity=1,unit_valid=0,missing_attachment=False,changes=(),fp=None,live=0):
        n=len(rows);banks={b['address']:bytearray.fromhex(b['before'])for b in source['banks']};banks[0x456000]=bytearray(image.read(0x456000,4096));banks[0x449000]=bytearray(image.read(0x449000,4096))
        def put(p,v,w=4):
            for first,data in banks.items():
                if first<=p and p+w<=first+len(data):data[p-first:p-first+w]=(v&((1<<(8*w))-1)).to_bytes(w,'little');return
            banks[p]=bytearray((v&((1<<(8*w))-1)).to_bytes(w,'little'))
        put(0x510fa8,OWNERS);put(OWNERS+0x44,OWNERROWS);put(OWNERROWS+0x18,0xe5000004 if objects else 0xffffffff)
        put(score.CLOCK+8,0x12345678)
        for axis in range(3):put(score.policy.BODIES[0]+0xc+axis*4,0)
        for i in range(3):
            body=score.policy.BODIES[4+i];attachment=body+offset
            put(body+0x134,0);put(body+0x13a,offset,2);put(attachment+0xc,0xe5000005+i if i+1<objects else 0xffffffff)
            put(attachment,0,1)
            for axis,value in enumerate((0x3f800000,0x40000000,0x40400000)):
                put(attachment+0x14+axis*4,value if i!=1 else 0x40800000);put(body+0x168+axis*4,value)
        if missing_attachment:put(score.policy.BODIES[4]+0x134,1)
        put(score.policy.BODIES[4]+0x33e,unit_offset,2);unit=score.policy.BODIES[4]+unit_offset+0x9c
        put(unit,unit_identity);put(unit+6,unit_valid,2);put(unit+8,0x0500000a)
        for i in range(3):put(DIRECTION+i*4,direction[i]);put(FACING+i*4,facing[i])
        for p,v,w in changes:put(p,v,w)
        gp=source['gp'].copy();gp[4]=geometry.SP+n%4*4;matrix=source['actor'];other=tracked;face=geometry.SP+0x400 if private_facing else FACING;dest=geometry.SP+0x500 if private_output else destination
        if entry==0x26f940:gp[1]=source['perception']
        elif entry==0x299860:gp[1]=0xe1000000;matrix=score.policy.BODIES[0];other=preferred
        elif entry==0xcdcc0:gp[0]=dest;gp[1]=0xe5000004
        elif entry==0x111fd0:gp[0]=0xe5000004
        elif entry==0x26df70:gp[0]=face;gp[1]=DIRECTION;matrix=distance
        else:raise ValueError(hex(entry))
        rows.append(dict(id=n,name=f'{name}_{n}',entry=entry,gp=gp,matrix=matrix,source=other,actor=source['actor'],perception=source['perception'],tracked=tracked,owner=0xe1000000,reference=score.policy.BODIES[0],preferred=preferred,object=0xe5000004,direction=DIRECTION,facing=face,facing_value=list(facing),distance=distance,destination=dest,private_facing=private_facing,private_output=private_output,expected_unmapped_read=missing_attachment,child_output=0,required_owner_boundary=source['required_owner_boundary']if entry==0x26f940 else False,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=fp or source['before_fp'],live_x87=source['live_x87']if fp is None else[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3).hex()for k in range(live)]))
    for source in seeds:
        if not source['name'].startswith('fault_seed'):
            for tracked in (0,1):make(source=source,tracked=tracked,name='real_timestamp_and_rescore')
    for tracked in (0,1,2,255):
        for query in (0xffffffff,0xe6000000):make(tracked=tracked,changes=[(score.policy.PROWS+0x14,query,4)],name='timestamp_flag_none')
    for base in (0,0xffffff90):make(changes=[(score.policy.QUERYPOOL+0x44,base,4)],name='timestamp_wrapped_null')
    for objects in (0,1,2,3):
        for preferred in (0xffffffff,0xe5000004,0xe5000005):
            for flags in (0,1):make(entry=0x299860,objects=objects,preferred=preferred,changes=[(score.policy.BODIES[4]+0x300,flags,1)],name='attachment_strict_selection')
    for offset in (-0x100,0,0x100,0x300):make(entry=0x299860,objects=2,offset=offset,name='attachment_signed_offset')
    make(entry=0x299860,missing_attachment=True,name='actual_unmapped_attachment_read')
    for value in (0,0x80000000,1,0x7f800000,0x7fc12345,0x7f812345):
        for rounding in range(4):make(entry=0x299860,changes=[(score.policy.BODIES[4]+0x314,value,4),(score.policy.BODIES[5]+0x314,0x7fc54321,4)],fp=[0x27f,0,0x1f80|(rounding<<13)|(0x8040 if value==1 else 0)],name='attachment_numeric')
    vector=score.policy.BODIES[4]+0x168
    for destination in (OUTPUT,vector-4,vector,vector+4,vector+8):
        for alignment in (0,1,3):make(entry=0xcdcc0,destination=destination+alignment,name='raw_vector_alias')
    make(entry=0xcdcc0,private_output=True,name='actual_private_unit_vector')
    for offset in (-0x400,-1,0,1,0x400):
        for identity in (0xffffffff,0,0x80000001):
            for valid in (0xffff,0,0x8001):make(entry=0x111fd0,unit_offset=offset,unit_identity=identity,unit_valid=valid,name='signed_unit_extension')
    for cosine in (-1.,-0.,0.,.1,.3,.5,.6,.8,.9,1.,1.1):
        for distance in (0.,.1,.5,1.,2.,10.):
            for private in (False,True):make(entry=0x26df70,direction=(0,0,struct.unpack('<I',struct.pack('<f',-cosine))[0]),distance=struct.unpack('<I',struct.pack('<f',distance))[0],private_facing=private,name='facing_class_buckets')
    for cw in (0x7f,0x27f,0x37f):
        for rounding in range(4):
            for bits in (0x80000000,1,0x7fc12345,0x7f812345,0x7f800000):
                make(entry=0x26df70,direction=(bits,0x7fc54321,0xbf000000),facing=(0x7fc98765,0x3f800000,0x3f800000),distance=bits,private_facing=True,fp=[cw|(rounding<<10),((rounding+1)<<11)|1,0x1f80|(rounding<<13)|(0x8040 if bits==1 else 0)],live=(0,1,7)[rounding%3],name='facing_numeric')
    for kwargs in [dict(name='fault_seed_timestamp',tracked=1),dict(entry=0x299860,name='fault_seed_attachment'),dict(entry=0xcdcc0,name='fault_seed_copy',destination=vector+4),dict(entry=0xcdcc0,name='fault_seed_private_copy',private_output=True),dict(entry=0x111fd0,name='fault_seed_unit'),dict(entry=0x26df70,name='fault_seed_facing',private_facing=True)]:make(**kwargs)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));fixture=transport.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Vision support independent physical repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),required_owner_boundaries=sum(r['boundary']is not None and r['boundary']['phase']=='required_owner'for r in rows),fault_prefixes=len(prefix),original_sites_executed=len(visited),uncovered_sites=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited],fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_vision_support_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_vision_support_tu15.cpp')});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
