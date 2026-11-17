#!/usr/bin/env python3
"""Unchanged physical279100 paired navigation endpoints, including real MCC link metadata."""
import argparse,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical
ENTRIES=(0x279100,0x30bd0)
BASE=0x1050000;NAV=BASE;LINKS=BASE+0x100;REQUEST=BASE+0x180;PREVIOUS=BASE+0x240
FIRST=BASE+0x280;SECOND=BASE+0x2a0;SECTOR_A=BASE+0x2c0;SECTOR_B=BASE+0x2c4;VERTICES=BASE+0x1000
NONE=0xffffffff
JOHNSON_WORDS=(1,2826,15316,15317,15318,15319,32512,708,13,65535)
JOHNSON_VERTICES=((1064152217,1062927347,1036832620),(1064161584,1047075364,1036832539),
    (1105993362,3258191823,1077936156),(1105760687,3258309471,1077936156))
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def make(image,name,*,kind=1,vertices=((0.,0.,0.),(4.,0.,0.),(10.,3.,2.),(10.,11.,2.)),
    previous=(1.,1.,.5),radius=.3,profile=physical.FP_PROFILES[0],frames=(0xffff,0xffff),actor=0xe3620001,missing=False,johnson=False):
    data=bytearray([0xa5]*0x400)
    def put(address,value,width=4):data[address-BASE:address-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
    def point(address,values,frame):
        for n,value in enumerate(values):put(address+n*4,bits(value))
        put(address+12,0x13570000|frame)
    put(NAV+0x2c,VERTICES);put(NAV+0x3c,LINKS);put(REQUEST,bits(radius));put(REQUEST+0x8c,NAV)
    words=list(JOHNSON_WORDS) if johnson else [kind,0xffff,0,1,2,3,0,0x2c4,frames[0],frames[1]]
    words[0]=kind
    for n,value in enumerate(words):put(LINKS+n*2,value,2)
    point(PREVIOUS,previous,frames[0]);point(FIRST,(-99.,-98.,-97.),0x2222);point(SECOND,(-88.,-87.,-86.),0x3333)
    put(SECTOR_A,0xdeadbeef);put(SECTOR_B,0xbeefcafe)
    vertex_bits=JOHNSON_VERTICES if johnson else tuple(tuple(bits(v) for v in row) for row in vertices)
    args=[actor,REQUEST,0 if missing else PREVIOUS,0xe94,SECTOR_A,SECOND,SECTOR_B]
    banks={BASE:data,VERTICES+words[2]*12:b''.join(v.to_bytes(4,'little') for row in vertex_bits for v in row),
        0x46a000:image.read(0x46a000,0x1000),ref.SP+4:b''.join(v.to_bytes(4,'little') for v in args)}
    gp=[0x11331100+n*0x10101 for n in range(8)];gp[0]=NAV;gp[1]=0;gp[4]=ref.SP;gp[7]=FIRST
    row=dict(name=name,entry=0x279100,gp=gp,matrix=args[0],source=args[1],stack_arguments=args[2:],
        banks=[dict(address=p,before=bytes(v).hex()) for p,v in sorted(banks.items())],before_fp=list(profile),live_x87=[],
        xmm=[[0x6b000000+r*4+k for k in range(4)] for r in range(8)],max_steps=2000,strict_writes=False,
        stop_before_entries=[0x27a110],refusal_site=0x27957e if kind==2 else 0x279479)
    if johnson:row['mcc_metadata_witness']=dict(words=list(JOHNSON_WORDS),vertices_xyz_bits=JOHNSON_VERTICES,
        map_sha256='37022bb3b9afa554a8566a065111fcf095a5b23f073b8922b935d1ceb47b4af1',live_link_index=0xb0d,
        scope='Exact link and vertex bytes; independently varied request margin and previous point, not a captured actor call')
    return row

def inputs(image):
    rows=[]
    geometries=(('straight',((0.,0.,0.),(4.,0.,0.),(10.,3.,2.),(10.,11.,2.))),
        ('first_zero',((1.,2.,3.),(1.,2.,3.),(10.,3.,2.),(10.,11.,2.))),
        ('second_zero',((0.,0.,0.),(4.,0.,0.),(10.,3.,2.),(10.,3.,2.))),
        ('oblique',((.1,.2,.3),(1.1,2.2,3.3),(-4.,8.,.01),(3.,.5,2.))))
    for profile in physical.FP_PROFILES:
        for kind in (1,6):
            for geometry,vertices in geometries:
                for radius in (0.,.3,3.):
                    for projection in (-3.,0.,.25,1.,8.):
                        rows.append(make(image,f'kind{kind}_{geometry}_r{radius}_p{projection}_fp{profile}',kind=kind,vertices=vertices,
                            previous=(projection,projection*.25,.75),radius=radius,profile=profile,frames=(13,0xffff) if kind==1 else (0xffff,2)))
            rows.append(make(image,f'kind{kind}_null_previous_fp{profile}',kind=kind,profile=profile,missing=True))
        for kind in (0,3,4,7,0x7fff,0x8000,0xffff):rows.append(make(image,f'invalid_kind{kind}_fp{profile}',kind=kind,profile=profile))
        for kind in (2,5):rows.append(make(image,f'child_prefix_kind{kind}_fp{profile}',kind=kind,profile=profile))
        for frame in (0,13,0xffff):rows.append(make(image,f'actor_none_kind5_frame{frame}_fp{profile}',kind=5,profile=profile,actor=NONE,frames=(frame,0xffff)))
        for radius in (0.,.1,.3):
            for projection in (-1.,.25,.5,.75,2.):
                a,b=JOHNSON_VERTICES[:2]
                aa=[struct.unpack('<f',v.to_bytes(4,'little'))[0] for v in a];bb=[struct.unpack('<f',v.to_bytes(4,'little'))[0] for v in b]
                previous=tuple(x+(y-x)*projection for x,y in zip(aa,bb))
                rows.append(make(image,f'johnson_linkB0D_r{radius}_p{projection}_fp{profile}',profile=profile,johnson=True,
                    previous=previous,radius=radius,frames=(13,0xffff)))
    for n,row in enumerate(rows):row['id']=n
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True);parser.add_argument('--verify',type=Path);parser.add_argument('--select')
    args=parser.parse_args();assert not args.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=ref.metadata(image,ENTRIES)
    owned=ref.prepare(image,meta,scratch);rows=inputs(image)
    if args.select:
        import re
        rows=[row for row in rows if re.search(args.select,row['name'])]
        for n,row in enumerate(rows):row['id']=n
    rows=physical.physical(owned,rows,'whole');assert all(row['completed'] for row in rows)
    reached=set().union(*(set(row['executed_sites']) for row in rows));code=_Code(image,'.text')
    coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code),
        missing=[hex(pc) for pc in sorted(set(complete_body(code,e).code)-reached)]) for e in ENTRIES}
    fixture=dict(fixture_kind='native-route-special-link-v1',image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',
        owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,bank_encoding='sparse-runs-v1',cases=ref.compact(rows))
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if args.verify:assert encoded==args.verify.read_bytes(),'Independent original repeat differs'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(encoded)
    report=dict(passed=not args.select,cases=len(rows),whole=sum(row.get('stopped_at',ref.STOP)==ref.STOP for row in rows),
        named_27a110_prefixes=sum(row.get('stopped_at')==0x27a110 for row in rows),coverage=coverage,
        stores=sum(len(row['writes']) for row in rows),johnson_metadata_cases=sum('mcc_metadata_witness' in row for row in rows),
        fixture=str(args.output),fixture_bytes=len(encoded),fixture_sha256=ref.sha(args.output),original_sha256=NEW_PIN,
        owned_scratch=str(scratch),retained_large_artifacts=[],repeat_identical=bool(args.verify))
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))

if __name__=='__main__':main()
