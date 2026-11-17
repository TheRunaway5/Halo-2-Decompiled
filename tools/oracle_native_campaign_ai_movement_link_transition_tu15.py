#!/usr/bin/env python3
"""Unchanged physical200420 movement-link transition with real ballistic children."""
import argparse,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical
import oracle_native_campaign_ai_movement_link_parameters_tu15 as parameters

ENTRIES=(0x200420,0x200310,0x278fa0,0x1ecd00,0xff1a0,0xff580,0x30bd0,0x2b3e0)
BASE=parameters.BASE;POINT=BASE+0x3340;MASK=BASE+0x3370;GRAVITY=BASE+0x3380;BASIS=BASE+0x3390
def f(value):return struct.unpack('<I',struct.pack('<f',value))[0]

def make(image,name,*,mask=0x7f,force=0x41e90000,count=6,target=(2.,1.,.5),orientation=(1.,0.,0.),
         speeds=(2.,3.,4.,5.,6.,7.),gravity=1.,mode=0,character=True,body_flag=0,scenario=True,scenario_count=1,profile=physical.FP_PROFILES[0]):
    row=parameters.make(image,name,count=count,character=character,body_flag=body_flag,scenario=scenario,
        scenario_count=scenario_count,index=1,profile=profile)
    banks={b['address']:bytearray.fromhex(b['before'])for b in row['banks']}
    def put(at,value,size=4):
        for base,raw in banks.items():
            if base<=at and at+size<=base+len(raw):raw[at-base:at-base+size]=(value&((1<<(size*8))-1)).to_bytes(size,'little');return
        banks[at]=bytearray((value&((1<<(size*8))-1)).to_bytes(size,'little'))
    actor=parameters.ACTORS+0x888
    for i,v in enumerate(target):put(POINT+i*4,v if isinstance(v,int)else f(v))
    for i,v in enumerate(orientation):put(actor+0x290+i*4,v if isinstance(v,int)else f(v))
    put(parameters.BODY+0x346,0x400,2);put(parameters.BODY+0x436,mode,2)
    put(MASK,mask or 0,1);put(GRAVITY,f(gravity));put(0x52be68,GRAVITY)
    for i,v in enumerate((1.,0.,0.)):put(BASIS+i*4,f(v))
    for p in (0x474d08,0x474d10):put(p,BASIS)
    for i,v in enumerate(speeds):put(parameters.VALUES+0x80+i*4,f(v))
    for address,size in ((0x46a000,0x1000),(0x45c618,4)):
        banks[address]=bytearray(image.read(address,size))
    row.update(entry=0x200420,matrix=POINT,source=force,max_steps=10000,compare_reads=False,stop_before_entries=[0xe8e50])
    row['gp'][1]=0 if mask is None else MASK
    row['banks']=[dict(address=p,before=bytes(v).hex())for p,v in sorted(banks.items())]
    return row

def inputs(image):
    rows=[]
    for fi,fp in enumerate(physical.FP_PROFILES):
        for mask in (None,0,1,2,4,0x20,0x40,0x7f,0x80,0xff):
            for force in (0,1,0x41e90000,0x41e90001):
                rows.append(make(image,f'mask{mask}_force{force:x}_{fi}',mask=mask,force=force,profile=fp))
        for count in (-32768,-1,0,1,5,6,7,32767):
            for force in (0,1):rows.append(make(image,f'count{count}_force{force}_{fi}',count=count,force=force,profile=fp))
        for target in ((0.,0.,0.),(0.,0.,2.),(2.,0.,0.),(10.,0.,-2.),(-2.,1.,.5),(0x7fc12345,0.,1.)):
            for orientation in ((1.,0.,0.),(0.,1.,1.),(0.,0.,1.)):
                rows.append(make(image,f'point{target}_orientation{orientation}_{fi}',mask=2,target=target,orientation=orientation,profile=fp))
        for character,body_flag in ((False,0),(False,1),(True,1)):
            for mask in (None,0x7f):rows.append(make(image,f'character{character}_body{body_flag}_mask{mask}_{fi}',character=character,body_flag=body_flag,mask=mask,profile=fp))
        for scenario,count in ((False,1),(True,0),(True,-1)):
            rows.append(make(image,f'scenario{scenario}_count{count}_{fi}',scenario=scenario,scenario_count=count,profile=fp))
        for gravity in (-1.,0.,.25,4.):rows.append(make(image,f'gravity{gravity}_{fi}',mask=2,gravity=gravity,profile=fp))
        for orientation in ((1.,0.,0.),(0.,1.,1.),(0.,0.,1.),(0x7fc12345,0.,1.)):
            rows.append(make(image,f'vertical_at_apex_orientation{orientation}_{fi}',mask=2,target=(0.,0.,2.),speeds=(.1,2.,3.,4.,5.,6.),orientation=orientation,profile=fp))
        rows.append(make(image,f'zero_speed_zero_distance_{fi}',mask=2,target=(0.,0.,0.),speeds=(0.,)*6,profile=fp))
        rows.append(make(image,f'forced_nan_target_{fi}',mask=2,force=1,target=(0x7fc12345,0.,1.),profile=fp))
        rows.append(make(image,f'authored_force_far_target_{fi}',mask=2,count=7,target=(100.,0.,20.),profile=fp))
        rows.append(make(image,f'action_mode5_{fi}',mode=5,profile=fp))
    for i,row in enumerate(rows):row['id']=i
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--select');a=p.parse_args()
    assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=ref.metadata(image,ENTRIES);owned=ref.prepare(image,meta,scratch)
    cases=inputs(image)
    if a.select:
        import re
        cases=[r for r in cases if re.search(a.select,r['name'])]
        for i,r in enumerate(cases):r['id']=i
    rows=physical.physical(owned,cases,'whole');assert all(r['completed']for r in rows)
    reached=set().union(*(set(r['executed_sites'])for r in rows));code=_Code(image,'.text')
    fixture=dict(fixture_kind='native-movement-link-transition-v1',image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,bank_encoding='sparse-runs-v1',cases=ref.compact(rows))
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
    report=dict(cases=len(rows),coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code),missing=[hex(pc)for pc in sorted(set(complete_body(code,e).code)-reached)])for e in ENTRIES},stores=sum(len(r['writes'])for r in rows),fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=ref.sha(a.output),original_sha256=NEW_PIN,scratch=str(scratch),retained_raw_banks=False)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='coverage'}))
if __name__=='__main__':main()
