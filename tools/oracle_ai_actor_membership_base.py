from pathlib import Path
import sys,hashlib,json,random,argparse
sys.path.insert(0,str(Path(__file__).resolve().parent))
from tag_match_memory_budget import enter_memory_budget
parser=argparse.ArgumentParser(description='Execute whole retail actor squad/global membership and reference clearing')
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
specs={'actors':(APOOL,ACTORS,ABITS,'actor',256,0x888,6),
       'perceptions':(0x10c0000,0x10c1000,0x10c2000,'swarm',32,0x34,2),
       'clumps':(0x10c3000,0x10c4000,0x10c5000,'clump',20,0x50,2),
       'squads':(0x10d0000,0x10d1000,0x10de000,'squad',335,0x98,2)}
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
for address,value in [(0x4f55f0,APOOL),(0x4e0300,OPOOL),(0x4e3b44,TAGS),(0x510c54,TIME),(0x5047f4,STATS),(0x4f55d0,AI),(0x5044c8,specs['perceptions'][0]),(0x502420,specs['clumps'][0]),(0x51e9d8,specs['squads'][0])]:write(address,value)
write(OPOOL+0x44,HEADERS);write(TAGS+8,CHARACTER)
g.mem_write(CHARACTER,bytes(0x40));write(CHARACTER+8,NONE);write(CHARACTER+0x34,1);write(CHARACTER+0x38,GENERAL)
g.mem_write(GENERAL,bytes(0xc));write(GENERAL+4,1,2)
rng=random.Random(0x203d70);cases=[]
modes=('join','activate_squad','add_global','remove_squad','remove_global','clear_references')
for case in range(256):
 for spec in specs.values():reset(spec)
 variant=case//6;mode=modes[case%6];active=bool(variant&1);bsp=variant%3-1
 write(GENERAL,0);write(TIME+2,30,2);write(TIME+8,417);write(0x4686c4,bsp,2)
 g.mem_write(STATS,bytes(0x30));g.mem_write(AI,b'\xa5'*0x400)
 write(AI+1,active,1);write(AI+0x36a,(0,0xffff,7,0x8000)[variant%4],2)
 ids=[call(0x1dfb90,arguments=[0xe1000000]) for i in range(6)]
 actor_baseline=bytes(g.mem_read(ACTORS,0x888))
 pool_ids={}
 for name,spec in specs.items():
  if name=='actors':continue
  pool,rows,bits,label,capacity,stride,count=spec;pool_ids[name]=[]
  for index in range(count):
   datum=call(0x16b840,{UC_X86_REG_EDX:pool});pool_ids[name].append(datum)
   salt=read(rows+index*stride,2);g.mem_write(rows+index*stride,b'\xa5'*stride);write(rows+index*stride,salt,2)
 target=variant%2
 members=[[ids[0],ids[1]],[ids[4]]];rng.shuffle(members[0]);globals_chain=[ids[2],ids[3]];rng.shuffle(globals_chain)
 write(AI+0x14,globals_chain[0])
 for squad_index,chain in enumerate(members):
  squad=specs['squads'][1]+squad_index*0x98
  write(squad+0x68,chain[0]);write(squad+8,(0,0xffff,len(chain))[variant%3],2)
  write(squad+2,0x24|(0x80 if bool(variant&2)!=bool(squad_index) else 0),1);write(squad+3,0xa4,1)
  write(squad+0x7e,(variant//2+squad_index)%3-1,2)
  write(squad+0x76,(-1,0,3,6,127,-128)[variant%6],1)
  for index,datum in enumerate(chain):
   record=ACTORS+(datum&0xffff)*0x888;write(record+0x20,chain[index+1] if index+1<len(chain) else NONE);write(record+0x30,squad_index)
 for index,datum in enumerate(globals_chain):
  record=ACTORS+(datum&0xffff)*0x888;write(record+0x20,globals_chain[index+1] if index+1<len(globals_chain) else NONE);write(record+0xa,1,1)
 for index,datum in enumerate(ids):
  record=ACTORS+index*0x888
  write(record+9,1 if bool(variant&4)!=bool(index&1) else 0,1);write(record+0xb,1,1)
  write(record+0x18,0xe26f0000+index if (variant+index)%5 else NONE)
  write(record+0x7c,pool_ids['clumps'][index%2] if index%3 else NONE)
  write(record+0x24,(0,3,6,0xffff)[(variant+index)%4],2)
  write(record+0x4ac,(0,4,5,6,7)[(variant+index)%5],2);write(record+0x4ba,0x8000 if (variant+index)%2 else 0,2)
  for offset,value,width in [(0x50c,1,1),(0x5ac,0x12345678,4),(0x5b0,7,2),(0x5b4,8,2),(0x5b6,9,2),(0x504,4,2)]:write(record+offset,value,width)
  for slot in range(4):write(record+0x402+slot*6,0x12341111+(0x80000000 if (variant+index+slot)%2 else 0))
  write(record+0x190,index%2,2)
  if index%2:write(record+0xd0,0x72,2)
 for index in range(2):write(specs['clumps'][1]+index*0x50+0x1c,1 if (variant+index)%3 else 0,1)
 for index in range(8):
  body=BODIES+index*0x1000;g.mem_write(body,b'\xa5'*0x500)
  write(HEADERS+index*12+8,body);write(HEADERS+index*12+3,0 if index<6 else 12,1);write(body+0xaa,0 if index<6 else 12,1)
  parent=6 if index<6 and bool(variant&8)!=bool(index%2) else 7 if index==6 else None
  write(body+0x14,0xe26f0000+parent if parent is not None else NONE)
  bits=(0,1<<7,1<<18,(1<<7)|(1<<18))[(variant+index)%4]
  write(body+4,(rng.getrandbits(32)&~((1<<7)|(1<<18)))|bits)
 if mode=='remove_squad':actor_id=members[target][variant%len(members[target])]
 elif mode=='remove_global':actor_id=globals_chain[variant%2]
 else:actor_id=ids[5]
 tick=(0,0x7fffffff,0x80000000,0xffffffff,1031+case)[variant%5];write(TIME+8,tick)
 keep=bool(variant&2);conditional=bool(variant&4);before=snapshot();result=None
 if mode=='join':call(0x203d70,arguments=[actor_id,target,int(keep)])
 elif mode=='activate_squad':result=bool(call(0x203bf0,arguments=[target])&0xff)
 elif mode=='add_global':call(0x203f60,{UC_X86_REG_ECX:actor_id})
 elif mode=='remove_squad':call(0x203ed0,{UC_X86_REG_EDI:actor_id},[int(keep)])
 elif mode=='remove_global':call(0x203fb0,{UC_X86_REG_EDI:actor_id})
 else:call(0x1e22d0,{UC_X86_REG_EBX:int(conditional)},[actor_id])
 cases.append(dict(mode=mode,actor=actor_id,squad=target,active=active,bsp=bsp,tick=tick,keep=keep,conditional=conditional,result=result,actor_baseline_hex=actor_baseline.hex(),before=before,after=snapshot()))
report=dict(passed=True,xbe_sha256=pin,scope='Whole unchanged203D70/203BF0/203F60/203ED0/203FB0/1E22D0 and reached204D30/1E11B0/1E13F0/1E1150/268810/BAF80. Real allocator-created banks and complete declared object inputs. No replaced success callbacks. Slots1/0x72 use actual NULL callbacks and followed references are the actual empty operand. Component evidence, not whole birth/admission or campaign completion.',pools={name:dict(name=s[3],capacity=s[4],stride=s[5],records=s[6]) for name,s in specs.items()},body_sizes=[0x500]*8,baseline_byte=0xa5,cases=cases)
with (output/'original.json').open('w') as file:json.dump(report,file,indent=2);file.write('\n')
if args.fixture:
 with args.fixture.open('w') as file:
  file.write('# Base retail actor membership reference\n\n'+report['scope']+'\n\n```json\n');json.dump(report,file,indent=2);file.write('\n```\n')
print('PASS '+str(len(cases))+' whole original squad/global membership and reference-clear cases')
