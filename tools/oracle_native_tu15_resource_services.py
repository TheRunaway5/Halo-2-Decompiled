"""Opus audit probe (2026-11-02): genuine TU1.5 130980 services on unchanged physical i386.

Run from the main checkout:
  python3 tools/run_test.py --scratch-root $PWD/.local/test-scratch -- \
    $PWD/.tools/python/bin/python -I <this file> <pinned TU1.5 default.xbe>
All transient ELF/observer/JSON outputs live in HALO2_TEST_SCRATCH and are removed by run_test.py.
No instruction patches, no supplied callee outcomes, no kernel-import stubs (.rdata 0x41B000 stays unmapped).
"""
import sys,json,os,subprocess,struct,hashlib,argparse
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'));sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
import oracle_native_tu15_model_mask_order as base
from xbe import Xbe
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
xbe=args.xbe;assert hashlib.sha256(xbe.read_bytes()).hexdigest()==base.PIN
image=Xbe(xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
DATA=0x1000000;STACK=0x62010000;SP=base.SP;STOP=base.STOP
base.SECTIONS_MAP={'objects':(0x4ec000,4096),'ioq':(0x4ef000,4096),'tags':(0x4f0000,4096),'resources':(0x4f2000,4096),
  'mainloop':(0x4f3000,4096),'policy':(0x475000,4096),'files':(0x566000,0x2000),'filesel':(0x569000,4096),
  'data':(DATA,0x10000),'stack':(STACK,0x9000)}
ENTRIES=(0x130980,0x130eb0,0x13fe50,0x32a660,0x16e6a0,0x16e770,0x16e860,0x1a3b70,0x1aab20,0x123270,0x2e0478,0x2e0362,0x2e01ba,
  0x1236a0,0x1232c0,0x1ab330,0x2e0342,0x2dbf74,0x128870,0x33b590,0x22a680)
meta=base.prepare(image,scratch,ENTRIES)
meta['trace_calls']=[dict(entry=0x130eb0,arguments=3,gp=True),dict(entry=0x13fe50,arguments=3),dict(entry=0x16e6a0,gp=True),
  dict(entry=0x16e770,gp=True),dict(entry=0x1a3b70,arguments=6,gp=True),dict(entry=0x1aab20,arguments=6,gp=True),
  dict(entry=0x123270,gp=True),dict(entry=0x1236a0,arguments=1,gp=True),dict(entry=0x1232c0,arguments=1),
  dict(entry=0x2dbf74,gp=True),dict(entry=0x22a680,arguments=1,gp=True)]
HDR,TAGS,PHYS,BLK,BLKBITS,BLKROWS,FL,FLBITS,FLROWS=DATA+0x100,DATA+0x1000,DATA+0x2000,DATA+0x3000,DATA+0x3080,DATA+0x3100,DATA+0x4000,DATA+0x4080,DATA+0x4100
rows=[]
def new(name,entry,**k):
    banks=[dict(address=at,before=bytearray(size))for at,size in base.SECTIONS_MAP.values()]
    def put(at,v,size=4):
        for b in banks:
            if b['address']<=at and at+size<=b['address']+len(b['before']):
                b['before'][at-b['address']:at-b['address']+size]=(v&((1<<(8*size))-1)).to_bytes(size,'little');return
        raise ValueError('unmapped '+hex(at))
    row=dict(id=len(rows),name=name,entry=entry,gp=[0,0,0,0,SP,0,0,0],matrix=0,source=0,before_fp=[0x27f,0,0x1f80],live_x87=[],max_steps=60000,**k)
    def finish():row['banks']=[dict(address=b['address'],before=b['before'].hex())for b in banks];rows.append(row)
    return row,put,finish
def world(put,token=0xffffffff,limit1=0x100,total=0x40,special=True):
    put(HDR,0x00001000);put(HDR+4,0x3000);put(HDR+8,16);put(HDR+0xc,0);put(HDR+0x10,0);put(HDR+0x14,0)
    put(HDR+0x18,0,2);put(HDR+0x1c,0x24,2);put(HDR+0x1e,0,1);put(HDR+0x20,token)
    put(0x4f0414,TAGS);put(TAGS,0x73627370 if special else 0x65646f6d)
    put(0x4f2d6c,PHYS);put(0x4f2d5c,FL);put(0x4f2d64,0x20000000)
    put(PHYS+0x30,total);put(PHYS+0x34,12);put(PHYS+0x38,0xc10c0001);put(PHYS+0x3c,0xffffffff);put(PHYS+0x40,0xffffffff)
    for i in range(8):put(PHYS+0x44+4*i,limit1 if i==1 else 0x100)
    put(PHYS+0x64,BLK)
    for pool,bits,rws,size in ((BLK,BLKBITS,BLKROWS,0x18),(FL,FLBITS,FLROWS,12)):
        put(pool+0x20,16);put(pool+0x24,size);put(pool+0x34,0);put(pool+0x38,0);put(pool+0x3c,0);put(pool+0x40,0x1234,2)
        put(pool+0x44,rws);put(pool+0x48,bits)
    put(0x569af0,0);put(0x566ad8,0xf11e0000);put(0x567ae0,0xf11e0001);put(0x5672dc,0xf11e0002)
    put(0x4ecc28,0x4d757401);put(0x4ecc2c,0x4d757402);put(0x4ecc30,0x53656d01);put(0x4ef1c0,0x41)
# A: 130EB0 -> 13FE50 immediate page-limit refusal (limit[1] <= pages=3)
row,put,fin=new('acquire-13fe50-limit-refusal',0x130980);row['gp'][0]=3;row['gp'][1]=HDR;world(put,limit1=3);fin()
# B: 13FE50 scan with zero total pages: refusal plus limit-hint store
row,put,fin=new('acquire-13fe50-empty-refusal',0x130980);row['gp'][0]=3;row['gp'][1]=HDR;world(put,total=0);fin()
# C: 13FE50 success on empty cache, prefix to first kernel import
row,put,fin=new('acquire-success-prefix',0x130980,stop_pc=0x2e037f);row['gp'][0]=3;row['gp'][1]=HDR;world(put);fin()
# D: token valid, not ready, no promote, blocking -> stop at NtYieldExecution
row,put,fin=new('blocking-loop-prefix',0x130980,stop_pc=0x2dbf74);row['gp'][0]=3;row['gp'][1]=HDR;world(put,token=0xabcd0001)
put(FLROWS+12+2,0,1);put(FLROWS+12+3,7,1);put(FLROWS+12+4,0x4201);put(BLKROWS+24+0x14,0);fin()
# E: token valid, not ready, promote (prio7 > 5) -> stop inside 1236A0 wait
row,put,fin=new('promote-prefix',0x130980,stop_pc=0x2e037f);row['gp'][0]=3;row['gp'][1]=HDR;world(put,token=0xabcd0001)
put(FLROWS+12+2,0,1);put(FLROWS+12+3,5,1);put(FLROWS+12+4,0x4201);fin()
SLOT=lambda j:0x4ecc38+64*(j&0xff)
# F: 1236A0 post-wait no-op (slot+4 != job), stop at NtReleaseMutant
row,put,fin=new('promote-noop-post-wait',0x1236b0,stop_pc=0x2e0348,return_pc=0x55667788);row['matrix']=STOP;row['source']=7
row['gp'][6]=0x4201;row['gp'][3]=0x11223300;world(put);put(SLOT(1)+4,0x4101);put(0x4ef1bc,SLOT(1));fin()
# G: 1236A0 post-wait queue move: head=slot2(prio5)->slot1(job) ; reinsertion at head
row,put,fin=new('promote-move-post-wait',0x1236b0,stop_pc=0x2e0348,return_pc=0x55667788);row['matrix']=STOP;row['source']=7
row['gp'][6]=0x4201;row['gp'][3]=0x11223300;world(put);put(SLOT(1)+4,0x4201);put(SLOT(1),2);put(SLOT(1)+0x3c,0)
put(SLOT(2),5);put(SLOT(2)+4,0x4302);put(SLOT(2)+0x3c,SLOT(1));put(0x4ef1bc,SLOT(2));fin()
# H: 128870 prefix to KeTickCount import read
row,put,fin=new('pump-prefix',0x128870,stop_pc=0x33b590);world(put);put(0x4f2c50,DATA+0x6000);put(DATA+0x6080,0x777);fin()
# I: 128870 post-tick with [4F3218]=0 (tick dead), whole tail
row,put,fin=new('pump-post-tick-idle',0x128881,return_pc=0x00000777);row['matrix']=STOP;row['gp'][0]=0x13572468;row['gp'][6]=0x777
world(put);put(0x4f3218,0);fin()
(scratch/'metadata.json').write_text(json.dumps(meta));(scratch/'inputs.json').write_text(json.dumps(rows))
r=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=240)
print(r.stdout[-2000:],r.stderr[-3000:])
if r.returncode:sys.exit(r.returncode)
proof=json.loads((scratch/'observed.json').read_text())
summaries=[]
for row,c in zip(rows,proof['cases'],strict=True):
    ws=[(hex(w['address']),w['written'])for w in c['writes']if not STACK<=w['address']<STACK+0x9000]
    rs=[(hex(r['instruction']),hex(r['address']))for r in c['reads']if not STACK<=r['address']<STACK+0x9000]
    print('==',row['name'],'completed',c['completed'],'boundary',c['boundary'],'steps',c['original_instructions'],'eax',hex(c['boundary_gp'][0]),'esp',hex(c['boundary_gp'][4]))
    for call in c['calls']:print('   call',hex(call['entry']),[hex(a)for a in call['arguments']],[hex(g)for g in call.get('gp',[])][:8])
    print('   writes',ws)
    print('   nonstack reads',len(rs),rs[:80])

    assert c['completed']
    summaries.append(dict(name=row['name'],entry=row['entry'],stop_pc=row.get('stop_pc',STOP),steps=c['original_instructions'],
        eax=c['boundary_gp'][0],esp=c['boundary_gp'][4],writes=ws,calls=c['calls'],after_fp=c['after_fp'],
        last_nonstack_read=rs[-1]if rs else None,whole_original_request=row['name']in ('acquire-13fe50-limit-refusal','acquire-13fe50-empty-refusal')))
assert len(summaries)==9
for index,sp in {2:0x62017f14,3:0x62017fe0,4:0x62017f9c,5:0x62017ff0,6:0x62017ff0,7:0x62017ff8}.items():assert summaries[index]['esp']==sp
assert summaries[0]['eax']&255==0 and summaries[1]['eax']&255==0
report=dict(passed=True,physical_cases=9,original_instructions=proof['original_instructions'],original_xbe_sha256=base.PIN,
    instruction_patches=proof['instruction_patches'],supplied_original_outcomes=proof['supplied_original_outcomes'],
    cases=summaries,owners=meta['owners'],helper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    scope='Two whole original130980 acquisitions refused by genuine130EB0/13FE50; one genuine acquisition through its first kernel-import boundary; promotion and pump prefixes/tails with dead imported results. No resumed or stubbed kernel result and no native-policy comparison claimed here.',retained_large_artifacts=[])
args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
