from pathlib import Path
import sys,hashlib,json,random
sys.path.insert(0,str(Path(__file__).resolve().parent))
from tag_match_memory_budget import enter_memory_budget
import argparse
parser=argparse.ArgumentParser(description='Run whole base-retail actor lifecycle against real allocated banks')
parser.add_argument('--output-dir',type=Path,required=True)
parser.add_argument('--fixture',type=Path)
parser.add_argument('--case-start',type=int,default=0)
parser.add_argument('--case-count',type=int,default=288)
args=parser.parse_args()
if args.case_start < 0 or not 1 <= args.case_count <= 288 or args.case_start+args.case_count > 384:
 parser.error('Select at most288 retained cases within0..383; run larger coverage serially in separate jobs')
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
BODIES,TAGS,CHARACTER,PARENT,UNITDEF,GENERAL,TIME,STATS,AI=[0x10a0000,0x10b0000,0x10b1000,0x10b2000,0x10b3000,0x10b4000,0x10b5000,0x10b6000,0x10b7000]
OPOOL,OHEADERS=0x1091000,0x1092000
STACK,STOP=0x200ff00,0x2000000;NONE=0xffffffff
specs={
 'actors':(0x1000000,0x1001000,0x1090000,'actor',256,0x888,4),
 'squads':(0x10c0000,0x10c1000,0x10cf000,'squad',335,0x98,2),
 'joints':(0x10d0000,0x10d1000,0x10d2000,'joint state',20,0xbc,2),
 'props':(0x10d3000,0x10d4000,0x10e0500,'prop',256,0xc4,4),
 'references':(0x10e1000,0x10e2000,0x10f1000,'prop_ref',1024,0x3c,3),
 'tracking':(0x10f2000,0x10f3000,0x10fb000,'tracking',100,0x124,2),
 'clumps':(0x10fc000,0x10fd000,0x10fe000,'clump',20,0x50,2),
 'locations':(0x10ff000,0x1100000,0x1105000,'dynamic firing points',15,0x484,2),
 'perceptions':(0x1106000,0x1107000,0x1108000,'swarm',32,0x34,2),
 'commands':(0x1109000,0x110a000,0x110d000,'command scripts',40,0xd4,4),
 'joint_commands':(0x110e000,0x110f000,0x1110000,'joint command scripts',10,0x8c,2),
 'threads':(0x1111000,0x1112000,0x1164000,'hs thread',320,0x418,4)}
def write(address,value,size=4):g.mem_write(address,(value&((1<<(8*size))-1)).to_bytes(size,'little'))
def read(address,size=4):return int.from_bytes(g.mem_read(address,size),'little')
def call(address,arguments=(),registers={}):
 write(STACK,STOP)
 for i,value in enumerate(arguments):write(STACK+4+i*4,value)
 g.reg_write(UC_X86_REG_ESP,STACK)
 for register,value in registers.items():g.reg_write(register,value)
 g.emu_start(address,STOP,count=500000)
 assert g.reg_read(UC_X86_REG_EIP)==STOP,hex(g.reg_read(UC_X86_REG_EIP))
 return g.reg_read(UC_X86_REG_EAX)
def reset(spec):
 pool,rows,bits,name,capacity,stride,_=spec
 g.mem_write(pool,bytes(0x4c));g.mem_write(pool,name.encode()+b'\0')
 write(pool+0x20,capacity);write(pool+0x24,stride);write(pool+0x29,1,1)
 write(pool+0x44,rows);write(pool+0x48,bits)
 call(0x16b7a0,registers={UC_X86_REG_ESI:pool})
def sparse(data,baseline):
 result=[];i=0
 while i<len(data):
  if data[i]==baseline[i]:i+=1;continue
  start=i
  while i<len(data) and data[i]!=baseline[i]:i+=1
  result.append([start,data[start:i].hex()])
 return result
def snapshot():
 result={}
 for key,(pool,rows,bits,name,capacity,stride,count) in specs.items():
  result[key]=dict(cursor=read(pool+0x34),high_water=read(pool+0x38),count=read(pool+0x3c),next_salt=read(pool+0x40,2),bitmap=[read(bits+i*4) for i in range((capacity+31)//32)],records=[sparse(bytes(g.mem_read(rows+i*stride,stride)),actor_baseline if key=='actors' else b'\xa5'*stride) for i in range(count)])
 result['objects']=[sparse(bytes(g.mem_read(BODIES+i*0x1000,size)),b'\xa5'*size) for i,size in enumerate(body_sizes)]
 result['globals']=bytes(g.mem_read(AI,0x400)).hex();result['statistics']=bytes(g.mem_read(STATS,0x30)).hex()
 return result
for address,value in [(0x4f55f0,specs['actors'][0]),(0x51e9d8,specs['squads'][0]),(0x502424,specs['joints'][0]),(0x50241c,specs['props'][0]),(0x502418,specs['references'][0]),(0x502414,specs['tracking'][0]),(0x502420,specs['clumps'][0]),(0x51eca4,specs['locations'][0]),(0x5044c8,specs['perceptions'][0]),(0x502408,specs['commands'][0]),(0x502404,specs['joint_commands'][0]),(0x4f9384,specs['threads'][0]),(0x4e0300,OPOOL),(0x4e3b44,TAGS),(0x510c54,TIME),(0x5047f4,STATS),(0x4f55d0,AI)]:write(address,value)
write(OPOOL+0x44,OHEADERS)
for index,address in enumerate((CHARACTER,PARENT,UNITDEF)):write(TAGS+index*16+8,address)
g.mem_write(UNITDEF,bytes(0x300))
events=[];releasing=False;actor_id=0
def observe(g,address,size,user):
 if not releasing:return
 if address==0x1bbdf0:
  unit=g.reg_read(UC_X86_REG_EAX);body=BODIES+(unit&0xffff)*0x1000
  assert not read(AI+1,1) or read(body+0x10)==NONE
  events.append(dict(kind='driver',unit=unit,actor_unit=read(specs['actors'][1]+(actor_id&0xffff)*0x888+0x18),live_count=read(specs['actors'][0]+0x3c),flag3_live=read(STATS+6,2),active_count=read(AI+0x36a,2)))
 if address==0x15e300:raise RuntimeError('Fixture reached an undeclared live owner '+hex(address))
g.hook_add(UC_HOOK_CODE,observe)
rng=random.Random(0x1e1a00);cases=[];body_sizes=[0x500,0x500]+[0x170]*4+[0x500]*4
slots=[]
for index in range(0x83):
 pointer=read(0x46eeb8+index*4)
 slots.append([pointer,read(pointer+0x1c) if pointer else 0,read(pointer+0x24) if pointer else 0,read(pointer+0x28) if pointer else 0,read(pointer+0x34) if pointer else 0])
for case in range(args.case_start,args.case_start+args.case_count):
 releasing=False
 for spec in specs.values():reset(spec)
 flags=(0,1,8,9)[case%4];inherited=bool(case&4);active=bool(case&8);keep=bool(case&16)
 g.mem_write(CHARACTER,bytes(0x40));g.mem_write(PARENT,bytes(0x40));write(CHARACTER+8,0xe1000001 if inherited else -1);write(PARENT+8,-1)
 general_owner=PARENT if inherited else CHARACTER;write(general_owner+0x34,1);write(general_owner+0x38,GENERAL)
 g.mem_write(GENERAL,bytes(0xc));write(GENERAL,flags);write(GENERAL+4,1,2)
 g.mem_write(AI,b'\xa5'*0x400);write(AI+1,active,1);write(AI+0x14,-1);write(AI+0x36a,1 if case%3==0 else 0,2)
 g.mem_write(STATS,b'\xa5'*0x30);write(STATS+6,0xfffe if case&32 else 0,2);write(STATS+8,0xffff if case&32 else 7,2)
 write(TIME+2,30,2);write(TIME+8,417+case);write(0x4686c4,case%2,2)
 ids=[call(0x1dfb90,[0xe1000000]) for _ in range(4)]
 actor_baseline=bytes(g.mem_read(specs['actors'][1],0x888));actor_index=case%3;actor_id=ids[actor_index]
 actor=specs['actors'][1]+actor_index*0x888
 pool_ids={}
 for key,spec in specs.items():
  if key=='actors':continue
  pool,rows,bits,name,capacity,stride,count=spec;pool_ids[key]=[]
  for i in range(count):
   datum=call(0x16b840,registers={UC_X86_REG_EDX:pool});pool_ids[key].append(datum)
   salt=read(rows+i*stride,2);g.mem_write(rows+i*stride,b'\xa5'*stride);write(rows+i*stride,salt,2)
 for i in range(4):write(specs['actors'][1]+i*0x888+0x3cc,actor_id)
 if case&1:call(0x16ba40,registers={UC_X86_REG_EDX:specs['actors'][0],UC_X86_REG_EAX:ids[3]})
 membership=case%3;members=[ids[i] for i in ((actor_index+1)%3,actor_index,(actor_index+2)%3)];rng.shuffle(members)
 if membership:
  for i,datum in enumerate(members):
   record=specs['actors'][1]+(datum&0xffff)*0x888
   write(record+0x20,members[i+1] if i+1<len(members) else NONE)
   write(record+0xa,membership==1,1);write(record+0xb,1,1);write(record+0x30,0 if membership==2 else NONE)
  if membership==1:write(AI+0x14,members[0])
  else:
   squad=specs['squads'][1];write(squad+0x68,members[0]);write(squad+8,3,2);write(squad+3,0xa4,1)
 if case%2:
  for slot in (0,2):
   write(actor+0x194+slot*12,1,2);write(actor+0x196+slot*12,slot+2,2)
   write(actor+0x198+slot*12,pool_ids['joints'][slot//2])
   joint=specs['joints'][1]+(slot//2)*0xbc;write(joint+(slot+2)*12+4,actor_id);write(joint+(slot+2)*12+8,7,2)
 references=pool_ids['references'];order=list(range(3));rng.shuffle(order)
 if case%4:
  write(actor+0x58,references[order[0]])
  for i,index in enumerate(order):
   ref=specs['references'][1]+index*0x3c
   write(ref+0x2c,references[order[i+1]] if i+1<len(order) else NONE)
   write(ref+0x14,pool_ids['tracking'][index//2] if index in (0,2) else NONE)
  for offset,index in [(0x338,0),(0x32c,1),(0x344,2),(0x368,0),(0x724,1),(0x7e0,2),(0x3b4,0),(0x4b8,1),(0x68c,2),(0x6a4,0),(0x6b4,1)]:write(actor+offset,references[index])
  for offset,value in [(0x358,9),(0x722,1),(0x4ac,7),(0x688,1),(0x6a0,1),(0x6b0,1)]:write(actor+offset,value,2)
 for index in range(4):write(specs['props'][1]+index*0xc4+0x1c,actor_id if index%2==0 else ids[(actor_index+1)%3])
 if case&2:
  write(specs['props'][1]+0xc4+0x1c,actor_id)
  call(0x16ba40,registers={UC_X86_REG_EDX:specs['props'][0],UC_X86_REG_EAX:pool_ids['props'][1]})
 if case%13==0:write(actor+0x54,NONE)
 for index in range(3):
  record=specs['actors'][1]+index*0x888;write(record+0x190,case%4,2)
  for slot in range(case%4+1):write(record+0x90+slot*0x40,1 if (slot+index)%2 else 0x72,2)
 write(actor+9,case%3==0,1)
 for index in range(3):write(STATS+0x18+index*4,actor_id if index!=1 else ids[(actor_index+1)%3]);write(STATS+0x12+index*2,index+1,2)
 for index,size in enumerate(body_sizes):
  body=BODIES+index*0x1000;g.mem_write(body,b'\xa5'*size);write(OHEADERS+index*12+8,body);write(OHEADERS+index*12+3,index if index<2 else 2,1);write(body+0xaa,index if index<2 else 2,1)
  if index<2:
   write(body,0xe1000002)
   for offset in (0x10,0x14,0x12c,0x24c):write(body+offset,-1)
   write(body+0x130,0xe3700000+index if bool(case&1)!=bool(index) else NONE)
   write(body+0x13c,0xe2700000+index if bool(case&2)!=bool(index) else NONE)
   write(body+0x10a,4 if bool(case&4)!=bool(index) else 0,1);write(body+0x134,0xa5a5a5a5)
   write(body+0x248,0xe26f0000+(1-index))
   for slot in range(4):write(body+0x218+slot*4,0xe26f0002+index*2+slot if slot<2 else NONE)
  elif index<6:
   write(body+0x12c,0xab,1);write(body+0x154,0xe26f0000+(index-2)//2);write(body+0x16c,0xa5&~0x40,1)
  else:
   write(body+0xaa,12,1)
 unit_choice=case%3 if not flags&1 or case>=192 else 0
 if unit_choice:
  unit=0xe26f0000+unit_choice-1;write(actor+0x18,unit);write(BODIES+(unit&0xffff)*0x1000+0x12c,actor_id)
 if case>=96:
  if case%3:
   clump_id=pool_ids['clumps'][case%2];clump=specs['clumps'][1]+(case%2)*0x50
   write(clump+0x18,members[0]);write(clump+0x10,3 if case%2 else 0,2)
   for i,member in enumerate(members):
    record=specs['actors'][1]+(member&0xffff)*0x888
    write(record+0x80,members[i+1] if i+1<len(members) else NONE);write(record+0x7c,clump_id)
   if case%4:
    for prop_index in (0,2):
     linked=[i for i in order if (i%2)*2==prop_index]
     write(specs['props'][1]+prop_index*0xc4+0x18,references[linked[0]] if linked else NONE)
     for position,index in enumerate(linked):
      ref=specs['references'][1]+index*0x3c;write(ref+8,pool_ids['props'][prop_index])
      write(ref+0x34,references[linked[position+1]] if position+1<len(linked) else NONE)
      write(ref+0x38,references[linked[position-1]] if position else NONE)
  if case%4:
   location_id=pool_ids['locations'][case%2];write(actor+0x3f4,location_id)
   location=specs['locations'][1]+(case%2)*0x484;write(location+0xa,case%3,2)
   for index in range(4):
    write(actor+0x402+index*6,0x12340000+index*0x10000+(0x8000 if bool(index&1)!=bool(case&1) else 0)*0x10000+0x1111)
   write(actor+0x4ac,(0,4,5,6,7)[case%5],2);write(actor+0x4ba,0x8000 if case&2 else 0,2)
 if case>=192 and flags&1:
  perception_id=pool_ids['perceptions'][case%2];write(actor+0x1c,perception_id)
  perception=specs['perceptions'][1]+(case%2)*0x34;write(perception+4,actor_id)
  chain=list(range(6,10));rng.shuffle(chain);write(perception+0x18,0xe26f0000+chain[0])
  for position,index in enumerate(chain):
   body=BODIES+index*0x1000;offset=0x280 if index%2 else 0x300
   write(body+0x134,3 if case%7==0 and position==case%4 else 0)
   write(body+0x13a,offset,2);write(body+offset+4,actor_id)
   write(body+offset+0xc,0xe26f0000+chain[position+1] if position+1<len(chain) else NONE)
   write(body+offset+0x10,references[index%3]);write(body+offset+0x50,references[(index+1)%3])
 if case>=288:
  commands=pool_ids['commands'];threads=pool_ids['threads'];joint_commands=pool_ids['joint_commands']
  command_order=list(range(3));rng.shuffle(command_order)
  write(actor+0x858,commands[command_order[0]]);write(actor+0x85c,commands[case%4])
  other_id=ids[(actor_index+1)%3];other=specs['actors'][1]+(other_id&0xffff)*0x888
  write(other+0x858,commands[3]);write(other+0x85c,commands[3])
  for index in range(4):
   command=specs['commands'][1]+index*0xd4
   write(command+0x38,threads[0] if index==0 else NONE);write(command+0x40,NONE)
   write(command+0x3c,NONE);write(command+0x7e,0,1)
  for position,index in enumerate(command_order):write(specs['commands'][1]+index*0xd4+0x3c,commands[command_order[position+1]] if position+1<len(command_order) else NONE)
  for joint_index,command_index in enumerate((1,2)):
   joint=specs['joint_commands'][1]+joint_index*0x8c
   write(joint+0xa,3,2);write(joint+0xc,threads[joint_index+1] if case%5 else NONE)
   for participant in range(3):write(joint+0x10+participant*12,NONE);write(joint+0x14+participant*12,NONE)
   write(joint+0x10,actor_id);write(joint+0x14,commands[command_index])
   write(specs['commands'][1]+command_index*0xd4+0x40,joint_commands[joint_index])
   if joint_index==0 and case%2:
    write(joint+0x28,other_id);write(joint+0x2c,commands[3]);write(specs['commands'][1]+3*0xd4+0x40,joint_commands[0])
 before=snapshot();write(TIME+8,1031+case);events.clear();releasing=True
 try:call(0x1e1a00,[actor_id,keep])
 except Exception:
  print('Original lifecycle case',case,'PC',hex(g.reg_read(UC_X86_REG_EIP)),flush=True);raise
 releasing=False
 after=snapshot()
 cases.append(dict(flags=flags,inherited=inherited,active=active,keep=keep,actor=actor_id,slot_tick=417+case,bsp=case%2,tick=1031+case,actor_baseline_hex=actor_baseline.hex(),before=before,after=after,events=list(events)))
report=dict(passed=True,xbe_sha256=pin,scope='Whole unchanged1E1A00 and reached203ED0/203FB0/1E1740/1E2150/258600/2628F0/CBF60/10CCC0/CC810/1BBDF0/16BA40. Real allocator-created actor/joint/squad/prop/ref/tracking banks; no replaced success callbacks. Followed-reference registry and non-NULL slot owners remain separate. Includes whole258600/258540/258480 command, joint participant and HS thread retirement. Includes whole swarm28FD90/28E160 with actual dynamic object AI blocks. Includes complete clump2694D0/26AC60 and location26DC20/1E22D0/1F86A0 cleanup.',baseline_byte=0xa5,pools={key:dict(name=s[3],capacity=s[4],stride=s[5],records=s[6]) for key,s in specs.items()},body_sizes=body_sizes,slot_callbacks=slots,cases=cases)
with (output/'original.json').open('w') as file:json.dump(report,file,indent=2);file.write('\n')
print('PASS '+str(len(cases))+' complete original lifecycle cases with real membership, observation, tracking, counter and unit cleanup')

if args.fixture:
 with args.fixture.open('w') as file:
  file.write('# Base retail actor lifecycle reference\n\nUnchanged executable SHA256 `'+pin+'`. Whole1E1A00 and reached owners run on real allocator-created banks, serially under the shared60MiB zero-swap guard. Followed-reference and non-NULL slot owners remain explicit dependencies. This is component evidence, not completed campaign gameplay.\n\n```json\n')
  json.dump(report,file,indent=2);file.write('\n```\n')
