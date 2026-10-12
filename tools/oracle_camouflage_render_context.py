#!/usr/bin/env python3
"""Whole original camouflage cap/callback and shader-parent scalar consumers.

Backed player/object/context/clock records are declared ABI inputs. Actual
registered Oddball callback, inventory tag search, selected trait callback,
camouflage cap/dirty-entity-NONE owner and scalar37 dispatch execute unchanged.
Render contexts include declared multi-parent chains and first-biped stopping.
CTF caller is preserved as complete static instructions, not supplied successful
weapon/objective services. No patched instruction or game return is supplied.
This is not complete objective gameplay, render submission, GPU or transport.
"""
import argparse,itertools,json,re,struct,sys
from pathlib import Path
from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_XMM0
from oracle_camouflage_fp import CamoFP
from oracle_king_trait_consumers import H,T,U,NONE
from oracle_result_statistics import R,G,M
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,CACHE_PIN,literal,complete_tokens,references,sha

OUT=ROOT/'analysis/multiplayer/game-variants/camouflage-render-context-20261011'
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
class Context(CamoFP):
 def __init__(self,name,cache):
  super().__init__(name,cache);self.name=name;self.cache=cache
  text=next(s for s in self.image.sections if s.name=='.text');self.text=text;self.raw=self.image.section_bytes(text)
  def find(pattern,tail=None):
   matches=[text.va+m.start()for m in re.finditer(pattern,self.raw,re.S)if tail is None or tail in self.raw[m.start():m.start()+0x90]];assert len(matches)==1,(name,matches);return matches[0]
  cap=find(rb'\x83\xf8\xff\x74.\x8b\x0d....\x8b\x51\x44\x25\xff\xff\x00\x00\x69\xc0\x1c\x02\x00\x00',bytes.fromhex('f30f1089b0020000'))
  parent=find(rb'\x56\x6a\xff\x8b\xf0\xe8....\x85\xc0\x0f\x57\xc9\x0f\x28\xc1\x74.')
  odd_v=self.u(self.u(self.spec['table']+3*4));ctf_v=self.u(self.u(self.spec['table']+4))
  self.d=dict(cap=cap,render_parent=parent,oddball_player=self.u(odd_v+15*4),ctf_player=self.u(ctf_v+15*4),oddball_trait=self.u(odd_v+35*4))
  _,ins=literal(self.code,self.d['oddball_player'],self.starts);calls=[int(i.op_str,16)for i in ins if i.mnemonic=='call'and i.op_str.startswith('0x')]
  assert len(calls)==2 and calls[1]==cap;self.d['inventory_kind_search']=calls[0]
  _,ins=literal(self.code,cap,self.starts);calls=[int(i.op_str,16)for i in ins if i.mnemonic=='call'];assert len(calls)==1;self.d['dirty_unit']=calls[0]
  _,ins=literal(self.code,parent,self.starts);calls=[int(i.op_str,16)for i in ins if i.mnemonic=='call'];assert len(calls)==1;self.d['initial_object_lookup']=calls[0]
  shims=[]
  for m in re.finditer(rb'\xe9',self.raw):
   off=m.start()
   if off>=8 and off+5<=len(self.raw)and text.va+off+5+struct.unpack_from('<i',self.raw,off+1)[0]==parent and self.raw[off-8:off-6]==b'\x8b\x0d'and self.raw[off-2:off]==b'\x8b\x01':shims.append(text.va+off-8)
  assert len(shims)==1;self.d['scalar37_shim']=shims[0];self.context_global=struct.unpack('<I',self.image.read(shims[0]+2,4))[0]
  dispatch=[]
  for m in re.finditer(rb'\xff\x24\x85',self.raw):
   table=struct.unpack_from('<I',self.raw,m.start()+3)[0]
   try:target=struct.unpack('<I',self.image.read(table+37*4,4))[0]
   except (ValueError,KeyError):continue
   if target==shims[0]:dispatch.append((text.va+m.start(),table))
  assert len(dispatch)==1;self.d['scalar_dispatch'],self.scalar_table=dispatch[0]
  self.trace=[];self.amount_reads=[];self.tracing=False;self.g.hook_add(UC_HOOK_CODE,self.observe_context);self.g.hook_add(UC_HOOK_MEM_READ,self.observe_amount)
  self.ball=next(tag for tag in cache.tags.values()if tag['group']=='weap'and tag['name']=='objects\\weapons\\multiplayer\\ball\\ball')
  self.flag=next(tag for tag in cache.tags.values()if tag['group']=='weap'and tag['name']=='objects\\weapons\\multiplayer\\flag\\flag')
  self.smg=next(tag for tag in cache.tags.values()if tag['group']=='weap'and tag['name']=='objects\\weapons\\rifle\\smg\\smg')
 def observe_context(self,g,pc,n,_):
  if self.tracing and pc in self.d.values():
   sp=g.reg_read(UC_X86_REG_ESP);self.trace.append(dict(entry=pc,eax=g.reg_read(UC_X86_REG_EAX),stack12_hex=bytes(g.mem_read(sp+4,12)).hex()))
 def observe_amount(self,g,access,at,n,value,_):
  if self.tracing and U<=at<U+0x4000 and (at-U)%0x800==0x2b0:self.amount_reads.append((at-U)//0x800)
 def object_records(self,objects):
  self.w(H+0x24,12);self.w(H+0x38,len(objects));self.w(H+0x44,T);self.g.mem_write(T,bytes(0x100));self.g.mem_write(U,bytes(0x4000))
  for slot,row in enumerate(objects):
   at=U+slot*0x800;self.g.mem_write(T+slot*12,struct.pack('<HBBII',row.get('salt',0x8aaa),0,row['type'],0,0 if row.get('null',False)else at))
   self.g.mem_write(at+0xaa,bytes([row['type']]));self.w(at+0x14,row.get('parent',NONE));self.w(at+0x2b0,row.get('primary_bits',0));self.w(at+0xd4,NONE)
 def callback(self,row):
  self.setup(dict(camo=row['primary'],unit_flags=row['flags'],unit_none=row['unit_none'],global_flags=row['global_flags']))
  self.w(M+0xc14,3);self.w(G+0x180,3);self.g.mem_write(G+0xc,bytes([row['mode']]))
  self.w(H+0x24,12);self.w(H+0x38,2);self.w(H+0x44,T);self.g.mem_write(T,struct.pack('<HBBIIHBBII',0x8aaa,0,0,0,U,0x8aaa,0,2,0,U+0x800))
  self.g.mem_write(U+0x218,b'\xff'*16)
  if row['item']!='none':
   tag={'ball':self.ball,'flag':self.flag,'smg':self.smg}[row['item']];self.w(U+0x218+row['slot']*4,0x8aaa0001);self.w(U+0x800,int(tag['datum'],16))
  before=self.state();players=bytes(self.g.mem_read(R,16*0x21c));objects=bytes(self.g.mem_read(U,0x1000));self.trace=[];self.tracing=True
  self.call(self.d['oddball_player'],(0x9abc0000,));self.tracing=False;actual=bytearray(self.g.mem_read(U,0x1000));actual[0x2b0:0x2b4]=objects[0x2b0:0x2b4]
  assert bytes(actual)==objects and bytes(self.g.mem_read(R,16*0x21c))==players
  return dict(before=before,after=self.state(),calls=self.trace.copy())
 def cap(self,row):
  self.setup(dict(camo=row['primary'],unit_flags=row['flags'],unit_none=row['unit_none']));before=self.state();self.trace=[];self.tracing=True
  self.call(self.d['cap'],registers=((UC_X86_REG_EAX,NONE if row['player_none']else 0x9abc0000),));self.tracing=False
  return dict(before=before,after=self.state(),calls=self.trace.copy())
 def render(self,row):
  self.object_records(row['objects']);self.w(H+0x9000,row['datum']);self.w(self.context_global,H+0x9000)
  objects=bytes(self.g.mem_read(U,0x4000));headers=bytes(self.g.mem_read(T,0x100));self.trace=[];self.amount_reads=[];self.tracing=True
  self.call(self.d['scalar_dispatch'],registers=((UC_X86_REG_EAX,37),));self.tracing=False
  assert bytes(self.g.mem_read(U,0x4000))==objects and bytes(self.g.mem_read(T,0x100))==headers
  return dict(scalar_bits=self.g.reg_read(UC_X86_REG_XMM0)&NONE,primary_read_slots=self.amount_reads.copy(),calls=self.trace.copy())
def render_inputs():
 rows=[]
 for value in (-.1,0,.25,.5,1,1.1):
  patterns=[
   ('biped',[(0,NONE,value)]),
   ('biped_parented_vehicle',[(0,0x8aaa0001,value),(1,NONE,.9)]),
   ('attached_weapon',[(2,0x8aaa0001,.9),(0,NONE,value)]),
   ('weapon_biped_vehicle',[(2,0x8aaa0001,.9),(0,0x8aaa0002,value),(1,NONE,.8)]),
   ('weapon_vehicle',[(2,0x8aaa0001,.9),(1,NONE,value)]),
   ('unattached_weapon',[(2,NONE,value)]),
   ('unparented_vehicle',[(1,NONE,value)]),
   ('weapon_prop_biped',[(2,0x8aaa0001,.9),(6,0x8aaa0002,.8),(0,NONE,value)]),
  ]
  for kind,objs in patterns:rows.append(dict(kind=kind,datum=0x8aaa0000,objects=[dict(type=t,parent=p,primary_bits=bits(v))for t,p,v in objs]))
 rows.extend((dict(kind='none',datum=NONE,objects=[]),dict(kind='initial_salt_mismatch',datum=0x8aab0000,objects=[dict(type=0,primary_bits=bits(.5))]),
  dict(kind='initial_zero_salt',datum=0x8aaa0000,objects=[dict(type=0,salt=0,primary_bits=bits(.5))]),
  dict(kind='null_parent_body',datum=0x8aaa0000,objects=[dict(type=2,parent=0x8aaa0001),dict(type=0,null=True)])))
 return rows
def xrefs(t,target):
 result=[]
 for m in re.finditer(rb'[\xe8\xe9]',t.raw):
  off=m.start()
  if off+5<=len(t.raw)and t.text.va+off+5+struct.unpack_from('<i',t.raw,off+1)[0]==target:
   i=t.code.at(t.text.va+off);assert i.mnemonic in ('call','jmp');result.append(dict(address=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str))
 return result
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/camouflage_render_context_original.json');a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN;cache=HudCache(ROOT/'assets/disc/maps/lockout.map');sources={};versions={};lists=[];pairs=[];render=render_inputs()
 callbacks=[dict(primary=p,flags=f,unit_none=n,global_flags=g,mode=m,item=i,slot=s)for p,f,n,g,m,i,s in itertools.product((.25,.5,.75),(0,8),(False,True),(0,4),(1,4),('none','smg','flag','ball'),range(4))]
 caps=[dict(primary=p,flags=f,unit_none=n,player_none=q)for p,f,n,q in itertools.product((-.1,0,.25,.5,.75,1),(0,8),(False,True),(False,True))]
 for name in ('retail','tu11','u5'):
  t=Context(name,cache);sources[name]=t;versions[name]=dict(image_sha256=t.spec['pin'],owners=t.d,cap=[t.cap(row)for row in caps],oddball_callback=[t.callback(row)for row in callbacks],render=[t.render(row)for row in render],cap_direct_xrefs=xrefs(t,t.d['cap']),
      objective_tags=[dict(**tag,weapon_kind=struct.unpack_from('<h',cache.data,tag['offset']+0x290)[0])for tag in (t.flag,t.ball)],scalar_dispatch_table=t.scalar_table,context_global=t.context_global)
  for key,entry in t.d.items():
   _,ins=literal(t.code,entry,t.starts);lists.append(dict(image=name,owner=key,entry=entry,instructions=[dict(address=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for i in ins]))
 for name in ('retail','tu11'):
  t=sources[name];u=sources['u5']
  for key in t.d:
   if key=='scalar_dispatch':continue # Jump-table function has other unrelated scalar slots.
   _,old=literal(t.code,t.d[key],t.starts);_,new=literal(u.code,u.d[key],u.starts);eq=complete_tokens(old,t.image)==complete_tokens(new,u.image)
   pairs.append(dict(earlier=name,later='u5',owner=key,complete_shape_equal=eq,instructions=len(old),later_instructions=len(new),relocated_operands=references(old,new,t.image,u.image)if eq else []))
 def normalize(version):return {key:[{k:v for k,v in row.items()if k!='calls'}for row in version[key]]for key in ('cap','oddball_callback','render')}
 assert normalize(versions['retail'])==normalize(versions['tu11'])==normalize(versions['u5'])
 data=dict(schema='original-camouflage-render-context-v1',boundary=__doc__,cap_inputs=caps,oddball_callback_inputs=callbacks,render_inputs=render,versions=versions,instruction_patches=[],supplied_returns=[])
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 deps={}
 for mod in list(sys.modules.values()):
  path=getattr(mod,'__file__',None)
  if path:
   p=Path(path).resolve()
   if p.is_relative_to(ROOT/'tools')and p.suffix=='.py':deps[str(p.relative_to(ROOT))]=sha(p)
 report=dict(schema='original-camouflage-render-context-report-v1',boundary=__doc__,oracle_sha256=sha(__file__),fixture_sha256=sha(a.fixture),lockout_sha256=CACHE_PIN,
   whole_cap_calls=len(caps)*3,whole_oddball_callback_calls=len(callbacks)*3,whole_render_scalar_calls=len(render)*3,
   full_instruction_listings=lists,complete_source_pairs=pairs,dependency_sha256=dict(sorted(deps.items())),instruction_patches=[],supplied_returns=[],
   limits=['CTF callback is static full-body source evidence only; no successful objective services are supplied.',
      'Render context/object chains are declared inputs. Initial salt/NONE/null-pointer guards execute; source parent creation and natural render-context publication are separate.',
      'No cycles, invalid/unbacked raw parent, NaN/infinity or replicated dirty entity are admitted.',
      'Extern37 scalar is not a complete camouflage overlay/distortion/shadow/render proof.'])
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('fixture_sha256','whole_cap_calls','whole_oddball_callback_calls','whole_render_scalar_calls')},indent=2))
if __name__=='__main__':main()
