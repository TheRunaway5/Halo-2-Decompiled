#!/usr/bin/env python3
"""Physical unchanged TU1.5 radius, arrival, and ordinary movement resume.

All points, path progress and velocity are independent inputs. No instruction
patches, native-result inputs, arrival overrides or supplied child outcomes.
"""
import argparse,gzip,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as search
import oracle_native_campaign_ai_publication_tu15 as pub

OWNED=(0x1ebbd0,0x1fb4f0,0x200a40,0x218eb0,0x218f60)
ENTRIES=tuple(dict.fromkeys(OWNED+pub.ENTRIES))
STOP=(0x218880,0x26c2a0,0x26c7a0,0x26c850)

def inputs(image):
    rows=[];ar=pub.Scene.row(pub.ACTOR)
    def base():
        s=pub.Scene();s.ws[:]=bytes(len(s.ws));s.actor(pub.ACTOR,position=(0.,0.,0.),action=1,raw={0x4cc:0,0x4ae:(0,1),0x227:(0,1),0x4e8:(0,1)})
        for at in (0x4ec,0x528,0x548):
            for i,v in enumerate((1.,0.,0.)):s.put(ar+at+i*4,pub.f(v))
            s.put(ar+at+12,0xffff)
        s.put(ar+0x539,1,1);s.put(ar+0x53a,0,1)
        s.world.put(search.BODY,0x34560004)
        for i in range(3):s.world.put(search.BODY+0x88+i*4,0)
        return s
    def make(entry,name,s,fp,arg=0):
        r=pub.make(image,rows,name,entry,s,(arg,0),fp=fp)
        r['gp'][0]=pub.ACTOR;r['gp'][1]=pub.ACTOR
        r.update(compare_reads=False,strict_store_pcs=False,stop_before_entries=list(STOP));return r
    for fi,fp in enumerate(pub.FP_PROFILES):
        for value in (0,0x80000000,pub.f(-1.),pub.f(.01),pub.f(.5),pub.f(4.),0x7fc00000,0x7f800001,1,0x7f800000):
            s=base();s.put(ar+0x4cc,value);s.put(pub.FLAGS+8,pub.f(.4));make(0x1ebbd0,f'radius_direct_{value:x}_{fi}',s,fp)
        for mode in ('ground','ground_none','ground_parent','flying_missing','flying','vehicle','vehicle_missing'):
            for value in (0,pub.f(-1.),pub.f(.75),0x7fc00000):
                s=base();s.put(pub.FLAGS+8,value)
                if mode=='ground_none':s.put(ar+0x54,0xffffffff)
                if mode=='ground_parent':s.character(behavior=0,parent=True)
                if mode.startswith('flying'):s.put(ar+0x229,1,1)
                if mode.startswith('vehicle'):s.put(ar+0x26c,pub.UNIT)
                if mode in ('flying','vehicle'):
                    s.put(pub.CHAR+0xe4,1);s.put(pub.CHAR+0xe8,pub.BASE+0x5400);s.put(pub.BASE+0x5404,0x34560004);s.put(pub.BASE+0x5418,value)
                make(0x1ebbd0,f'radius_{mode}_{value:x}_{fi}',s,fp)
        for distance in (0.,.49,.5,.51,1.,2.,0x7fc00000,1):
            for frame in (0xffff,0,1,3):
                s=base();s.put(ar+0x4ec,pub.fv(distance));s.put(ar+0x4f8,frame);make(0x1fb4f0,f'arrival_distance_{distance}_{frame}_{fi}',s,fp,pub.f(.5))
        for current,count in ((0,1),(0,2),(1,2),(2,3),(-1,0),(1,-1)):
            for position in (.5,1.,1.5):
                s=base();s.put(ar+0x4ae,1,1);s.put(ar+0x238,pub.f(position));s.put(ar+0x539,count,1);s.put(ar+0x53a,current,1)
                prev=ar+0x528 if current<=0 else ar+0x52c+current*0x1c
                for i in range(3):s.put(prev+i*4,0)
                s.put(prev+12,0xffff);make(0x1fb4f0,f'arrival_path_{current}_{count}_{position}_{fi}',s,fp,pub.f(.5))
        for speed in (-1.,0.,1.):
            for distance in (.4,.6,1.1,2.):
                s=base();s.put(ar+0x4ae,1,1);s.put(ar+0x26c,pub.UNIT);s.put(ar+0x4ec,pub.f(distance));s.world.put(search.BODY+0x88,pub.f(speed));make(0x1fb4f0,f'arrival_velocity_{speed}_{distance}_{fi}',s,fp,pub.f(.5))
        for mode in (0,1,2,3,4,5,6,7):
            for active,flag in ((0,0),(0,1),(1,0)):
                s=base();s.put(ar+0x4ac,mode,2);s.put(ar+0x227,active,1);s.put(ar+0x4e8,flag,1);make(0x200a40,f'resume_mode{mode}_active{active}_flag{flag}_{fi}',s,fp)
        for stance in (0,1,2):
            s=base();s.world.put(search.BODY+0x3dc,stance,1);make(0x200a40,f'resume_action_stance{stance}_{fi}',s,fp,1)
        for first,second in ((0xffff,0xffff),(0,0xffff),(0xffff,3),(1,0),(0,0)):
            s=base();s.put(ar+0x528+12,first);s.put(ar+0x4f8,second)
            r=make(0x218eb0,f'difference_frames_{first}_{second}_{fi}',s,fp);r['gp'][0]=ar+0x600;r['gp'][1]=ar+0x528;r['gp'][3]=ar+0x4ec
        for frame in (0xffff,0,1,3):
            s=base();s.put(ar+0x4f8,frame);r=make(0x218f60,f'difference_world_{frame}_{fi}',s,fp);r['gp'][0]=ar+0x4ec;r['gp'][3]=ar+0x238;r['gp'][7]=ar+0x600
    return rows

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--select');a=ap.parse_args();assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=ref.metadata(image,ENTRIES);owned=ref.prepare(image,meta,scratch);selected=inputs(image)
    if a.select:
        import re
        selected=[r for r in selected if re.search(a.select,r['name'])]
    for i,r in enumerate(selected):r['id']=i
    rows=search.physical(owned,selected,'whole');assert all(r['completed'] for r in rows);reached=set().union(*(set(r['executed_sites'])for r in rows));code=_Code(image,'.text')
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',instruction_patches=[],supplied_original_outcomes=False,bank_encoding='sparse-runs-v1',write_encoding='rep-runs-v1',owners=meta['owners'],cases=ref.compact(rows))
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
    report=dict(cases=len(rows),coverage={hex(e):dict(reached=len(reached&set(complete_body(code,e).code)),total=len(complete_body(code,e).code))for e in OWNED},stores=sum(len(r['writes'])for r in rows),reads=sum(len(r['reads'])for r in rows),fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=ref.sha(a.output),original_xbe_sha256=NEW_PIN,source_sha256=ref.sha(__file__),scratch=str(scratch),retained_raw_banks=False)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
