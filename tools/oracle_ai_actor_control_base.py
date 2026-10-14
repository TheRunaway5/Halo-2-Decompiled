from pathlib import Path
import sys,hashlib,json,random,argparse
sys.path.insert(0,str(Path(__file__).resolve().parent))
from tag_match_memory_budget import enter_memory_budget
parser=argparse.ArgumentParser(description='Execute whole retail actor eligibility, activation and team owners')
parser.add_argument('--output-dir',type=Path,required=True)
parser.add_argument('--fixture',type=Path)
args=parser.parse_args();output=args.output_dir
status=enter_memory_budget(output,60)
if status is not None:raise SystemExit(status)
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
x=Xbe(Path('/home/eric/.local/share/halo2-native/cache-reset-backups/20261012T201413Z/assets/disc/default.xbe'))
pin=hashlib.sha256(x.data).hexdigest();assert pin=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
g.mem_map(0x1000000,0x200000);g.mem_map(0x2000000,0x10000)
STACK,STOP=0x200ff00,0x2000000;NONE=0xffffffff
APOOL,ACTORS,ABITS=0x1000000,0x1001000,0x1090000
OPOOL,HEADERS,BODIES=0x1091000,0x1092000,0x10a0000
TAGS,CHARACTER,GENERAL,TIME,STATS,AI=[0x10b0000+i*0x1000 for i in range(6)]
specs={'actors':(APOOL,ACTORS,ABITS,'actor',256,0x888,3),
       'perceptions':(0x10c0000,0x10c1000,0x10c2000,'swarm',32,0x34,2),
       'clumps':(0x10c3000,0x10c4000,0x10c5000,'clump',20,0x50,2)}
def write(address,value,size=4):g.mem_write(address,(value&((1<<(8*size))-1)).to_bytes(size,'little'))
def read(address,size=4):return int.from_bytes(g.mem_read(address,size),'little')
def call(address,registers={},arguments=()):
 write(STACK,STOP)
 for i,value in enumerate(arguments):write(STACK+4+i*4,value)
 g.reg_write(UC_X86_REG_ESP,STACK)
 for register,value in registers.items():g.reg_write(register,value)
 g.emu_start(address,STOP,count=100000)
 assert g.reg_read(UC_X86_REG_EIP)==STOP,hex(g.reg_read(UC_X86_REG_EIP))
 return g.reg_read(UC_X86_REG_EAX)
def reset(spec):
 pool,rows,bits,name,capacity,stride,_=spec
 g.mem_write(pool,bytes(0x4c));g.mem_write(pool,name.encode()+b'\0')
 write(pool+0x20,capacity);write(pool+0x24,stride);write(pool+0x29,1,1)
 write(pool+0x44,rows);write(pool+0x48,bits)
 call(0x16b7a0,{UC_X86_REG_ESI:pool})
def sparse(data,baseline):
 result=[];i=0
 while i<len(data):
  if data[i]==baseline[i]:i+=1;continue
  start=i
  while i<len(data) and data[i]!=baseline[i]:i+=1
  result.append([start,data[start:i].hex()])
 return result
def snapshot():
 state={}
 for name,(pool,rows,bits,label,capacity,stride,count) in specs.items():
  state[name]=dict(cursor=read(pool+0x34),high_water=read(pool+0x38),count=read(pool+0x3c),next_salt=read(pool+0x40,2),bitmap=[read(bits+i*4) for i in range((capacity+31)//32)],records=[sparse(bytes(g.mem_read(rows+i*stride,stride)),actor_baseline if name=='actors' else b'\xa5'*stride) for i in range(count)])
 state['objects']=[sparse(bytes(g.mem_read(BODIES+i*0x1000,0x500)),b'\xa5'*0x500) for i in range(8)]
 state['globals']=bytes(g.mem_read(AI,0x400)).hex()
 return state
for address,value in [(0x4f55f0,APOOL),(0x4e0300,OPOOL),(0x4e3b44,TAGS),(0x510c54,TIME),(0x5047f4,STATS),(0x4f55d0,AI),(0x5044c8,specs['perceptions'][0]),(0x502420,specs['clumps'][0])]:write(address,value)
write(OPOOL+0x44,HEADERS);write(TAGS+8,CHARACTER)
g.mem_write(CHARACTER,bytes(0x40));write(CHARACTER+8,NONE);write(CHARACTER+0x34,1);write(CHARACTER+0x38,GENERAL)
g.mem_write(GENERAL,bytes(0xc));write(GENERAL+4,1,2)
rng=random.Random(0x1e11b0);cases=[]
for case in range(256):
 for spec in specs.values():reset(spec)
 swarm=bool(case&1);write(GENERAL,int(swarm));write(TIME+2,30,2);write(TIME+8,417);write(0x4686c4,case%2,2)
 g.mem_write(STATS,bytes(0x30));g.mem_write(AI,b'\xa5'*0x400)
 write(AI+1,bool(case&2),1);write(AI+0x36a,(0,0xffff,7,0x8000)[case%4],2)
 ids=[call(0x1dfb90,arguments=[0xe1000000]) for i in range(3)]
 actor_baseline=bytes(g.mem_read(ACTORS,0x888));actor_id=ids[case%3];actor=ACTORS+(actor_id&0xffff)*0x888
 pool_ids={}
 for name,spec in specs.items():
  if name=='actors':continue
  pool,rows,bits,label,capacity,stride,count=spec;pool_ids[name]=[]
  for index in range(count):
   datum=call(0x16b840,{UC_X86_REG_EDX:pool});pool_ids[name].append(datum)
   salt=read(rows+index*stride,2);g.mem_write(rows+index*stride,b'\xa5'*stride);write(rows+index*stride,salt,2)
 write(actor+9,(0,1,2,0xff)[case%4],1)
 mode=case%5;unit=0xe26f0000+(case%2) if mode in (1,2,3) else NONE
 perception=pool_ids['perceptions'][case%2] if swarm or mode in (3,4) else NONE
 write(actor+0x18,unit);write(actor+0x1c,perception)
 write(actor+0x7c,pool_ids['clumps'][case%2] if case%3 else NONE)
 for index in range(2):write(specs['clumps'][1]+index*0x50+0x1c,(0,1,0xa5)[(case+index)%3],1)
 chain=list(range(4));rng.shuffle(chain)
 for index in range(2):write(specs['perceptions'][1]+index*0x34+0x18,0xe26f0000+chain[0] if case%7 else NONE)
 for index in range(8):
  body=BODIES+index*0x1000;g.mem_write(body,b'\xa5'*0x500)
  write(HEADERS+index*12+8,body);write(HEADERS+index*12+3,index if index<2 else 12,1)
  write(body+0xaa,index if index<2 else 12,1)
  depth=(case//4)%4
  parent=(7 if depth==1 else 6 if depth==2 else 5) if index<4 and depth else index+1 if 4<=index<7 else None
  write(body+0x14,0xe26f0000+parent if parent is not None else NONE)
  bits=(0,1<<7,1<<18,(1<<7)|(1<<18))[(case+index)%4]
  write(body+4,(rng.getrandbits(32)&~((1<<7)|(1<<18)))|bits)
  write(body+0x134,3 if (case+index)%5==0 else 0)
  offset=0x280 if index%2 else 0x300;write(body+0x13a,offset,2)
  if index in chain:
   position=chain.index(index);write(body+offset+0xc,0xe26f0000+chain[position+1] if position+1<len(chain) else NONE)
  else:write(body+offset+0xc,NONE)
 before=snapshot();eligible=call(0x1e13f0,{UC_X86_REG_EAX:actor_id})&0xff
 assert eligible in (0,1)
 requested=bool(case&4);tick=(0,0x7fffffff,0x80000000,0xffffffff,1031+case)[case%5];write(TIME+8,tick)
 activated=call(0x1e11b0,{UC_X86_REG_EAX:actor_id,UC_X86_REG_ECX:int(requested)})&0xff
 assert activated in (0,1)
 after_activation=snapshot()
 team=(-1,0,1,6,127,128,-128,32767,-32768)[case%9]
 call(0x1e1150,{UC_X86_REG_EAX:actor_id,UC_X86_REG_ECX:team&0xffffffff})
 cases.append(dict(actor=actor_id,swarm=swarm,requested=requested,tick=tick,team=team,eligible=bool(eligible),activated=bool(activated),actor_baseline_hex=actor_baseline.hex(),before=before,after_activation=after_activation,after_team=snapshot()))
report=dict(passed=True,xbe_sha256=pin,scope='Whole unchanged1E13F0/BAF80 eligibility,1E11B0/268810 activation and1E1150/290BF0 team propagation. No replaced callbacks or fabricated returned success. Real allocator-created actors, swarms and clumps; declared complete object inputs with parent/AI chains. Component evidence only, not full actor birth/admission or campaign gameplay.',pools={name:dict(name=s[3],capacity=s[4],stride=s[5],records=s[6]) for name,s in specs.items()},body_sizes=[0x500]*8,baseline_byte=0xa5,cases=cases)
with (output/'original.json').open('w') as file:json.dump(report,file,indent=2);file.write('\n')
if args.fixture:
 with args.fixture.open('w') as file:
  file.write('# Base retail actor control reference\n\n'+report['scope']+'\n\n```json\n');json.dump(report,file,indent=2);file.write('\n```\n')
print('PASS '+str(len(cases))+' whole original actor eligibility/activation/team cases')
