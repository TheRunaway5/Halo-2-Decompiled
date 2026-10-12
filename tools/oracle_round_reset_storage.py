#!/usr/bin/env python3
"""Bind original runtime-storage owners to the unchanged authored round reset.

The game arena's raw backing, empty cursor/tail and checksum seed are declared
API inputs. Original physical allocation/protection and whole title startup are
not executed. Original allocator construction/allocation, selected storage/map
initializers and each reached reset instruction execute without replacement.
The decal initializer's VirtualAlloc boundary stays stopped. Its vertex cache
is independently allocated/constructed through original APIs and explicitly
bound as a caller context; the stopped frame is never resumed or supplied a
return. Players retain declared NONE-unit API records, migrated into original
allocated pool slots. This is neither live object construction/teardown nor a
normal Elimination match. The sound initializer is independently probed with
its process arena unavailable, then excluded from the selected game-arena
initializers. Its original stop and the subsequent round-reset stop are kept.
Missing dependencies remain original stops.
"""
import argparse,json,struct,sys
from pathlib import Path
from capstone.x86_const import X86_OP_MEM,X86_OP_IMM,X86_OP_REG,X86_REG_ECX,X86_REG_EAX,X86_REG_ESI
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_round_lifecycle import LifecycleTrace,ROOT,HudCache,literal,sha,variants,compact_raw_records,CACHE_PIN,R,G,complete_tokens,references
from oracle_game_over import SPECS
from oracle_game_variants import MENU_PIN
from oracle_mp_full_scoreboard_1_5 import REGS

OUT=ROOT/'analysis/multiplayer/game-variants/round-reset-storage'
TABLES={'retail':0x440dd8,'tu11':0x447428,'u5':0x44aed0}
GAME_INIT={'retail':0x137c20,'tu11':0x139390,'u5':0x13a700}
INDEX_ALLOC={'retail':0x16b990,'tu11':0x16d410,'u5':0x16e7f0}
ARENA,ALLOCATOR=0x1200000,0x1190000
ROWS=(19,20,27,34,37,39,40,44,58)
SELECTED_ROWS=tuple(n for n in ROWS if n!=44)

def ins(t,e):return literal(t.code,e,t.starts|{e})[1]
def direct_calls(rows):return [i.operands[0].imm for i in rows if i.mnemonic=='call'and i.operands[0].type==X86_OP_IMM]
def absolute_stores(rows,register=X86_REG_ESI):
 return [i.operands[0].mem.disp for i in rows if i.mnemonic=='mov'and len(i.operands)==2 and
  i.operands[0].type==X86_OP_MEM and not i.operands[0].mem.base and not i.operands[0].mem.index and
  i.operands[1].type==X86_OP_REG and i.operands[1].reg==register]

def metadata(t):
 rows={n:dict(initialize=t.u(TABLES[t.name]+n*36),reset=t.u(TABLES[t.name]+n*36+8),
  table_word=f'{TABLES[t.name]+n*36:08x}',record_hex=t.image.read(TABLES[t.name]+n*36,36).hex())for n in ROWS}
 init=ins(t,GAME_INIT[t.name]);arena_init=direct_calls(init)[0]
 def loaded(reg):return next(i.operands[1].mem.disp for i in init if i.mnemonic=='mov'and
  i.operands[0].type==X86_OP_REG and i.operands[0].reg==reg and i.operands[1].type==X86_OP_MEM)
 base,cursor=loaded(X86_REG_ECX),loaded(X86_REG_EAX);allocator_global=SPECS[t.name]['clock']-0x28
 pattern=b'\xa1'+struct.pack('<I',allocator_global)+b'\x85\xc0\x74\x06\xc7\x00'
 text=t.image.section('.text');code=t.image.section_bytes(text);positions=[];p=code.find(pattern)
 while p>=0:positions.append(text.va+p);p=code.find(pattern,p+1)
 assert len(positions)==1
 allocator_ctor=positions[0];vtable=struct.unpack('<I',t.image.read(allocator_ctor+11,4))[0]
 assert any(i.mnemonic=='mov'and i.operands[-1].type==X86_OP_IMM and i.operands[-1].imm==vtable for i in ins(t,arena_init))
 decal=ins(t,rows[37]['initialize']);decal_globals=absolute_stores(decal)
 assert len(decal_globals)==2
 cache_storage=direct_calls(decal)[-1];cache=ins(t,cache_storage)
 cache_ctor=direct_calls(cache)[-1];cache_global=absolute_stores(cache)[-1];platform=direct_calls(cache)[0]
 ctor_call=next(n for n,i in enumerate(cache)if i.mnemonic=='call'and i.operands[0].type==X86_OP_IMM and i.operands[0].imm==cache_ctor)
 pushes=[i.operands[0].imm for i in cache[:ctor_call]if i.mnemonic=='push'and i.operands[0].type==X86_OP_IMM]
 # Last seven immediate pushes are the original cache constructor inputs,
 # reversed around its dynamic destination and allocator owner.
 assert pushes[-4:-1]==[0x400,6,0x500]
 cache_args=[pushes[-1],0x500,6,0x400,pushes[-5],pushes[-6],pushes[-7],ALLOCATOR]
 players=absolute_stores(ins(t,rows[19]['initialize']));views=absolute_stores(ins(t,rows[20]['initialize']))
 masks=absolute_stores(ins(t,rows[34]['initialize']))
 sound=ins(t,rows[44]['initialize']);sound_globals=absolute_stores(sound)
 assert len(sound_globals)==2
 reset_calls=direct_calls(ins(t,t.phase_owners['reset_objects']))
 # Source positions after decal, contrail, effect, object registry and light
 # resets. These complete source instruction shapes are checked across images.
 sound_reset=reset_calls[-4]
 assert len(players)==2 and len(views)==1 and len(masks)==1
 return dict(rows=rows,game_initialize=GAME_INIT[t.name],arena_initialize=arena_init,arena_base_global=base,
  arena_cursor_global=cursor,arena_tail_global=base+8,arena_crc_global=base+12,allocator_global=allocator_global,
  allocator_constructor=allocator_ctor,allocator_vtable=vtable,allocator_allocate=t.u(vtable),
  decal_pool=decal_globals[0],decal_context=decal_globals[1],cache_storage=cache_storage,
  cache_constructor=cache_ctor,cache_global=cache_global,cache_arguments_after_destination=cache_args,
  platform_entry=platform,player_pool=players[0],player_context=players[1],view_context=views[0],mask_context=masks[0],
  sound_context=sound_globals[0],sound_pool=sound_globals[1],sound_reset=sound_reset,indexed_allocator=INDEX_ALLOC[t.name],
  object_teardown_owners=reset_calls[1:3],unreached_reset_tail=reset_calls[-3:])

class ResetTrace(LifecycleTrace):
 def __init__(self,name,cache):
  super().__init__(name,cache);self.meta=metadata(self);self.stages=[];self.guard_stop=None
  self.g.hook_add(UC_HOOK_CODE,self.guard)
  for row in self.meta['rows'].values():self.starts.update((row['initialize'],row['reset']))
  self.starts.update(self.meta[k]for k in('allocator_constructor','allocator_allocate','cache_storage','cache_constructor'))
  self.g.mem_map(ARENA,0x400000);self.g.mem_map(ALLOCATOR,0x1000);self.bindings=[]
 def guard(self,g,a,n,user):
  if a==self.meta['platform_entry']:
   sp=g.reg_read(UC_X86_REG_ESP)
   self.guard_stop=dict(kind='unprovided_original_VirtualAlloc_boundary',entry=f'{a:08x}',
    return_address=f'{self.u(sp):08x}',arguments=[self.u(sp+4+n*4)for n in range(4)])
   g.emu_stop()
 def snapshot(self):
  result=self.record();pool=self.u(self.meta['player_pool'])
  if pool:result['players_hex']=bytes(self.g.mem_read(self.u(pool+0x44),16*0x21c)).hex()
  result['arena_cursor']=self.u(self.meta['arena_cursor_global']);result['arena_crc']=self.u(self.meta['arena_crc_global'])
  result['storage']={}
  for key,size in(('player_context',0x130),('view_context',0x264),('mask_context',0x4201),
   ('decal_context',0x3c2c),('cache_global',0x613c)):
   at=self.u(self.meta[key]);result['storage'][key]=dict(pointer=f'{at:08x}',raw_hex=bytes(self.g.mem_read(at,size)).hex()if at else None)
  for key in('player_pool','decal_pool'):
   at=self.u(self.meta[key])
   result['storage'][key]=dict(pointer=f'{at:08x}',header_hex=bytes(self.g.mem_read(at,0x4c)).hex()if at else None)
  result['sound_context_pointer']=f'{self.u(self.meta["sound_context"]):08x}'
  result['sound_pool_pointer']=f'{self.u(self.meta["sound_pool"]):08x}'
  result['object_pool_pointer']=f'{self.u(self.phase_owners["objects"]):08x}'
  result['object_pool_header_hex']=bytes(self.g.mem_read(self.u(self.phase_owners['objects']),0x4c)).hex()
  return result
 def registers(self):
  return {name:f'{self.g.reg_read(reg):032x}'if name.startswith('xmm')else f'{self.g.reg_read(reg):08x}'for name,reg in REGS}
 def stage(self,name,entry,args=(),regs=(),require=False):
  self.guard_stop=None;self.faults=[];before=self.snapshot()
  try:calls=self.invoke(entry,args,regs);complete=True;error=None
  except Exception as e:
   if not self.guard_stop and not self.faults:raise
   calls=[f'{a:08x}'for a in self.calls];complete=False;error=str(e)
  row=dict(name=name,entry=f'{entry:08x}'if isinstance(entry,int)else f'{self.phase_owners[entry]:08x}',
   arguments=[f'{v&0xffffffff:08x}'for v in args],input_registers={next(n for n,r in REGS if r==reg):f'{v&0xffffffff:08x}'for reg,v in regs},
   after_registers=self.registers(),
   completion='complete_original_return'if complete else'stopped_original_instruction',
   calls=calls,before=before,after=self.snapshot(),stop=None if complete else
    dict(eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',guard=self.guard_stop,faults=self.faults,error=error))
  self.stages.append(row)
  assert not require or complete,(self.name,name,row['stop'])
  return complete

def prepared(name,cache):
 t=ResetTrace(name,cache);t.prepare(dict(index=1,lifecycle=3,status=1));m=t.meta
 for key,value in(('arena_base_global',ARENA),('arena_cursor_global',0),('arena_tail_global',0),
  ('arena_crc_global',0xffffffff),('allocator_global',ALLOCATOR)):t.w(m[key],value)
 return t

def run(name,cache,variant):
 cold=prepared(name,cache);m=cold.meta
 assert not cold.stage('independent_sound_storage_without_process_arena',m['rows'][44]['initialize'])
 sound_stop=cold.stages[-1]
 assert sound_stop['stop']['faults'][0]['address']=='00000000' and not sound_stop['stop']['guard']
 t=prepared(name,cache);m=t.meta;old_players=bytes(t.g.mem_read(R,16*0x21c))
 t.stage('original_game_arena_allocator_constructor',m['allocator_constructor'],(0,),require=True)
 for n in SELECTED_ROWS:
  if not t.stage(f'original_storage_row_{n}',m['rows'][n]['initialize']):
   assert n==37 and t.guard_stop and t.guard_stop['arguments']==[0,12,0x101000,4],(name,n,t.stages[-1]['stop'])
   # A separate original API allocation/constructor, not a resumed initializer
   # frame or a supplied result for its platform call.
   t.stage('independent_original_vertex_cache_allocation',m['allocator_allocate'],(0x613c,),
    ((UC_X86_REG_ECX,ALLOCATOR),),True)
   pointer=t.g.reg_read(UC_X86_REG_EAX)
   t.stage('independent_original_vertex_cache_constructor',m['cache_constructor'],
    (pointer,*m['cache_arguments_after_destination']),require=True)
   assert t.u(pointer+0x38)==1
   t.bindings.append(dict(kind='independent_original_cache_context',after_stage=len(t.stages)-1,
    address=f'{m["cache_global"]:08x}',before=f'{t.u(m["cache_global"]):08x}',value=f'{pointer:08x}'))
   t.w(m['cache_global'],pointer)
 for n in SELECTED_ROWS:
  t.stage(f'original_map_reset_row_{n}',m['rows'][n]['reset'],require=True)
 # Preserve the declared occupied roster using genuine original indexed pool
 # allocation and salts. Source join/profile/current-unit producers are excluded.
 pool=t.u(m['player_pool']);data=t.u(pool+0x44)
 for slot in(0,3):
  t.stage(f'original_player_slot_{slot}',INDEX_ALLOC[name],regs=((UC_X86_REG_EDX,pool),(UC_X86_REG_EDI,slot)),require=True)
  datum=t.g.reg_read(UC_X86_REG_EAX);assert datum!=0xffffffff and datum&65535==slot
  t.g.mem_write(data+slot*0x21c+2,old_players[slot*0x21c+2:(slot+1)*0x21c])
 t.g.mem_write(G+0x13c,bytes.fromhex(variant['raw_hex']))
 assert t.u(G+0x184)&8 # literal original full-reset owner tests bit3 here
 assert not t.stage('unchanged_authored_Elimination_next_active','phase')
 stop=t.stages[-1]['stop'];assert stop['faults'][0]['address']=='00000078' and not stop['guard']
 assert t.g.reg_read(UC_X86_REG_EIP)==ins(t,m['sound_reset'])[1].address
 assert t.u(t.u(m['cache_global'])+0x38)==2
 assert bytes(t.g.mem_read(t.u(m['mask_context']),0x4201))==b'\xff'*0x4201
 calls={int(e,16)for e in t.stages[-1]['calls']}
 assert not calls.intersection(m['object_teardown_owners']+m['unreached_reset_tail'])
 assert all(t.u(data+slot*0x21c+0x2c)==0xffffffff for slot in(0,3))
 # Guest .text is compared with the unchanged image after the selected calls.
 section=t.image.section('.text');original=t.image.section_bytes(section)
 assert bytes(t.g.mem_read(section.va,len(original)))==original
 return t,dict(image_sha256=t.spec['pin'],metadata=m,variant=variant,stages=t.stages,
  caller_binding_events=t.bindings,independent_sound_storage_stop=sound_stop,whole_guest_text_unchanged=True)

def source_evidence(traces,directory):
 directory.mkdir(parents=True,exist_ok=True);listings=[];pairs=[]
 def owners(t):
  entries={f'row_{n}_{key}':row[key]for n,row in t.meta['rows'].items()for key in('initialize','reset')}
  entries.update({k:t.meta[k]for k in('game_initialize','arena_initialize','allocator_constructor',
   'allocator_allocate','cache_storage','cache_constructor','platform_entry','sound_reset','indexed_allocator')})
  entries.update({k:t.phase_owners[k]for k in('phase','reset_objects')})
  return entries
 for name,t in traces.items():
  entries=set(owners(t).values())|{int(e,16)for stage in t.stages for e in stage['calls']}
  for entry in sorted(entries):
   rows=ins(t,entry);p=directory/f'{name}-{entry:08x}.asm'
   for i in rows:assert t.image.read(i.address,len(i.bytes))==i.bytes
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rows))
   listings.append(dict(file=p.name,instructions=len(rows),sha256=sha(p)))
  if name=='u5':continue
  later=traces['u5'];later_entries=owners(later)
  for key,entry in owners(t).items():
   left,right=ins(t,entry),ins(later,later_entries[key])
   assert complete_tokens(left,t.image)==complete_tokens(right,later.image),(name,key)
   pairs.append(dict(earlier=name,owner=key,earlier_entry=f'{entry:08x}',later_entry=f'{later_entries[key]:08x}',
    complete_shape_equal=True,instructions=len(left),relocated_operands=references(left,right,t.image,later.image)))
 return dict(instruction_listings=listings,complete_source_pairs=pairs)

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--verify-report',type=Path);a=ap.parse_args()
 if a.verify_report:assert a.output.resolve()!=a.verify_report.resolve()
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 cache=HudCache(ROOT/'assets/disc/maps/lockout.map');variant=variants()['7'];versions={};traces={}
 for name in TABLES:
  t,data=run(name,cache,variant);traces[name]=t;versions[name]=data
 evidence=source_evidence(traces,a.output.parent/'instructions')
 dependencies={}
 for module in tuple(sys.modules.values()):
  path=getattr(module,'__file__',None)
  if path:
   path=Path(path).resolve()
   if path.suffix=='.py' and path.is_relative_to(ROOT/'tools'):dependencies[str(path.relative_to(ROOT))]=sha(path)
 for spec in SPECS.values():dependencies[spec['tsv']]=sha(ROOT/spec['tsv'])
 data=compact_raw_records(dict(schema='original-round-reset-storage-v1',boundary=__doc__,versions=versions,**evidence,
  selected_callback_rows=list(SELECTED_ROWS),independently_stopped_callback_rows=[44],
  declared_arena=dict(base=f'{ARENA:08x}',mapped_bytes=0x400000,original_arena_bytes=0x3fe000,
   allocator_storage=f'{ALLOCATOR:08x}',cursor=0,tail=0,crc_seed=0xffffffff),
  lockout_sha256=CACHE_PIN,mainmenu_sha256=MENU_PIN,oracle_sha256=sha(__file__),dependencies=dependencies,
  instruction_patches=[],supplied_returns=[],
  caller_context_bindings=['Raw arena and allocator-object storage API inputs','Independent original vertex-cache API object',
   'Declared NONE-unit player records copied after original slot allocation; source-created salts retained']))
 serialized=json.dumps(data,sort_keys=True,separators=(',',':'))+'\n'
 if a.verify_report:assert serialized==a.verify_report.read_text(),'Complete independent report differs'
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(serialized)
 print(json.dumps(dict(images=3,original_complete_stages={n:sum(s['completion']=='complete_original_return'for s in d['stages'])for n,d in versions.items()},
  complete_source_pairs=len(evidence['complete_source_pairs']),instruction_listings=len(evidence['instruction_listings']),
  final_boundaries={n:d['stages'][-1]['stop']for n,d in versions.items()},report_sha256=sha(a.output)),indent=2))
if __name__=='__main__':main()
