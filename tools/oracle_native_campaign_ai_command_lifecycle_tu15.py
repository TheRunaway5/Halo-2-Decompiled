#!/usr/bin/env python3
"""Unchanged physical i386 command activation and actual HS record construction."""
import argparse,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical
OWNED=(0x2801d0,0x261be0,0x261a90,0x261d50,0x262250,0x262190,0x262870,0x2629d0,0x263b80,
       0x2117e0,0x211a80,0x211cb0,0x211840,0x262590,0x262710)
ENTRIES=OWNED+(0x16e6a0,0x16e8a0,0x27caa0,0x27cb30,0x27c880,0x20cff0,0x20d030,0x20d070,0x20d0d0)
BASE=0x1050000;SIZE=0x18000;NONE=0xffffffff
AH,AR,CH,CR,HH,HR,JH,JR,PH,PR,OH,OR,SH,SR,GH,GR,SYH,SY,SC,SD,CLOCK,AIG=(BASE+n for n in
    (0,0x100,0x2500,0x2600,0x3500,0x3600,0x6000,0x6100,0x6900,0x6a00,0x7000,0x7100,0x7200,0x7300,
     0x7600,0x7700,0x7800,0x7900,0x8000,0x8400,0x9000,0x9100))
BODY=BASE+0xa000
def inputs(image,extra=False,identity=False):
    rows=[]
    def make(entry,mode,profile):
        data=bytearray([0xa5]*SIZE);banks={BASE:data}
        def put(p,v,width=4):data[p-BASE:p-BASE+width]=(v&((1<<(8*width))-1)).to_bytes(width,'little')
        def global_(p,v):banks[p]=struct.pack('<I',v)
        def pool(h,r,count,stride,live=0):
            data[h-BASE:h-BASE+0x4c]=bytes(0x4c);put(h+0x20,count);put(h+0x24,stride);put(h+0x29,1,1)
            put(h+0x34,live);put(h+0x38,live);put(h+0x3c,live);put(h+0x40,0x9123,2);put(h+0x44,r);put(h+0x48,h+0x50)
            put(h+0x50,(1<<live)-1)
            for i in range(live):put(r+i*stride,0x9123,2)
        for h,r,count,stride,live in ((AH,AR,4,0x888,4),(CH,CR,8,0xd4,3),(HH,HR,8,0x418,3),(JH,JR,4,0x8c,1),
            (PH,PR,4,0x34,1),(OH,OR,4,12,4),(SH,SR,4,0x98,1),(GH,GR,4,0x38,1),(SYH,SY,16,20,3)):
            pool(h,r,count,stride,live)
        for at,value in ((0x5020c8,AH),(0x50eee0,CH),(0x505e5c,HH),(0x50eedc,JH),(0x510fa8,PH),
            (0x4ecbd0,OH),(0x52be7c,SH),(0x52be80,GH),(0x505e6c,SYH),(0x4ecc20,SC),(0x51df54,CLOCK),(0x5020a8,AIG),
            (0x50ef00,NONE),(0x50ef04,0x12345678),(0x50eee4,NONE),(0x50eee8,NONE),(0x50eed4,NONE),(0x50eed8,NONE)):
            global_(at,value)
        banks[0x456910]=image.read(0x456910,4)
        aid=0x91230001;cid=0x91230001;hid=0x91230001;actor=AR+0x888;command=CR+0xd4;thread=HR+0x418
        put(AIG+1,1,1);put(AIG+0x14,aid);put(SR+0x68,aid);put(SR+0x74,NONE,2);put(SR+0x6c,0)
        put(GR+4,NONE);put(GR+8,0);put(GR+0xc,NONE);put(GR+0x10,NONE);put(SC+0x1bc,SD);put(SC+0x160,1)
        put(SC+0x164,SD+0x200);put(SD+0x200+0x48,1);put(SD+0x200+0x4c,SD+0x300);put(SD+0x300,17)
        for i in range(3):
            put(SD+i*0x28+0x20,5,2);put(SD+i*0x28+0x24,0x91230001)
            put(CR+i*0xd4+0x34,i);put(CR+i*0xd4+0x38,0x91230000+i);put(CR+i*0xd4+0x3c,NONE);put(CR+i*0xd4+0x40,NONE)
            put(CR+i*0xd4+0x44,0,1);put(CR+i*0xd4+0x7e,0,1);put(CR+i*0xd4+0x30,1,2)
            put(HR+i*0x418+8,2000);put(HR+i*0x418+0x10,HR+i*0x418+0x18);put(HR+i*0x418+0x24,0,2)
        for i in range(4):
            p=AR+i*0x888;put(p+7,0,1);put(p+0x18,0x91230000+i);put(p+0x1c,0x91230000);put(p+0x20,NONE)
            put(p+0x30,0);put(p+0x38,17);put(p+0x858,NONE);put(p+0x85c,NONE)
            put(OR+i*12+8,BODY+i*0x300);put(BODY+i*0x300+0x134,0);put(BODY+i*0x300+0x13a,0x160,2)
            put(BODY+i*0x300+0x16c,0x91230000+i+1 if i<3 else NONE)
        put(PR+0x18,0x91230000);put(CLOCK+8,1000);put(SY+20+6,0,2);put(SY+20+2,0,2)
        put(JR+0xa,2,2);put(JR+0xc,0x91230002)
        for i in range(2):put(JR+0x10+i*12,aid+i);put(JR+0x14+i*12,0x91230000+i);put(CR+i*0xd4+0x40,0x91230000)
        # Ordinary initialized records are independent inputs, not results from a dependency.
        gp=[0x11331100+n*0x10101 for n in range(8)];gp[4]=ref.SP;args=[];stops=[]
        if extra and entry in (0x211840,0x262590,0x262710):
            put(thread+8,[0xfffffffd,0,1,999,1000,0][mode]);stops=[0x211b10]
        if entry==0x261a90:
            gp[3]=NONE if mode<3 else hid;args=[mode%3]
            if mode==1:put(SD+0x28+0x20,3,2)
            if mode==2:put(HH+0x34,8);put(HH+0x38,8)
            if mode==4:put(CH+0x34,8);put(CH+0x38,8)
            if mode==5:put(CH+0x34,0);put(CH+0x50,6)
        elif entry in (0x261be0,0x261d50,0x2801d0):
            if mode in (1,2,3):put(actor+0x858,cid);put(actor+0x85c,cid);put(command+0x3c,0x91230002);put(CR+2*0xd4+0x40,NONE)
            if mode==2:global_(0x50ef00,0x80000001)
            if mode==3:put(command+0x40,0x91230000)
            if entry==0x261be0:gp[0]=NONE;args=[aid,1]
            elif entry==0x261d50:gp[0]=aid;gp[2]=1;args=[NONE]
            else:gp[1]=(0,0x40000000,0x80000001,0xc0000000,NONE,0)[mode];gp[7]=1
            if identity:
                put(SR+0x68,0x12340001);put(AIG+0x14,0x12340001)
                if mode==2:gp[1]=0
        elif entry==0x262250:
            gp[1]=aid;args=[cid];put(actor+0x858,cid);put(actor+0x85c,cid);put(command+0x3c,0x91230002)
            if mode==1:put(command+0x40,NONE)
            if mode==2:put(command+0x40,NONE);put(command+0x38,NONE)
            if mode==3:put(actor+0x858,0x91230000);put(CR+0x3c,cid)
        elif entry==0x262190:
            args=[0x91230000,aid]
            if mode==1:put(JR+0x1c,NONE)
            if mode==2:put(JR+0x14,NONE);put(JR+0x1c,NONE)
            if mode==3:put(JR+0xa,0,2)
            if mode==4:put(JR+0xc,NONE);put(JR+0xa,0,2)
        elif entry==0x262870:
            args=[aid,0x2629d0,cid]
            if mode:put(actor+7,1,1)
            if mode==2:put(actor+0x1c,NONE)
            if mode==3:put(PR+0x18,NONE)
            if mode==4:put(BODY+0x134,1)
        elif entry==0x2629d0:args=[aid,0x91230000,actor+0x864,cid]
        elif entry==0x263b80:
            gp[0]=cid;put(command+0xac,mode&1,1);put(command+0xb0,NONE if mode&2 else 0)
        elif entry==0x211a80:
            gp[0]=mode%3;args=[4]
            if mode==1:put(SD+0x28+0x20,1,2)
            if mode==2:gp[0]=NONE
            if mode==3:put(HH+0x34,8);put(HH+0x38,8)
        elif entry==0x2117e0:
            gp[0]=mode%3
            if mode==1:put(SD+0x28+0x20,3,2)
            if mode==2:put(HH+0x34,8);put(HH+0x38,8)
        elif entry==0x211cb0:
            gp[2]=hid;gp[3]=thread+0x14;args=[0x91230001]
            put(thread+0x24,mode*4,2)
        elif entry==0x211840:
            gp[0]=hid
            if not extra:put(thread+8,[NONE,0xfffffffe,1001,2000,NONE,0xfffffffe][mode])
        elif entry==0x262590:
            args=[aid,cid]
            if mode==0:put(command+0x38,NONE)
            if mode==1:put(command+0x44,1,1)
            if mode==3:put(actor+0x30,NONE)
        elif entry==0x262710:
            gp[3]=aid;gp[7]=cid;args=[BASE+0x9400]
            put(command+0x30,[1,2,4,1,1,2][mode],2)
            if mode==3:put(command+0x38,NONE)
            if mode==4:put(command+0x7e,1,1)
        elif entry==0x280400:
            gp[1]=(0,0x40000000,0x80000001,0xc0000000,NONE,0)[mode];args=[1]
            put(actor+0x858,0x91230000);put(actor+0x85c,cid)
            if mode==1:put(command+0x34,2)
            if mode==3:put(actor+0x85c,NONE)
            if mode==5:args=[0x10001]
        else:raise AssertionError(hex(entry))
        n=len(rows);args=args+[0,0]
        rows.append(dict(id=n,name=f'{entry:x}_m{mode}_fp{profile}',entry=entry,gp=gp,matrix=args[0],source=args[1],stack_arguments=args[2:],
            banks=[dict(address=p,before=bytes(v).hex()) for p,v in sorted(banks.items())],before_fp=list(profile),live_x87=[],
            xmm=[[0x6b000000+n*17+r*4+k for k in range(4)] for r in range(8)],stop_before_entries=stops,max_steps=100000,strict_writes=False))
    for profile in physical.FP_PROFILES[:3]:
        for entry in ((0x2801d0,) if identity else (0x280400,0x211840,0x262590,0x262710) if extra else OWNED):
            for mode in range(6):make(entry,mode,profile)
    return rows
def composed_inputs(image,kind3=False):
    import copy
    import oracle_native_campaign_ai_command_movement_tu15 as movement
    import oracle_native_campaign_ai_publication_tu15 as publication
    rows=[]
    prefixes=('init_point_0_','init_point_2_','update_r1_f0_a0_p1_',
              'update_r1_f0_a0_p3_','update_r0_f0_a0_p1_','composed_262a00_disabled0_resume0_')
    if kind3:
        prefixes=('kind3_init_face1_point0_','kind3_init_face2_point0_',
                  'kind3_update_r0_active0_path1_',
                  'kind3_turn_delta(1.0, 0.0, 0.0)_facing(1.0, 0.0, 0.0)_',
                  'kind3_turn_delta(1.0, 0.0, 0.0)_facing(0.0, 1.0, 0.0)_',
                  'kind3_composed_262a00_','kind3_armory_zapper_262a00_')
    for seed in movement.inputs(image,kind3):
        if not seed['name'].startswith(prefixes):continue
        row=copy.deepcopy(seed);banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}
        hs=0x1170000;hr=hs+0x100;clock=hs+0x600;status=hs+0x610
        banks[hs]=bytearray(0x700)
        def put(at,value,size=4):
            for base,raw in banks.items():
                if base<=at and at+size<=base+len(raw):raw[at-base:at-base+size]=(value&((1<<(size*8))-1)).to_bytes(size,'little');return
            banks[at]=(value&((1<<(size*8))-1)).to_bytes(size,'little')
        put(hs+0x44,hr);put(hr,0xcafe,2);put(hr+8,2000);put(clock+8,1000)
        put(0x505e5c,hs);put(0x51df54,clock)
        for p in (0x50eed4,0x50eed8,0x50eee4,0x50eee8,0x50ef00,0x50ef04):put(p,NONE)
        put(movement.ROWS+0x38,0xcafe0000);put(movement.ROWS+0x30,3 if seed['entry']==0x262a00 else 0,2)
        row['entry']=0x262710;row['gp'][3]=publication.ACTOR;row['gp'][7]=movement.COMMAND
        row['matrix']=status;row['source']=0;row['stack_arguments']=[]
        row['name']='scheduler_'+seed['name'];row['id']=len(rows);row['native_movement_children']=True
        row['banks']=[dict(address=at,before=bytes(raw).hex()) for at,raw in sorted(banks.items())]
        row['stop_before_entries'] += [0x211b10];rows.append(row)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--select');p.add_argument('--verify',type=Path);p.add_argument('--extra',action='store_true');p.add_argument('--identity',action='store_true');p.add_argument('--composed',action='store_true');p.add_argument('--kind3',action='store_true');a=p.parse_args()
    if a.kind3 and not a.composed:p.error('--kind3 requires --composed')
    assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN;scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT)
    entries=ENTRIES+(0x280400,) if a.extra else ENTRIES
    if a.composed:
        import oracle_native_campaign_ai_command_movement_tu15 as movement
        entries=tuple(dict.fromkeys(entries+movement.ENTRIES))
    meta=ref.metadata(image,entries);owned=ref.prepare(image,meta,scratch);cases=composed_inputs(image,a.kind3) if a.composed else inputs(image,a.extra,a.identity)
    if a.select:
        import re
        cases=[r for r in cases if re.search(a.select,r['name'])]
        for n,r in enumerate(cases):r['id']=n
    rows=physical.physical(owned,cases,'whole')
    assert all(r['completed'] and r.get('stopped_at',ref.STOP) in (ref.STOP,0x211b10) for r in rows)
    reached=set().union(*(set(r['executed_sites']) for r in rows));code=_Code(image,'.text')
    coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code)) for e in entries}
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,
        bank_encoding='sparse-runs-v1',write_encoding='rep-runs-v1',cases=ref.compact(rows))
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert encoded==a.verify.read_bytes()
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(cases=len(rows),coverage=coverage,stores=sum(len(r['writes']) for r in rows),fixture=str(a.output),fixture_bytes=len(encoded),
        fixture_sha256=ref.sha(a.output),original_sha256=NEW_PIN,owned_scratch=str(scratch),large_artifacts_retained=[],repeat_identical=bool(a.verify))
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
