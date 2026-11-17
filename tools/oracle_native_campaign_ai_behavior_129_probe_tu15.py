#!/usr/bin/env python3
"""Physical behavior129 direction probes; stop at the genuine1CA560 entry."""
import argparse,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical
BASE=0x1050000;POOL=BASE;ACTORS=BASE+0x100;PROPERTIES=BASE+0x2000;PROWS=BASE+0x2100
GROUPS=BASE+0x3000;GROWS=BASE+0x3100;TRACKING=BASE+0x4000;TROWS=BASE+0x4100;REQUEST=BASE+0x5000
ENTRIES=(0x1ca900,0x1caa20,0x2673a0,0x30bd0)
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def make(image,name,*,entry=0x1ca900,flags=(2,),distances=(0x3f800000,),tracked=True,policy=0,
         facing=(0x3f800000,0,0),target=(0x40400000,0x40800000,0x40a00000),profile=physical.FP_PROFILES[0]):
    raw=bytearray([0xa5]*0x5100)
    def put(at,value,width=4):raw[at-BASE:at-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
    for p,v in((POOL+0x44,ACTORS),(PROPERTIES+0x44,PROWS),(GROUPS+0x44,GROWS),(TRACKING+0x44,TROWS),
              (ACTORS+0x58,0xe3000000 if flags else 0xffffffff)):
        put(p,v)
    for n,value in enumerate(facing):put(ACTORS+0x290+n*4,value)
    for n in range(3):put(ACTORS+0x238+n*4,0)
    for n,flag in enumerate(flags):
        row=PROWS+n*0x3c;put(row+8,0xe4000000+n);put(row+0x14,0xe5000000+n if tracked else 0xffffffff)
        put(row+0x18,0,2);put(row+0x27,flag,1);put(row+0x28,distances[n%len(distances)])
        put(row+0x2c,0xe3000000+n+1 if n+1<len(flags)else 0xffffffff)
        for lane,value in enumerate(target):
            put(TROWS+n*0x124+8+lane*4,value);put(GROWS+n*0xc4+0x5c+lane*4,value)
    private=ref.SP-0xc if entry==0x1ca900 else ref.SP-0x60
    child=ref.SP-0x28 if entry==0x1ca900 else ref.SP-0x98
    banks={BASE:raw,0x5020c8:POOL.to_bytes(4,'little'),0x50eef0:PROPERTIES.to_bytes(4,'little'),0x50eef4:GROUPS.to_bytes(4,'little'),
        0x50eeec:TRACKING.to_bytes(4,'little'),0x47d636:policy.to_bytes(2,'little'),0x45695c:image.read(0x45695c,4),
        0x46a000:image.read(0x46a000,0x1000),0x474d08:image.read(0x474d08,4),0x44ac28:image.read(0x44ac28,12),
        private:bytes([0x93]*(12 if entry==0x1ca900 else 96)),child:bytes([0x97]*16)}
    gp=[0x11331100+k*0x10101 for k in range(8)];gp[3]=REQUEST;gp[4]=ref.SP;gp[7]=0xe3620000
    rows=[dict(address=p,before=bytes(v).hex(),role='child_arguments'if p==child else'private_directions'if p==private else'retained')for p,v in sorted(banks.items())]
    return dict(name=name,entry=entry,gp=gp,matrix=0xe3620000,source=REQUEST,stack_arguments=[],banks=rows,
        before_fp=list(profile),live_x87=[],xmm=[[0x6b000000+r*4+k for k in range(4)]for r in range(8)],max_steps=2000,strict_writes=False,
        stop_before_entries=[0x1ca560],refusal_site=0x1ca9ff if entry==0x1ca900 else 0x1cac7d)
def inputs(image):
    rows=[]
    for p,profile in enumerate(physical.FP_PROFILES):
        def add(name,**kwargs):rows.append(make(image,name+f'_fp{p}',profile=profile,**kwargs))
        for flags in((),(0,),(1,),(0x80,),(0xff,),(2,),(3,),(2,2,3)):
            for tracked in(False,True):add(f'nearest_flags{flags}_tracked{tracked}',flags=flags,tracked=tracked)
        for distances in((bits(1.),bits(2.),bits(.5)),(bits(1.),bits(1.),bits(1.)),(0x7fc12345,0x7f812345,0x7f800000),(0,0x80000000,bits(-1.))):
            add(f'nearest_distances{distances}',flags=(2,2,2),distances=distances)
        for policy in(0,1,2):
            for target in((0,0,0),(bits(.0001),0,0),(0x80000000,0,0),(0x7f812345,0,0),(0x7fc12345,0,0),(bits(3),bits(4),bits(5))):
                add(f'nearest_policy{policy}_point{target}',policy=policy,target=target)
        for facing in((bits(1),0,0),(0,bits(1),0),(bits(1),bits(2),bits(3)),(0,0,bits(1)),(0,0,0),
                       (0x80000000,0,0x80000000),(1,1,1),(0x7f800000,0,bits(1)),(0x7f812345,0,0),(0,0,0x7fc12345)):
            add(f'fan_facing{facing}',entry=0x1caa20,facing=facing)
    for n,row in enumerate(rows):row['id']=n
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args()
    assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=ref.metadata(image,ENTRIES)
    owned=ref.prepare(image,meta,scratch);rows=physical.physical(owned,inputs(image),'whole');assert all(r['completed']for r in rows)
    reached=set().union(*(set(r['executed_sites'])for r in rows));code=_Code(image,'.text')
    coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code),missing=[hex(pc)for pc in sorted(set(complete_body(code,e).code)-reached)])for e in ENTRIES}
    counts=dict(cases=len(rows),whole=sum(r.get('stopped_at')==ref.STOP for r in rows),named_1ca560=sum(r.get('stopped_at')==0x1ca560 for r in rows))
    fixture=dict(fixture_kind='native-behavior-129-probe-v1',image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,bank_encoding='sparse-runs-v1',cases=ref.compact(rows))
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert encoded==a.verify.read_bytes(),'Independent original repeat differs'
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,**counts,coverage=coverage,fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=ref.sha(a.output),original_sha256=NEW_PIN,owned_scratch=str(scratch),repeat_identical=bool(a.verify),post_1ca560_suffix_claimed=False)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
