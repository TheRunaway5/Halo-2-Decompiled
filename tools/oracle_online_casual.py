#!/usr/bin/env python3
"""Execute unchanged retail hopper result and public-property dispatch.
All inputs are synthetic; no native rating implementation or call stubs are used.
The local identity sentinel bypasses only 65530's unrelated recent-player cache write.
Full 741F0 and 73E00 emit operations; final Live arbitration is outside this fixture.
"""
import csv,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_game_variants import Source,VERSIONS
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn.x86_const import *
OUT=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'.local/casual-skill-oracle'
OUT.mkdir(parents=True,exist_ok=True)
s=Source(VERSIONS['retail']);g=s.g
g.mem_map(0x1100000,0x60000)
MATCH,BATCH,SESSION=0x1140000,0x1120000,0x1150000
HOPPER=0x551ae8+0x614

def put(a,fmt,*values):g.mem_write(a,struct.pack('<'+fmt,*values))
def read(a,fmt):return struct.unpack('<'+fmt,g.mem_read(a,struct.calcsize('<'+fmt)))
def hopper(kind):
 g.mem_write(0x551ae8,bytes(16*0x614));put(0x47d8f8,'IHH',2,2,0)
 put(HOPPER,'iiI',kind,1,7)
 g.mem_write(HOPPER+0x51c,bytes(range(128)))
 put(HOPPER+0x5ec,'iiiiii',2,2,2,0,1,1)
 assert s.call(0x192e60,registers=[(UC_X86_REG_EAX,1)])==HOPPER

def make_match():
 g.mem_write(MATCH,bytes(0x10000));put(MATCH+4,'i',1)
 for i,(previous,xp,place) in enumerate([(0,0,0),(15,5500,1)]):
  a=MATCH+0x384+i*0xa4;g.mem_write(a,b'\1');g.mem_write(a+2,bytes([i+1])*8+bytes(4))
  put(a+0x8c,'b',i);put(a+0x94,'ihhi',1,15,previous,xp);put(a+0xa0,'b',place)
 g.mem_write(0x509454,bytes(5*(0x1c+16*0x98)))
 g.mem_write(BATCH,bytes(0xc090))

s.call(0x66330)
result_controls=[]
for kind in [1,2,3,4]:
 hopper(kind);make_match();s.call(0x741f0,(MATCH,BATCH))
 attributes=[]
 for i in range(read(BATCH,'i')[0]):
  a=BATCH+8+i*16;attr_id,datatype=read(a,'HH')
  value=read(a+8,'q' if datatype==2 else 'i')[0]
  attributes.append(dict(id=attr_id,datatype=datatype,value=value))
 assert len(attributes)==28
 assert attributes[0]['value']==100 and attributes[5]['value']==1 and attributes[12]['value']==1
 assert attributes[14]['value']==-100 and attributes[19]['value']==14 and attributes[26]['value']==15
 operations=read(BATCH+0xe08,'i')[0]
 assert operations==42
 blob=bytes(g.mem_read(BATCH,0xc090))
 result_controls.append(dict(hopper_type=kind,statistics_key=7,attributes=attributes,
   operation_count=operations,batch_sha256=hashlib.sha256(blob).hexdigest()))
assert len({r['batch_sha256'] for r in result_controls})==1

# Existing cached data survive visibility-type changes because storage is keyed by
# hopper slot/key, not by the visibility classification. No service is emulated.
identity=bytes.fromhex('01020304050607080000d0ba')
g.mem_write(0x4cd904,b'\1'+identity)
g.mem_write(0x509454+4,identity)
put(0x4cd900,'i',1)
g.mem_write(0x527330,b'\1');put(0x527364,'I',SESSION)
put(SESSION+0x741c,'i',5);put(SESSION+0x49c8,'i',1)
cached=0x509454+0x1c+0x98
put(cached,'i',3);put(cached+0xc,'i',5500);put(cached+0x14,'i',15)
property_controls=[]
for kind in [1,2,3,4]:
 hopper(kind);s.call(0x65530,(0,))
 shown,highest=read(0x4cd918+0x7e,'bb')
 key,effective,previous,xp=read(0x4cd918+0x84,'ihhi')
 expected=15 if kind in [2,4] else -1
 assert (shown,highest,key,effective,previous,xp)==(expected,expected,1,-1,15,5500)
 property_controls.append(dict(hopper_type=kind,display_rank=shown,highest_display_rank=highest,
   current_hopper=key,effective_skill=effective,previous_raw_rank=previous,xp=xp))

# Execute original expansion through the actual search-query boundary. Stop before
# XOnlineMatchSearch's wrapper; no instruction/call is replaced. This proves the
# filter envelope, not a service response or a recovered time cadence.
from unicorn import UC_HOOK_CODE
from oracle_game_variants import STACK,STOP
SEARCH,RESULTS,PRIOR=0x1100000,0x1101000,0x1110000

def stop_boundary(g,address,size,unused):
 if address in (STOP,0x8fa80):g.emu_stop()
stop_hook=g.hook_add(UC_HOOK_CODE,stop_boundary)
search_controls=[]
for ranked,current,maximum in [(False,2,3),(True,2,3),(False,3,3),(True,3,3)]:
 g.mem_write(SEARCH,bytes(0xa0));g.mem_write(RESULTS,bytes(50*0x204));g.mem_write(PRIOR,bytes(50*0x204))
 g.mem_write(SEARCH,b'\1');put(SEARCH+8,'i',1);g.mem_write(SEARCH+0x14,bytes([int(ranked)]))
 put(SEARCH+0x1c,'ii',current,maximum);put(SEARCH+0x38,'i',1)
 put(SEARCH+0x50,'i',15);put(SEARCH+0x60,'iii',10,20,15)
 put(SEARCH+0x70,'iii',-1,-1,-1);put(SEARCH+0x88,'iII',50*0x204,RESULTS,PRIOR)
 put(STACK,'I',STOP);g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_EAX,SEARCH)
 g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
 g.emu_start(0x90f70,0,count=200000)
 boundary=g.reg_read(UC_X86_REG_EIP)
 radius=read(SEARCH+0x1c,'i')[0];state=read(SEARCH+8,'i')[0]
 min_enabled=bool(read(SEARCH+0x2c,'B')[0]);max_enabled=bool(read(SEARCH+0x24,'B')[0])
 if current<maximum:
  assert (boundary,radius,min_enabled,max_enabled)==(0x8fa80,3,True,True)
 elif ranked:
  assert (boundary,radius,state)==(STOP,3,0)
 else:
  assert (boundary,radius,min_enabled,max_enabled,read(SEARCH+0x34,'i')[0])==(0x8fa80,-1,False,False,100)
 search_controls.append(dict(ranked_or_clan_flag=ranked,initial_expansion=current,synthetic_maximum=maximum,
   final_expansion=radius,state=state,retries=boundary==0x8fa80,
   minimum_rank_filter=min_enabled,maximum_rank_filter=max_enabled,
   minimum_candidate_score=read(SEARCH+0x34,'i')[0],boundary=hex(boundary)))
g.hook_del(stop_hook)

decoder=Cs(CS_ARCH_X86,CS_MODE_32)
lines=[]
for a in sorted(s.executed):
 i=next(decoder.disasm(s.image.read(a,15),a))
 lines.append(f'{a:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
trace='\n'.join(lines)+'\n';(OUT/'retail-casual-executed.txt').write_text(trace)
# Preserve complete exact function spans from the original function inventory.
wanted={0x741f0,0x73ca0,0x65530,0x193470,0x193490,0x74aa0,0x90880,0x90f70,0x90b30}
spans={}
with (ROOT/'tools/reference/halo2-decompiled/config/functions.csv').open() as f:
 for row in csv.reader(f):
  try: a=int(row[0],16)
  except (ValueError,IndexError):continue
  if a not in wanted:continue
  n=int(row[1]);data=bytes(g.mem_read(a,n))
  listing='\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}' for i in decoder.disasm(data,a))+'\n'
  (OUT/f'retail-{a:06x}.txt').write_text(listing)
  spans[hex(a)]=dict(size=n,sha256=hashlib.sha256(data).hexdigest())
result=dict(scope=__doc__,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
 xbe=VERSIONS['retail'],source_spans=spans,
 instruction_trace_sha256=hashlib.sha256(trace.encode()).hexdigest(),
 controls=len(result_controls)+len(property_controls)+len(search_controls),result_controls=result_controls,
 search_controls=search_controls,
 cached_property_controls=property_controls,
 limits='Retail defaults and synthetic hoppers only. 73CA0 round-report gate checked statically from original bytes, not executed; final XOnline arbitration, downloaded later config, guest clients, live service and complete casual playlist rotations are outside this fixture.')
print(json.dumps(result,indent=2))
