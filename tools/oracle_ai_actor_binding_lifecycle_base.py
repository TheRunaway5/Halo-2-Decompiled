from pathlib import Path
import sys,hashlib,json
sys.path.insert(0,str(Path(__file__).resolve().parent))
from tag_match_memory_budget import enter_memory_budget
import argparse
parser=argparse.ArgumentParser(description='Run unchanged base-retail actor unit-binding owners through complete cold actor retirement')
parser.add_argument('--output-dir',type=Path,required=True)
parser.add_argument('--fixture',type=Path)
args=parser.parse_args()
output=args.output_dir
status=enter_memory_budget(output,60)
if status is not None:raise SystemExit(status)
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
x=Xbe(Path('/home/eric/.local/share/halo2-native/cache-reset-backups/20261012T201413Z/assets/disc/default.xbe'))
pin=hashlib.sha256(x.data).hexdigest();assert pin=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
g.mem_map(0x1000000,0x200000);g.mem_map(0x2000000,0x10000)
APOOL,ACTORS,BITMAP=0x1000000,0x1001000,0x1090000
OPOOL,HEADERS,BODIES=0x1091000,0x1092000,0x10a0000
TAGS,CHARACTER,UNITDEF,GENERAL,TIME,COUNTER,AI=[0x10b0000+i*0x1000 for i in range(7)]
STACK,STOP=0x200ff00,0x2000000;NONE=0xffffffff;ACTOR=0xe3610000;OTHER=0xe3620001
def write(address,value,size=4):g.mem_write(address,(value&((1<<(8*size))-1)).to_bytes(size,'little'))
def read(address,size=4):return int.from_bytes(g.mem_read(address,size),'little')
for address,value in [(0x4f55f0,APOOL),(0x4e0300,OPOOL),(0x4e3b44,TAGS),(0x510c54,TIME),(0x5047f4,COUNTER),(0x4f55d0,AI)]:write(address,value)
write(OPOOL+0x44,HEADERS);write(TAGS+8,CHARACTER);write(TAGS+16+8,UNITDEF)
g.mem_write(CHARACTER,bytes(0x40));write(CHARACTER+8,-1);write(CHARACTER+0x34,1);write(CHARACTER+0x38,GENERAL)
g.mem_write(UNITDEF,bytes(0x300));g.mem_write(GENERAL,bytes(0xc));write(GENERAL+4,1,2)
g.mem_write(AI,bytes(0x400));write(TIME+8,417);write(TIME+2,30,2);write(0x4686c4,0,2)
# Original25C170's real-shaped empty observed-prop pool.
PROPS,PROP_ROWS,PROP_BITS=0x10c0000,0x10c1000,0x10ce000
write(0x50241c,PROPS);g.mem_write(PROPS,bytes(0x4c));g.mem_write(PROPS,b'prop\0')
write(PROPS+0x20,256);write(PROPS+0x24,0xc4);write(PROPS+0x29,1,1)
write(PROPS+0x44,PROP_ROWS);write(PROPS+0x48,PROP_BITS)
# Actual reset has its original ESI register contract.
write(STACK,STOP);g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_ESI,PROPS)
g.emu_start(0x16b7a0,STOP,count=100000)
events=[];binding=False
def observe(g,address,size,user):
 if not binding:return
 if address==0x1e1a00:
  stack=g.reg_read(UC_X86_REG_ESP)
  events.append(dict(kind='release',actor=read(stack+4),argument=read(stack+8)))
 if address==0x1bbdf0:
  assert read(AI+1,1)==0
  events.append(dict(kind='driver',unit=g.reg_read(UC_X86_REG_EAX),actor_unit=read(ACTORS+0x18),owners=[[read(BODIES+i*0x1000+offset) for offset in (0x12c,0x134,0x248,0x24c)] for i in range(2)]))
 if address==0x15e300:raise RuntimeError('Fixture needs an actual live item-effect owner')
g.hook_add(UC_HOOK_CODE,observe)
def call(address,arguments=(),registers={}):
 write(STACK,STOP)
 for i,value in enumerate(arguments):write(STACK+4+i*4,value)
 g.reg_write(UC_X86_REG_ESP,STACK)
 for register,value in registers.items():g.reg_write(register,value)
 g.emu_start(address,STOP,count=200000)
 assert g.reg_read(UC_X86_REG_EIP)==STOP
 return g.reg_read(UC_X86_REG_EAX)
def deltas(data,baseline):
 result=[];i=0
 while i<len(data):
  if data[i]==baseline[i]:i+=1;continue
  start=i
  while i<len(data) and data[i]!=baseline[i]:i+=1
  result.append([start,data[start:i].hex()])
 return result
sizes=[0x500,0x500]+[0x170]*4;records=[];actor_baseline=None
def pool_state():
 return dict(cursor=read(APOOL+0x34),high_water=read(APOOL+0x38),count=read(APOOL+0x3c),next_salt=read(APOOL+0x40,2),bitmap=[read(BITMAP+i*4) for i in range(8)])
for other_unit in (NONE,0xe26f0000):
 for mode in ('bind','unbind'):
  for prior_unit in (NONE,0xe26f0000,0xe26f0001):
   for prior_actor in (NONE,ACTOR,OTHER):
    for owners in range(4):
     for blocked in (False,True):
      binding=False;g.mem_write(APOOL,bytes(0x4c));g.mem_write(APOOL,b'actor\0');write(APOOL+0x20,256);write(APOOL+0x24,0x888);write(APOOL+0x29,1,1);write(APOOL+0x44,ACTORS);write(APOOL+0x48,BITMAP)
      call(0x16b7a0,registers={UC_X86_REG_ESI:APOOL});write(COUNTER+6,0,2)
      assert call(0x1dfb90,[0xe1000000])==ACTOR and call(0x1dfb90,[0xe1000000])==OTHER
      current=bytes(g.mem_read(ACTORS,0x888))
      if actor_baseline is None:actor_baseline=current
      assert current==actor_baseline
      write(ACTORS+0x18,prior_unit);write(ACTORS+0x888+0x18,other_unit)
      for i,size in enumerate(sizes):
       body=BODIES+i*0x1000;g.mem_write(body,b'\xa5'*size);write(HEADERS+i*12+8,body);write(HEADERS+i*12+3,i if i<2 else 2,1);write(body+0xaa,i if i<2 else 2,1)
       if i<2:
        write(body,0xe1000001)
        for offset in (0x10,0x14,0x24c):write(body+offset,-1)
        write(body+0x12c,prior_actor if i==0 else ACTOR if prior_unit==0xe26f0001 else NONE)
        write(body+0x130,0xe3700000+i if bool(owners&1)!=bool(i) else NONE)
        write(body+0x13c,0xe2700000+i if bool(owners&2)!=bool(i) else NONE)
        write(body+0x10a,0xa0|(4 if blocked!=bool(i) else 0),1)
        write(body+0x134,0xa5a5a5a5)
        write(body+0x248,0xe26f0000+(1-i) if len(records)%(i+2)==0 else NONE)
        for slot in range(4):write(body+0x218+slot*4,0xe26f0002+i*2+slot if slot<2 else NONE)
       else:
        write(body+0x12c,0xab,1);write(body+0x154,0xe26f0000+(i-2)//2);write(body+0x16c,0xa5&~0x40,1)
      before_actor=deltas(bytes(g.mem_read(ACTORS,0x888)),actor_baseline)
      other_before=deltas(bytes(g.mem_read(ACTORS+0x888,0x888)),actor_baseline)
      before=[deltas(bytes(g.mem_read(BODIES+i*0x1000,size)),b'\xa5'*size) for i,size in enumerate(sizes)]
      events.clear();binding=True;complete=True
      if mode=='bind':call(0x1e0d50,[0xe26f0000],{UC_X86_REG_EDI:ACTOR})
      else:call(0x1e1250,[ACTOR])
      binding=False
      after_actor=deltas(bytes(g.mem_read(ACTORS,0x888)),actor_baseline)
      after=[deltas(bytes(g.mem_read(BODIES+i*0x1000,size)),b'\xa5'*size) for i,size in enumerate(sizes)]
      assert read(APOOL+0x3c) in (1,2) and read(COUNTER+6,2)==0
      records.append(dict(other_before=other_before,other_after=deltas(bytes(g.mem_read(ACTORS+0x888,0x888)),actor_baseline),pool_after=pool_state(),mode=mode,prior_unit=prior_unit,prior_actor=prior_actor,owners=owners,blocked=blocked,complete=complete,actor_before=before_actor,actor_after=after_actor,before=before,after=after,events=list(events)))
report=dict(passed=True,full_lifecycle=True,xbe_sha256=pin,scope='Whole unchanged1E0D50/1E1250 through complete cold1E1A00, including actual empty prop iteration and inactive1BBDF0. No replaced release callbacks. Both actor byte records, allocation metadata, unit/item bodies and ordered owner entries retained. Full birth/admission, active AI response and populated perception/command owners remain separate.',actor_baseline_hex=actor_baseline.hex(),baseline_byte=0xa5,record_sizes=sizes,cases=records)
with (output/'original.json').open('w') as file:json.dump(report,file,indent=2);file.write('\n')
print('PASS '+str(len(records))+' complete original bind/unbind cases with cold retirement')
if args.fixture:
 with args.fixture.open('w') as file:
  file.write('# Base retail actor binding through cold retirement\n\n'+report['scope']+'\n\n```json\n')
  json.dump(report,file,indent=2);file.write('\n```\n')
