#!/usr/bin/env python3
"""Whole original cold damage inputs and target identity in three Xbox images.

Original default configuration, complete nonlocal player birth/configuration/
sanitizer, mode4 no-allocation spawn branch, shared player tick and target
resolver execute unchanged. Occupied player storage, creation identity, requested
property byte, clock, object header/body/parent records are declared API inputs.
Mode4 is not ordinary live spawn; current-unit NONE skips live configuration
notifications. No original instruction patch or supplied game/platform return.
"""
import argparse,itertools,json,re,struct,sys
from pathlib import Path
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_EIP
from oracle_king_trait_consumers import TraitTrace,H,T,U,NONE
from oracle_result_statistics import R,G,M,P
from oracle_player_lives import C
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,CACHE_PIN,literal,complete_tokens,references,sha
OUT=ROOT/'analysis/multiplayer/game-variants/damage-player-inputs-20261011'
CREATION,PG=H+0x6000,H+0x7000
OWNERS={
 'retail':dict(birth=0x14c090,configuration=0x14be90,sanitize=0x14bc80,default=0x1537f0,spawn=0x14f2e0,life=0x15bb20),
 'tu11':dict(birth=0x14d800,configuration=0x14d600,sanitize=0x14d3d0,default=0x154f40,spawn=0x1509c0,life=0x15d160),
 'u5':dict(birth=0x14ebe0,configuration=0x14e9e0,sanitize=0x14e7b0,default=0x156320,spawn=0x151da0,life=0x15e540)}
class Inputs(TraitTrace):
 def __init__(self,name,cache):
  super().__init__(name,cache);self.i=dict(OWNERS[name]);self.stores=[]
  section=next(s for s in self.image.sections if s.name=='.text')
  pattern=rb'\x55\x56\x8b\xf0\x83\xcd\xff\x83\xfe\xff\x0f\x84....\x57\x8b\x3d....\x8b\x47\x38'
  matches=[section.va+m.start()for m in re.finditer(pattern,self.image.section_bytes(section),re.S)]
  assert len(matches)==1,(name,matches);self.i['resolver']=matches[0]
  _,ins=literal(self.code,matches[0],self.starts)
  calls=[int(i.op_str,16)for i in ins if i.mnemonic=='call'];assert len(calls)==1
  self.i['object_get_mask3']=calls[0];self.g.hook_add(UC_HOOK_MEM_WRITE,self.write)
 def write(self,g,access,address,size,value,_):
  for off in (0xc1,0x194):
   if address<=R+off<address+size:self.stores.append(dict(pc=g.reg_read(UC_X86_REG_EIP),offset=address-R,size=size,value=value&((1<<(size*8))-1)))
 def birth(self,strength):
  self.setup(dict(unit_none=True));self.w(self.ka['pool']-4,PG);self.g.mem_write(PG,bytes(0x200));self.w(PG+0x98,-1)
  self.g.mem_write(CREATION,bytes(0x100));self.g.mem_write(CREATION+2,b'\xff\xff');self.w(CREATION+4,NONE)
  self.call(self.i['default'],registers=((UC_X86_REG_EDX,CREATION+0x1c),))
  default=bytes(self.g.mem_read(CREATION+0x1c,0x90));self.g.mem_write(CREATION+0x1c+0x7d,bytes([strength]))
  self.g.mem_write(R,b'\xa5'*0x21c);self.g.mem_write(R,b'\xbc\x9a');self.stores=[]
  self.call(self.i['birth'],(0x9abc0000,1,CREATION))
  return dict(default_configuration_hex=default.hex(),player_hex=bytes(self.g.mem_read(R,0x21c)).hex(),
      requested_configuration_hex=bytes(self.g.mem_read(R+0xd4,0x90)).hex(),effective_configuration_hex=bytes(self.g.mem_read(R+0x44,0x90)).hex(),
      strength=self.g.mem_read(R+0xc1,1)[0],field194=self.u(R+0x194),stores=self.stores.copy())
 def timeline(self,rate):
  self.birth(0);self.g.mem_write(G+0xc,b'\x04');self.g.mem_write(C+2,struct.pack('<h',rate));self.stores=[]
  self.call(self.i['spawn'],(0x9abc0000,));spawn=self.u(R+0x194);spawn_stores=self.stores.copy();states=[spawn]
  for _ in range(max(2,rate+2)if rate<=60 else 2):
   self.call(self.i['life'],(0x9abc0000,));states.append(self.u(R+0x194))
  return dict(rate=rate,mode=4,current_unit=self.u(R+0x2c),spawn_field194=spawn,states=states,spawn_stores=spawn_stores)
 def resolve(self,row):
  self.setup(dict(unit_none=True));self.w(H+0x24,12);self.w(H+0x38,row.get('count',4));self.w(H+0x44,T)
  self.g.mem_write(T,bytes(0x100));self.g.mem_write(U,bytes(0x3000))
  for slot,body in enumerate(row['objects']):
   datum=0x8aaa0000+slot;pointer=U+slot*0x800
   self.g.mem_write(T+slot*12,struct.pack('<HBBII',body.get('salt',0x8aaa),0,body['kind'],0,pointer))
   self.w(pointer+0x14,body.get('parent',NONE));self.w(pointer+0x13c,body.get('player',NONE));self.w(pointer+0x140,body.get('previous',NONE))
  before=bytes(self.g.mem_read(T,0x100))+bytes(self.g.mem_read(U,0x3000))
  self.call(self.i['resolver'],registers=((UC_X86_REG_EAX,row.get('datum',0x8aaa0000)),))
  assert before==bytes(self.g.mem_read(T,0x100))+bytes(self.g.mem_read(U,0x3000))
  return self.g.reg_read(UC_X86_REG_EAX)
def cases():
 rows=[dict(objects=[],datum=NONE)]
 for kind,controller,previous,parent in itertools.product((0,1,2,3,6,7,8,10,11,12),
     (NONE,0x9abc0000,0x1234000f),(NONE,0x9abc0003),(NONE,0x8aaa0001)):
  rows.append(dict(objects=[dict(kind=kind,player=controller,previous=previous,parent=parent),dict(kind=0,player=0x9abc0003)]))
 for bad_salt,count,parent in itertools.product((0,0x8aab),(0,1,4),(NONE,0x8aaa0001)):
  rows.append(dict(count=count,objects=[dict(kind=0,salt=bad_salt,player=0x9abc0000,parent=parent),dict(kind=0,player=0x9abc0003)]))
 for depth in (1,2,3):
  rows.append(dict(objects=[dict(kind=2,parent=0x8aaa0000+slot+1)for slot in range(depth)]+[dict(kind=1,player=0x9abc0003)]))
 return rows
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/damage_player_inputs_original.json');a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN;cache=HudCache(ROOT/'assets/disc/maps/lockout.map');sources={};versions={};rows=cases();rates=(-32768,-1,0,1,15,30,31,60,32767)
 for name in OWNERS:
  t=Inputs(name,cache);sources[name]=t;versions[name]=dict(image_sha256=t.spec['pin'],owners=t.i,
      births=[t.birth(s)for s in range(256)],timelines=[t.timeline(rate)for rate in rates],resolved=[t.resolve(q)for q in rows])
  assert all(r['field194']==0 and r['strength']==min(3,s)if s<128 else r['field194']==0 and r['strength']==0 for s,r in enumerate(versions[name]['births']))
 # Retail retains the profile representation while TU maps multiplayer0->2.
 # Keep complete produced records and every changed byte; compare only the
 # actual damage-owned inputs across all three images.
 birth_differences=[]
 for name in ('retail','tu11'):
  dif={}
  for n,(old,new) in enumerate(zip(versions[name]['births'],versions['u5']['births'])):
   for off,(x,y) in enumerate(zip(bytes.fromhex(old['player_hex']),bytes.fromhex(new['player_hex']))):
    if x!=y:dif.setdefault(str(off),[]).append(dict(strength_input=n,earlier=x,later=y))
  birth_differences.append(dict(earlier=name,later='u5',changed_player_bytes=dif))
 def normalized(v):return dict(births=[{k:q[k]for k in ('default_configuration_hex','strength','field194')}for q in v['births']],
     timelines=[{k:x for k,x in q.items()if k!='spawn_stores'}for q in v['timelines']],resolved=v['resolved'])
 assert normalized(versions['retail'])==normalized(versions['tu11'])==normalized(versions['u5'])
 listings=[];pairs=[]
 for name,t in sources.items():
  for key,entry in t.i.items():
   _,ins=literal(t.code,entry,t.starts);listings.append(dict(image=name,owner=key,entry=entry,instructions=[dict(address=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for i in ins]))
   if name!='u5':
    u=sources['u5'];_,later=literal(u.code,u.i[key],u.starts);equal=complete_tokens(ins,t.image)==complete_tokens(later,u.image)
    pairs.append(dict(earlier=name,later='u5',owner=key,complete_instructions=len(ins),later_complete_instructions=len(later),complete_shape_equal=equal,relocated_operands=references(ins,later,t.image,u.image)if equal else [],scope='full bodies retained even where changed'))
 data=dict(schema='original-damage-player-inputs-v1',boundary=__doc__,strength_inputs=list(range(256)),rates=list(rates),resolver_inputs=rows,versions=versions,instruction_patches=[],supplied_returns=[])
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 deps={}
 for mod in list(sys.modules.values()):
  path=getattr(mod,'__file__',None)
  if path:
   p=Path(path).resolve()
   if p.is_relative_to(ROOT/'tools')and p.suffix=='.py':deps[str(p.relative_to(ROOT))]=sha(p)
 prior_path=ROOT/'analysis/multiplayer/integration/spawn-source/player-visibility/report.json'
 assert sha(prior_path)=='f58232e963b26bcb4a6471db8c40488f2bb195c4be5cf8e0617331ae4d5f78dd'
 prior=json.loads(prior_path.read_text());ordinary=[]
 for q in prior['cases']:
  for stage in q['stages']:
   if stage['name']=='original_normal_player_spawn':
    ordinary.append(dict(case=q['case'],completion=stage['completion'],entry=stage['entry'],
        supplied_service_entries=sorted(set(s.get('entry',s.get('service'))for s in stage['services'])),
        field194_stores=[s for s in q['player_and_object_stores']if s.get('kind')=='player'and s['phase']=='original_normal_player_spawn'and int(s['offset'],16)<=0x194<int(s['offset'],16)+s['size']]))
 report=dict(schema='original-damage-player-inputs-report-v1',oracle_sha256=sha(__file__),fixture_sha256=sha(a.fixture),lockout_sha256=CACHE_PIN,
     dependency_sha256=dict(sorted(deps.items())),exhaustive_birth_cases=768,timeline_preparation_birth_calls=len(rates)*3,
     whole_birth_calls=768+len(rates)*3,whole_mode4_spawn_calls=len(rates)*3,
     whole_player_tick_calls=sum(len(q['states'])-1 for v in versions.values()for q in v['timelines']),whole_resolver_calls=len(rows)*3,
     complete_source_pairs=pairs,instruction_listings=listings,cross_image_birth_differences=birth_differences,
     prior_ordinary_spawn=dict(report=str(prior_path.relative_to(ROOT)),report_sha256=sha(prior_path),cases=ordinary,
       scope='Pinned prior TU1.1 ordinary-spawn evidence re-read, not rerun; original lower-owner service boundaries remain declared and are not new no-service execution'),
     boundary=__doc__,instruction_patches=[],supplied_returns=[])
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('whole_birth_calls','whole_mode4_spawn_calls','whole_player_tick_calls','whole_resolver_calls','fixture_sha256')},indent=2));print('Complete equal owner pairs:',sum(p['complete_shape_equal']for p in pairs),'of',len(pairs))
if __name__=='__main__':main()
