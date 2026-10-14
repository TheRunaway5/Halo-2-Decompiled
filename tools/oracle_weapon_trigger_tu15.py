#!/usr/bin/env python3
"""Execute unchanged original charge/lock transition and analog trigger owners.

Explicit raw weapon/trigger/barrel/input/time storage is declared ABI input.
Effect allocation returns a supplied handle; zoom and weapon/unit-action
requests are captured unavailable services. The real CRT conversion executes.
This does not establish whole trigger scheduling, targeting or weapon-world parity.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Trace,ROOT,WEAPON,DEF,TRIGGER,BARREL,TIME,POOL,TAGS,WI,UI
from weapon_reference_versions import images
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP
CORRESPONDENCE=ROOT/'analysis/weapons-tu15-goal-20261013/trigger-correspondence/report.json'
OUT=0x10f0000
ENTRIES={'I':0x106960,'A':0x107c40,'B':0x107e90,'C':0x106f10,'D':0x106fc0,
         'E':0x107cc0,'F':0x107d20,'G':0x107df0,'H':0x107d80,'J':0x107120}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
class Machine(Trace):
 def __init__(self,later,report):
  self.instructions={};self.calls=[];self.allocated=0xffffffff
  super().__init__(1)
  mapping={int(r['old'],16):int(r['u5'],16) for r in report['owners']}
  self.entries={k:mapping[v] if later else v for k,v in ENTRIES.items()}
  calls={int(c['old_target'],16):int(c['new_target'],16) for r in report['owners'] for c in r['calls']}
  self.supplied={calls[a] if later else a:(kind,pop) for a,kind,pop in
      ((0x107b50,1,12),(0xc9a40,2,4),(0x109a60,3,12),(0x10a140,4,4))}
  operands={int(m['old_va'],16):int(m['new_va'],16) for r in report['owners'] for m in r['memory']}
  if later:
   self.image=next(image for label,image,_ in images() if label=='TU1.5')
   for section in self.image.sections:self.g.mem_write(section.va,self.image.section_bytes(section))
  self.globals={operands[a] if later else a:value for a,value in ((0x4e78d0,POOL),(0x4eb114,TAGS),(0x518444,TIME))}
  for a,value in self.globals.items():self.w(a,value)
 def hook(self,g,address,size,user):
  self.instructions.setdefault(address,bytes(g.mem_read(address,size)).hex())
  if address not in getattr(self,'supplied',{}):return
  kind,pop=self.supplied[address];sp=g.reg_read(UC_X86_REG_ESP)
  args=[self.u(sp+4+n*4) for n in range(pop//4)]
  if kind==1:self.calls.append([kind,g.reg_read(UC_X86_REG_EAX),*args]);result=self.allocated
  elif kind==2:self.calls.append([kind,g.reg_read(UC_X86_REG_EAX),*args]);result=0
  elif kind==3:
   assert args[0]==WI;self.calls.append([kind,*args[1:]]);result=0
  else:self.calls.append([kind,*args]);result=0
  g.reg_write(UC_X86_REG_EAX,result);g.reg_write(UC_X86_REG_ESP,sp+4+pop);g.reg_write(UC_X86_REG_EIP,self.u(sp))
 def execute(self,type_,slot,argument,rate,count,item_flags,owner,spent,allocated,definition,state,barrels,input_):
  self.g.mem_write(WEAPON,bytes(0x400));self.w(WEAPON,0);self.w(TAGS+8,DEF)
  self.w(DEF+0x2cc,TRIGGER);self.w(DEF+0x2d0,count);self.w(DEF+0x2d4,BARREL)
  self.g.mem_write(TRIGGER+slot*64,definition);self.g.mem_write(WEAPON+0x20c+slot*12,state)
  self.g.mem_write(WEAPON+0x1a4,barrels);self.g.mem_write(WEAPON+0x170,input_)
  self.g.mem_write(WEAPON+0x12c,bytes([item_flags]));self.w(WEAPON+0x154,owner);self.w(WEAPON+0x184,spent)
  self.sh(TIME+2,rate);self.allocated=allocated;self.calls=[];self.g.mem_write(OUT,bytes(16))
  registers=[(UC_X86_REG_EAX,WI)];args=[]
  if type_=='I':registers=[(UC_X86_REG_EAX,slot),(UC_X86_REG_ECX,WI)];args=[OUT,OUT+1,OUT+2]
  elif type_=='B':registers=[(UC_X86_REG_ECX,WI)];args=[slot]
  elif type_ in ('C','J'):registers.append((UC_X86_REG_ECX,slot if type_=='C' else argument))
  elif type_=='D':registers.append((UC_X86_REG_EDX,slot))
  else:args=[slot]
  before=bytes(self.g.mem_read(WEAPON,0x400));self.run(self.entries[type_],args,registers)
  raw=bytes(self.g.mem_read(WEAPON,0x400));normalized=bytearray(raw)
  for start,size in ((0x20c+slot*12,12),(0x1a4,104),(0x170,5)):normalized[start:start+size]=before[start:start+size]
  assert bytes(normalized)==before,'Unexpected original write outside declared trigger/barrel/input storage'
  result=(self.g.reg_read(UC_X86_REG_EAX)&255) if type_=='A' else sum(self.u(OUT+n,1)<<n for n in range(3)) if type_=='I' else 0
  return [raw[0x20c+slot*12:0x218+slot*12].hex(),raw[0x1a4:0x20c].hex(),raw[0x170:0x175].hex(),result,self.calls]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--scope',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
 report=json.loads(CORRESPONDENCE.read_text());machines=[Machine(later,report) for later in (False,True)]
 scope=json.loads(a.scope.read_text());contracts={};paths={}
 for map_ in scope['maps']:
  for weapon in map_['weapons']:
   if not weapon['resident']:continue
   name=weapon['tag']['name'];count=len(weapon['barrels']);paths.setdefault(name,dict(triggers=len(weapon['triggers']),barrels=count))
   assert count<=2
   for trigger in weapon['triggers']:
    raw=trigger['raw_hex'];contracts.setdefault((raw,count),dict(weapon=name,map=map_['path'],definition=raw,barrels=count))
 rng=random.Random(0x107880);rows=[];counts={k:0 for k in ENTRIES};controls=[0,0x3f800000,0xbf800000,0x3f000000,0x7fc00000,0x7f800000,0xff800000,0x4f800000,0xcf800000]
 def add(type_,definition,count,n):
  slot=n&1;argument=(-1,0,1,2)[n%4] if type_=='J' else slot
  rate=(-32768,-1,0,1,30,60,32767)[n%7];flags=(0,1,8,255)[n%4];owner=UI if n%3 else 0xffffffff
  spent=controls[n%len(controls)];allocated=(0xffffffff,0x81234567)[n%2]
  state=bytes([n%8,0xa5])+struct.pack('<hII',rng.choice((-1,0,1,32767)),rng.getrandbits(32),rng.getrandbits(32))
  barrels=bytearray(rng.randbytes(104))
  for i in range(2):barrels[i*52+1]=(0,1,2,255)[(n+i)%4]
  input_=bytes(rng.choice((0,1,20,21,127,128,249,250,255)) for _ in range(5))
  if type_=='I':input_=bytes((rng.randrange(256),rng.randrange(256),rng.randrange(256),(0,1,2,255)[n%4],(0,1,127,128,255)[n%5]))
  values=[type_,slot,argument,rate,count,flags,owner,spent,allocated,definition,state,bytes(barrels),input_]
  expected=[machine.execute(*values) for machine in machines];assert expected[0]==expected[1],(type_,n,expected)
  after,after_barrels,after_input,result,calls=expected[0]
  integers=[slot,argument&65535,rate&65535,count,flags,owner,spent,allocated]
  fields=[type_,*(f'{v&0xffffffff:08x}' for v in integers),definition.hex(),state.hex(),bytes(barrels).hex(),input_.hex(),after,after_barrels,after_input,f'{result:08x}',str(len(calls))]
  for call in calls:fields.extend([str(len(call)),*(f'{v&0xffffffff:08x}' for v in call)])
  rows.append(' '.join(fields));counts[type_]+=1
 for (raw,count),contract in contracts.items():
  definition=bytes.fromhex(raw)
  for type_ in ENTRIES:
   for n in range(32):add(type_,definition,count,n)
 # Additional raw analog inputs cover signed timer rollover and every byte.
 base=bytearray(next(iter(contracts))[0] and bytes.fromhex(next(iter(contracts))[0]))
 for n in range(4096):
  struct.pack_into('<I',base,0,rng.getrandbits(32));struct.pack_into('<II',base,0x10,rng.choice(controls),rng.choice(controls));add('I',bytes(base),2,n)
 a.fixture.write_text('\n'.join(rows)+'\n')
 for machine in machines:
  for address,raw in machine.instructions.items():assert machine.image.read(address,len(bytes.fromhex(raw)))==bytes.fromhex(raw)
 data=dict(scope=__doc__,schema='original-weapon-trigger-transitions-v1',cases=len(rows),counts=counts,authored_paths=paths,contracts=list(contracts.values()),instruction_patches=[],image_results_equal=True,fixture_sha256=sha(a.fixture),tool_sha256=sha(__file__),base_oracle_sha256=sha(ROOT/'tools/oracle_smg_firing_1_1.py'),scope_sha256=sha(a.scope),correspondence_sha256=sha(CORRESPONDENCE),images=[dict(xbe_sha256=hashlib.sha256(m.image.data).hexdigest(),supplied_services={f'{k:08x}':v for k,v in m.supplied.items()},globals={f'{k:08x}':f'{v:08x}' for k,v in m.globals.items()},reached_instruction_bytes={f'{k:08x}':v for k,v in sorted(m.instructions.items())}) for m in machines])
 a.output.write_text(json.dumps(data,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(cases=len(rows),contracts=len(contracts),paths=len(paths),counts=counts)))
if __name__=='__main__':main()
