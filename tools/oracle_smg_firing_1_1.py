#!/usr/bin/env python3
"""Bounded original 1.1 SMG trigger trace; supplied services are recorded.

Executes original constructor/input/update/fire/error instructions unchanged.
Explicit engine service fixtures are not an entire original game simulation.
"""
import argparse,csv,hashlib,json,struct,sys
from collections import Counter,deque
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from inspect_multiplayer import Cache
from functions import _Code,_trace
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_INVALID,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
PIN='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
CACHE_PIN='14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
POOL,HEADERS,BITMAP,TAGS,WEAPON,UNIT,DEF,MAG,TRIGGER,BARREL,EFFECTS,TIME,SEED,GAME,FPS,USERS,PLAYERS,PLAYER_DATA,UNIT_DEF,PROJECTILE=[0x1000000+n*0x2000 for n in range(20)]
STACK,STOP=0x21fd000,0x3000000
TAGS=0x1080000
FPS=0x1048000
WI,UI=0x10000,0x10001
WORLD,WORLD_DATA,ENTITY_FLAGS=0x109a000,0x10a0000,0x10d0000
class Trace:
 def __init__(self,game_mode=1,aim=(1.,0.,0.)):
  self.image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
  assert hashlib.sha256(self.image.data).hexdigest()==PIN
  c=Cache(ROOT/'assets/disc/maps/lockout.map');assert hashlib.sha256(c.data).hexdigest()==CACHE_PIN
  t=next(t for t in c.tags.values() if t['group']=='weap' and t['name']=='objects\\weapons\\rifle\\smg\\smg')
  self.authored={'tag':t,'fields':{}}
  def record_values(offset,fields):
   return {name:{'relative_offset':f'{at:02x}','type':kind,'bits':bytes(c.data[offset+at:offset+at+struct.calcsize(kind)]).hex(),'value':struct.unpack_from('<'+kind,c.data,offset+at)[0]} for name,at,kind in fields}
  self.authored['weapon_values']=record_values(t['offset'],[('flags_12c',0x12c,'I'),('rate_scale_258',0x258,'f'),('style_292',0x292,'h')])
  self.g=Uc(UC_ARCH_X86,UC_MODE_32);self.g.mem_map(0x10000,0x800000)
  for s in self.image.sections:self.g.mem_write(s.va,self.image.section_bytes(s))
  self.g.mem_map(POOL,0x100000);self.g.mem_map(0x2000000,0x200000);self.g.mem_map(STOP,0x1000)
  metadata_page=c.magic&~0xfff;self.g.mem_map(metadata_page,((len(c.data)+(c.magic-metadata_page)+0xfff)//0x1000)*0x1000);self.g.mem_write(c.magic,bytes(c.data))
  for datum,tag in c.tags.items():
   if tag['offset'] is not None:self.w(TAGS+(datum&0xffff)*16+8,tag['offset']+c.magic)
  self.g.mem_write(DEF,bytes(c.data[t['offset']:t['offset']+0x310]))
  spartan=next(t for t in c.tags.values() if t['group']=='bipd' and t['name']=='objects\\characters\\masterchief\\masterchief_mp')
  self.authored['spartan']=spartan
  model_datum=c.u(t['offset']+0x38)
  model=next(tag for tag in c.tags.values() if int(tag['datum'],16)==model_datum)
  self.g.mem_write(0x1044000,bytes(c.data[model['offset']:model['offset']+model['size']]))
  self.w(TAGS+(model_datum&0xffff)*16+8,0x1044000)
  for offset,stride,target in ((0x2c0,0x5c,MAG),(0x2c8,0x40,TRIGGER),(0x2d0,0xec,BARREL)):
   offsets=c.block(t['offset']+offset,stride)
   assert len(offsets)==1
   self.g.mem_write(target,bytes(c.data[offsets[0]:offsets[0]+stride]));self.w(DEF+offset+4,target)
   self.authored['fields'][f'{offset:x}']={'file_offset':offsets[0],'size':stride}
   self.authored['fields'][f'{offset:x}']['raw_bytes']=bytes(c.data[offsets[0]:offsets[0]+stride]).hex()
  barrel_offset=self.authored['fields']['2d0']['file_offset']
  self.authored['barrel_values']=record_values(barrel_offset,[(f'field_{at:02x}',at,'f') for at in (0x04,0x08,0x18,0x20,0x24,0x50,0x54,0x58,0x5c,0x60,0x70,0x74,0x78,0x9c,0xa0,0xc4,0xc8,0xcc,0xd0,0xd4,0xd8,0xdc,0xe0)]+[(f'field_{at:02x}',at,'h') for at in (0x1c,0x1e,0x28,0x2a,0x2c,0x2e,0x34,0x36,0x6a)])
  fp=c.block(t['offset']+0x2a8,16)[0]
  graph=c.tags[c.u(fp+12)];clips=c.block(graph['offset']+0x2c,0x6c)
  self.authored['first_person_graph']=graph
  self.authored['reload_clips']={}
  for index in (30,31):
   clip=clips[index];sid=c.u(clip);string_index=sid&0xffff
   name=c.text(c.u(368)+c.u(c.u(364)+string_index*4))
   self.authored['reload_clips'][str(index)]={'name':name,'name_string_id':f'{sid:08x}','file_offset':clip,'original_va':f'{clip+c.magic:08x}','frames':c.h(clip+0x14),'frame_event_count':c.u(clip+0x4c),'raw_bytes':bytes(c.data[clip:clip+0x6c]).hex()}
  effects=c.block(c.block(t['offset']+0x2d0,0xec)[0]+0xe4,0x34)
  self.authored['firing_effect_entries']=[{'file_offset':offset,'count_bounds':[c.h(offset),c.h(offset+2)],'raw_bytes':bytes(c.data[offset:offset+0x34]).hex()} for offset in effects]
  self.w(BARREL+0xe8,EFFECTS)
  for index,offset in enumerate(effects):self.g.mem_write(EFFECTS+index*0x34,bytes(c.data[offset:offset+0x34]))
  c.data.close();c.stream.close()
  self.w(0x4e78d0,POOL);self.w(POOL+0x24,12);self.w(POOL+0x38,2);self.w(POOL+0x44,HEADERS);self.w(POOL+0x48,BITMAP);self.w(BITMAP,3)
  for i,obj,typ in ((0,WEAPON,2),(1,UNIT,0)):
   self.g.mem_write(HEADERS+i*12,struct.pack('<HBBII',1,3,typ,0,obj))
   self.w(obj,0 if i==0 else int(spartan['datum'],16))
   for off in (0xc,0x10,0x14,0xb4,0x13c,0x194,0x24c):self.w(obj+off,-1)
   self.sh(obj+0x12a,-1);self.sh(obj+0x2c,-1);self.g.mem_write(obj+0xaa,bytes([typ]))
  self.w(WEAPON+0x14,UI);self.w(WEAPON+0x154,UI);self.g.mem_write(WEAPON+0x12c,b'\x09')
  self.w(UNIT+0x12c,-1)
  self.sh(UNIT+0x346,0x700)
  self.w(UNIT+0x13c,0)
  self.g.mem_write(UNIT+0x212,b'\x00\xff');self.w(UNIT+0x218,WI)
  for off in (0x21c,0x220,0x224):self.w(UNIT+off,-1)
  for obj in (UNIT,WEAPON):
   self.w(obj+0x116,0x500);self.g.mem_write(obj+0x500,struct.pack('<13f',1,1,0,0,0,1,0,0,0,1,0,0,0))
  self.aim=list(aim);self.game_mode=game_mode;self.g.mem_write(UNIT+0x168,struct.pack('<3f',*aim))
  self.w(0x4eb114,TAGS);self.w(TAGS+8,DEF);self.w(TAGS+24,UNIT_DEF)
  self.w(0x518444,TIME);self.sh(TIME+2,30);self.f(TIME+4,1/30)
  self.w(0x4ee9d8,SEED);self.w(SEED,0x12345678)
  self.w(0x4edf18,GAME);self.w(GAME+8,2);self.g.mem_write(GAME+12,bytes([game_mode]))
  self.w(0x4d6d44,WORLD);self.w(WORLD+4,WORLD_DATA);self.w(WORLD+8,4 if game_mode==5 else 1);self.w(WORLD_DATA+0x20a4,ENTITY_FLAGS)
  for index,obj in ((WI,WEAPON),(UI,UNIT)):
   self.w(obj+0xd4,index);self.g.mem_write(ENTITY_FLAGS+0x44+(index&0x3ff)*8,b'\x04')
  self.w(0x4e58c4,0x1040000)
  self.w(0x4f1198,FPS);self.g.mem_write(FPS,b'\xff'*0x8000);self.w(FPS+0x10,WI)
  self.w(0x4f01f0,USERS);self.w(USERS+12,0);self.w(0x4f01f4,PLAYERS);self.w(PLAYERS+0x44,PLAYER_DATA);self.w(PLAYER_DATA+0x2c,UI)
  self.w(0x4f10b8,0x1098000);self.sh(PLAYER_DATA+0x28,-1);self.sh(PLAYER_DATA+0x1a,-1)
  self.tick=-1;self.events=[];self.rows=[];self.recent=deque(maxlen=24);self.active='init';self.count=0;self.random_return=None;self.watch_returns=[];self.entry_counts=Counter()
  self.code=_Code(self.image,'.text');self.starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
  self.services={0x16a356:('first-person animation notification',0,12),0xcb060:('unit animation/action notification',0,12),0xc9a40:('unit animation/action request',0,4),0x107b50:('effect allocation/notification',0xffffffff,12),0x1cdb20:('animation runtime notification',0,0),0x115650:('unit animation transition: supplied unavailable',0,16),0xd1890:('firing damage/physics effect submission',0,4),0xd2310:('unit firing feedback/aim response',0,8),0x1dc8f0:('model marker resource lookup',0xffffffff,0),0x1dc940:('model marker query: no active instance',0,24),0x1e20c0:('collision BSP query: supplied clear segment',0,16),0xb8a30:('projectile allocator: unavailable boundary',0xffffffff,4),0x8c0a0:('distributed event transport submission',0,32)}
  self.g.hook_add(UC_HOOK_CODE,self.hook)
  self.g.hook_add(UC_HOOK_MEM_INVALID,self.invalid)
  self.g.hook_add(UC_HOOK_MEM_WRITE,self.mem_write)
 def w(self,a,v):self.g.mem_write(a,struct.pack('<I',v&0xffffffff))
 def sh(self,a,v):self.g.mem_write(a,struct.pack('<H',v&0xffff))
 def f(self,a,v):self.g.mem_write(a,struct.pack('<f',v))
 def u(self,a,n=4):return int.from_bytes(self.g.mem_read(a,n),'little')
 def fv(self,a):return struct.unpack('<f',self.g.mem_read(a,4))[0]
 def vec(self,a):return {'bits':[f'{v:08x}' for v in struct.unpack('<3I',self.g.mem_read(a,12))],'value':list(struct.unpack('<3f',self.g.mem_read(a,12)))}
 def registers(self):return {name:f'{self.g.reg_read(reg):08x}' for name,reg in (('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),('edx',UC_X86_REG_EDX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI),('ebp',UC_X86_REG_EBP))}
 def state(self):
  b=WEAPON+0x1a4
  return {'mode':self.u(b+1,1),'counter':self.u(b,1),'ticks':self.u(b+2,2),'flags':self.u(b+4,2),'shot_count':self.u(b+6,2),'error_bits':f'{self.u(b+0x1c):08x}','angle_change_bits':f'{self.u(b+0x20):08x}','rate_bits':f'{self.u(b+0x10):08x}','accumulator_bits':f'{self.u(b+0x24):08x}','recovery_fraction_bits':f'{self.u(b+0x28):08x}','loaded':self.u(WEAPON+0x22c,2),'reserve':self.u(WEAPON+0x22a,2),'magazine_mode':self.u(WEAPON+0x224,2),'magazine_ticks':self.u(WEAPON+0x226,2),'reload_transfer_ticks':self.u(WEAPON+0x230,2),'trigger_state':self.u(WEAPON+0x20c,1),'trigger_ticks':self.u(WEAPON+0x20e,2),'trigger_flags':f'{self.u(WEAPON+0x210):08x}','weapon_flags':f'{self.u(WEAPON+0x16c,2):04x}','input_flags':f'{self.u(WEAPON+0x16e,2):04x}','weapon_state':self.u(WEAPON+0x178,1),'hand_timers':[self.u(UNIT+0x1fa,1),self.u(UNIT+0x1fb,1)],'seed':f'{self.u(SEED):08x}','barrel_state_hex':bytes(self.g.mem_read(b,0x34)).hex(),'trigger_state_hex':bytes(self.g.mem_read(WEAPON+0x20c,12)).hex(),'magazine_state_hex':bytes(self.g.mem_read(WEAPON+0x224,16)).hex(),'unit_action_state_hex':bytes(self.g.mem_read(UNIT+0x346,8)).hex()}
 def hook(self,g,a,size,user):
  self.recent.append(a);self.count+=1
  if a in (0x1021d0,0x1019f0,0x101a60,0x108300,0x107160,0x102090,0x147a90,0xa38f0,0xa8b30,0x8c0a0,0x105640,0x106c30,0x109c30,0x109d50):self.entry_counts[f'{self.active}:{a:08x}']+=1
  for ret,row in self.watch_returns[:]:
   if a==ret:
    row['returned_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}'
    if row.get('entry') in ('001066f0','001062b0'):row['return_state']=self.state()
    if row.get('entry')=='001062b0':row['returned_minimum_gate']=self.u(row.pop('_minimum_pointer'),1);row['returned_maximum_gate']=self.u(row.pop('_maximum_pointer'),1)
    self.watch_returns.remove((ret,row))
  if self.random_return and a==self.random_return[0]:
   _,out=self.random_return;self.events.append({'tick':self.tick,'phase':self.active,'random_result':self.vec(out),'state':self.state()});self.random_return=None
  if a in self.services:
   name,result,pop=self.services[a];sp=g.reg_read(UC_X86_REG_ESP)
   row={'tick':self.tick,'phase':self.active,'service':f'{a:08x}','description':name,'return_value':f'{result:08x}','supplied':True,'state':self.state(),'registers':self.registers(),'stack_arguments':[f'{self.u(sp+4+n*4):08x}' for n in range(pop//4)]}
   if a==0xb8a30:
    descriptor=self.u(sp+4);row['request_descriptor_hex']=bytes(g.mem_read(descriptor,0xc4)).hex();row['request_position']=self.vec(descriptor+0x1c);row['request_forward']=self.vec(descriptor+0x28);row['request_up']=self.vec(descriptor+0x34);row['request_velocity']=self.vec(descriptor+0x40);row['request_definition']=f'{self.u(descriptor):08x}'
   if a==0x8c0a0:
    row['event_type']=self.u(sp+8);row['entity_count']=self.u(sp+12);row['entity_indices']=[f'{self.u(self.u(sp+16)+i*4):08x}' for i in range(self.u(sp+12))];row['machine_mask']=f'{self.u(sp+20):08x}';row['event_payload_hex']=bytes(g.mem_read(self.u(sp+28),self.u(sp+24))).hex()
   self.events.append(row)
   g.reg_write(UC_X86_REG_EAX,result);g.reg_write(UC_X86_REG_ESP,sp+4+pop);g.reg_write(UC_X86_REG_EIP,self.u(sp));return
  if a in (0x107160,0x102090,0x10363e,0x10364e,0x101a60,0x108300,0x147a90,0x105640,0x101d50,0xe7ef0,0x106d40,0x169f3b,0x11cf00,0x1ceac0,0x1cf470,0x1066f0,0x1062b0):
   row={'tick':self.tick,'phase':self.active,'entry':f'{a:08x}','state':self.state()}
   if a==0x10363e:row['original_marker']=self.u(g.reg_read(UC_X86_REG_ESP)+0x15,1)
   if a==0x147a90:
    sp=g.reg_read(UC_X86_REG_ESP);row['minimum_angle_bits']=f'{self.u(sp+4):08x}';row['maximum_angle_bits']=f'{self.u(sp+8):08x}';row['input_direction']=self.vec(g.reg_read(UC_X86_REG_ECX));self.random_return=(self.u(sp),g.reg_read(UC_X86_REG_EAX))
   if a in (0x105640,0x101d50,0xe7ef0):row['registers']=self.registers()
   if a in (0x169f3b,0x11cf00,0x1ceac0,0x1cf470):
    row['registers']=self.registers();self.watch_returns.append((self.u(g.reg_read(UC_X86_REG_ESP)),row))
   if a in (0x1066f0,0x1062b0):
    sp=g.reg_read(UC_X86_REG_ESP);row['registers']=self.registers();row['stack_arguments']=[f'{self.u(sp+4+i*4):08x}' for i in range(3)];self.watch_returns.append((self.u(sp),row))
    if a==0x1062b0:row['_minimum_pointer']=self.u(sp+8);row['_maximum_pointer']=self.u(sp+12)
   self.events.append(row)
 def mem_write(self,g,access,address,size,value,user):
  if address==SEED:
   self.events.append({'tick':self.tick,'phase':self.active,'rng_write_instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}','before':f'{self.u(SEED):08x}','after':f'{value&0xffffffff:08x}'})
 def invalid(self,g,access,address,size,value,user):
  print(json.dumps({'unmapped':hex(address),'eip':hex(g.reg_read(UC_X86_REG_EIP)),'phase':self.active,'tick':self.tick,'recent':[hex(a) for a in self.recent]},indent=2),file=sys.stderr);return False
 def run(self,a,argv=(),regs=()):
  self.count=0;self.w(STACK,STOP)
  for i,v in enumerate(argv):self.w(STACK+4+i*4,v)
  self.g.reg_write(UC_X86_REG_ESP,STACK);self.g.reg_write(UC_X86_REG_FPCW,0x027f);self.g.reg_write(UC_X86_REG_MXCSR,0x1f80);self.g.reg_write(UC_X86_REG_FPSW,0);self.g.reg_write(UC_X86_REG_FPTAG,0xffff)
  for reg,value in regs:self.g.reg_write(reg,value)
  self.g.emu_start(a,STOP,count=1000000)
  assert self.g.reg_read(UC_X86_REG_EIP)==STOP,(hex(a),hex(self.g.reg_read(UC_X86_REG_EIP)))
 def execute(self,hold_ticks=45,release_ticks=15,reload_tick=None,pattern=None,loaded_ammo=None,reserve_ammo=None):
  self.active='constructor';self.run(0x101bb0,(WI,))
  constructor_state=self.state()
  if loaded_ammo is not None:self.sh(WEAPON+0x22c,loaded_ammo)
  if reserve_ammo is not None:self.sh(WEAPON+0x22a,reserve_ammo)
  initial_state=self.state()
  pattern=pattern if pattern is not None else [True]*hold_ticks+[False]*release_ticks
  previous_held=False
  for tick,held in enumerate(pattern):
   self.tick=tick;self.w(TIME+8,tick)
   event_start=len(self.events);tick_start=self.state()
   self.active='timer_decay';self.g.reg_write(UC_X86_REG_EBP,UNIT);self.g.emu_start(0xc6cd8,0xc6d06,count=50)
   after_timer=self.state()
   flags=(2 if held else 0)|(8 if held and not previous_held else 0);previous_held=held
   self.active='input';self.run(0x1045e0,(WI,flags,0x3f800000 if held else 0))
   after_input=self.state()
   if tick==reload_tick:
    self.active='explicit_reload_command';self.run(0x105640,(WI,),((UC_X86_REG_ECX,0),))
   self.active='update';before=self.state();self.run(0x1021d0,(WI,));after=self.state()
   self.active='projectile_pass';self.run(0x1019f0);post=self.state()
   event_count=len(self.events);self.active='second_projectile_pass';self.run(0x1019f0);second=self.state();assert second==post and len(self.events)==event_count
   self.rows.append({'tick':tick,'held':held,'tick_start':tick_start,'after_timer_decay':after_timer,'after_input':after_input,'explicit_reload_command':tick==reload_tick,'before':before,'after':after,'post_dispatch':post,'second_pass_unchanged':True,'event_range':[event_start,len(self.events)]})
  requests=[e for e in self.events if e.get('service')=='000b8a30']
  return {'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'authored':self.authored,'options_state':2,'options_mode':self.game_mode,'simulation_world_state':self.u(WORLD+8),'supplied_aim':self.aim,'constructor_state':constructor_state,'initial_state':initial_state,'supplied_initial_ammo':{'loaded':loaded_ammo,'reserve':reserve_ammo},'reload_command_tick':reload_tick,'rows':self.rows,'events':self.events,'entry_counts':dict(sorted(self.entry_counts.items())),'summary':{'ticks':len(self.rows),'projectile_requests':len(requests),'request_ticks':[e['tick'] for e in requests],'rng_stores':sum('rng_write_instruction' in e for e in self.events),'final_state':self.state(),'all_second_passes_unchanged':all(r['second_pass_unchanged'] for r in self.rows)},'supplied_services':{f'{a:08x}':{'name':n,'return':r,'stack_cleanup':p} for a,(n,r,p) in self.services.items()},'scope':'Original weapon instructions unchanged. Authored Lockout SMG/masterchief_mp and original graph/action/animation timing lookup; explicit supplied service boundaries. Requests are recorded before supplied allocation failure; no successful projectile allocation or full physics/world/network simulation.'}
def generate_suite(directory):
 directory=directory.resolve();directory.mkdir(parents=True,exist_ok=True);reports={};cases=[]
 scenarios=[('held_x',dict()),('held_y',dict(aim=(0.,1.,0.))),('held_z',dict(aim=(0.,0.,1.))),('pulse_x',dict(pattern=[True,False]*30)),('pulse_y',dict(aim=(0.,1.,0.),pattern=[True,False]*30)),('pulse_z',dict(aim=(0.,0.,1.),pattern=[True,False]*30)),('full_reload',dict(hold_ticks=45,release_ticks=90,reload_tick=45)),('empty_reload',dict(hold_ticks=135,release_ticks=90)),('empty_initial',dict(hold_ticks=0,release_ticks=90,loaded_ammo=0)),('empty_no_reserve',dict(hold_ticks=45,release_ticks=15,loaded_ammo=0,reserve_ammo=0)),('reload_held_resume',dict(hold_ticks=225,release_ticks=15))]
 for mode in (1,5):
  for name,config in scenarios:
   settings=dict(config);aim=settings.pop('aim',(1.,0.,0.));t=Trace(mode,aim);result=t.execute(**settings)
   assert not any('000a38f0' in k or '000a8b30' in k or '0008c0a0' in k for k in result['entry_counts'])
   request_count=result['summary']['projectile_requests'];assert result['summary']['rng_stores']==request_count*2
   path=directory/f'{name}-mode{mode}.json';path.write_text(json.dumps(result,indent=2)+'\n')
   reports[(mode,name)]=result
   cases.append({'name':name,'mode':mode,'report':str(path.relative_to(ROOT)),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'ticks':len(result['rows']),'requests':request_count,'rng_stores':result['summary']['rng_stores'],'final_loaded':result['summary']['final_state']['loaded'],'final_reserve':result['summary']['final_state']['reserve']})
 comparisons=[]
 for name,_ in scenarios:
  a,b=reports[(1,name)],reports[(5,name)];assert a['rows']==b['rows']
  comparisons.append({'name':name,'mode1_mode5_state_rows_equal':True,'ticks':len(a['rows'])})
 summary={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'cases':cases,'comparisons':comparisons,'total_cases':len(cases),'total_ticks':sum(c['ticks'] for c in cases),'total_requests':sum(c['requests'] for c in cases),'scope':'Each case executes untouched original scheduling/dispatch instructions. Two RNG writes per request is asserted only for these nondegenerate fixed-axis cases and supplied service boundaries. Projectile allocation is supplied failure. No complete game/world/network proof.'}
 (directory/'suite.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps({k:summary[k] for k in ('total_cases','total_ticks','total_requests')}))

def dump_instructions(directory):
 directory=directory.resolve();directory.mkdir(parents=True,exist_ok=True)
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(image.data).hexdigest()==PIN
 code=_Code(image,'.text');starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
 entries=(0x101bb0,0x1045e0,0x1021d0,0x1062b0,0x1066f0,0x107120,0x107160,0x107f80,0x108010,0x108110,0x1019f0,0x101a60,0x108300,0x105640,0x106580,0x101d50,0x106c30,0x106d40,0x109a60,0x109c30,0x109d50,0x10a140,0x169f3b,0x16a06a,0x1680b3,0x1de7a0,0x1de810,0xe7ef0,0xe88b0,0xe8740,0x139750,0xb7e00,0xb7f50,0xb8040,0xbd360,0x10cdf0,0xc6c50,0xa38f0,0xa8b30,0xb6960,0xa66e0,0xa6730)
 rows=[]
 for entry in entries:
  f=_trace(code,entry,starts|set(entries));path=directory/f'{entry:08x}.asm'
  lines=[];source_bytes=[]
  for address in sorted(f.code):
   i=code.at(address);lines.append(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}');source_bytes.append(i.bytes)
  path.write_text('\n'.join(lines)+'\n');rows.append({'start':f'{entry:08x}','end_exclusive':f'{f.end:08x}','instruction_count':len(f.code),'reachable_instruction_bytes_sha256':hashlib.sha256(b''.join(source_bytes)).hexdigest(),'listing':str(path.relative_to(ROOT)),'listing_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'direct_calls':[{'site':f'{s:08x}','target':f'{t:08x}'} for s,t,k in f.sites if k=='call']})
 output={'xbe_sha256':PIN,'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'routines':rows,'scope':'Read-only reachable instruction listings from pinned original executable; names are addresses. Static listings establish direct operands and calls, not execution of every branch.'}
 (directory/'inventory.json').write_text(json.dumps(output,indent=2)+'\n');print(json.dumps({'routines':len(rows)}))

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/map-pack/smg-firing-1.1-oracle.json');p.add_argument('--hold-ticks',type=int,default=45);p.add_argument('--release-ticks',type=int,default=15);p.add_argument('--reload-tick',type=int);p.add_argument('--mode',type=int,choices=(1,5),default=1);p.add_argument('--aim',type=float,nargs=3,default=(1,0,0));p.add_argument('--pattern',help='Literal 0/1 trigger sequence; rising edges also set original pressed bit.');p.add_argument('--loaded-ammo',type=int);p.add_argument('--reserve-ammo',type=int);p.add_argument('--suite',type=Path,help='Write reproducible held/pulsed/empty/reload cases and comparison summary into this directory.');p.add_argument('--instruction-inventory',type=Path,help='Write original reachable instruction listings and hashes, without running a scenario.');args=p.parse_args()
 if args.pattern is not None and (not args.pattern or any(x not in '01' for x in args.pattern)):p.error('--pattern requires a nonempty sequence of 0/1 characters')
 if args.hold_ticks<0 or args.release_ticks<0:p.error('tick counts must be nonnegative')
 if any(v is not None and not 0<=v<=32767 for v in (args.loaded_ammo,args.reserve_ammo)):p.error('supplied ammo must fit a nonnegative signed short')
 if args.instruction_inventory is not None:dump_instructions(args.instruction_inventory);return
 if args.suite is not None:generate_suite(args.suite);return
 t=Trace(args.mode,args.aim)
 try:result=t.execute(args.hold_ticks,args.release_ticks,args.reload_tick,[x=='1' for x in args.pattern] if args.pattern else None,args.loaded_ammo,args.reserve_ammo)
 except Exception:
  print('last instructions',[(hex(a),t.code.at(a).mnemonic,t.code.at(a).op_str) for a in t.recent],file=sys.stderr);raise
 args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'ticks':len(result['rows']),'events':len(result['events'])}))
if __name__=='__main__':main()
