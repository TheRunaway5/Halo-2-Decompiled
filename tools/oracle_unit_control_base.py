from pathlib import Path
import sys,hashlib,json
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
g.mem_map(0x1000000,0x100000);g.mem_map(0x2000000,0x10000)
POOL,HEADERS,BODIES,TAGS,DEFINITION=0x1000000,0x1001000,0x1002000,0x1010000,0x1011000
STACK,STOP=0x200ff00,0x2000000
def write(address,value,size=4):g.mem_write(address,(value&((1<<(8*size))-1)).to_bytes(size,'little'))
def read(address,size=4):return int.from_bytes(g.mem_read(address,size),'little')
write(0x4e0300,POOL);write(POOL+0x44,HEADERS);write(0x4e3b44,TAGS);write(TAGS+8,DEFINITION)
g.mem_write(DEFINITION,bytes(0x300))
events=[]
def observe(g,address,size,user):
 if address==0x10ccc0:events.append(['item',g.reg_read(UC_X86_REG_EAX)])
 if address==0xcc810:events.append(['seats',read(g.reg_read(UC_X86_REG_ESP)+4)])
 if address in (0x15e300,0x1bbdf0):raise RuntimeError('Fixture needs an additional live effect/driver owner: '+hex(address))
g.hook_add(UC_HOOK_CODE,observe)
records=[]
for owners in range(8):
 for blocked in (False,True):
  for requested in (False,True):
   for inventory in (0,1,5,15):
    for other_holder in (False,True):
     for i in range(6):
      body=BODIES+i*0x1000;g.mem_write(body,b'\xa5'*0x500)
      write(HEADERS+i*12+8,body);write(HEADERS+i*12+3,0 if i<2 else 2,1)
      write(body,0xe1000000)
     unit=BODIES;other=BODIES+0x1000
     for offset in (0x10,0x14,0x248,0x24c):write(unit+offset,-1)
     write(unit+0x12c,0xe3610000 if owners&1 else -1)
     write(unit+0x130,0xe3700000 if owners&2 else -1)
     write(unit+0x13c,0xe2700000 if owners&4 else -1)
     write(other+0x13c,-1 if owners&4 else 0xe2700001)
     write(unit+0x10a,0xa0|(4 if blocked else 0),1)
     write(unit+0x134,(0,0xffffffff,0xa5a5a5a5)[len(records)%3])
     for i in range(4):
      item=BODIES+(i+2)*0x1000
      write(unit+0x218+i*4,0xe26f0002+i if inventory&(1<<i) else -1)
      write(item+0x12c,0xaa|(i&1),1)
      write(item+0x154,0xe26f0001 if other_holder else 0xe26f0000)
      write(item+0x16c,0xa5&~0x40,1)
      write(item+0xaa,2,1)
      write(item+0xc,0xe26f0003+i if i<3 else -1)
      write(item+0x14,0xe26f0000)
     if owners&1:write(unit+0x10,0xe26f0002)
     regions=[(unit,0x300)]+[(BODIES+(i+2)*0x1000,0x170) for i in range(4)]
     before=[bytes(g.mem_read(address,size)).hex() for address,size in regions]
     events.clear();write(STACK,STOP);write(STACK+4,0xe26f0000);write(STACK+8,requested)
     g.reg_write(UC_X86_REG_ESP,STACK);g.emu_start(0xcbf60,STOP,count=100000)
     assert g.reg_read(UC_X86_REG_EIP)==STOP
     after=[bytes(g.mem_read(address,size)).hex() for address,size in regions]
     expected_active=not blocked and (owners!=0 or requested)
     assert read(unit+0x134)&3==(3 if expected_active else 0)
     assert events==[['item',0xe26f0002+i] for i in range(4) if inventory&(1<<i)]+[['seats',0xe26f0000]]
     records.append(dict(owners=owners,blocked=blocked,requested=requested,inventory=inventory,other_holder=other_holder,before=before,after=after,events=list(events)))
report=dict(passed=True,xbe_sha256=pin,scope='Whole unchanged CBF60,10CCC0 andCC810 on declared root-unit/object-header/tag/item banks. No supplied success callbacks. Attached non-unit children included; seated unit riders, driver changes, active item-effect bit16C6, destruction and actor lifecycle remain separate.',cases=records)
(output/'original.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS '+str(len(records))+' whole original unit/item control cases; real CC810 root-unit path, no callback replacements')

if args.fixture:
 args.fixture.parent.mkdir(parents=True,exist_ok=True)
 args.fixture.write_text('# Base-retail unit and item control golden\n\n'+report['scope']+'\n\nRegenerate with the named oracle under `tools/`, a fresh `--output-dir`,\nand this path as `--fixture`. The shared guard enforces serial60MiB, zero\nswap, and a measured64MiB ceiling.\n\n```json\n'+json.dumps(report,indent=2)+'\n```\n')
