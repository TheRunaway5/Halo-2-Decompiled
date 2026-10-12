#!/usr/bin/env python3
"""Unchanged three-image game-over session owner with declared API records.

The original719D0 preflight runs with secondary session state0 and owner role
bytes+C/+D zero. Ready game identities intentionally differ, so the subsequent
map/settings transition is not supplied. Phase/request writes, force-end and
countdown queries execute their actual owners, with a declared frozen session
millisecond clock. No platform/game return or original instruction is replaced.
This is not the request consumer, world teardown, cache load or lobby UI flow.
"""
import argparse,csv,json,random,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_EDI
from oracle_game_over import SPECS as BASE
from oracle_game_variants import Source,ROOT
from oracle_mp_full_scoreboard_1_5 import _Code,literal,complete_tokens,references,sha
from inspect_multiplayer import Cache

OUT=ROOT/'analysis/multiplayer/game-variants/session-game-over'
FIXTURE=ROOT/'tests/fixtures/session_game_over_original.json'
# update, preflight, phase-set, state-range-query, closing-query, force-query,
# ready-query, request, network-mode-query, non-mode4-query, force-end-owner
OWNERS={
 'retail':(0x6ea20,0x6dfa0,0x5a220,0x58d20,0x682c0,0x5b040,0x138800,0x6df60,0x138840,0x138860,0x1388e0),
 'tu11':(0x6ef30,0x6e480,0x5a1f0,0x58d10,0x684d0,0x5b020,0x139f70,0x6e440,0x139fb0,0x139fd0,0x13a050),
 'u5':(0x72650,0x719d0,0x5aed0,0x59940,0x696b0,0x5bd00,0x13b2e0,0x71990,0x13b320,0x13b340,0x13b3c0)}
GLOBALS={'retail':(0x4cf770,0x4cf772,0x510548,0x51054c),
 'tu11':(0x4d6d38,0x4d6d3a,0x517d38,0x517d3c),
 'u5':(0x4dc040,0x4dc042,0x51d048,0x51d04c)}
G,C,O,N,A,B=0x1000000,0x1001200,0x1010000,0x1010100,0x1011000,0x1019000
PHASE_FIELDS={'phase':0x4980,'previous_phase':0x7660,'record_count':0x54,
 'generation':0x4978,'sequence':0x497c,'phase_time':0x4984,'previous_phase_time':0x7664}
REQUEST_FIELDS={'kind':0x4c,'length':0x50,'first_payload_word':0x54}

class SessionTrace(Source):
 def __init__(self,name):
  super().__init__(BASE[name]);self.name=name;self.owners=OWNERS[name]
  self.g.mem_map(O,0x12000)
  self.starts={int(r['address'],16) for r in csv.DictReader((ROOT/self.spec['tsv']).open(),delimiter='\t')}
  self.code=_Code(self.image,'.text');self.entries=set();self.calls=[];self.stores=[]
  self.allowed=set(self.owners)|set(self.spec['owners'][:2])
  self.g.hook_add(UC_HOOK_CODE,self.hook);self.g.hook_add(UC_HOOK_MEM_WRITE,self.write)
 def hook(self,g,a,n,user):
  if a in self.starts:
   self.entries.add(a);self.calls.append(a)
   assert a in self.allowed,(self.name,hex(a),'Unrecovered dependency reached; no success supplied')
 def write(self,g,access,a,n,value,user):
  if G<=a<G+0x1200 or O<=a<O+0x12000:
   self.stores.append(dict(address=f'{a:08x}',size=n,value=f'{value&((1<<(n*8))-1):0{n*2}x}'))
 def w(self,a,v):self.g.mem_write(a,struct.pack('<I',v&0xffffffff))
 def setup(self,input,phase,request,over,rate,ms):
  self.g.mem_write(G,bytes(0x1200));self.g.mem_write(O,bytes(0x12000))
  self.w(self.spec['globals'],G);self.w(self.spec['clock'],C)
  self.w(O+8,N);self.w(N+0x30,A);self.w(N+0x38,B)
  self.w(A+0x741c,input['main_session_state']);self.g.mem_write(A+0x49c4,bytes([input['force_end']]))
  self.g.mem_write(G+0x1120,bytes([input['game_ready']]))
  self.w(G+8,input['game_kind']);self.w(G+0x10,44);self.w(O+0x10,99)
  self.g.mem_write(C+2,struct.pack('<h',rate))
  self.g.mem_write(G+0x1128,bytes([over['active'],over['fade_started']]))
  self.w(G+0x112c,over['countdown'])
  for key,offset in PHASE_FIELDS.items():self.w(A+offset,phase[key])
  self.g.mem_write(A+0x765c,bytes([phase['transition_pending']]))
  for i,row in enumerate(phase['records']):
   # Full20-byte records start at72DC, one byte before the occupied byte.
   # Give every unclassified byte a declared nonzero pattern, then preserve
   # all18 of them through the actual owner rather than relying on BSS zeros.
   self.g.mem_write(A+0x72dc+i*20,bytes(((i*37+j*19+0x5a)&255) or 0x7d for j in range(20)))
   self.g.mem_write(A+0x72dd+i*20,bytes(row))
  for key,offset in REQUEST_FIELDS.items():self.w(N+offset,request[key])
  self.g.mem_write(N+0x4a,bytes([request['pending']]))
  active,closing,clock_flag,clock_ms=GLOBALS[self.name]
  self.g.mem_write(active,bytes([input['world_active']]))
  self.g.mem_write(closing,bytes([input['world_closing']]))
  self.g.mem_write(clock_flag,b'\x01');self.w(clock_ms,ms)
  self.before_memory=(bytes(self.g.mem_read(G,0x1200)),bytes(self.g.mem_read(O,0x12000)))
  self.calls=[];self.stores=[]
 def phase(self):
  out={key:self.u(A+offset) for key,offset in PHASE_FIELDS.items()}
  for key in ('phase','previous_phase','record_count'):
   if out[key]>=0x80000000:out[key]-=0x100000000
  out['transition_pending']=self.g.mem_read(A+0x765c,1)[0]
  out['records']=[list(self.g.mem_read(A+0x72dd+i*20,2)) for i in range(16)]
  return out
 def request(self):
  out={key:self.u(N+offset) for key,offset in REQUEST_FIELDS.items()}
  for key in ('kind','length'):
   if out[key]>=0x80000000:out[key]-=0x100000000
  out['pending']=self.g.mem_read(N+0x4a,1)[0];return out
 def over(self):
  count=self.u(G+0x112c)
  return dict(active=bool(self.g.mem_read(G+0x1128,1)[0]),fade_started=bool(self.g.mem_read(G+0x1129,1)[0]),
   countdown=count if count<0x80000000 else count-0x100000000)
 def snapshot(self):return dict(phase=self.phase(),request=self.request(),game_over=self.over())
 def unchanged_other_fields(self):
  current=[bytearray(self.g.mem_read(G,0x1200)),bytearray(self.g.mem_read(O,0x12000))]
  before=[bytearray(v) for v in self.before_memory]
  permitted=[(0,0x1128,2),(0,0x112c,4),(1,A-O+0x765c,1)]
  permitted.extend((1,A-O+v,4) for v in PHASE_FIELDS.values())
  permitted.extend((1,N-O+v,4) for v in REQUEST_FIELDS.values());permitted.append((1,N-O+0x4a,1))
  permitted.extend((1,A-O+0x72de+i*20,1) for i in range(16))
  for index,offset,size in permitted:current[index][offset:offset+size]=before[index][offset:offset+size]
  assert current==before,(self.name,'Unclassified source record write')

def initial_phase(rng,phase=4,count=4):
 return dict(phase=phase,previous_phase=rng.randrange(-10,10),record_count=count,
  generation=rng.choice((0,0xffffffff,rng.getrandbits(32))),sequence=rng.choice((0,0xffffffff,rng.getrandbits(32))),
  phase_time=rng.getrandbits(32),previous_phase_time=rng.getrandbits(32),transition_pending=rng.choice((0,1,255)),
  records=[[rng.choice((0,1,2,255)),rng.randrange(256)] for _ in range(16)])
def initial_request():return dict(pending=0,kind=-27,length=17,first_payload_word=0x87654321)
def initial_input(state=5):return dict(preflight_changed=False,main_session_state=state,
 world_active=False,world_closing=False,force_end=False,game_ready=True,game_kind=2,same_game_identity=False)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
 args.output.parent.mkdir(parents=True,exist_ok=True);sources={};versions={}
 for name in BASE:
  t=SessionTrace(name);sources[name]=t;rng=random.Random(0x72650);setters=[];updates=[];sequences=[];gates=[]
  for n in range(96):
   phase=initial_phase(rng,rng.choice((-2147483648,-1,0,1,4,5,2147483647)),rng.choice((-1,0,1,4,16)))
   requested=phase['phase'] if n%3==0 else rng.choice((-2147483648,-1,0,1,4,5,2147483647))
   input=initial_input();request=initial_request();over=dict(active=False,fade_started=True,countdown=-7);ms=rng.getrandbits(32)
   t.setup(input,phase,request,over,30,ms);before=t.snapshot()
   t.call(t.owners[2],registers=((UC_X86_REG_ESI,A),(UC_X86_REG_EDI,requested)))
   t.unchanged_other_fields();setters.append(dict(before=before,requested=requested,milliseconds=ms,after=t.snapshot(),stores=t.stores.copy()))
  for state in (-1,0,1,2,3,4,5,6,7,8,9):
   for phase_value in (-1,0,1,3,4,5,6):
    for pending in (0,1):
     for active,count,force in ((False,0,False),(True,-1,False),(True,0,False),(True,1,False),(False,0,True),(True,0,True)):
      input=initial_input(state);input['force_end']=force
      phase=initial_phase(rng,phase_value,rng.choice((-1,0,4,16)))
      request=initial_request();request['pending']=pending
      over=dict(active=active,fade_started=False,countdown=count);ms=rng.getrandbits(32)
      t.setup(input,phase,request,over,30,ms);before=t.snapshot()
      changed=bool(t.call(t.owners[0],registers=((UC_X86_REG_ECX,O),))&255)
      t.unchanged_other_fields();updates.append(dict(input=input,before=before,tick_rate=30,milliseconds=ms,
       changed=changed,after=t.snapshot(),calls=[f'{e:08x}' for e in t.calls],stores=t.stores.copy()))
  # Original closing predicate requires BOTH bytes. Closing state3/4 service
  # calls are deliberately not supplied. Incomplete/negative count and forced
  # end paths retain the full original ready/game-kind checks.
  for state in (3,4,5,6,7,8):
   for world_active,world_closing in ((False,True),(True,False),(True,True)):
    if state<5 and world_active and world_closing:continue
    for ready,kind in ((False,1),(False,2),(True,0),(True,2),(True,3)):
     input=initial_input(state);input.update(world_active=world_active,world_closing=world_closing,game_ready=ready,game_kind=kind,force_end=True)
     phase=initial_phase(rng,4,16);request=initial_request();over=dict(active=False,fade_started=True,countdown=-7);ms=rng.getrandbits(32)
     t.setup(input,phase,request,over,15,ms);before=t.snapshot()
     changed=bool(t.call(t.owners[0],registers=((UC_X86_REG_ECX,O),))&255)
     t.unchanged_other_fields();updates.append(dict(input=input,before=before,tick_rate=15,milliseconds=ms,
      changed=changed,after=t.snapshot(),calls=[f'{e:08x}' for e in t.calls],stores=t.stores.copy()))
  for state in (5,6,7,8):
   for count in (-1,0,1,210):
    input=initial_input(state);phase=initial_phase(rng,4,4);request=initial_request();over=dict(active=True,fade_started=True,countdown=count);ms=rng.getrandbits(32)
    t.setup(input,phase,request,over,30,ms);before=t.snapshot();steps=[]
    for n in range(3):
     t.calls=[];t.stores=[];changed=bool(t.call(t.owners[0],registers=((UC_X86_REG_ECX,O),))&255)
     t.unchanged_other_fields();steps.append(dict(changed=changed,after=t.snapshot(),calls=[f'{e:08x}' for e in t.calls],stores=t.stores.copy()))
    sequences.append(dict(input=input,before=before,tick_rate=30,milliseconds=ms,steps=steps))
  for mode in range(-128,128):
   for active,count in ((False,0),(True,-1),(True,0),(True,1)):
    t.g.mem_write(G+0xc,bytes([mode&255]));t.g.mem_write(G+0x1128,bytes([active]));t.w(G+0x112c,count)
    network=bool(t.call(t.owners[8])&255);non4=bool(t.call(t.owners[9])&255);complete=bool(t.call(t.spec['owners'][1])&255)
    gates.append(dict(mode=mode,active=active,countdown=count,network_mode=network,non_mode4=non4,complete=complete,
     request_menu=non4 and complete and not network))
  versions[name]=dict(xbe_sha256=t.spec['pin'],phase_setters=setters,updates=updates,sequences=sequences,menu_gates=gates)
 listingdir=args.output.parent/'instructions';listingdir.mkdir(exist_ok=True);listings=[];pairs=[];new=sources['u5']
 for name,t in sources.items():
  for e in sorted(t.allowed):
   _,ins=literal(t.code,e,t.starts);p=listingdir/f'{name}-{e:08x}.asm'
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
   listings.append(dict(file=p.name,sha256=sha(p),entry=f'{e:08x}',instructions=len(ins)))
  if name=='u5':continue
  for index,e in enumerate(t.owners):
   _,oi=literal(t.code,e,t.starts);ne=new.owners[index];_,ni=literal(new.code,ne,new.starts)
   assert complete_tokens(oi,t.image)==complete_tokens(ni,new.image),(name,hex(e))
   # Actual direct incoming call identifies the repeated closing predicate;
   # both target instructions and its two shared globals remain explicit.
   known=dict(zip(t.owners,new.owners))|dict(zip(t.spec['owners'][:2],new.spec['owners'][:2]))
   calls=[]
   for old,later in zip(oi,ni):
    if old.mnemonic=='call' and old.op_str.startswith('0x'):
     target=int(old.op_str,16);ntarget=int(later.op_str,16)
     if target in known:assert known[target]==ntarget
     calls.append(dict(old_site=f'{old.address:08x}',later_site=f'{later.address:08x}',old_target=f'{target:08x}',later_target=f'{ntarget:08x}',callee_identified=target in known))
   pairs.append(dict(earlier=name,old=f'{e:08x}',later=f'{ne:08x}',instructions=len(oi),calls=calls,
    relocations=references(oi,ni,t.image,new.image)))
 # This is literal instruction evidence only. Neither successful world
 # teardown/map-load nor the widget allocator is provided for these owners.
 static=[]
 for e in (0x694e0,0x191fb0,0x191580,0x1916a0,0x14af00,0x14b031,
  0x239d34,0x239b87,0x236071,0x718f0,0x727c0,0x72810):
  _,ins=literal(new.code,e,new.starts);p=listingdir/f'u5-static-{e:08x}.asm'
  p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
  listings.append(dict(file=p.name,sha256=sha(p),entry=f'{e:08x}',instructions=len(ins)))
  static.append(dict(entry=f'{e:08x}',calls=[i.op_str for i in ins if i.mnemonic=='call']))
 state_vtable=dict(address='0045c6d0',bytes_hex=new.image.read(0x45c6d0,12).hex(),
  update='00072650',entry='000727c0',exit='00072810')
 assert state_vtable['bytes_hex']==struct.pack('<III',0x72650,0x727c0,0x72810).hex()
 for category in ('phase_setters','updates','sequences','menu_gates'):
  def comparable(rows):
   if category=='sequences':return [dict(row,steps=[{k:v for k,v in step.items() if k not in ('calls','stores')} for step in row['steps']]) for row in rows]
   return [{k:v for k,v in row.items() if k not in ('calls','stores')} for row in rows]
  assert comparable(versions['retail'][category])==comparable(versions['tu11'][category])==comparable(versions['u5'][category]),category
 cache=Cache(ROOT/'assets/disc/maps/mainmenu.map')
 assert sha(cache.path)=='1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
 menu_assets=dict(cache_sha256=sha(cache.path),screens=[])
 for tag in cache.tags.values():
  if tag['group']=='wgit' and cache.h(tag['offset']+4) in (14,16):
   menu_assets['screens'].append(dict(tag,id=cache.h(tag['offset']+4),
    header_word_hex=f'{cache.u(tag["offset"]+44):08x}',pane_count=cache.u(tag['offset']+32)))
 assert {row['id']:row['name'] for row in menu_assets['screens']}=={
  14:r'ui\screens\game_shell\pregame_lobby\pregame_lobby',16:r'ui\screens\game_shell\postgame_statistics\pcr'}
 (args.output.parent/'menu-assets.json').write_text(json.dumps(menu_assets,indent=2)+'\n')
 fixture=dict(schema='original-session-game-over-v1',scope=__doc__,instruction_patches=[],supplied_returns=[],versions=versions)
 FIXTURE.write_text(json.dumps(fixture,indent=2,sort_keys=True)+'\n')
 report=dict(fixture,oracle_sha256=sha(__file__),fixture_sha256=sha(FIXTURE),specs={n:dict(BASE[n],session_owners=OWNERS[n],session_globals=GLOBALS[n]) for n in BASE},
  complete_source_pairs=pairs,instruction_listings=listings,static_return_owners=static,
  state_vtable=state_vtable,menu_assets=menu_assets,
  executed_routines={n:[f'{e:08x}' for e in sorted(t.entries)] for n,t in sources.items()},
  executed_instruction_counts={n:len(t.executed) for n,t in sources.items()},
  unclassified_record_input='20-byte record at72DC+i*20: ((i*37+j*19+0x5A)&255) or0x7D; occupied/changed overwrite bytes1/2; all other18 bytes checked unchanged',
  dependencies={p:sha(ROOT/p) for p in ('tools/oracle_game_over.py','tools/oracle_game_variants.py','tools/oracle_mp_full_scoreboard_1_5.py','tools/research_mp_lobby_later_update.py','tools/inspect_multiplayer.py')},
  summary=dict(versions=3,phase_setters=sum(len(v['phase_setters']) for v in versions.values()),
   updates=sum(len(v['updates']) for v in versions.values()),sequences=sum(len(v['sequences']) for v in versions.values()),
   menu_gates=sum(len(v['menu_gates']) for v in versions.values()),complete_pairs=len(pairs),supplied_returns=0,instruction_patches=0))
 args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report['summary']))
if __name__=='__main__':main()
