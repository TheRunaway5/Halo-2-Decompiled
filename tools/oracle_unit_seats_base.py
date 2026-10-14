from pathlib import Path
import sys,hashlib,json,random
sys.path.insert(0,str(Path(__file__).resolve().parent))
from tag_match_memory_budget import enter_memory_budget
import argparse
parser=argparse.ArgumentParser(description='Run unchanged base-retail unit control owners under the shared serial memory guard')
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
POOL,HEADERS,BODIES,TAGS,DEFS,SEATS,AI=0x1000000,0x1001000,0x1002000,0x1010000,0x1020000,0x1030000,0x1040000
STACK,STOP=0x200ff00,0x2000000
def write(address,value,size=4):g.mem_write(address,(value&((1<<(8*size))-1)).to_bytes(size,'little'))
def read(address,size=4):return int.from_bytes(g.mem_read(address,size),'little')
write(0x4e0300,POOL);write(POOL+0x44,HEADERS);write(0x4e3b44,TAGS);write(0x4f55d0,AI)
g.mem_write(AI,bytes(0x400))  # Real1BBDF0's independently verified inactive path.
events=[];visits=[]
def observe(g,address,size,user):
 if address==0xcc810:visits.append(read(g.reg_read(UC_X86_REG_ESP)+4))
 if address==0x1bbdf0:
  assert read(AI+1,1)==0
  events.append(dict(unit=g.reg_read(UC_X86_REG_EAX),owners=[[read(BODIES+i*0x1000+0x248),read(BODIES+i*0x1000+0x24c)] for i in range(6)]))
g.hook_add(UC_HOOK_CODE,observe)
def sparse(data):
 result=[];i=0
 while i<len(data):
  if data[i]==0xa5:i+=1;continue
  start=i
  while i<len(data) and data[i]!=0xa5:i+=1
  result.append([start,data[start:i].hex()])
 return result
rng=random.Random(0xcc810);cases=[]
for case in range(256):
 definitions=[]
 for i in range(8):
  size=0x500 if i<6 else 0x170;body=BODIES+i*0x1000
  g.mem_write(body,b'\xa5'*size)
  write(HEADERS+i*12+8,body);write(HEADERS+i*12+3,i%2 if i<6 else 2,1)
  write(body+0xaa,i%2 if i<6 else 2,1)
  for offset in (0xc,0x10,0x14):write(body+offset,-1)
  if i>=6:continue
  definition=DEFS+i*0x1000;seats=SEATS+i*0x1000
  write(TAGS+i*16+8,definition);g.mem_write(definition,bytes(0x300));g.mem_write(seats,bytes(4*0xb0))
  flags=rng.choice((0,1<<25,1<<26,(1<<25)|(1<<26)))
  write(definition+0xbc,flags);write(definition+0x1c8,4);write(definition+0x1cc,seats)
  authored=[]
  for j in range(4):
   seat_flags=rng.choice((0,4,8,12,4|(1<<18),8|(1<<11),4|(1<<11)|(1<<13),12|(1<<11)|(1<<13)|(1<<18)))
   other=rng.randrange(-1,4)
   write(seats+j*0xb0,seat_flags);write(seats+j*0xb0+0x3e,other,2)
   authored.append(dict(flags=seat_flags,other=other))
  definitions.append(dict(flags=flags,seats=authored))
  write(body,0xe1000000+i);write(body+0x12c,0xe3610000+i if rng.randrange(2) else -1)
  write(body+0x13c,0xe2700000+i if rng.randrange(2) else -1)
  write(body+0x134,rng.choice((0,1,2,3,0xa5a50000)))
  write(body+0x1fc,rng.randrange(-1,4),2)
  write(body+0x346,0x400,2);write(body+0x404,rng.choice((0,0x80000000)))
  write(body+0x408,rng.randrange(2),1)
  write(body+0x248,rng.choice((-1,0xe26f0001,0xe26f0002)))
  write(body+0x24c,rng.choice((-1,0xe26f0001,0xe26f0003)))
 children=[[] for _ in range(6)]
 for i in range(1,5):
  parent=rng.randrange(i);children[parent].append(i);write(BODIES+i*0x1000+0x14,0xe26f0000+parent)
  index=int.from_bytes(g.mem_read(BODIES+i*0x1000+0x1fc,2),'little',signed=True)
  if index==-1 or (definitions[parent]['seats'][index]['flags']&(1<<11) and not definitions[parent]['seats'][index]['flags']&(1<<13)):
   write(BODIES+i*0x1000+0x346,-1,2) # This state must never be read.
 children[0].append(6);write(BODIES+6*0x1000+0x14,0xe26f0000)
 for parent,indices in enumerate(children):
  rng.shuffle(indices)
  if indices:write(BODIES+parent*0x1000+0x10,0xe26f0000+indices[0])
  for index,i in enumerate(indices):write(BODIES+i*0x1000+0xc,0xe26f0000+indices[index+1] if index+1<len(indices) else -1)
 if case%3:
  parent=5 if case%3==1 else 7
  write(BODIES+0x14,0xe26f0000+parent);write(BODIES+parent*0x1000+0x10,0xe26f0000)
 sizes=[0x500]*6+[0x170]*2
 before=[sparse(bytes(g.mem_read(BODIES+i*0x1000,size))) for i,size in enumerate(sizes)]
 events.clear();visits.clear();write(STACK,STOP);write(STACK+4,0xe26f0000);g.reg_write(UC_X86_REG_ESP,STACK)
 g.emu_start(0xcc810,STOP,count=500000)
 assert g.reg_read(UC_X86_REG_EIP)==STOP
 after=[sparse(bytes(g.mem_read(BODIES+i*0x1000,size))) for i,size in enumerate(sizes)]
 cases.append(dict(definitions=definitions,before=before,after=after,events=list(events),visits=list(visits)))
report=dict(passed=True,xbe_sha256=pin,scope='Whole unchanged CC810 and actual1BBDF0 inactive-AI early-out; root, nested riders, ordinary/grouped seats, other-seat flags, dynamic-state short-circuits, attached non-unit children, parent gunner and driver-change ordering. Active-AI actor birth/release remains a separate owner.',baseline_byte=0xa5,record_sizes=[0x500]*6+[0x170]*2,cases=cases)
(output/'original.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS '+str(len(cases))+' complete original recursive seat graphs, '+str(sum(len(c['events']) for c in cases))+' actual inactive-AI driver callbacks')

if args.fixture:
 args.fixture.parent.mkdir(parents=True,exist_ok=True)
 args.fixture.write_text('# Base-retail recursive unit seat golden\n\n'+report['scope']+'\n\nRegenerate with the named oracle under `tools/`, a fresh `--output-dir`,\nand this path as `--fixture`. The shared guard enforces serial60MiB, zero\nswap, and a measured64MiB ceiling.\n\n```json\n'+json.dumps(report,indent=2)+'\n```\n')
