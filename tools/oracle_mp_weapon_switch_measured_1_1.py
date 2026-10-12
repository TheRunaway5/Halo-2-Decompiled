#!/usr/bin/env python3
"""New observation-only full-buffer original ordinary switch measurement."""
import argparse, hashlib, json, struct
from pathlib import Path
import oracle_mp_inventory_equipped_1_1 as base
from oracle_mp_inventory_1_1 import ROOT, UNIT, UI, POOL, HEADERS, TIME, SEED, STOP, _trace
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_ESI, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/weapon-switch-measured'
FROZEN={
 'tools/oracle_mp_inventory_equipped_1_1.py':'47f56a9236ea2efe870d1ad82af16ace214f4598294f8c826966cced94dee655',
 'analysis/multiplayer/integration/inventory-source/equipped-pickup/report.json':'d6cec76de0a155f2c5817b61d8073d4555ddb70efa48e731b349fd8ff742c00b',
 'tools/research_mp_inventory_switch_u5.py':'fe51d55efa84a5e6bc4809bedf424fa9d56aa6320ac2cdeed7ed520e77f1c219',
 'analysis/multiplayer/integration/inventory-source/inventory-switch-u5/report.json':'358511090ee3be1cce652923afc1979b759d2d48b63b45287ca25ff48d3251d2',
}
ENTRIES=(0xe9fe0,0xea180,0xea370,0xea270,0xe9b40,0xe9a90,0xe9d50,
 0x112270,0x115800,0x115870,0x1042e0,0x1040f0,0x115650,0x111500,
 0xc9a40,0xefe30,0xefee0,0x104500,0x10a140,0xe7ff0,
 0x1d0ed0,0x1cf100,0x1ca2b0,0x1ca090,0x1cae40,0x1de890,
 0x16a06a,0x169f3b,0x1de7a0,0x148160,0x167d32,
 0xe8610,0xceaf0,0xe9c50,0x111490,0xd1d20,0x105750,0x108230,0xfb3f0)
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,data):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==data,p
 else:p.write_bytes(data)

class SwitchTrace(base.EquippedPickupTrace):
 def __init__(self,*args,**kwargs):
  self.switch_calls=[];self.switch_returns=[];self.switch_stores=[];self.switch_order=0
  super().__init__(*args,**kwargs)
 def switch_snapshot(self):
  regions=[dict(kind='unit_prefix',pointer=f'{UNIT:08x}',offset='000',hex=bytes(self.g.mem_read(UNIT,0x800)).hex())]
  for at,size,name in((0x12a,0x90,'primary_manager'),(0x33e,0x124,'extension'),(0x342,0x58,'extra'),(0x346,0x44,'control')):
   offset=struct.unpack('<h',bytes(self.g.mem_read(UNIT+at,2)))[0]
   if offset!=-1:regions.append(dict(kind=name,pointer=f'{UNIT+offset:08x}',offset=f'{offset:04x}',hex=bytes(self.g.mem_read(UNIT+offset,size)).hex()))
  objects=[]
  n=self.u(POOL+0x38)
  assert n<=64,n
  for index in range(n):
   h=HEADERS+index*12;pointer=self.u(h+8);length=self.u(h+6,2)
   if pointer and self.u(h+3,1)==2 and length:
    objects.append(dict(datum=f'{self.u(h,2)<<16|index:08x}',pointer=f'{pointer:08x}',header_hex=bytes(self.g.mem_read(h,12)).hex(),
     raw_hex=bytes(self.g.mem_read(pointer,length)).hex(),definition=f'{self.u(pointer):08x}'))
  return dict(regions=regions,weapons=objects,tick=self.u(TIME+8),time12_hex=bytes(self.g.mem_read(TIME,12)).hex(),seed=f'{self.u(SEED):08x}',
   fpcw=f'{self.g.reg_read(UC_X86_REG_FPCW):04x}',mxcsr=f'{self.g.reg_read(UC_X86_REG_MXCSR):08x}')
 def finish_switch_returns(self,address):
  for ret,row in self.switch_returns[:]:
   if ret==address:
    self.switch_order+=1;row.update(return_order=self.switch_order,return_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',after=self.switch_snapshot())
    if 'owning_buffer_pointer'in row:
     row['owning_buffer_after_hex']=bytes(self.g.mem_read(int(row['owning_buffer_pointer'],16),row['owning_buffer_bytes'])).hex()
    if row['entry'] in ('001de890','00169f3b'):
     p=self.g.reg_read(UC_X86_REG_EAX)
     row['animation_record_pointer']=f'{p:08x}'
     if p:
      row['animation_record_raw6c_hex']=bytes(self.g.mem_read(p,0x6c)).hex()
      n=self.u(p+0x4c);assert n<=64,n
      row['frame_event_count']=n
      row['frame_event_raw_hex']=bytes(self.g.mem_read(self.u(p+0x50),n*4)).hex()if n else''
    self.switch_returns.remove((ret,row))
 def hook(self,g,address,size,user):
  self.finish_switch_returns(address)
  if address in ENTRIES:
   sp=g.reg_read(UC_X86_REG_ESP);self.switch_order+=1
   row=dict(order=self.switch_order,phase=getattr(self,'active',None),entry=f'{address:08x}',registers=self.registers(),
    return_address=f'{self.u(sp):08x}',stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)],before=self.switch_snapshot())
   if address==0xe9fe0:row['request32_hex']=bytes(g.mem_read(self.u(sp+8),32)).hex()
   if address==0x16a06a:
    datum=int(row['registers']['ecx'],16)
    pointer=self.u(HEADERS+(datum&0xffff)*12+8)
    tag=self.cache.tags[self.u(pointer)]
    row['duration_weapon_definition']=dict(datum=f'{datum:08x}',pointer=f'{pointer:08x}',
     tag=tag,raw360_hex=bytes(g.mem_read(tag['offset']+self.cache.magic,0x360)).hex())
   if address==0x1de7a0:
    pointer=int(row['registers']['ecx'],16);n=self.u(pointer+0x4c);assert n<=64,n
    row['event_selector_input']=dict(pointer=f'{pointer:08x}',event_type=f'{g.reg_read(UC_X86_REG_ESI)&0xffff:04x}',
     record_raw6c_hex=bytes(g.mem_read(pointer,0x6c)).hex(),frame_event_count=n,
     frame_event_raw_hex=bytes(g.mem_read(self.u(pointer+0x50),n*4)).hex()if n else'')
   buffer=None
   if address==0x1ca2b0:buffer=(g.reg_read(UC_X86_REG_EAX),0x20)
   if address==0x1ca090:buffer=(self.u(sp+4),0x20)
   if address in(0x1cf100,0x1d0ed0):buffer=(g.reg_read(UC_X86_REG_ESI),0x90)
   if buffer:
    row.update(owning_buffer_pointer=f'{buffer[0]:08x}',owning_buffer_bytes=buffer[1],owning_buffer_before_hex=bytes(g.mem_read(*buffer)).hex())
   self.switch_calls.append(row);self.switch_returns.append((self.u(sp),row))
  super().hook(g,address,size,user)
 def run(self,*args,**kwargs):
  super().run(*args,**kwargs);self.finish_switch_returns(STOP)
 def mem_write(self,g,access,address,size,value,user):
  if self.switch_returns:
   owner=None
   if UNIT<=address and address+size<=UNIT+0x800:owner=dict(kind='unit_prefix',pointer=f'{UNIT:08x}',offset=f'{address-UNIT:03x}')
   for at,length,name in((0x12a,0x90,'primary_manager'),(0x33e,0x124,'extension'),(0x342,0x58,'extra'),(0x346,0x44,'control')):
    offset=struct.unpack('<h',bytes(g.mem_read(UNIT+at,2)))[0]
    if offset!=-1 and UNIT+offset<=address and address+size<=UNIT+offset+length:
     owner=dict(kind=name,pointer=f'{UNIT+offset:08x}',offset=f'{address-UNIT-offset:03x}')
   for index in range(self.u(POOL+0x38)):
    h=HEADERS+index*12;pointer=self.u(h+8);length=self.u(h+6,2)
    if pointer and self.u(h+3,1)==2 and pointer<=address and address+size<=pointer+length:
     owner=dict(kind='weapon',datum=f'{self.u(h,2)<<16|index:08x}',pointer=f'{pointer:08x}',offset=f'{address-pointer:03x}');break
   if owner:
    self.switch_order+=1
    self.switch_stores.append(dict(order=self.switch_order,phase=getattr(self,'active',None),instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',bytes=size,
     before_hex=bytes(g.mem_read(address,size)).hex(),after_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),active_calls=[r['order']for _,r in self.switch_returns],**owner))
  super().mem_write(g,access,address,size,value,user)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);a=p.parse_args()
 for path,pin in FROZEN.items():assert sha(ROOT/path)==pin,path
 frozen=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/equipped-pickup/report.json').read_text())
 for pin in frozen['tool_dependencies']:assert sha(ROOT/pin['path'])==pin['sha256'],pin['path']
 # First rerun the exact unmodified prior suite and compare its complete cases.
 prior_traces,prior_cases=base.run_suite();assert prior_cases==frozen['cases']
 # Observation-only Python subclass; no source guest state/instruction/result
 # is changed. The prior suite's case payloads must still match exactly.
 base.EquippedPickupTrace=SwitchTrace
 traces,cases=base.run_suite()
 if cases!=prior_cases:
  def differences(x,y,path=''):
   if type(x)!=type(y):return [(path,x,y)]
   if isinstance(x,dict):return [d for k in x.keys()|y.keys()for d in differences(x.get(k),y.get(k),path+'/'+str(k))]
   if isinstance(x,list):
    if len(x)!=len(y):return [(path+'/length',len(x),len(y))]
    return [d for k,(a,b)in enumerate(zip(x,y))for d in differences(a,b,path+'/'+str(k))]
   return []if x==y else[(path,x,y)]
  delta=differences(prior_cases,cases)
  print(json.dumps(dict(changed_prior_paths=[d[0]for d in delta[:20]],count=len(delta))),flush=True)
  raise AssertionError('Observation changes prior output; new evidence remains unfrozen')
 listings=[];executed=set().union(*(t.executed for t in traces));t=traces[0]
 for entry in ENTRIES:
  f=_trace(t.code,entry,t.starts|set(ENTRIES));ins=[t.code.at(x)for x in sorted(f.code)]
  path=a.output_dir/'instructions'/f'{entry:08x}.asm'
  for i in ins:assert t.image.read(i.address,i.size)==i.bytes
  save(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
  listings.append(dict(entry=f'{entry:08x}',file=str(path.relative_to(a.output_dir)),sha256=sha(path),rows=len(ins),executed_rows=sum(i.address in executed for i in ins)))
 measured=[]
 for index,(t,c)in enumerate(zip(traces,cases)):
  assert not t.switch_returns
  measured.append(dict(case=index,name=c['name'],original_case_correlation='complete case equality before and after observation subclass',
   calls=t.switch_calls,stores=t.switch_stores,initial_inventory=c['initial_inventory'],continuation=c['continuation'],
   authored_animation_records=c['authored_animation_records'],authored_target=c['target'],initial_weapon_resources=c['initial_weapon_resources'],
   inherited_supplied_services=c['supplied_services'],original_input_and_pose_scope=c['pose']))
 report=dict(schema=1,tool_sha256=sha(__file__),frozen_pins=FROZEN,inherited_dependency_pins=frozen['tool_dependencies'],
  original_source_pins=dict(xbe=frozen['xbe_sha256'],cache=frozen['cache_sha256'],shared=frozen['shared_cache_sha256']),
  cases=measured,listings=listings,prior_complete_case_repeat_verified=True,new_observation_changes_prior_cases=False,
  new_instruction_patches=[],new_guest_state_or_service_outputs=[],
  scope='Actual original initial-carry/type8/type18 manager/weapon/channel calls in immutable allocated-item held-input fixture. Explicit inherited preallocated-unit/player, near-root/contact/Havok/attachment/world-capacity and supplied continuation tick/input boundaries are retained. No new live natural-world contact, source tick-owner or1.5 call is asserted.')
 save(a.output_dir/'report.json',(json.dumps(report,sort_keys=True,indent=2)+'\n').encode())
 for path,pin in FROZEN.items():assert sha(ROOT/path)==pin,path
 print(json.dumps(dict(output=str(a.output_dir/'report.json'),sha256=sha(a.output_dir/'report.json'),cases=len(measured),calls=sum(len(c['calls'])for c in measured),stores=sum(len(c['stores'])for c in measured))))

if __name__=='__main__':main()
