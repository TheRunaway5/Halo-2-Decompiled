#!/usr/bin/env python3
"""Whole base and TU1.5 kill-volume state/scheduling owners on controlled banks.

HS argument delivery and return publication are declared interpreter services.
Object containment is a declared required callee; geometry, damage and whole
campaign boot are outside this component fixture. No original instruction is
patched and no kill-volume state or scheduling result is supplied.
"""
from pathlib import Path
import argparse,sys,json,hashlib,struct,random,gc
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from tag_match_memory_budget import enter_memory_budget
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output-dir',type=Path,required=True);ap.add_argument('--fixture',type=Path,required=True);args=ap.parse_args()
status=enter_memory_budget(args.output_dir,120)
if status is not None:raise SystemExit(status)
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP
images=[dict(name='base',path='/home/eric/.local/share/halo2-native/cache-reset-backups/20261012T201413Z/assets/disc/default.xbe',pin='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d',state_pointer=0x51e9c8,scenario_pointer=0x4e0350,time_pointer=0x510c54,ctor=0x1ec420,new_map=0x1ec480,dispose_map=0x1ec4b0,enable=0x2a0d30,disable=0x2a0d70,query=0x1ec500,argument_provider=0x209d50,publisher=0x209ae0,contains=0x11c5b0),dict(name='tu15',path='/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe',pin='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd',state_pointer=0x52be6c,scenario_pointer=0x4ecc20,time_pointer=0x51df54,ctor=0x1f4700,new_map=0x1f4760,dispose_map=0x1f4790,enable=0x2aaa40,disable=0x2aaa80,query=0x1f47e0,argument_provider=0x212010,publisher=0x211da0,contains=0x11ece0)]
STATE,SCENARIO,VOLUMES,KILLERS,TIME,ARGUMENT=0x1000000,0x1001000,0x1002000,0x1008000,0x1009000,0x100a000
STACK,STOP=0x200f000,0x2000000
allcases=[]
for spec in images:
 image=Xbe(spec['path']);assert hashlib.sha256(image.data).hexdigest()==spec['pin']
 g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
 for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
 g.mem_map(STATE,0x10000);g.mem_map(STOP,0x10000)
 def w(p,v,n=4):g.mem_write(p,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
 def u(p,n=4):return int.from_bytes(g.mem_read(p,n),'little')
 def ret(value,cleanup=0):
  sp=g.reg_read(UC_X86_REG_ESP);target=u(sp);g.reg_write(UC_X86_REG_EAX,value&0xffffffff);g.reg_write(UC_X86_REG_ESP,sp+4+cleanup);g.reg_write(UC_X86_REG_EIP,target)
 calls=[];published=[];ready=True;containment=False
 def hook(g,pc,size,user):
  sp=g.reg_read(UC_X86_REG_ESP)
  if pc==spec['argument_provider']:
   assert u(sp+4)==0x12345678 and u(sp+8)==1
   calls.append(dict(kind='arguments',ready=ready));ret(ARGUMENT if ready else 0,16)
  elif pc==spec['publisher']:
   published.append([g.reg_read(UC_X86_REG_ECX),g.reg_read(UC_X86_REG_EAX)]);ret(0)
  elif pc==spec['contains']:
   calls.append(dict(kind='contains',object=g.reg_read(UC_X86_REG_EAX),volume=u(sp+4)));ret(int(containment),4)
 g.hook_add(UC_HOOK_CODE,hook)
 def call(entry,argwords=(),eax=0,edi=0):
  w(STACK,STOP)
  for i,v in enumerate(argwords):w(STACK+4+i*4,v)
  g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_EAX,eax);g.reg_write(UC_X86_REG_EDI,edi)
  g.emu_start(entry,STOP,count=100000)
  assert g.reg_read(UC_X86_REG_EIP)==STOP
  return g.reg_read(UC_X86_REG_EAX)&255
 w(spec['state_pointer'],STATE);w(spec['scenario_pointer'],SCENARIO);w(spec['time_pointer'],TIME);w(SCENARIO+0x10c,VOLUMES);w(SCENARIO+0x234,KILLERS)
 rng=random.Random(0x1ec500)
 cases=[]
 for i in range(256):
  count=[0,1,2,31,32,33,64,255,256][i%9]
  indices=[-1,0,1,31,32,63,64,127,128,254,255]
  mappings=[indices[(i+j)%len(indices)] for j in range(24)]
  killers=[-1 if (i+j)%7==0 else (i+j)%24 for j in range(count)]
  initial=bytes(rng.getrandbits(8) for _ in range(36));g.mem_write(STATE,initial)
  w(SCENARIO+0x230,count);w(SCENARIO+0x108,len(mappings))
  for j,k in enumerate(mappings):w(VOLUMES+j*0x44+0x40,k,2)
  for j,k in enumerate(killers):w(KILLERS+j*2,k,2)
  row=dict(case=i,initial=initial.hex(),trigger_kill_indices=mappings,kill_triggers=killers,operations=[])
  operations=['new_map','disable','disable','enable','dispose_map','query','query','enable','query','new_map','query']
  # Alternate maps begin without a reset to retain disabled-bit boundaries.
  if i%2:operations[0]='dispose_map'
  for n,operation in enumerate(operations):
   before=bytes(g.mem_read(STATE,36));calls.clear();published.clear()
   trigger=-1 if (i+n)%13==0 else (i+n)%24
   ready=(i+n)%11!=0;containment=bool((i+n)&1);tick=(0,1,417,65536,0x7ffe0000)[(i+n)%5]
   obj=(0xffffffff if (i+n)%17==0 else ((i+1)<<16)|((i*157+n*71)&65535))
   result=None
   if operation in ('enable','disable'):
    w(ARGUMENT,trigger);call(spec[operation],(29 if operation=='enable' else 30,0x12345678,1))
   elif operation=='query':
    w(TIME+8,tick);result=call(spec['query'],edi=obj)
   else:call(spec[operation])
   row['operations'].append(dict(name=operation,trigger=trigger,arguments_ready=ready,tick=tick,object=obj,containment=containment,before=before.hex(),after=bytes(g.mem_read(STATE,36)).hex(),calls=list(calls),published=list(published),result=result))
  cases.append(row)
 allcases.append(cases);print(spec['name'],len(cases),'complete original state/scheduling cases',flush=True)
 del g,image;gc.collect()
assert allcases[0]==allcases[1],'Base/TU1.5 controlled banks differ'
report=dict(scope=__doc__,images=images,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),original_instruction_patches=[],declared_services=['HS argument delivery','HS zero-result publication','object containment callee with explicit result'],base_tu15_cases_equal=True,cases=allcases[1])
assert not args.fixture.exists();args.fixture.write_text(json.dumps(report,indent=2)+'\n')
print('PASS 256 paired base/TU1.5 cases and 2816 state/scheduling operations per original image')
