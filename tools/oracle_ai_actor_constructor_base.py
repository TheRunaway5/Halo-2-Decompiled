from pathlib import Path
import sys,struct,json,hashlib
import argparse
from tag_match_memory_budget import enter_memory_budget
parser=argparse.ArgumentParser(description='Run the whole base-retail actor constructor in the serial memory guard')
parser.add_argument('--output-dir',type=Path,required=True)
parser.add_argument('--fixture',type=Path,help='Also export the tracked Markdown golden fixture')
args=parser.parse_args()
contained=enter_memory_budget(args.output_dir,60)
if contained is not None:raise SystemExit(contained)
ROOT=Path('/home/eric/Developer/Halo-2-Decompiled');sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
x=Xbe(Path('/home/eric/.local/share/halo2-native/cache-reset-backups/20261012T201413Z/assets/disc/default.xbe'))
pin=hashlib.sha256(x.data).hexdigest();assert pin=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
g.mem_map(0x1000000,0x200000);g.mem_map(0x2000000,0x10000)
POOL=0x1000000;ROWS=POOL+0x1000;BITMAP=POOL+0x90000;TAGS=POOL+0x91000;TAG=POOL+0x92000;PARENT=POOL+0x93000;GENERAL=POOL+0x94000;TIME=POOL+0x95000;COUNTER=POOL+0x96000;STACK=0x200ff00;STOP=0x2000000
w=lambda p,v:g.mem_write(p,struct.pack('<I',v&0xffffffff))
r=lambda p:struct.unpack('<I',g.mem_read(p,4))[0]
for a,v in [(0x4f55f0,POOL),(0x4e3b44,TAGS),(0x510c54,TIME),(0x5047f4,COUNTER)]:w(a,v)
w(TAGS+8,TAG);w(TAGS+16+8,PARENT)
for t in (1,0x72):
 handler=struct.unpack('<I',x.read(0x46eeb8+t*4,4))[0]
 assert x.read(handler+0x18,8)==bytes(8),'Constructor callback owner unexpectedly required'
rows=[]
def call(at,args,registers={}):
 w(STACK,STOP)
 for i,v in enumerate(args):w(STACK+4+i*4,v)
 g.reg_write(UC_X86_REG_ESP,STACK)
 for reg,val in registers.items():g.reg_write(reg,val)
 g.emu_start(at,STOP,count=100000)
 assert g.reg_read(UC_X86_REG_EIP)==STOP,hex(g.reg_read(UC_X86_REG_EIP))
 return g.reg_read(UC_X86_REG_EAX)
for inherited in (False,True):
 for flags in (0,1,2,3,8,0xffffffff):
  for actor_type in (0,1,3,5,7,9,11,19):
   g.mem_write(POOL,bytes(0x4c));g.mem_write(POOL,b'actor\0');w(POOL+0x20,256);w(POOL+0x24,0x888);g.mem_write(POOL+0x29,b'\1');w(POOL+0x44,ROWS);w(POOL+0x48,BITMAP)
   call(0x16b7a0,[],{UC_X86_REG_ESI:POOL})
   g.mem_write(TAG,bytes(0x40));g.mem_write(PARENT,bytes(0x40));w(TAG+8,0xe1000001 if inherited else -1);w(PARENT+8,-1)
   source=PARENT if inherited else TAG;w(source+0x34,1);w(source+0x38,GENERAL);g.mem_write(GENERAL,struct.pack('<Ih',flags,actor_type)+bytes(6))
   tick=417+len(rows);rate=30 if len(rows)%2 else 60;counter=0xffff if len(rows)%3==0 else 7
   bsp=(-1,0,1,4)[len(rows)%4];g.mem_write(0x4686c4,struct.pack('<h',bsp))
   w(TIME+8,tick);g.mem_write(TIME+2,struct.pack('<h',rate));g.mem_write(COUNTER+6,struct.pack('<H',counter))
   actor=call(0x1dfb90,[0xe1000000]);assert actor==0xe3610000,hex(actor)
   record=bytes(g.mem_read(ROWS,0x888));next_counter=struct.unpack('<H',g.mem_read(COUNTER+6,2))[0]
   assert next_counter==(counter+bool(flags&8))&65535
   rows.append(dict(inherited=inherited,definition=0xe1000000,general_flags=flags,actor_type=actor_type,tick=tick,rate=rate,bsp=bsp,counter_before=counter,counter_after=next_counter,actor=actor,record_hex=record.hex()))
# Both original early-out paths must preserve pool allocation and counters.
before=bytes(g.mem_read(POOL,0x4c));counter=bytes(g.mem_read(COUNTER+6,2));assert call(0x1dfb90,[0xffffffff])==0xffffffff
assert bytes(g.mem_read(POOL,0x4c))==before and bytes(g.mem_read(COUNTER+6,2))==counter
w(POOL+0x20,1);before=bytes(g.mem_read(POOL,0x4c));assert call(0x1dfb90,[0xe1000000])==0xffffffff
assert bytes(g.mem_read(POOL,0x4c))==before and bytes(g.mem_read(COUNTER+6,2))==counter
report=dict(passed=True,scope='Whole retail1DFB90 with unchanged16B840 allocation,1E4990 inheritance and1A80E0 cold slot initialization, real-shaped owned pools/tag data/time/counter, and original NULL callbacks. Unit binding, activation, perception, AI updates and gameplay remain separate.',xbe_sha256=pin,constructor_sha256=hashlib.sha256(x.read(0x1dfb90,1474)).hexdigest(),cases=rows,early_outs=['NONE definition','exhausted actual pool'])
(args.output_dir/'actor-constructor-original.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS '+str(len(rows))+' complete original actor constructor cases and both early-outs')

if args.fixture:
 import copy
 fixture=copy.deepcopy(report);baseline=bytes.fromhex(fixture['cases'][0]['record_hex'])
 fixture['baseline_record_hex']=baseline.hex()
 for row in fixture['cases']:
  data=bytes.fromhex(row.pop('record_hex'));delta=[];i=0
  while i<len(data):
   if data[i]==baseline[i]:i+=1;continue
   start=i
   while i<len(data) and data[i]!=baseline[i]:i+=1
   delta.append([start,data[start:i].hex()])
  row['record_delta']=delta
 header='# Base-retail AI actor constructor golden\n\nWhole original1DFB90,16B840,1E4990 and1A80E0 output, with actual NULL cold callbacks.\nThe BSP index, clock and counter are supplied live owners. This covers cold\nactor records/identity only, not unit binding, perception, AI behavior or gameplay.\nThe baseline plus byte deltas reconstructs every original0x888-byte result.\n\nRegenerate with `.tools/python/bin/python tools/oracle_ai_actor_constructor_base.py`\nusing a fresh `--output-dir` and `--fixture tests/fixtures/source_ai_actor_ctor_base.md`.\nThe tool enforces the shared serial 64 MiB measured ceiling and zero swap.\n\n```json\n'
 args.fixture.parent.mkdir(parents=True,exist_ok=True)
 args.fixture.write_text(header+json.dumps(fixture,indent=2)+'\n```\n')
