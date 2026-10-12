#!/usr/bin/env python3
"""Execute original PC/Xbox birth->simulation-query->Slayer publication.

This proof uses explicit creation records and simulation/session inputs. Original
configuration application, local output binding, controller binding, replication
and event delivery are recorded external services. No instruction is patched.
It is not a live world, profile sanitizer, network session or audible cue proof.
"""
from pathlib import Path
import argparse, hashlib, itertools, json, struct, sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_game_variants import Source
from oracle_game_over import SPECS
from oracle_vista_pcr_screen import machine
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
PC_SHA='33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109'
LOCAL=bytes.fromhex('123456789abc'); OTHER=bytes.fromhex('a1b2c3d4e5f6')
PC_BASE,PC_STOP,PC_STACK,PC_DATA=0x400000,0x1800000,0x1900000,0x1a00000
X_DATA=0x1100000
NATIVE={
 'retail':dict(birth=0x14c090,activation=0x14d2a0,pool=0x4e8c24,players=0x4e8c20,config=0x14be90,user=0x14f190,controller=0x14f270,query=0x686e0,siminit=0x4cf770,simworld=0x4cf77c,lookup=0x6ad40,network=0xa7c50,local=0x19eb30,notify=0xb58c0),
 'tu11':dict(birth=0x14d800,activation=0x14e9e0,pool=0x4f01f4,players=0x4f01f0,config=0x14d600,user=0x1508b0,controller=0x150950,query=0x688f0,siminit=0x4d6d38,simworld=0x4d6d44,lookup=0x6b210,network=0xa8a30,local=0x1a2490,notify=0xb6760),
 'u5':dict(birth=0x14ebe0,activation=0x14fdc0,pool=0x4f54f4,players=0x4f54f0,config=0x14e9e0,user=0x151c90,controller=0x151d30,query=0x69ad0,siminit=0x4dc040,simworld=0x4dc04c,lookup=0x6c3f0,network=0xb1e80,local=0x1a8740,notify=0xb7e50),
}
def r32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def w32(u,a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
def r16(u,a):return struct.unpack('<H',u.mem_read(a,2))[0]
def w16(u,a,v):u.mem_write(a,struct.pack('<H',v&0xffff))
def hx(v):return f'{v:08x}'

class Original:
 def __init__(self,name):
  self.name=name;self.ispc=name=='pc';self.executed=set();self.instructions=0
  if self.ispc:
   self.path=ROOT/'Halo 2 Vista installer/halo2.exe';self.raw=self.path.read_bytes();assert hashlib.sha256(self.raw).hexdigest()==PC_SHA
   self.u=machine(self.raw);self.data=PC_DATA;self.stack=PC_STACK+0x10000;self.stop=PC_STOP;self.s=None
   self.distributed=0x1b00000;self.u.mem_map(self.distributed,0x20000)
   self.meta=dict(birth=0x455163,activation=0x45339b,pool=0x8a8260,players=0x8a825c,config=0x45502a,user=0x4539dc,controller=0x451720,query=0x5ae0cb,siminit=0x9178d0,simworld=0x9178dc,lookup=0x5dcf5f,network=0x5b67ea,local=0x45dbf0,notify=0x5b6662)
   self.callback=0x510c2b
  else:
   self.s=Source(SPECS[name]);self.u=self.s.g;self.s.call(SPECS[name]['ctor']);self.instructions=0
   self.path=ROOT/SPECS[name]['path'];self.raw=self.path.read_bytes();self.data=X_DATA;self.stack=0x2008000;self.stop=0x3000000;self.u.mem_map(X_DATA,0x20000)
   self.meta=NATIVE[name];obj=r32(self.u,SPECS[name]['table']+8);self.callback=r32(self.u,r32(self.u,obj)+0x18)
  self.world=self.data;self.sim=self.data+0x2000;self.watcher=self.data+0x4000;self.pool=self.data+0x5000;self.playerbase=self.data+0x6000;self.bits=self.data+0xb000;self.creation=self.data+0xc000;self.pg=self.data+0xd000;self.clock=self.data+0xe000;self.engine=self.data+0xf000;self.session=self.data+0x10000
  self.u.hook_add(UC_HOOK_CODE,self.hook);self.u.hook_add(UC_HOOK_MEM_WRITE,self.write)
  self.calls=[];self.stores=[];self.phase='setup'
  self.fragment_stop=None;self.fragment_external=None
 def ret(self,value=None,pop=0):
  sp=self.u.reg_read(UC_X86_REG_ESP)
  if value is not None:self.u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
  self.u.reg_write(UC_X86_REG_EIP,r32(self.u,sp));self.u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def event(self,name,**args):self.calls.append(dict(phase=self.phase,call=name,**args))
 def hook(self,u,pc,size,_):
  self.executed.add(pc);self.instructions+=1;sp=u.reg_read(UC_X86_REG_ESP);m=self.meta
  if pc==self.fragment_stop:u.emu_stop();return
  if pc==self.fragment_external:self.ret();return
  if pc==self.stop:u.emu_stop();return
  if self.ispc and pc==0x455163:
   self.current_creation=u.reg_read(UC_X86_REG_EBX);self.event('player_constructor_input',datum=r32(u,sp+4),new_byte=r32(u,sp+8)&255,creation_pointer=self.current_creation)
  if self.ispc and pc in (0x5adc4b,0x4b5044,0x5b6609):
   self.event('world_join_followup_external' if pc in (0x5adc4b,0x4b5044) else 'replication_mask_external',entry=hx(pc));self.ret();return
  if pc==m['config']:
   pointer=r32(u,sp+8) if self.ispc else u.reg_read(UC_X86_REG_EAX)
   self.event('configuration_application_external',datum=r32(u,sp+4),creation_offset=pointer-getattr(self,'current_creation',self.creation),profile_hex=bytes(u.mem_read(pointer,0x84 if self.ispc else 0x90)).hex());self.ret(pop=0 if self.ispc else 4);return
  if pc==m['user']:
   self.event('local_output_binding_external',datum=r32(u,sp+4),user=r32(u,sp+8));self.ret(pop=0 if self.ispc else 8);return
  if pc==m['controller']:
   self.event('controller_binding_external',datum=r32(u,sp+4),argument=r32(u,sp+8));self.ret(pop=0 if self.ispc else 8);return
  if pc==m['query']:self.event('simulation_query',pointer=r32(u,sp+4) if self.ispc else u.reg_read(UC_X86_REG_EAX))
  if pc==m['lookup']:
   self.event('remote_view_lookup_external',provided_result=0);self.ret(0,pop=4);return
  if pc==self.callback:self.event('slayer_activation',datum=r32(u,sp+4))
  if pc in (m['network'],m['local']):
   pointer=r32(u,sp+4) if self.ispc else u.reg_read(UC_X86_REG_EDX if pc==m['network'] else UC_X86_REG_EDI)
   self.event('network_delivery_external' if pc==m['network'] else 'local_delivery_external',defined34=bytes(u.mem_read(pointer,0x22)).hex(),active_flags_at_delivery=r16(u,self.player+2));self.ret();return
  if pc==m['notify']:
   self.event('replication_notification_external',datum=r32(u,sp+4),mask=r32(u,sp+8));self.ret(pop=0 if self.ispc else 8);return
  if self.ispc and pc in (0x5dbf99,0x5dbfb5):
   self.event('publish_simulation_identity' if pc==0x5dbf99 else 'publish_simulation_local_index',owner=u.reg_read(UC_X86_REG_ECX),argument=r32(u,sp+4))
 def write(self,u,access,a,n,value,_):
  if self.phase=='spawn_completion_fragment':return
  if a==getattr(self,'player',-1)+2:self.stores.append(dict(phase=self.phase,pc=hx(u.reg_read(UC_X86_REG_EIP)),size=n,value=value&((1<<(8*n))-1)))
 def call(self,entry,args=(),registers=()):
  u=self.u;u.mem_write(self.stack,struct.pack('<I',self.stop)+b''.join(struct.pack('<I',a&0xffffffff) for a in args));u.reg_write(UC_X86_REG_ESP,self.stack);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.reg_write(UC_X86_REG_EFLAGS,2)
  for reg,val in registers:u.reg_write(reg,val&0xffffffff)
  try:u.emu_start(entry,self.stop+1,count=250000)
  except Exception as e:raise RuntimeError(f'{self.name} entry{entry:x} pc{u.reg_read(UC_X86_REG_EIP):x} phase{self.phase}: {e}') from e
  assert u.reg_read(UC_X86_REG_EIP)==self.stop,(self.name,hx(entry),hx(u.reg_read(UC_X86_REG_EIP)))
 def setup(self,q,watcher='options'):
  u=self.u;m=self.meta;u.mem_write(self.data,bytes(0x20000));u.mem_write(self.stack-0x8000,b'\xa5'*0x10000)
  self.current_creation=self.creation;self.calls=[];self.stores=[];self.phase='setup';self.before=self.instructions;self.stride=0x204 if self.ispc else 0x21c;self.player=self.playerbase+q['slot']*self.stride;self.datum=(q['salt']<<16)|q['slot']
  for owner,pointer in ((m['pool'],self.pool),(m['players'],self.pg),(m['simworld'],self.sim)):w32(u,owner,pointer)
  w32(u,self.pool+0x20,16);w32(u,self.pool+0x24,self.stride);w32(u,self.pool+0x38,16);w32(u,self.pool+0x44,self.playerbase);w32(u,self.pool+0x48,self.bits);w32(u,self.bits,1<<q['slot']);w16(u,self.player,q['salt']);w16(u,self.player+2,0xa5a5)
  w32(u,self.pg+0x2c,1);u.mem_write(self.pg+0x30,LOCAL);w32(u,self.pg+(0xa0 if self.ispc else 0x98),0)
  u.mem_write(self.creation,bytes([0xa6])*0xd4);u.mem_write(self.creation,bytes([1,q['left']]));w16(u,self.creation+2,q['user']);w32(u,self.creation+4,q['controller']);u.mem_write(self.creation+8,OTHER if q['world']=='mismatched_machine' else LOCAL)
  identity=bytes.fromhex('0011223344556677') if self.ispc else bytes.fromhex('001122334455667789abd0ba')
  u.mem_write(self.creation+0xe,identity);off=0x18 if self.ispc else 0x1c;u.mem_write(self.creation+off,bytes((i*17+q['slot'])&255 for i in range(0x84 if self.ispc else 0x90)))
  w32(u,self.clock+8,0x12345678)
  if self.ispc:
   mode=q.get('mode',1)
   w32(u,0x882d3c,self.world);w32(u,0x8c06e4,self.clock);w32(u,self.world+8,2);u.mem_write(self.world+0xc,bytes([mode]));u.mem_write(0x91abc8,b'\x01');w32(u,0x91abcc,0x34567890)
   options=self.world+8;w32(u,options+0x3d4,1);u.mem_write(options+0x3d8,LOCAL);u.mem_write(options+0x43e,b'\x01');u.mem_write(options+0x43f,LOCAL)
   u.mem_write(0x91c45c,bytes([watcher=='session']));u.mem_write(0x820fc0,bytes([watcher=='session']));w32(u,0x820fe8,self.session);w32(u,0x820ff4,self.data+0x18000)
   w32(u,self.session+0x73a0,5);w32(u,self.session+0x70,7);w32(u,self.session+0x84,1);w32(u,self.session+0x64,0);w32(u,self.session+0x71c0,0);u.mem_write(self.session+0x92,LOCAL)
   u.mem_write(self.distributed,bytes(0x20000))
   self.phase='simulation_constructor';self.call(0x5ddb4e,(0,self.watcher,self.distributed),((UC_X86_REG_ECX,self.sim),));assert r32(u,self.sim+8)=={1:1,2:3,3:2,4:5,5:4}[mode]
   self.phase='watcher_constructor';self.call(0x5d5a7f,(self.sim,),((UC_X86_REG_ECX,self.watcher),))
   self.phase='watcher_publication';self.call(0x5d484f,registers=((UC_X86_REG_ECX,self.watcher),));assert bytes(u.mem_read(self.sim+0xd,6))==LOCAL;assert r32(u,self.sim+0x14)==0;assert u.mem_read(self.sim+0xc,1)==b'\x01'
   w32(u,0x8bf8f8,self.engine);w32(u,self.engine+0xc54,1);w32(u,0x8d8548+4,self.engine);w32(u,self.engine,0x7c31d4)
  else:
   w32(u,SPECS[self.name]['globals'],self.world);w32(u,SPECS[self.name]['clock'],self.clock);w32(u,SPECS[self.name]['engine'],self.engine);w32(u,self.engine+0xc14,2)
   for i in range(16):w32(u,self.engine+0x2c+i*4,0xffffffff)
   w32(u,self.sim+8,1);u.mem_write(self.sim+0xd,LOCAL)
  u.mem_write(m['siminit'],bytes([q['world']!='simulation_not_initialized']));u.mem_write(self.world+0xc,bytes([4 if q['world']=='film' else q.get('mode',1)]))
 def run(self,q,watcher='options'):
  self.setup(q,watcher);u=self.u;m=self.meta;self.phase='player_birth'
  if self.ispc:self.call(m['birth'],(self.datum,q['new_byte']),((UC_X86_REG_EBX,self.creation),))
  else:self.call(m['birth'],(self.datum,q['new_byte'],self.creation))
  born=r16(u,self.player+2);expected=(8 if q['new_byte'] else 0)|(2 if q['left'] else 0);assert born==expected,(self.name,q,born,expected)
  machine=bytes(u.mem_read(self.player+(0x10 if self.ispc else 0x14),6)).hex();assert machine==bytes(u.mem_read(self.creation+8,6)).hex()
  user=r16(u,self.player+(0x18 if self.ispc else 0x1c));controller=r32(u,self.player+(0x1c if self.ispc else 0x20));assert user==q['user']&0xffff and controller==q['controller']&0xffffffff
  self.phase='activation_first';self.call(m['activation']);first=r16(u,self.player+2);first_events=[c['defined34'] for c in self.calls if c['phase']=='activation_first' and c['call']=='local_delivery_external']
  active=not q['left'] and q['world']!='film' and q['world']!='mismatched_machine';assert first==(born|int(active))
  assert len(first_events)==int(active),(self.name,q,first_events)
  self.phase='activation_second';self.call(m['activation']);second=r16(u,self.player+2);assert second==first
  assert not any(c['phase']=='activation_second' and c['call']=='local_delivery_external' for c in self.calls)
  if active:
   event=bytes.fromhex(first_events[0]);assert struct.unpack_from('<III',event)==(2,0,self.datum)
   assert all(c['active_flags_at_delivery']&1 for c in self.calls if c['call'].endswith('delivery_external'))
  return dict(input=q,watcher_input=watcher if self.ispc else 'explicit_simulation_world',simulation_kind=r32(u,self.sim+8),born_flags=born,first_flags=first,second_flags=second,machine_hex=machine,creation_user=user,creation_controller=controller,event_defined34_hex=first_events,calls=self.calls,flag_stores=self.stores,instructions=self.instructions-self.before)
 def load_world_row(self,q,watcher):
  assert self.ispc;self.setup(q,watcher);u=self.u
  row=self.world+8+0x448+q['slot']*0xd4;u.mem_write(row,bytes(u.mem_read(self.creation,0xd4)))
  u.mem_write(self.playerbase,bytes(16*self.stride));w32(u,self.bits,0);w32(u,self.pool+0x38,0);w32(u,self.pool+0x3c,0);w16(u,self.pool+0x40,q['salt']);w32(u,self.pg,0)
  u.mem_write(0x86dcf0,b'\x00');u.mem_write(0x9178db,b'\x00');u.mem_write(0x8b1c80,b'\x00');u.mem_write(0xe9e9e4,b'\x00'*4);w32(u,0x9178e0,self.watcher)
  u.mem_write(self.clock+4,struct.pack('<f',1/30));w32(u,self.clock+8,0);self.phase='original_world_player_loader'
  self.call(0x449ebf)
  constructors=[c for c in self.calls if c['call']=='player_constructor_input'];assert len(constructors)==1;assert constructors[0]['creation_pointer']==row;assert constructors[0]['new_byte']==1;assert constructors[0]['datum']==self.datum
  flags=r16(u,self.player+2);assert flags==8|(2 if q['left'] else 0);assert r16(u,self.player)==q['salt'];assert r32(u,self.bits)==1<<q['slot'];assert r32(u,self.pool+0x3c)==1
  self.phase='activation_first';self.call(self.meta['activation']);first=r16(u,self.player+2)
  events=[c['defined34'] for c in self.calls if c['phase']=='activation_first' and c['call']=='local_delivery_external'];active=not q['left'];assert len(events)==int(active);assert first==flags|int(active)
  self.phase='activation_second';self.call(self.meta['activation']);assert not any(c['phase']=='activation_second' and c['call']=='local_delivery_external' for c in self.calls)
  return dict(input=q,watcher_input=watcher,simulation_kind=r32(u,self.sim+8),distributed_resource_present=r32(u,self.sim+4)==self.distributed,original_entry='00449ebf',options_hex=bytes(u.mem_read(self.world+8,0x1188)).hex(),published_machine_hex=bytes(u.mem_read(self.sim+0xd,6)).hex(),options_row_offset=hx(row-self.world-8),allocated_datum=hx(self.datum),born_flags=flags,first_flags=first,defined_event_hex=events,calls=self.calls,flag_stores=self.stores,instructions=self.instructions-self.before)
 def probe_watcher(self,label,changes):
  assert self.ispc
  q=dict(slot=0,salt=0x1234,new_byte=1,left=0,user=0,controller=0,world='local');self.setup(q,'session')
  u=self.u;self.calls=[];self.stores=[];self.phase='watcher_guard_probe'
  options=self.world+8
  for at,value,width in changes:u.mem_write(at(self) if callable(at) else at,(value&((1<<(width*8))-1)).to_bytes(width,'little'))
  u.mem_write(self.sim+0xd,b'\xcc'*6)
  self.call(0x5ddb4e,(0,self.watcher,0),((UC_X86_REG_ECX,self.sim),));self.call(0x5d5a7f,(self.sim,),((UC_X86_REG_ECX,self.watcher),));self.call(0x5d484f,registers=((UC_X86_REG_ECX,self.watcher),))
  return dict(label=label,changes=[dict(address=hx(at(self) if callable(at) else at),value=value,width=width) for at,value,width in changes],simulation_kind=r32(u,self.sim+8),valid_byte=u.mem_read(self.sim+0xc,1)[0],local_index=hx(r32(u,self.sim+0x14)),identifier6_hex=bytes(u.mem_read(self.sim+0xd,6)).hex(),watcher_session_present=r32(u,self.watcher+0xc)!=0,calls=self.calls,instructions=self.instructions-self.before)
 def spawn_flags(self,output):
  # Successful-spawn continuation only; no spawn selection/allocation claimed.
  # Output user NONE skips HUD owners. The final presentation helper is external.
  fragments={'pc':(0x455cce,0x455d46,0x45422e,0x8c06e4),
      'retail':(0x14f64f,0x14f6c5,0x152340,0x510c54),
      'tu11':(0x150d2f,0x150da5,0x153a90,0x518444),
      'u5':(0x15210f,0x152185,0x154e70,0x51df54)}
  entry,end,external,clock_owner=fragments[self.name];u=self.u
  self.player=self.playerbase;self.phase='spawn_completion_fragment';self.fragment_stop=end;self.fragment_external=external
  w32(u,clock_owner,self.clock);w16(u,self.clock+2,30);w16(u,self.player+(0x24 if self.ispc else 0x28),0xffff)
  result=bytearray();before=self.instructions
  for flags in range(65536):
   w16(u,self.player+2,flags);u.mem_write(self.stack,struct.pack('<I',self.stop));u.reg_write(UC_X86_REG_ESP,self.stack);u.reg_write(UC_X86_REG_EBP,self.player);u.reg_write(UC_X86_REG_EAX,0);u.reg_write(UC_X86_REG_EFLAGS,2)
   u.emu_start(entry,end+1,count=1000);assert u.reg_read(UC_X86_REG_EIP)==end
   actual=r16(u,self.player+2);assert actual==(flags&(0xf3f7 if self.ispc else 0xf9f7));result.extend(struct.pack('<H',actual))
  path=output/f'{self.name}-spawn-flags.bin';path.write_bytes(result);self.fragment_stop=None;self.fragment_external=None
  return dict(entry=hx(entry),end_exclusive=hx(end),external_presentation=hx(external),output_user='NONE',clock_rate=30,cases=65536,instructions=self.instructions-before,path=path.name,sha256=hashlib.sha256(result).hexdigest())
 def disasm(self):
  md=Cs(CS_ARCH_X86,CS_MODE_32);lines=[]
  for a in sorted(self.executed):
   if a==self.stop:continue
   raw=self.raw[a-PC_BASE:a-PC_BASE+15] if self.ispc else self.s.image.read(a,15)
   ins=next(md.disasm(raw,a));lines.append(f'{a:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}')
  return '\n'.join(lines)+'\n'

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/vista-session-flow-audit/player-birth-original');args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
 cases=[dict(slot=slot,salt=0x1234,new_byte=new,left=left,user=0 if bound else -1,controller=0 if bound else -1,world=world) for slot,new,left,bound,world in itertools.product((0,3,15),(0,1,2,255),(0,1),(False,True),('local','mismatched_machine','simulation_not_initialized','film'))]
 result=dict(source={},cases_per_input=len(cases),scope=__doc__,input_services=['configuration application','local output/controller binding','remote view lookup=NULL','replication notification','network/local event delivery'],cases={})
 reference=None
 for name in ('pc','retail','tu11','u5'):
  o=Original(name);rows=[]
  for q in cases:
   row=o.run(q);rows.append(row)
   if name=='pc':
    alternate=o.run(q,'session');assert all(row[k]==alternate[k] for k in ('born_flags','first_flags','second_flags','machine_hex','creation_user','creation_controller','event_defined34_hex'));rows.append(alternate)
  if name=='pc':
   reference=rows[::2]
   watcher_cases=[('session_active',[]),('network_inactive_fallback',[(0x91c45c,0,1)]),('manager_inactive_fallback',[(0x820fc0,0,1)]),('session_inactive_fallback',[(lambda x:x.session+0x73a0,0,4)]),('revision_NONE_fallback',[(lambda x:x.session+0x70,0xffffffff,4)]),('options_identity_absent',[(0x91c45c,0,1),(lambda x:x.world+8+0x43e,0,1)]),('options_no_valid_machine',[(0x91c45c,0,1),(lambda x:x.world+8+0x3d4,0,4)]),('options_identity_unmatched',[(0x91c45c,0,1),(lambda x:x.world+8+0x43f,0,4)]),('session_count_zero_publishes_zero',[(lambda x:x.session+0x84,0,4)]),('option_mode2_constructs_kind3',[(lambda x:x.world+0xc,2,1)]),('non_multiplayer_kind3_uses_options',[(lambda x:x.world+8,3,4)])]
   result['watcher_guard_cases']=[o.probe_watcher(label,changes) for label,changes in watcher_cases]
   for row in result['watcher_guard_cases']:
    if row['label'] in ('options_identity_absent','options_no_valid_machine','options_identity_unmatched'):
     assert row['valid_byte']==0 and row['local_index']=='ffffffff' and row['identifier6_hex']=='cccccccccccc'
    elif row['label']=='session_count_zero_publishes_zero':assert row['valid_byte']==1 and row['identifier6_hex']=='000000000000'
    else:assert row['valid_byte']==1 and row['local_index']=='00000000' and row['identifier6_hex']==LOCAL.hex()
   result['world_loader_cases']=[o.load_world_row(dict(slot=slot,salt=0x1234,new_byte=255,left=left,user=0 if bound else -1,controller=0 if bound else -1,world='local'),watcher) for slot,left,bound,watcher in itertools.product((0,3,15),(0,1),(False,True),('options','session'))]
   mode5_inputs=[(dict(slot=slot,salt=0x1234,new_byte=1,left=left,user=0 if bound else -1,controller=0 if bound else -1,world='local',mode=5),watcher) for slot,left,bound,watcher in itertools.product((0,3,15),(0,1),(False,True),('options','session'))]
   result['mode5_cases']=[o.run(q,watcher)for q,watcher in mode5_inputs]
   result['world_loader_cases'] += [o.load_world_row(q,watcher)for q,watcher in mode5_inputs]
   mode3_inputs=[(dict(slot=slot,salt=0x1234,new_byte=1,left=left,user=0 if bound else -1,controller=0 if bound else -1,world='local',mode=3),watcher) for slot,left,bound,watcher in itertools.product((0,3,15),(0,1),(False,True),('options','session'))]
   result['mode3_cases']=[o.run(q,watcher)for q,watcher in mode3_inputs]
   result['world_loader_cases'] += [o.load_world_row(q,watcher)for q,watcher in mode3_inputs]
   assert all(r['simulation_kind']==2 for r in result['mode3_cases'])
   assert all(r['simulation_kind']==4 for r in result['mode5_cases'])
   assert all(r['distributed_resource_present'] for r in result['world_loader_cases'] if r['input'].get('mode')==5)
  else:
   for pc,native in zip(reference,rows):
    assert all(pc[k]==native[k] for k in ('born_flags','first_flags','second_flags','machine_hex','creation_user','creation_controller','event_defined34_hex')),(name,pc,native)
  result.setdefault('spawn_completion',{})[name]=o.spawn_flags(args.output)
  result['source'][name]=dict(path=str(o.path),sha256=hashlib.sha256(o.raw).hexdigest(),entries={k:hx(v) for k,v in o.meta.items()},slayer_callback=hx(o.callback),executed_instructions=o.instructions,executed_unique_instructions=len(o.executed),cases=len(rows)+65536+(len(result.get('world_loader_cases',[]))+len(result.get('watcher_guard_cases',[]))+len(result.get('mode5_cases',[]))+len(result.get('mode3_cases',[])) if name=='pc' else 0))
  result['cases'][name]=rows;(args.output/f'{name}-executed-original.asm').write_text(o.disasm())
  print(name,len(rows),o.instructions,flush=True)
 result['total_cases']=sum(x['cases'] for x in result['source'].values());result['executed_instructions']=sum(x['executed_instructions'] for x in result['source'].values());result['tool_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();(args.output/'execution.json').write_text(json.dumps(result,indent=2)+'\n')
 fixture=dict(source=result['source'],total_cases=result['total_cases'],executed_instructions=result['executed_instructions'],tool_sha256=result['tool_sha256'],birth_cases=[dict(source=name,input=row['input'],born_flags=row['born_flags']) for name,rows in result['cases'].items() for row in rows]+[dict(source='pc',input=row['input'],born_flags=row['born_flags'])for row in result['mode5_cases']+result['mode3_cases']],world_loader_cases=result['world_loader_cases'],spawn_completion=result['spawn_completion'])
 (ROOT/'tests/fixtures/vista_player_birth_original.json').write_text(json.dumps(fixture,separators=(',',':'))+'\n');print('PASS',result['total_cases'],result['executed_instructions'])
if __name__=='__main__':main()
