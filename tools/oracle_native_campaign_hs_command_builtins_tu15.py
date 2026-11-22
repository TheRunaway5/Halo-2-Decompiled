#!/usr/bin/env python3
"""Physical unchanged TU1.5 HS builtins and genuine scheduler/parser parents."""
import argparse,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as ref
import oracle_native_campaign_ai_path_search_tu15 as physical
OWNED=(0x2b2470,0x2b2670,0x210a10)
LOOK_OWNED=(0x2b1000,0x2b10b0,0x2b11b0,0x2b1300,0x2b2410)
TWO_ARGUMENTS=(0x2b0e30,0x2b1200,0x2b1000,0x2b10b0,0x2b11b0,0x2b1300)
COMBAT_ABORT=0x2b2340
ENTRIES=OWNED+(0x211b10,0x212010,0x211cb0,0x211da0,0x212560)
BASE=0x1050000;HH=BASE;HR=BASE+0x100;CH=BASE+0x1000;CR=BASE+0x1100
SH=BASE+0x1800;SY=BASE+0x1900;OUT=BASE+0x2000;NONE=0xffffffff

def make(image,entry,value=0,index=1,command=True,parent=False,optional=False,profile=(0x37f,0,0x1f80),point=0):
    data=bytearray(0x2800);banks={BASE:data}
    def put(p,v,w=4):
        if BASE<=p and p+w<=BASE+len(data):data[p-BASE:p-BASE+w]=(v&((1<<(8*w))-1)).to_bytes(w,'little')
        else:banks[p]=(v&((1<<(8*w))-1)).to_bytes(w,'little')
    thread=HR+index*0x418;hid=(0x8306<<16)|index;cmd=CR+index*0xd4
    put(HH+0x44,HR);put(CH+0x44,CR);put(SH+0x44,SY)
    put(0x4ecc20,BASE+0x2200);put(BASE+0x2200+0x1bc,BASE+0x2500);put(BASE+0x2500+0x20,5,2)
    for p,v in ((0x505e5c,HH),(0x505e6c,SH),(0x50eee0,CH),(0x50eee8,(0xa7230000|index) if command else NONE),
                (0x505e64,NONE),(0x505e60,1),(0x4f3218,0)) :put(p,v)
    put(thread,0x8306,2);put(thread+2,4,1);put(thread+3,1,1);put(thread+8,12345)
    base=thread+0x18;frame=thread+0x40
    put(thread+0x10,frame);put(base,0);put(base+4,NONE);put(base+8,OUT);put(base+0xc,0,2)
    put(frame,base);put(frame+4,0x9abc0000);put(frame+0xc,0,2)
    opcode={0x2b2470:443,0x2b2670:448,0x210a10:20,0x2b1200:398,0x2b1000:392,0x2b10b0:394,0x2b11b0:397,0x2b1300:400,0x2b2410:442,0x2b0e30:388,COMBAT_ABORT:440}[entry]
    descriptor=int.from_bytes(image.read(0x480c00+opcode*4,4),'little')
    banks[0x480c00+opcode*4]=image.read(0x480c00+opcode*4,4);banks[descriptor]=image.read(descriptor,18 if entry in TWO_ARGUMENTS else 16)
    for node in range(4 if entry in TWO_ARGUMENTS else 3):put(SY+node*20,0x9abc,2);put(SY+node*20+8,NONE)
    put(SY+2,opcode,2);put(SY+4,4,2);put(SY+6,0,2);put(SY+16,0x9abc0001)
    put(SY+20+2,2,2);put(SY+20+4,2,2);put(SY+20+6,1,2)
    put(SY+20+8,0x9abc0002 if entry!=0x210a10 or optional else NONE)
    literal_type=28 if entry==0x2b0e30 else 7 if entry in (0x210a10,COMBAT_ABORT) else 5
    put(SY+40+2,literal_type,2);put(SY+40+4,literal_type,2)
    put(SY+40+6,1,2);put(SY+40+16,value)
    if entry in TWO_ARGUMENTS:
        put(SY+40+8,0x9abc0003);put(SY+60+2,50 if entry in (0x2b10b0,0x2b11b0,0x2b1300) else 28,2);put(SY+60+4,50 if entry in (0x2b10b0,0x2b11b0,0x2b1300) else 28,2)
        put(SY+60+6,1,2);put(SY+60+16,point)
        for off in (0x46,0x50,0x51,0x52):put(cmd+off,0xa5,1)
        put(cmd+0x48,0x1234,2);put(cmd+0x4c,0xdeadbeef);put(cmd+0x54,0x5678,2);put(cmd+0x58,0xbeefcafe)
    put(cmd+0x80,0x3c,1) if entry==0x2b2410 else None
    put(cmd+0x81,0x5a,1);put(cmd+0x76,0xa5,1);put(OUT,0x12345678)
    if entry==COMBAT_ABORT:
        put(cmd+0x7b,0xa5,1);put(cmd+0x7c,0x236b,2);put(cmd+0x7e,0xcf,1)
    gp=[0x11331100+n*0x10101 for n in range(8)];gp[4]=ref.SP;gp[7]=hid
    initial=0x83060001 if entry==COMBAT_ABORT else 1
    return dict(name=(f'point{point:x}_' if entry in TWO_ARGUMENTS else '')+f'{entry:x}_{"parent" if parent else "direct"}_v{value:x}_i{index}_cmd{int(command)}_optional{int(optional)}_fp{profile}',entry=0x211b10 if parent else entry,
        owned_entry=entry,gp=gp,matrix=opcode,source=hid,stack_arguments=[initial],
        banks=[dict(address=p,before=bytes(v).hex()) for p,v in sorted(banks.items())],before_fp=list(profile),live_x87=[],
        xmm=[[0x6b000000+r*4+k for k in range(4)] for r in range(8)],stop_before_entries=[0x212130],max_steps=10000,strict_writes=False)

def inputs(image):
    rows=[]
    for fp in physical.FP_PROFILES:
        for entry in OWNED[:2]:
            for value in (0,1,0x80,0xffffffff):
                for index in (0,1,2):
                    for command in (False,True):rows.append(make(image,entry,value,index,command,True,profile=fp))
                rows.append(make(image,entry,value,profile=fp))
        for index in (0,1,2):
            for parent in (False,True):rows.append(make(image,0x210a10,index=index,parent=parent,profile=fp))
        rows.append(make(image,0x210a10,value=2,optional=True,profile=fp))
    for n,row in enumerate(rows):row['id']=n
    return rows

def combat_abort_inputs(image):
    rows=[]
    for index,value in enumerate((0,5,0x8000,0xffff)):
        fp=physical.FP_PROFILES[index%len(physical.FP_PROFILES)]
        for command in (False,True):
            for parent in (False,True):
                rows.append(make(image,COMBAT_ABORT,value,index,command,parent,profile=fp))
    for n,row in enumerate(rows):
        row['id']=n
    return rows

def face_inputs(image,entries=(0x2b1200,)):
    all_rows=[]
    for entry in entries:
        all_rows.extend(target_inputs(image,entry))
    for n,row in enumerate(all_rows):row["id"]=n
    return all_rows

def target_inputs(image,entry):
    rows=[]
    for fp in physical.FP_PROFILES:
        for value in (0,1,0x80,0xffffffff):
            for index in (0,1,2):
                for command in (False,True):
                    for point in ((0,1,0xffff,0x10002) if entry in TWO_ARGUMENTS else (0,)):
                        rows.append(make(image,entry,value,index,command,True,profile=fp,point=point))
            rows.append(make(image,entry,value,profile=fp,point=0x10002))
    for n,row in enumerate(rows):row['id']=n
    return rows

def go_face_inputs(image):
    rows=[]
    def begin(row,destination,facing):
        banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}
        raw=banks[BASE]
        def put(at,value,width=4):raw[at-BASE:at-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
        def node(n,op,kind,flags,next=NONE,value=0):
            for off,v,w in ((0,0x9abc,2),(2,op,2),(4,kind,2),(6,flags,2),(8,next,4),(12,0,4),(16,value,4)):put(SY+n*20+off,v,w)
        datum=lambda n:0x9abc0000|n
        node(0,0,4,0,value=datum(1));node(1,2,2,1,next=datum(2))
        node(2,388,4,0,value=datum(3));node(3,2,2,1,next=datum(4))
        node(4,28,28,1,next=datum(5),value=destination);node(5,28,28,1,value=facing)
        descriptor=int.from_bytes(image.read(0x480c00,4),'little');banks[0x480c00]=image.read(0x480c00,4);banks[descriptor]=image.read(descriptor,16)
        row['banks']=[dict(address=p,before=bytes(v).hex()) for p,v in sorted(banks.items())]
        row['name']='begin_'+row['name']
    for fp in physical.FP_PROFILES:
        for destination in (0,2,0x10001):
            for facing in (0,3,0x10002):
                for index in (0,1,2):
                    for command in (False,True):
                        for enclosed in (False,True):
                            row=make(image,0x2b0e30,destination,index,command,True,profile=fp,point=facing)
                            if enclosed:begin(row,destination,facing)
                            rows.append(row)
            rows.append(make(image,0x2b0e30,destination,profile=fp,point=0x10002))
    for n,row in enumerate(rows):row['id']=n
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--face',action='store_true');p.add_argument('--look',action='store_true');p.add_argument('--go-face',action='store_true');p.add_argument('--combat-abort',action='store_true');p.add_argument('--select');p.add_argument('--verify',type=Path);a=p.parse_args()
    assert not a.output.exists();assert ref.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);meta=ref.metadata(image,ENTRIES+(COMBAT_ABORT,) if a.combat_abort else ENTRIES+(0x2b0e30,0x20fee0) if a.go_face else ENTRIES+LOOK_OWNED+(0x280d70,) if a.look else ENTRIES+(0x2b1200,0x280c90) if a.face else ENTRIES);owned=ref.prepare(image,meta,scratch);rows=combat_abort_inputs(image) if a.combat_abort else go_face_inputs(image) if a.go_face else face_inputs(image,LOOK_OWNED) if a.look else face_inputs(image) if a.face else inputs(image)
    if a.select:
        import re
        rows=[r for r in rows if re.search(a.select,r['name'])]
        for n,r in enumerate(rows):r['id']=n
    rows=physical.physical(owned,rows,'whole');assert all(r['completed'] for r in rows)
    reached=set().union(*(set(r['executed_sites']) for r in rows));code=_Code(image,'.text')
    coverage={hex(e):dict(reached=len(set(complete_body(code,e).code)&reached),total=len(complete_body(code,e).code),missing=[hex(pc) for pc in sorted(set(complete_body(code,e).code)-reached)]) for e in ((COMBAT_ABORT,) if a.combat_abort else (0x2b0e30,) if a.go_face else LOOK_OWNED+(0x280d70,) if a.look else (0x2b1200,0x280c90) if a.face else OWNED)}
    if a.combat_abort and not a.select:
        assert not coverage[hex(COMBAT_ABORT)]['missing']
        # Direct entry stops after the actual parser schedules its subexpression.
        # The genuine evaluator parent resumes it and publishes only with a command.
        assert sum(0x2b2395 not in row['executed_sites'] for row in rows)==8
        status_writes=[w for row in rows for w in row['writes'] if w['instruction']==0x2b238c]
        assert [w['before'] for w in status_writes]==['6b23']*4
        assert [w['written'] for w in status_writes]==['0000','0500','0080','ffff']
    fixture=dict(fixture_kind='native-hs-command-builtins-v1',image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,
        bank_encoding='sparse-runs-v1',cases=ref.compact(rows))
    encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert encoded==a.verify.read_bytes()
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=not a.select,cases=len(rows),coverage=coverage,stores=sum(len(r['writes']) for r in rows),fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=ref.sha(a.output),original_sha256=NEW_PIN,owned_scratch=str(scratch),large_artifacts_retained=[],repeat_identical=bool(a.verify))
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
