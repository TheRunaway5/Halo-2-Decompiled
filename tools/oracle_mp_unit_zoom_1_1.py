#!/usr/bin/env python3
"""Independent original zoom leaves and full equipped unit zoom consumer.

Native requested selectors are explicit incoming command inputs. Admission
controls deliberately vary individual context fields, labelled as such; they
do not claim those fields were produced by an ordinary game frame. The stock
composition uses unchanged allocated/carried SMG and source unit construction
from the frozen attachment fixture. No action, sound or world success is added.
"""
import argparse,csv,hashlib,json,struct
from pathlib import Path
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
 UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_XMM0)
from oracle_mp_weapon_attachment_1_1 import AttachmentTrace
from oracle_mp_inventory_1_1 import ROOT,PIN,CACHE_PIN,UNIT,UI,HEADERS,TIME,STOP
from oracle_mp_inventory_controls_1_1 import ACTION
from oracle_smg_firing_1_1 import _trace

DEPENDENCY_PIN='9895480d4d864d67ee92307d627c04a005e4ffb8f06761be68c9de0d7274563c'
WATCH=(0xc9920,0xc9cc0,0xc9be0,0x1051c0,0x105120,0x105240,0x104a30,
 0xe8610,0x14e780,0x150dd0,0x150f00,0x151c90,0xc8140,0xc6c50,0x1045e0)
EVIDENCE=(0xc52c0,0xbd270,0xc9a40,0xe88b0,0xe8950,0xe8740,0xe7e60,0xe7ff0)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def h(v):return f'{v&0xffffffff:08x}'
def sh(t,p):return struct.unpack('<h',t.g.mem_read(p,2))[0]
def sb(t,p):return struct.unpack('<b',t.g.mem_read(p,1))[0]
class ZoomTrace(AttachmentTrace):
 def __init__(self):
  self.observe=False;self.zoom_calls=[];self.zoom_returns=[];self.zoom_stores=[]
  super().__init__()
  starts={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
  self.cleanup={};self.listings={}
  for p in (*WATCH,*EVIDENCE):
   f=_trace(self.code,p,starts|set(WATCH));ins=[self.code.at(q)for q in sorted(f.code)]
   ret={int(i.op_str,0)if i.op_str else 0 for i in ins if i.mnemonic.startswith('ret')}
   # C9CC0/C9BE0 tail-call leaves have a returning branch with the same ABI.
   assert len(ret)==1,(h(p),ret)
   if p in WATCH:self.cleanup[p]=next(iter(ret))
   self.listings[h(p)]='\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'for i in ins)+'\n'
  self.wp=self.u(HEADERS+(self.initial_datum&65535)*12+8)
  self.wdef=self.u(0x4eb114)+(self.u(self.wp)&65535)*16
  self.wdef=self.u(self.wdef+8)
  self.info=self.u(self.u(0x4e791c)+0x134)
  self.control=UNIT+sh(self,UNIT+0x346)
  self.observe=True
 def context(self):
  return {'parent':h(self.u(UNIT+0x14)),'action_word':h(self.u(self.control+4)),
   'unit_definition_flags':h(self.u(self.u(self.u(0x4eb114)+(self.u(UNIT)&65535)*16+8)+0xbc)),
   'unit_object_flags_10a':self.u(UNIT+0x10a,1),
   'current':sb(self,UNIT+0x240),'requested':sb(self,UNIT+0x241),'motion':h(self.u(UNIT+0x26c)),
   'weapon_flags':h(self.u(self.wdef+0x12c)),'weapon_count':sh(self,self.wdef+0x1fe),
   'weapon_minimum':h(self.u(self.wdef+0x200)),'weapon_maximum':h(self.u(self.wdef+0x204)),
   'magazine_count':self.u(self.wdef+0x2c0),'magazine_modes':[sh(self,self.wp+0x224+n*16)for n in range(self.u(self.wdef+0x2c0))],
   'binocular_count':struct.unpack('<i',self.g.mem_read(self.info+0xb8,4))[0],
   'binocular_minimum':h(self.u(self.info+0xbc)),'binocular_maximum':h(self.u(self.info+0xc0)),
   'input_flags':self.u(self.wp+0x16e,2),'unit_full_800_hex':bytes(self.g.mem_read(UNIT,0x800)).hex(),
   'action_record_44_hex':bytes(self.g.mem_read(self.control,0x44)).hex(),
   'weapon_runtime_16c_260_hex':bytes(self.g.mem_read(self.wp+0x16c,0xf4)).hex()}
 def hook(self,g,p,size,user):
  if self.observe:
   sp=g.reg_read(UC_X86_REG_ESP)
   for ret,end_sp,row in self.zoom_returns[:]:
    if p==ret and sp==end_sp:
     row['return_eax']=h(g.reg_read(UC_X86_REG_EAX));row['return_xmm0_low']=h(g.reg_read(UC_X86_REG_XMM0));row['after']=self.context()
     self.zoom_returns.remove((ret,end_sp,row))
   if p in WATCH:
    row={'phase':self.active,'entry':h(p),'entry_esp':h(sp),'return_site':h(self.u(sp)),
     'source_ret_cleanup':self.cleanup[p],'eax':h(g.reg_read(UC_X86_REG_EAX)),
     'ebx':h(g.reg_read(UC_X86_REG_EBX)),'ecx':h(g.reg_read(UC_X86_REG_ECX)),
     'edx':h(g.reg_read(UC_X86_REG_EDX)),'before':self.context(),
     'stack_args':[h(self.u(sp+4+n*4))for n in range(3)]}
    self.zoom_calls.append(row);self.zoom_returns.append((self.u(sp),sp+4+self.cleanup[p],row))
  super().hook(g,p,size,user)
 def mem_write(self,g,access,p,size,value,user):
  if self.observe and any(a<=p and p+size<=a+n for a,n in ((UNIT,0x800),(self.control,0x44),(self.wp+0x16c,0xf4),(ACTION,0x5c))):
   self.zoom_stores.append({'phase':self.active,'instruction':h(g.reg_read(UC_X86_REG_EIP)),
    'address':h(p),'size':size,'before_hex':bytes(g.mem_read(p,size)).hex(),
    'after_hex':(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()})
  super().mem_write(g,access,p,size,value,user)
 def run(self,a,argv=(),regs=()):
  super().run(a,argv,regs)
  if self.observe:
   sp=self.g.reg_read(UC_X86_REG_ESP)
   for ret,end_sp,row in self.zoom_returns[:]:
    if ret==STOP and end_sp==sp:
     row['return_eax']=h(self.g.reg_read(UC_X86_REG_EAX));row['return_xmm0_low']=h(self.g.reg_read(UC_X86_REG_XMM0));row['after']=self.context()
     self.zoom_returns.remove((ret,end_sp,row))
 def measure(self,name,entry,argv=(),regs=(),boundary=None):
  self.active=name;before=self.context();calls=len(self.zoom_calls);stores=len(self.zoom_stores)
  self.run(entry,argv,regs)
  return {'name':name,'entry':h(entry),'supplied_boundary':boundary,'before':before,'after':self.context(),
   'return_eax':h(self.g.reg_read(UC_X86_REG_EAX)),'return_xmm0_low':h(self.g.reg_read(UC_X86_REG_XMM0)),
   'calls':self.zoom_calls[calls:],'ordered_stores':self.zoom_stores[stores:]}

def generate(out):
 assert sha(ROOT/'tools/oracle_mp_weapon_attachment_1_1.py')==DEPENDENCY_PIN
 t=ZoomTrace();initial=t.context();cases=[];composition=[]
 cases.append(t.measure('stock_admission',0xc9920,regs=((UC_X86_REG_EAX,UI),)))
 for current in (-32768,-2,-1,0,1,32767):
  cases.append(t.measure(f'stock_cycle_{current}',0xc9cc0,regs=((UC_X86_REG_EAX,UI),(UC_X86_REG_ECX,current&65535)),
   boundary={'incoming_current_signed16':current}))
 for current in (-1,0,1):
  cases.append(t.measure(f'stock_magnification_{current}',0xc9be0,regs=((UC_X86_REG_EAX,UI),(UC_X86_REG_EBX,current&65535)),
   boundary={'incoming_current_signed16':current}))
 # All bit tests are explicit synthetic branch boundaries, not active-action
 # producers. Restore source context after each measured admission leaf.
 for bit in range(32):
  t.w(t.control+4,1<<bit)
  cases.append(t.measure(f'action_bit_{bit}',0xc9920,regs=((UC_X86_REG_EAX,UI),),boundary={'modified_context_action_word':h(1<<bit)}))
 t.w(t.control+4,int(initial['action_word'],16))
 t.w(UNIT+0x14,UI)
 cases.append(t.measure('parent_present',0xc9920,regs=((UC_X86_REG_EAX,UI),),boundary={'modified_parent':h(UI)}))
 t.w(UNIT+0x14,int(initial['parent'],16))
 for mode in (-1,0,1,2,3,4,5,6,7):
  t.sh(t.wp+0x224,mode)
  cases.append(t.measure(f'magazine_mode_{mode}',0xc9920,regs=((UC_X86_REG_EAX,UI),),boundary={'modified_magazine_mode':mode}))
 t.sh(t.wp+0x224,initial['magazine_modes'][0])
 # Leaf count/flag/range controls retain source arithmetic and signed boundaries.
 for count in (0,1,2,3,-1):
  t.w(t.info+0xb8,count)
  for current in (-1,0,1,2):
   cases.append(t.measure(f'binocular_count_{count}_current_{current}',0xc9cc0,regs=((UC_X86_REG_EAX,UI),(UC_X86_REG_ECX,current&65535)),
    boundary={'modified_binocular_count':count,'incoming_current_signed16':current}))
 t.w(t.info+0xb8,initial['binocular_count'])
 t.w(t.wdef+0x12c,int(initial['weapon_flags'],16)|0x10000)
 cases.append(t.measure('no_binocular_flag',0xc9cc0,regs=((UC_X86_REG_EAX,UI),(UC_X86_REG_ECX,65535)),boundary={'modified_weapon_flags':h(int(initial['weapon_flags'],16)|0x10000),'incoming_current_signed16':-1}))
 t.w(t.wdef+0x12c,int(initial['weapon_flags'],16))
 for count in (1,2,3):
  t.sh(t.wdef+0x1fe,count)
  for mode in (0,2):
   t.sh(t.wp+0x224,mode)
   for current in (-1,0,1,2):
    cases.append(t.measure(f'weapon_count_{count}_mode_{mode}_current_{current}',0xc9cc0,regs=((UC_X86_REG_EAX,UI),(UC_X86_REG_ECX,current&65535)),
     boundary={'modified_weapon_zoom_count':count,'modified_magazine_mode':mode,'incoming_current_signed16':current}))
 t.sh(t.wdef+0x1fe,initial['weapon_count']);t.sh(t.wp+0x224,initial['magazine_modes'][0])
 for count,lo,hi in ((1,3.,3.),(1,2.,8.),(2,2.,8.),(3,2.,8.),(3,0.,-1.)):
  t.w(t.info+0xb8,count);t.f(t.info+0xbc,lo);t.f(t.info+0xc0,hi)
  for current in (-1,0,1,2):
   cases.append(t.measure(f'binocular_range_{count}_{lo}_{hi}_current_{current}',0xc9be0,regs=((UC_X86_REG_EAX,UI),(UC_X86_REG_EBX,current&65535)),
    boundary={'modified_binocular_definition':{'count':count,'minimum':h(t.u(t.info+0xbc)),'maximum':h(t.u(t.info+0xc0))},'incoming_current_signed16':current}))
 t.w(t.info+0xb8,initial['binocular_count']);t.w(t.info+0xbc,int(initial['binocular_minimum'],16));t.w(t.info+0xc0,int(initial['binocular_maximum'],16))
 for count,lo,hi in ((1,2.,2.),(1,2.,8.),(1,0.,8.),(1,-1.,8.),(2,2.,8.)):
  t.sh(t.wdef+0x1fe,count);t.f(t.wdef+0x200,lo);t.f(t.wdef+0x204,hi)
  for current in (-1,0,1):
   cases.append(t.measure(f'weapon_magnification_{count}_{lo}_{hi}_current_{current}',0xc9be0,
    regs=((UC_X86_REG_EAX,UI),(UC_X86_REG_EBX,current&65535)),
    boundary={'modified_weapon_definition':{'count':count,'minimum':h(t.u(t.wdef+0x200)),'maximum':h(t.u(t.wdef+0x204))},'incoming_current_signed16':current,
     'native_multilevel_power_consumer_unported':count>1 and 0<=current<count}))
 t.sh(t.wdef+0x1fe,initial['weapon_count']);t.w(t.wdef+0x200,int(initial['weapon_minimum'],16));t.w(t.wdef+0x204,int(initial['weapon_maximum'],16))
 for tick,selector in enumerate((0,-1,0,-1)):
  t.tick=tick;t.w(TIME+8,tick)
  neutral=t.measure(f'composition_{tick}_neutral',0x14e780,regs=((UC_X86_REG_EDX,ACTION),))
  t.sh(ACTION+0x24,selector)
  supplied=bytes(t.g.mem_read(ACTION,0x5c)).hex()
  player=t.measure(f'composition_{tick}_pressed_edges',0x150dd0,regs=((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,ACTION)),boundary={'native_incoming_zoom_selector':selector,'incoming_neutral_command_with_selector_hex':supplied})
  actions=t.measure(f'composition_{tick}_player_actions',0x150f00,(0,UI,ACTION))
  inputs=t.measure(f'composition_{tick}_unit_input',0x151c90,(UI,ACTION),((UC_X86_REG_EAX,0),))
  unit=t.measure(f'composition_{tick}_full_unit_phase',0xc6c50,(UI,))
  composition.append({'tick':tick,'native_incoming_zoom_selector':selector,'neutral':neutral,'player':player,'actions':actions,'inputs':inputs,'unit_phase':unit})
 update_controls=[]
 ud=t.u(t.u(0x4eb114)+(t.u(UNIT)&65535)*16+8)
 for name,action,mode,definition_bits,object_bits in (
   ('admitted_on',0,0,0,0),('magazine_denied',0,2,0,0),
   ('action26_denied',1<<26,0,0,0),('action27_denied',1<<27,0,0,0),
   ('definition_bit10_skips',0,0,1<<10,0),('object_bit2_skips',0,0,0,4)):
  t.g.mem_write(UNIT+0x240,b'\x00\xff' if definition_bits or object_bits else b'\xff\x00')
  t.w(UNIT+0x26c,0xaabbccdd);t.w(t.control+4,action);t.sh(t.wp+0x224,mode)
  t.w(ud+0xbc,int(initial['unit_definition_flags'],16)|definition_bits)
  t.g.mem_write(UNIT+0x10a,bytes([initial['unit_object_flags_10a']|object_bits]))
  update_controls.append(t.measure(name,0xc6c50,(UI,),boundary={'modified_branch_context':True,
   'action_word':h(action),'magazine_mode':mode,'unit_definition_added_bits':h(definition_bits),'unit_object_added_bits':object_bits,
   'zoom_pair_and_motion_explicit_fixture':True}))
 t.w(t.control+4,int(initial['action_word'],16));t.sh(t.wp+0x224,initial['magazine_modes'][0])
 t.w(ud+0xbc,int(initial['unit_definition_flags'],16));t.g.mem_write(UNIT+0x10a,bytes([initial['unit_object_flags_10a']]))
 out.mkdir(parents=True,exist_ok=True);(out/'instructions').mkdir(exist_ok=True)
 for p,b in t.listings.items():(out/'instructions'/f'{p}.asm').write_text(b)
 report={'tool_sha256':sha(__file__),'attachment_dependency_sha256':DEPENDENCY_PIN,'original_xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
  'initial':initial,'leaf_cases':cases,'composition':composition,'unit_update_controls':update_controls,
  'inherited_services':[{ 'entry':h(p),'contract':v}for p,v in t.services.items()],
  'owning_unit_fixture':t.player_pose,'source_constructor_calls':t.source_calls,
  'scope':'Original zoom leaves and full equipped unit consumer. Native incoming selectors and synthetic branch contexts are explicitly labelled. The inherited unit is preallocated, its initial action-word allocation/reset producer remains outside this fixture; source unit constructor owns zoom FF defaults. No new game or platform return is supplied. No camera FOV/overlay, zoom animation, full match action lifecycle or local-session claim.',
  'native_gameplay_update_applied':False}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({'cases':len(cases),'compositions':len(composition),'report_sha256':sha(out/'report.json')}),flush=True)
 t.cache.data.close();t.cache.stream.close()
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args();generate(a.output)
if __name__=='__main__':main()
