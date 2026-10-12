#!/usr/bin/env python3
"""Original arena/provider construction and replication allocation prerequisite.

Execute a bounded original boot suffix through real provider initialization,
the complete allocator constructor and wrapper publication. Arena and object
backing, caller stack and FS exception-chain storage are declared API inputs;
earlier physical allocation/operator-new and complete world boot are outside
this proof. Then execute actual allocator virtual allocate/info/compact/free
owners. No instruction patch, positive readiness or game return is supplied.
"""
from pathlib import Path
import argparse, collections, csv, hashlib, json, struct
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_INVALID, UcError
from unicorn.x86_const import *
from oracle_game_variants import ROOT, Source, STACK, STOP
from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import _Code, literal, complete_tokens, references, sha
from research_mp_lobby_later_update import table_pair

OUT=ROOT/'analysis/multiplayer/game-variants/replication-allocator-source-20261012'
ORCHESTRATION=ROOT/'analysis/multiplayer/game-variants/king-replication-orchestration-source-20261012/complete/report.json'
ORCHESTRATION_PIN='9713a9e33cf9b140c00268245578c7aa1037d54f9d14ce8d2ec15d1be6f536ff'
BOOT={
 'retail':dict(owner=0x81780,begin=0x81e99,end=0x81f49,constructor=0x32a850,
  wrapper_global=0x4d87f8,arena_global=0x4d87fc,size_global=0x4d87f4,
  object_vtable=0x454980,provider_vtable=0x454970,
  tables=(0x46e490,0x46e690,0x46e7a0)),
 'tu11':dict(owner=0x82190,begin=0x828a9,end=0x82959,constructor=0x330de0,
  wrapper_global=0x4dfdc0,arena_global=0x4dfdc4,size_global=0x4dfdbc,
  object_vtable=0x45bb30,provider_vtable=0x45bb1c,
  tables=(0x475978,0x475b78,0x475c88)),
 'u5':dict(owner=0x85370,begin=0x85a89,end=0x85b39,constructor=0x334950,
  wrapper_global=0x4e5458,arena_global=0x4e545c,size_global=0x4e5454,
  object_vtable=0x460a78,provider_vtable=0x460a64,
  tables=(0x47b150,0x47b350,0x47b460)),
}
REGS=(('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),
 ('edx',UC_X86_REG_EDX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI),
 ('ebp',UC_X86_REG_EBP),('esp',UC_X86_REG_ESP),('eflags',UC_X86_REG_EFLAGS))

def map_fresh(g,address,size):
 assert address%4096==0 and size%4096==0
 assert all(address+size-1<lo or address>hi for lo,hi,_ in g.mem_regions()),hex(address)
 g.mem_map(address,size)

class AllocatorBoundary(RuntimeError):pass

def setup_allocator(t,name,arena_base=0x5000000,arena_size=0x1000000,context_base=0x6000000,
                    stack=STACK,fs_exception_head=0xffffffff):
 """Reusable bounded boot composition on an existing original Source/KingTrace.

 All backing is fresh. Context fields are wrapper+0, object+0x100,
 diagnostic info+0x600 and allocation-size output+0x700. FS:[0] is explicitly
 backed and initialized; no caller thread identity is fabricated or needed by
 the authentic boot configuration (arg10=arg13=0). Earlier arena globals are
 recorded but not initialized: this suffix consumes ESI and stack+0x14.
 """
 b=BOOT[name];g=t.g
 assert t.spec['pin']==SPECS[name]['pin']
 assert g.reg_read(UC_X86_REG_FS_BASE)==0,'This bounded ABI uses a flat backed FS exception-chain cell.'
 map_fresh(g,arena_base,arena_size);map_fresh(g,context_base,0x1000)
 if not any(lo<=0 and hi>=4095 for lo,hi,_ in g.mem_regions()):map_fresh(g,0,4096)
 wrapper,object_=context_base,context_base+0x100
 g.mem_write(arena_base,b'\xcd'*arena_size)
 g.mem_write(context_base,b'\xa5'*0x1000)
 g.mem_write(0,struct.pack('<I',fs_exception_head))
 g.mem_write(stack,b'\0'*0x40)
 g.mem_write(stack+0x14,struct.pack('<I',arena_size))
 for reg,value in ((UC_X86_REG_ESI,arena_base),(UC_X86_REG_EBP,wrapper),
                   (UC_X86_REG_EBX,object_),(UC_X86_REG_ESP,stack),
                   (UC_X86_REG_EFLAGS,2)):
  g.reg_write(reg,value)
 before={k:g.reg_read(v)for k,v in REGS};before_arena_globals={k:t.u(b[k])for k in ('arena_global','size_global')}
 expected_args=[b['tables'][0],b['tables'][1],0x800,0,0x400,0x100000,b['tables'][2],arena_base,1,0,4,0x400,0]
 constructor_observations=[]
 def observe(g,address,size,_):
  if address==b['constructor']:
   sp=g.reg_read(UC_X86_REG_ESP)
   actual_args=list(struct.unpack('<13I',g.mem_read(sp+4,52)))
   assert actual_args==expected_args and g.reg_read(UC_X86_REG_ECX)==object_
   constructor_observations.append(dict(kind='whole_constructor_entry',entry=address,
    registers={k:g.reg_read(v)for k,v in REGS},stack56_hex=bytes(g.mem_read(sp,56)).hex(),arguments=actual_args))
  if address in (b['end']-0x20,):
   # The +20 relationship is checked against the genuine boot instruction;
   # this records the immediate constructor return before wrapper stores.
   constructor_observations.append(dict(kind='whole_constructor_return',entry=address,
    registers={k:g.reg_read(v)for k,v in REGS}))
 observation_hook=g.hook_add(UC_HOOK_CODE,observe)
 try:g.emu_start(b['begin'],b['end'],count=2000000)
 finally:g.hook_del(observation_hook)
 if g.reg_read(UC_X86_REG_EIP)!=b['end']:raise AllocatorBoundary('constructor did not complete')
 assert t.u(b['wrapper_global'])==wrapper and t.u(wrapper)==object_
 assert t.u(object_)==b['object_vtable'] and t.u(arena_base)==b['provider_vtable']
 assert t.u(object_+0x28)==0xdeadbeef and t.u(wrapper+4)==0
 assert bytes(g.mem_read(object_+0x24,1))==b'\0'
 assert t.u(arena_base+4)==arena_base+4096 and t.u(arena_base+8)==arena_base+arena_size
 assert t.u(arena_base+0xffc)==wrapper+12 and t.u(0)==fs_exception_head
 assert {k:t.u(b[k])for k in before_arena_globals}==before_arena_globals
 assert len(constructor_observations)==2 and constructor_observations[1]['registers']['eax']==object_
 return dict(arena_base=arena_base,arena_size=arena_size,context_base=context_base,
  wrapper=wrapper,object=object_,info=context_base+0x600,size_output=context_base+0x700,
  entry_stack=stack,stack64_before_hex=(bytes(0x14)+struct.pack('<I',arena_size)+bytes(0x28)).hex(),
  fs_exception_head=fs_exception_head,before_registers=before,after_registers={k:g.reg_read(v)for k,v in REGS},
  original_begin=b['begin'],original_end_exclusive=b['end'],constructor=b['constructor'],
  constructor_args=expected_args,constructor_observations=constructor_observations,
  declared_backing=dict(arena=dict(address=arena_base,size=arena_size,initial_byte=0xcd),
    context=dict(address=context_base,size=4096,initial_byte=0xa5),
    fs=dict(base=0,exception_chain_cell=0,initial_exception_head=fs_exception_head),
    stack=dict(address=stack,initial_hex=(bytes(0x14)+struct.pack('<I',arena_size)+bytes(0x28)).hex())),
  earlier_arena_globals=before_arena_globals,
  earlier_arena_globals_scope='Original suffix does not consume or produce these earlier physical-allocation globals; they remain original image/caller bytes.',
  wrapper_hex=bytes(g.mem_read(wrapper,16)).hex(),object_hex=bytes(g.mem_read(object_,0x34)).hex(),
  provider_page_hex=bytes(g.mem_read(arena_base,4096)).hex(),
  arena_sha256=hashlib.sha256(g.mem_read(arena_base,arena_size)).hexdigest(),pages=t.u(wrapper+12))

class AllocatorTrace(Source):
 def __init__(self,name):
  super().__init__(SPECS[name]);self.name=name
  self.starts={int(r['address'],16)for r in csv.DictReader((ROOT/self.spec['tsv']).open(),delimiter='\t')}
  self.code=_Code(self.image,'.text');self.recent=collections.deque(maxlen=32);self.current='initial';self.stopped=None
  self.boundaries=set();self.reached=collections.Counter();self.completed_calls=[]
  self.g.hook_add(UC_HOOK_CODE,self.hook);self.g.hook_add(UC_HOOK_MEM_INVALID,self.invalid)
 def hook(self,g,a,n,_):
  self.recent.append(a)
  if a in self.starts:self.reached[a]+=1
  if a in self.boundaries:
   self.stopped=dict(kind='original_boundary',instruction=a,operation=self.current);g.emu_stop()
 def invalid(self,g,access,a,n,v,_):
  self.stopped=dict(kind='unavailable_memory',instruction=g.reg_read(UC_X86_REG_EIP),
    operation=self.current,address=a,size=n,access=access,value=v,recent=list(self.recent));return False
 def invoke(self,operation,address,args,object_):
  self.current=operation;self.stopped=None
  result=self.call(address,args,((UC_X86_REG_ECX,object_),))
  self.completed_calls.append(dict(operation=operation,entry=address,arguments=list(args),eax=result&0xffffffff,
    al=result&255,fs_exception_head=self.u(0)))
  return result&0xffffffff

def run_image(name):
 t=AllocatorTrace(name);b=BOOT[name]
 t.current='original_boot_suffix';setup=setup_allocator(t,name)
 a=setup['arena_base'];wrapper=setup['wrapper'];obj=setup['object'];info=setup['info'];sizeout=setup['size_output']
 methods=list(struct.unpack('<14I',t.image.read(b['object_vtable'],56)))
 providers=list(struct.unpack('<4I',t.image.read(b['provider_vtable'],16)))
 rows=[];live=[]
 # The actual replication caller uses (+14)(state_size,0,0), info on failure
 # cleanup and (+00)(block,-1). Include repeated 40-byte banks and other
 # declared byte sizes without inventing physical allocation success.
 for index,size in enumerate((-1,0,1,4,39,40,40,40,41,64,127,128,1024,2048,4096,8192,65536)):
  before_pages=t.u(wrapper+12);before=bytes(t.g.mem_read(obj,0x34)).hex()
  ptr=t.invoke(f'allocate-{index}',methods[5],(size,0,0),obj)
  row=dict(requested_size=size,pointer=ptr,pages_before=before_pages,pages_after=t.u(wrapper+12),object_before_hex=before,
    object_after_hex=bytes(t.g.mem_read(obj,0x34)).hex())
  if ptr:
   assert a+4096<=ptr<a+setup['arena_size']
   t.g.mem_write(info-8,b'\x7b'*8+b'\xa5'*32+b'\x7b'*8)
   ok=t.invoke(f'info-{index}',methods[1],(ptr,info),obj)
   row.update(info_al=ok&255,info32_before_hex=(b'\xa5'*32).hex(),info32_hex=bytes(t.g.mem_read(info,32)).hex(),
     actual_size=t.u(info),payload_head_hex=bytes(t.g.mem_read(ptr,min(40,t.u(info)))).hex())
   assert ok&255 and bytes(t.g.mem_read(info-8,8))==bytes(t.g.mem_read(info+32,8))==b'\x7b'*8
   live.append(ptr)
  rows.append(row)
 # Free in reverse allocation order through the actual info and release
 # owners. Do not query stale addresses or replace an allocation failure.
 releases=[]
 for index,ptr in enumerate(reversed(live)):
  before=t.u(wrapper+12);value=t.invoke(f'release-{index}',methods[0],(ptr,0xffffffff),obj)
  releases.append(dict(pointer=ptr,returned_al=value&255,pages_before=before,pages_after=t.u(wrapper+12)))
  assert value&255
 compact=t.invoke('compact-zero',methods[10],(0,),obj)
 ptr=t.invoke('allocate-reuse40',methods[5],(40,0,0),obj)
 assert ptr
 t.g.mem_write(sizeout,b'\xa5'*4)
 zero_ptr=t.invoke('allocate-zeroed40',methods[5],(40,sizeout,1),obj)
 assert zero_ptr and bytes(t.g.mem_read(zero_ptr,t.u(sizeout)))==bytes(t.u(sizeout))
 zero=dict(pointer=zero_ptr,actual_size=t.u(sizeout),payload_hex=bytes(t.g.mem_read(zero_ptr,t.u(sizeout))).hex())
 for label,p in (('release-reuse40',ptr),('release-zeroed40',zero_ptr)):
  assert t.invoke(label,methods[0],(p,0xffffffff),obj)&255
 # Every reached instruction, including original runtime helpers, must be
 # present and byte-identical in this exact owned original image.
 digest=hashlib.sha256();owners=set();missing=[]
 ordered=sorted(t.starts)
 import bisect
 for address in sorted(t.executed):
  i=t.code.at(address);actual=bytes(t.g.mem_read(address,i.size));assert actual==bytes(i.bytes),(name,hex(address))
  digest.update(struct.pack('<I',address)+actual)
  if address in t.starts:owners.add(address)
  else:
   at=bisect.bisect_right(ordered,address)-1
   if at>=0:owners.add(ordered[at])
 return t,dict(setup=setup,allocator_vtable=methods,provider_vtable=providers,allocations=rows,releases=releases,
  compact_zero_al=compact&255,reused40_pointer=ptr,zeroed_allocation=zero,
  final_wrapper_hex=bytes(t.g.mem_read(wrapper,16)).hex(),final_object_hex=bytes(t.g.mem_read(obj,0x34)).hex(),
  final_provider_page_hex=bytes(t.g.mem_read(a,4096)).hex(),
  final_arena_sha256=hashlib.sha256(t.g.mem_read(a,setup['arena_size'])).hexdigest(),
  original_calls=t.completed_calls,observed_instructions=len(t.executed),executed_instruction_sha256=digest.hexdigest(),
  containing_owner_entries=sorted(owners),reached_entries={hex(k):v for k,v in sorted(t.reached.items())}),owners

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'complete')
 args=ap.parse_args();args.output.mkdir(parents=True,exist_ok=False)
 assert sha(ORCHESTRATION)==ORCHESTRATION_PIN
 pinned=json.loads(ORCHESTRATION.read_text());versions={};traces={};owner_sets={};listings=[]
 for name in SPECS:
  assert pinned['entries'][name]['distribution_allocation']==BOOT[name]['owner']
  try:t,row,owners=run_image(name)
  except Exception as e:
   (args.output/f'{name}-failure.json').write_text(json.dumps(dict(error=repr(e)),indent=2)+'\n');raise
  versions[name]=row;traces[name]=t;owner_sets[name]=owners
  for e in sorted(owners|{BOOT[name]['constructor']}):
   f,ins=literal(t.code,e,t.starts);p=args.output/f'{name}-owner-{e:08x}.asm'
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
   listings.append(dict(image=name,entry=e,file=p.name,sha256=sha(p),instructions=len(ins),
     calls=[dict(site=i.address,target=int(i.op_str,16))for i in ins if i.mnemonic=='call'and i.op_str.startswith('0x')],
     jump_tables=[dict(address=a,width=w,count=n,bytes_hex=t.image.read(a,w*n).hex())for a,w,n in f.tables]))
 # Literal source tables are terminated, not padded to inferred native sizes.
 tables=[]
 for name,t in traces.items():
  for index,address in enumerate(BOOT[name]['tables']):
   stride=4 if index==2 else 16;count=0
   while t.image.read(address+count*stride,4)!=bytes(4):count+=1;assert count<256
   raw=t.image.read(address,(count+1)*stride)
   tables.append(dict(image=name,index=index,address=address,stride=stride,nonzero_rows=count,bytes_hex=raw.hex(),sha256=hashlib.sha256(raw).hexdigest()))
 for index in range(3):assert len({v['bytes_hex']for v in tables if v['index']==index})==1
 # Map complete reached owners by call topology and full instruction shapes.
 pairs=[]
 for later in ('tu11','u5'):
  old=traces['retail'];new=traces[later];pending=[(BOOT['retail']['constructor'],BOOT[later]['constructor'])]
  pending+=[(versions['retail']['allocator_vtable'][slot],versions[later]['allocator_vtable'][slot])for slot in (0,1,5,10)]
  pending+=list(zip(versions['retail']['provider_vtable'],versions[later]['provider_vtable']))
  seen=set()
  while pending:
   oe,ne=pending.pop()
   if(oe,ne)in seen:continue
   seen.add((oe,ne));of,oi=literal(old.code,oe,old.starts);nf,ni=literal(new.code,ne,new.starts)
   equal=complete_tokens(oi,old.image)==complete_tokens(ni,new.image)
   topology=table_pair(of,nf,oi,ni,old.image,new.image) if equal else None
   pair=dict(later=later,retail_entry=oe,later_entry=ne,complete_shape_equal=equal,
     retail_instructions=len(oi),later_instructions=len(ni),jump_table_topology=topology)
   if equal:
    pair['relocated_operands']=references(oi,ni,old.image,new.image)
    assert topology is not None and all(v['topology_equal']for v in topology)
    oc=[int(i.op_str,16)for i in oi if i.mnemonic=='call'and i.op_str.startswith('0x')]
    nc=[int(i.op_str,16)for i in ni if i.mnemonic=='call'and i.op_str.startswith('0x')]
    assert len(oc)==len(nc);pending += [(o,n)for o,n in zip(oc,nc)if o in old.executed]
   pairs.append(pair)
 # Compare actual allocator results across images; relocated object/vtable
 # internals stay raw and are not falsely described as byte-identical.
 def behavior(row):
  return dict(allocations=[{k:v for k,v in r.items()if not k.startswith('object_')}for r in row['allocations']],
   releases=row['releases'],compact_zero_al=row['compact_zero_al'],reused40_pointer=row['reused40_pointer'],
   zeroed_allocation=row['zeroed_allocation'],pages=row['setup']['pages'])
 assert behavior(versions['retail'])==behavior(versions['tu11'])==behavior(versions['u5'])
 report=dict(schema='original-replication-allocator-source-v1',scope=__doc__,tool_sha256=sha(__file__),
  dependencies=[dict(path=str(ORCHESTRATION.relative_to(ROOT)),sha256=ORCHESTRATION_PIN)],
  images={name:dict(path=spec['path'],sha256=spec['pin'],functions_tsv=spec['tsv'],functions_sha256=sha(ROOT/spec['tsv']))for name,spec in SPECS.items()},
  versions=versions,boot=BOOT,tables=tables,complete_correspondence=pairs,listings=listings,
  summary=dict(images=3,boot_prefixes=3,whole_allocator_calls=sum(len(v['original_calls'])for v in versions.values()),
    observed_instructions=sum(v['observed_instructions']for v in versions.values()),
    complete_owner_pairs=len(pairs),equal_owner_pairs=sum(p['complete_shape_equal']for p in pairs),
    instruction_listings=len(listings)),instruction_patches=[],supplied_returns=[],
  limits=['Arena, wrapper/object backing and ABI frames are declared inputs; no physical arena allocation or operator-new owner runs.',
    'Boot prefix is bounded; complete allocator constructor and all reached allocation/info/release helpers execute unchanged.',
    'Authentic boot arg10=0 skips global shared allocator/CRT path and arg13=0 disables lock thread ownership checks.',
    'Single-thread uncontended path; no wait/semaphore, kernel allocation, packet decode, native-session or King admission evidence.'])
 (args.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print(json.dumps(report['summary']),flush=True)

if __name__=='__main__':main()
