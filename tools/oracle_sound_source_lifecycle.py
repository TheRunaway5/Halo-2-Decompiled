#!/usr/bin/env python3
"""Unchanged sound round/source cleanup with declared raw API controls.

The retained TU1.1 startup-prefix arena is actual original output stopped before
its device constructor. It is explicitly reused as ABI backing in all images,
not completed startup. Availability, occupied slots and their raw fields are
declared controls. Actual original pool constructors/indexed allocations execute.
Additional raw metadata/cache/driver backing is an API control, not loaded audio.
No game/platform instruction or return is replaced. Nonempty callbacks stop
before the indirect call; unavailable hardware stops on its original NULL read.
No full sound startup, world reset, native playback or operational variant is
established by these comparisons.
"""
import argparse,json,sys
from pathlib import Path
from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EIP
from capstone.x86_const import X86_OP_MEM,X86_OP_IMM,X86_OP_REG
from oracle_round_reset_storage import ResetTrace,ins,direct_calls,INDEX_ALLOC
from oracle_round_lifecycle import ROOT,HudCache,sha,compact_raw_records,complete_tokens,references,CACHE_PIN
from oracle_game_over import SPECS

PRIOR=ROOT/'analysis/multiplayer/integration/sound-startup-native/measured/report.json'
PRIOR_PIN='b2a1175e7dd524fc7d9587333e8055abd8cb4d30f6417e0ff293e9d1e4444287'
OUT=ROOT/'analysis/multiplayer/game-variants/sound-source-lifecycle'
AUDIO,SIZE,RAW,RAW_SIZE=0xa000000,0x23000,0x1600000,0x20000
BASE=dict(system=0x4ed950,sources=0x4ed94c,static_voices=0x4ed948,controllers=0x526468,
 effects=0x526470,tag_table=0x4eb114,ugh=0x526464,cache_pool=0x5098dc,shared_voices=0x5098ec,driver=0x526474)
NONE=0xffffffff

def owners(t):
 reset=t.meta['sound_reset'];calls=direct_calls(ins(t,reset));stop=calls[2]
 helpers=direct_calls(ins(t,stop));remove=helpers[-1];voice=helpers[0];sub=direct_calls(ins(t,voice))
 return dict(reset=reset,effects=calls[0],stop=stop,release_voice=voice,unpin=helpers[1],remove=remove,
  detach=direct_calls(ins(t,remove))[0],shared_voice=sub[-1],stream_stop=sub[0],
  free=direct_calls(ins(t,calls[0]))[-1])

def metadata(traces):
 all_owners={n:owners(t)for n,t in traces.items()};pairs=[];bindings={};reference=traces['tu11']
 for n,t in traces.items():
  relocated={}
  for key,entry in all_owners[n].items():
   left,right=ins(reference,all_owners['tu11'][key]),ins(t,entry)
   assert complete_tokens(left,reference.image)==complete_tokens(right,t.image),(n,key)
   refs=references(left,right,reference.image,t.image)
   for row in refs:
    if row['kind']=='image_memory_displacement':
     old,new=int(row['old_address'],16),int(row['later_address'],16)
     assert old not in relocated or relocated[old]==new
     relocated[old]=new
   if n!='tu11':pairs.append(dict(image=n,owner=key,complete_shape_equal=True,instructions=len(left),references=refs))
  bindings[n]={key:relocated[address]for key,address in BASE.items()}
 return all_owners,bindings,pairs

class Trace(ResetTrace):
 def __init__(self,name,cache,owner,binding,initial):
  super().__init__(name,cache);self.owner=owner;self.binding=binding;self.current=None
  self.starts.update(owner.values());self.starts.add(INDEX_ALLOC[name])
  self.g.mem_map(AUDIO,SIZE);self.g.mem_map(RAW,RAW_SIZE);self.g.mem_write(AUDIO,initial)
  self.g.hook_add(UC_HOOK_MEM_WRITE,self.observe);self.g.hook_add(UC_HOOK_CODE,self.callback_guard)
  prefix=json.loads(PRIOR.read_text())['stopped_original_caller']['globals']
  values={key:int(prefix[key],16)for key in('system','sources','static_voices','controllers','effects','shared_voices')}
  values.update(tag_table=RAW,ugh=RAW+0x2000,cache_pool=RAW+0x6000,driver=RAW+0x9000)
  for key,value in values.items():self.w(binding[key],value)
  # This is raw metadata API backing for index/refcount contracts, not an
  # authored sound selection, resident chunk or cache-ready producer.
  self.w(RAW+8,RAW+0x1000);self.w(RAW+0x2000+0x24,RAW+0x3000)
  self.w(RAW+0x2000+0x2c,RAW+0x4000);self.w(RAW+0x2000+0x44,RAW+0x5000)
  storage=direct_calls(ins(self,self.meta['rows'][44]['initialize']))[1]
  code=ins(self,storage);constructor=direct_calls(code)[0]
  call=next(i for i,z in enumerate(code)if z.mnemonic=='call'and z.operands[0].type==X86_OP_IMM and z.operands[0].imm==constructor)
  name=next(z.operands[0].imm for z in reversed(code[:call])if z.mnemonic=='push'and z.operands[0].type==X86_OP_IMM)
  allocator=next(z.operands[1].mem.disp for z in code if z.mnemonic=='mov'and z.operands[0].type==X86_OP_REG and
   z.operands[1].type==X86_OP_MEM and not z.operands[1].mem.base and not z.operands[1].mem.index)
  self.call(constructor,(RAW+0x6000,name,self.u(allocator)),((UC_X86_REG_EBX,512),(UC_X86_REG_EDX,16),(UC_X86_REG_ECX,0)))
  reset_pool=direct_calls(ins(self,self.meta['rows'][37]['reset']))[0]
  self.call(reset_pool,registers=((UC_X86_REG_ESI,RAW+0x6000),))
  self.constructor=dict(entry=f'{constructor:08x}',cache_pool=f'{RAW+0x6000:08x}',name=f'{name:08x}',allocator_global=f'{allocator:08x}')
  self.baseline=(bytes(self.g.mem_read(AUDIO,SIZE)),bytes(self.g.mem_read(RAW,RAW_SIZE)))
 def byte(self,address,value,size=1):self.g.mem_write(address,(value&((1<<(size*8))-1)).to_bytes(size,'little'))
 def value(self,key):return self.u(self.binding[key])
 def allocate(self,key,index):
  pool=self.value(key);self.call(INDEX_ALLOC[self.name],registers=((UC_X86_REG_EDX,pool),(UC_X86_REG_EDI,index)))
  datum=self.g.reg_read(UC_X86_REG_EAX);assert datum!=NONE and datum&65535==index
  return datum,self.u(pool+0x44)+index*self.u(pool+0x24)
 def snapshots(self):
  regions=[dict(address=f'{at+offset:08x}',raw_hex=bytes(self.g.mem_read(at+offset,min(4096,size-offset))).hex())
   for at,size in((AUDIO,SIZE),(RAW,RAW_SIZE))for offset in range(0,size,4096)]
  return regions+[dict(address=f'{at:08x}',raw_hex=bytes(self.g.mem_read(at,4)).hex())for at in sorted(self.binding.values())]
 def observe(self,g,access,address,size,value,user):
  if self.current is not None and (AUDIO<=address<AUDIO+SIZE or RAW<=address<RAW+RAW_SIZE):
   self.current['writes'].append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,
    before=int.from_bytes(g.mem_read(address,size),'little'),after=value&((1<<(size*8))-1)))
 def callback_guard(self,g,address,size,user):
  if self.current is None or not self.current['input'].get('callback'):return
  rows=ins(self,self.owner['stop'])
  site=next(i.address for i in rows if i.mnemonic=='call'and i.operands[0].type==X86_OP_REG)
  if address==site:
   self.current['callback_stop']=dict(site=f'{address:08x}',method=f'{g.reg_read(UC_X86_REG_EAX):08x}')
   g.emu_stop()
 def run(self,row):
  self.current=None;self.g.mem_write(AUDIO,self.baseline[0]);self.g.mem_write(RAW,self.baseline[1])
  self.faults=[];self.byte(self.value('system')+0x78,row.get('available',1))
  controller,c=self.allocate('controllers',3);self.byte(c+2,row.get('references',2)|0x80)
  self.byte(c+3,0x82);self.w(c+4,0x55550001)
  cache,cache_record=self.allocate('cache_pool',3);self.byte(cache_record+4,5);self.w(RAW+0x5000+8,cache)
  shared,shared_record=self.allocate('shared_voices',2);self.byte(shared_record+0xa,row.get('voice_references',1),2)
  self.byte(shared_record+2,33,2);self.w(self.value('system')+0x74,2)
  handles=[]
  for index in(0,35,383):
   handle,source=self.allocate('sources',index);handles.append(handle)
   self.byte(source+2,row.get('persistent',0)if index==383 else 0)
   self.byte(source+3,255);self.w(source+0xa8,controller if index==0 else NONE)
   self.byte(source+0xac,0 if row.get('voice')and index==0 else 65535,2)
   self.w(source+0x14,RAW+0x11000 if row.get('callback')and index==0 else 0)
   self.w(source+0x10,0x12345678)
   if row.get('pin')and index==0:
    self.byte(source+4,2);self.w(source+0xc,0)
    signed=row.get('signed',0)
    self.byte(source+0x9c,signed);self.byte(source+0x9d,signed);self.byte(source+0x9e,signed,2)
    self.byte(RAW+0x1000+8,-signed,2);self.byte(RAW+0x3000+8,-signed,2);self.byte(RAW+0x4000+0xc,-signed,2)
  self.w(RAW+0x11000+0x10,self.owner['reset']) # declared nonempty callback ABI; never entered
  if row.get('voice'):
   voice=self.value('static_voices');self.byte(voice+0xc,0,2);self.byte(voice+0xe,7,2)
   self.w(voice+4,shared);self.byte(voice+9,1);self.byte(voice+0xa,2);self.byte(voice+0xb,3)
   self.w(voice+0x1c,RAW+0x4000 if row.get('pin')else 0);self.w(voice+0x20,RAW+0x4000 if row.get('pin')else 0)
   self.byte(voice+0x18,row.get('signed',0),2);self.byte(voice+0x1a,row.get('signed',0),2)
   self.w(self.value('system')+0x60,1)
   if row.get('hardware'):self.byte(self.value('driver')+0xc+1,1)
  if row.get('effects',True):
   for index in(0,15):
    _,effect=self.allocate('effects',index);self.w(effect+8,controller if index==0 else NONE)
  if row.get('poison'):
   for key in('sources','controllers','effects','shared_voices'):self.byte(self.value(key)+0x2a,self.u(self.value(key)+0x2a)&255|8)
  current=dict(input=row,before=self.snapshots(),writes=[],callback_stop=None);self.current=current
  args=(row.get('reason',10),)if row.get('direct')else()
  regs=((UC_X86_REG_EAX,handles[0]),)if row.get('direct')else()
  self.calls=[]
  try:self.call(self.owner['stop'if row.get('direct')else'reset'],args,regs);outcome='complete_original_return'
  except Exception:
   assert self.faults or current['callback_stop'];outcome='stopped_original_instruction'
  current.update(after=self.snapshots(),outcome=outcome,calls=[f'{e:08x}'for e in self.calls],faults=self.faults,
   eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',source=f'{handles[0]:08x}')
  self.current=None;section=self.image.section('.text');assert bytes(self.g.mem_read(section.va,section.rsize))==self.image.section_bytes(section)
  return current

def inputs():
 rows=[dict(available=0),dict(available=2,persistent=255),dict(available=255,persistent=1)]
 for refs in(0,1,2,127,128,255):
  for poison in(False,True):
   for persistent in(0,1,2,255):rows.append(dict(references=refs,poison=poison,persistent=persistent))
 rows +=[dict(pin=True,signed=s,voice=v,voice_references=r)for s in(0,-1,-128)for v in(False,True)for r in(0,1,2)]
 rows +=[dict(direct=True,reason=r,pin=True,voice=True)for r in(0,10,0xffffffff)]
 rows +=[dict(callback=True,pin=True),dict(hardware=True,voice=True,pin=True),dict(direct=True,callback=True,voice=True)]
 return rows

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/sound_source_lifecycle_original.json')
 ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--verify-fixture',type=Path);a=ap.parse_args()
 assert sha(PRIOR)==PRIOR_PIN;assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 prefix=json.loads(PRIOR.read_text())['stopped_original_caller'];initial=bytes.fromhex(prefix['arena_hex']);assert len(initial)==SIZE
 cache=HudCache(ROOT/'assets/disc/maps/lockout.map');probes={n:ResetTrace(n,cache)for n in SPECS}
 owner,binding,pairs=metadata(probes);versions={};listings=[];directory=a.output.parent/'instructions';directory.mkdir(parents=True,exist_ok=True)
 for name in SPECS:
  t=Trace(name,cache,owner[name],binding[name],initial);records=[t.run(row)for row in inputs()]
  versions[name]=dict(image_sha256=t.spec['pin'],owners=owner[name],bindings=binding[name],constructor=t.constructor,records=records)
  for key,entry in owner[name].items():
   rows=ins(t,entry);p=directory/f'{name}-{key}-{entry:08x}.asm'
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rows))
   listings.append(dict(file=p.name,sha256=sha(p),instructions=len(rows)))
  if name=='u5':
   rows=ins(t,0x12000)
   assert t.meta['rows'][44]['initialize']in direct_calls(rows)
   assert any(i.mnemonic=='push'and i.operands[0].type==X86_OP_IMM and i.operands[0].imm==0x23000 for i in rows)
   p=directory/'u5-process-startup-00012000.asm'
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rows))
   listings.append(dict(file=p.name,sha256=sha(p),instructions=len(rows),scope='Static process-arena/startup authority; not executed'))
 data=compact_raw_records(dict(schema='original-sound-source-lifecycle-v1',boundary=__doc__,prior_prefix_sha256=PRIOR_PIN,
  versions=versions,instruction_patches=[],supplied_returns=[],whole_guest_text_unchanged=True))
 serialized=json.dumps(data,sort_keys=True,separators=(',',':'))+'\n'
 if a.verify_fixture:assert a.fixture.resolve()!=a.verify_fixture.resolve() and serialized==a.verify_fixture.read_text()
 a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(serialized)
 dependencies={}
 for module in tuple(sys.modules.values()):
  path=getattr(module,'__file__',None)
  if path:
   path=Path(path).resolve()
   if path.suffix=='.py'and path.is_relative_to(ROOT/'tools'):dependencies[str(path.relative_to(ROOT))]=sha(path)
 for spec in SPECS.values():dependencies[spec['tsv']]=sha(ROOT/spec['tsv'])
 a.output.write_text(json.dumps(dict(fixture_sha256=sha(a.fixture),oracle_sha256=sha(__file__),complete_source_pairs=pairs,
  instruction_listings=listings,source_image_pins={n:s['pin']for n,s in SPECS.items()},cache_sha256=CACHE_PIN,
  dependencies=dependencies,boundary=__doc__),sort_keys=True,separators=(',',':'))+'\n')
 print(json.dumps(dict(cases=sum(len(v['records'])for v in versions.values()),complete=sum(r['outcome']=='complete_original_return'for v in versions.values()for r in v['records']),
  stops={n:[r['eip']for r in v['records']if r['outcome']!='complete_original_return']for n,v in versions.items()},fixture_sha256=sha(a.fixture)),indent=2))
if __name__=='__main__':main()
