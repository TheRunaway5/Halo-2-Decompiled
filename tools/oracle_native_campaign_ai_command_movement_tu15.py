#!/usr/bin/env python3
"""Unchanged physical TU1.5 command kind1 initialization/update/goal producer.

Uses real command/actor/scenario banks and native publication child inputs.
No supplied child outcome, instruction patch, simulated motor or position.
"""
import argparse,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as search
import oracle_native_campaign_ai_publication_tu15 as publication

OWNED=(0x262a00,0x263140,0x263aa0,0x263bd0,0x264860)
ENTRIES=tuple(dict.fromkeys(OWNED+(0x2b3e0,0x30bd0,0x263b80,0x1fc720,0x2009b0,0x2009e0,0x200920,0x2b0bf0)+publication.ENTRIES))
COMMAND=0xd3010000
DATA=0x1160000
POOL,ROWS,BLOCK,SETS,POINTS=tuple(DATA+n for n in(0,0x100,0x300,0x340,0x400))

def inputs(image,kind3=False):
    rows=[]
    def case(entry,label,*,pointref=0,nav=0,unit=publication.UNIT,active=0,ready=0,failsafe=0,path=1,moving=1,
             action=2,disabled=0,resume=0,radius=0,flags=0,count=0,fp=None,newgoal=False,context=COMMAND,
             kind=1,face=1,delta=(1.,1.,0.),orientation=(1.,0.,0.),wantface=False,swarm=0xffffffff,mode270=0,origin=None,goal_frame=0xffff,face_frame=0xffff):
        s=publication.Scene();publication.standard_zone(s)
        s.actor(publication.ACTOR,unit=unit,active=active,action=action,path=path,disabled=disabled,resume=resume,
            point=(1.,2.,3.,0xffff),raw={0x1c:swarm,0x4c8:nav,0x4d5:(int(wantface),1),0x270:(mode270,2),0x506:(0,1),0x50c:(moving,1),
                0x5b4:(0,2),0x858:COMMAND,0x85c:COMMAND},keep=(0x4c8,0x4d5,0x506,0x50c,0x5b4))
        ar=publication.Scene.row(publication.ACTOR)
        base_point=origin or (2. if newgoal else 1.,2.,3.)
        if kind==3:
            for i,v in enumerate(origin or (1.,2.,3.)):s.put(ar+0x4b8+i*4,publication.fv(v))
            s.put(ar+0x4c4,goal_frame)
        s.put(publication.SCEN+0x1d8,1);s.put(publication.SCEN+0x1dc,BLOCK)
        if kind==3:
            s.ws[:]=bytes(len(s.ws)) if not active else s.ws
            for i,v in enumerate(orientation):s.put(ar+0x290+i*4,publication.fv(v))
            for i,v in enumerate(delta):s.put(ar+0x4d8+i*4,publication.fv(v))
        s.put(ar+0x864,count,2);s.put(ar+0x867,flags,1);s.put(ar+0x86c,count,2)
        data=bytearray(0x800)
        def put(at,value,size=4):data[at-DATA:at-DATA+size]=(value&((1<<(size*8))-1)).to_bytes(size,'little')
        put(POOL+0x44,ROWS);put(ROWS,COMMAND>>16,2);put(ROWS+4,kind,2);put(ROWS+0x2c,face);put(ROWS+8,radius);put(ROWS+0x28,pointref)
        put(ROWS+0x76,failsafe,1);put(ROWS+0x77,ready,1);put(ROWS+0xb0,0xffffffff)
        put(BLOCK,2);put(BLOCK+4,SETS)
        for i in range(2):
            put(SETS+i*0x30+0x20,2);put(SETS+i*0x30+0x24,POINTS+i*0x80)
            for n in range(2):
                p=POINTS+i*0x80+n*0x3c
                for a,v in enumerate(base_point):put(p+0x20+a*4,publication.f(v))
                if kind==3 and n==1:
                    for a,v in enumerate(delta):
                        value=v if isinstance(v,int) else publication.f(base_point[a]+v)
                        put(p+0x20+a*4,value)
                put(p+0x2c,0x7ab10000|(face_frame if kind==3 and n==1 else goal_frame));put(p+0x30,nav)
        row=publication.make(image,rows,label,entry,s,(publication.ACTOR,unit,ar+0x864,COMMAND),fp=fp)
        row['banks'] += [dict(address=DATA,before=data.hex()),dict(address=0x50eee0,before=struct.pack('<I',POOL).hex()),
                         dict(address=0x50eee8,before=struct.pack('<I',context).hex())]
        row.update(compare_reads=False,strict_store_pcs=False,stop_before_entries=list(publication.STOP_ENTRIES))
        row['gp'][0]=COMMAND;row['gp'][1]=publication.ACTOR;row['gp'][2]=pointref
        if entry==0x2b0c1b:
            row['gp'][1]=pointref;row['stop_before_entries']=[0x2b0c4a];row['producer_region']=True
        return row
    if not kind3:
        for fp in publication.FP_PROFILES[:4]:
            for pointref in (0,1,0x10000,0x10001,2,0x20000,0xffffffff):
                case(0x264860,f'valid_{pointref:x}_{fp}',pointref=pointref,fp=fp)
                case(0x262a00,f'init_point_{pointref:x}_{fp}',pointref=pointref,fp=fp)
            for nav in (0,0xffffffff):
                for radius in (0,0x80000000,0x3f000000,0x7fc00000):
                    case(0x263bd0,f'goal_nav{nav:x}_radius{radius:x}_{fp}',nav=nav,radius=radius,fp=fp)
            for ready in (0,1):
                for failsafe in (0,1):
                    for active in (0,1):
                        for path in (0,1,2,3):
                            case(0x263140,f'update_r{ready}_f{failsafe}_a{active}_p{path}_{fp}',ready=ready,failsafe=failsafe,active=active,path=path,fp=fp)
            for unit in (publication.UNIT,0xffffffff):
                for fail in (0,1):case(0x262a00,f'init_unit{unit:x}_fail{fail}_{fp}',unit=unit,failsafe=fail,nav=0xffffffff,fp=fp)
                case(0x263aa0,f'cleanup_unit{unit:x}_{fp}',unit=unit,fp=fp)
            for flags in (0,1,12):
                for count in (0,1,7,0xffff):case(0x263140,f'common_flags{flags}_count{count}_{fp}',ready=1,path=1,flags=flags,count=count,fp=fp)
            for entry in (0x263bd0,0x262a00,0x263140):
                for disable,resume in ((1,0),(0,1),(0,0)):
                    case(entry,f'composed_{entry:x}_disabled{disable}_resume{resume}_{fp}',active=1,disabled=disable,resume=resume,newgoal=True,fp=fp)
            for point in (0,2,0x10001,0xffffffff):case(0x2b0c1b,f'producer_{point:x}_{fp}',pointref=point,fp=fp)
            for point in (0,2):case(0x2b0c1b,f'producer_no_context_{point:x}_{fp}',pointref=point,fp=fp,context=0xffffffff)
    else:
        for fi,fp in enumerate(publication.FP_PROFILES):
            for face in (0,1,0x10001,2,0x20000,0xffffffff):
                for point in (0,2):
                    case(0x262a00,f'kind3_init_face{face:x}_point{point:x}_{fi}',kind=3,face=face,pointref=point,count=7,fp=fp)
            for nav in (0,0xffffffff):
                for delta in ((1.,1.,0.),(0.,0.,0.),(0.,0.,1.),(-2.,.25,1.),(0x7fc12345,1.,0.),(0x7f800001,0.,0.)):
                    case(0x263bd0,f'kind3_goal_nav{nav:x}_delta{delta}_{fi}',kind=3,nav=nav,delta=delta,fp=fp)
            for ready in (0,1):
                for active in (0,1):
                    for path in (0,1,2,3):
                        case(0x263140,f'kind3_update_r{ready}_active{active}_path{path}_{fi}',kind=3,ready=ready,active=active,path=path,fp=fp)
            for delta in ((1.,0.,0.),(0.,1.,0.),(0.,0.,1.),(-1.,0.,0.),(.3,.4,0.),(1e-8,0.,0.),(0x7fc12345,1.,0.),(0x7f800001,0.,0.)):
                for facing in ((1.,0.,0.),(0.,1.,0.),(-1.,0.,0.)):
                    case(0x263140,f'kind3_turn_delta{delta}_facing{facing}_{fi}',kind=3,ready=1,path=2,wantface=True,delta=delta,orientation=facing,fp=fp)
            for unit in (publication.UNIT,0xffffffff):
                case(0x262a00,f'kind3_init_unit{unit:x}_{fi}',kind=3,unit=unit,count=7,fp=fp)
                case(0x263aa0,f'kind3_cleanup_unit{unit:x}_{fi}',kind=3,unit=unit,fp=fp)
            case(0x262a00,f'kind3_init_swarm_{fi}',kind=3,swarm=0x12340000,count=7,fp=fp)
            for goal_frame,face_frame in ((0,0xffff),(3,0xffff),(0xffff,0),(0,0)):
                for entry in (0x262a00,0x263bd0):
                    case(entry,f'kind3_frames_{entry:x}_{goal_frame}_{face_frame}_{fi}',kind=3,goal_frame=goal_frame,face_frame=face_frame,fp=fp)
            # Exact imported Armory zapper/zapper_face REAL coordinates.
            origin=(32.26728057861328,-45.42582321166992,3.0002501010894775)
            target=(33.18600845336914,-46.38871765136719,3.706341028213501)
            direction=tuple(b-a for a,b in zip(origin,target))
            for entry in (0x262a00,0x263bd0,0x263140):
                case(entry,f'kind3_armory_zapper_{entry:x}_{fi}',kind=3,origin=origin,delta=direction,ready=1,path=2,wantface=True,fp=fp)

            for entry in (0x262a00,0x263bd0,0x263140):
                case(entry,f'kind3_composed_{entry:x}_{fi}',kind=3,active=1,newgoal=True,fp=fp)
    return rows

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--select');ap.add_argument('--kind3',action='store_true')
    args=ap.parse_args();assert not args.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=ref.metadata(image,ENTRIES);owned=ref.prepare(image,meta,scratch)
    selected=inputs(image,args.kind3)
    if args.select:
        import re
        selected=[r for r in selected if re.search(args.select,r['name'])]
    for n,row in enumerate(selected):row['id']=n
    rows=search.physical(owned,selected,'whole');assert all(r['completed'] for r in rows)
    reached=set().union(*(set(r['executed_sites'])for r in rows));code=_Code(image,'.text')
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',instruction_patches=[],supplied_original_outcomes=False,
        bank_encoding='sparse-runs-v1',write_encoding='rep-runs-v1',owners=meta['owners'],cases=ref.compact(rows))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
    report=dict(cases=len(rows),coverage={hex(e):dict(reached=len(reached&set(complete_body(code,e).code)),total=len(complete_body(code,e).code))for e in OWNED},
        stores=sum(len(r['writes'])for r in rows),reads=sum(len(r['reads'])for r in rows),fixture=str(args.output),fixture_bytes=args.output.stat().st_size,
        fixture_sha256=ref.sha(args.output),original_xbe_sha256=NEW_PIN,source_sha256=ref.sha(__file__),scratch=str(scratch),retained_raw_banks=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
