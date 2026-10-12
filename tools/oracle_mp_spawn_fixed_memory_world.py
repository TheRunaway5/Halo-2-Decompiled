#!/usr/bin/env python3
"""Separate original fixed-memory/new-map allocation registry investigation.

Uses frozen whole compatibility producer executable without rewriting it.
No mode, capacity, backing, object or allocation result is supplied anew.
"""
import argparse,hashlib,json,subprocess,os
from pathlib import Path
from oracle_mp_spawn_complete_kpcr import ContextTrace,OUT as CONTEXT_OUT,FROZEN as OLDER,ROOT,PIN,CACHE_PIN,digest,ENTRIES as PRIOR_ENTRIES
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ESI
from functions import _trace
OUT=ROOT/'analysis/multiplayer/integration/spawn-source/fixed-memory-world'
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_complete_kpcr.py':'a29845443fe79c75572c127f6e22a6da25cc0a266bc249d073f0d94b6df0b336',
 'docs/MP_COMPLETE_COMPAT_KPCR_RESEARCH.md':'75543439d7bb17e8804a38f2d1eaffdf3b44bae715c0713ea79715f543eedb57',
 'analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/report.json':'f99ab55c6a3de3fbe48087b96d9db31e8a50e38c73b959b263b52b0bec98f8a6',
 'analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/generator.exe':'4d8de99dd61203a801e256770edeba0af50ca05068203abdb8040faa0ed3967e'}
EXTRA=(0x148860,0x148930,0x148980,0x1488b0,0x148990,0x148a70,0x22e1a0,0x22e1c0,0x22e380,0x22e400,0x22e420,0x22e490,0x22e950,0x2dd590,0x2dd660,0x1482f0,0x148360,0x148510,0x148550,0x313c90,0x37d8c0,0x37fee0,0x32cffc,0x32c7a6,0x3274e4,0x327544,0x32750c,0x327464,0x32d120,0x32d080,0x32ebdf,0x32ebcf,0x125280,0x125e50,0x1656a0,0x165700,0xb76c2,0x16cff0,0x1c89f0,0x133760,0x2d8155,0x32753e,0x32a866,0x32a86f,0x313d20,0x313e50,0x2e8090)
ENTRIES=tuple(dict.fromkeys(PRIOR_ENTRIES+EXTRA))
class FixedTrace(ContextTrace):
 def __init__(self,data):
  self.allocations=[];self.returns=[];self.alloc_stores=[];self.free_calls=[];self.scratch_stores=[];self.stage_marks=[]
  super().__init__(data,True)
 def registry(self):
  obj=self.u(0x480d9c)
  if not obj:return None
  b,n,count=(self.u(obj+x)for x in(0x18,0x1c,0x20))
  assert count<1000,count
  rows=[dict(offset=self.u(b+n-(i+1)*8),size=self.u(b+n-(i+1)*8+4))for i in range(count)]
  last=0;holes=[]
  for i,r in enumerate(rows):
   assert r['offset']>=last,(i,r,last)
   if r['offset']>last:holes.append(dict(offset=last,size=r['offset']-last,aligned16_size=r['offset']-((last+15)&~15)))
   last=r['offset']+r['size']
  limit=n-(count+1)*8
  if limit>last:holes.append(dict(offset=last,size=limit-last,aligned16_size=limit-((last+15)&~15)))
  return dict(object=f'{obj:08x}',vtable=f'{self.u(obj):08x}',backing=f'{b:08x}',capacity=n,count=count,
   registry_reserved_bytes=count*8,allocation_bytes=sum(r['size']for r in rows),rows=rows,holes=holes,
   next_registry_start_offset=limit,last_allocation_end_offset=last,
   maximum_aligned16_hole=max((r['aligned16_size']for r in holes),default=0))
 def allocation_hook(self,g,a,size,user):
  for ret,sp,row in self.returns[:]:
   if a==ret and g.reg_read(UC_X86_REG_ESP)==sp:
    row.update(returned_eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',after=self.registry())
    self.returns.remove((ret,sp,row))
  if self.instrument and a==0x22e1d0:
   sp=g.reg_read(UC_X86_REG_ESP)
   row=dict(entry=f'{a:08x}',phase=self.active,return_address=f'{self.u(sp):08x}',
    alignment=self.u(sp+4),requested_bytes=self.u(sp+8),memory_class=self.u(sp+12),before=self.registry())
   self.allocations.append(row);self.returns.append((self.u(sp),sp+16,row))
  if self.instrument and a==0x22e380:
   sp=g.reg_read(UC_X86_REG_ESP)
   row=dict(entry=f'{a:08x}',phase=self.active,return_address=f'{self.u(sp):08x}',
    ecx=f'{g.reg_read(UC_X86_REG_ECX):08x}',stack=[f'{self.u(sp+i*4):08x}'for i in range(1,5)],before=self.registry())
   self.free_calls.append(row);self.returns.append((self.u(sp),sp+8,row))
  if self.instrument and a in set(EXTRA)-set(PRIOR_ENTRIES):
   sp=g.reg_read(UC_X86_REG_ESP);row=dict(entry=f'{a:08x}',phase=self.active,registers=self.registers(),return_address=f'{self.u(sp):08x}',stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)])
   self.new_calls.append(row);self.return_watch.append((self.u(sp),row))
  super().hook(g,a,size,user)

 def mem_write(self,g,access,address,size,value,user):
  if self.instrument and 0x480db0<=address<0x480dd0:
   self.scratch_stores.append(dict(phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
    address=f'{address:08x}',size=size,before=bytes(g.mem_read(address,size)).hex(),
    value=f'{value&((1<<(size*8))-1):0{size*2}x}'))
  super().mem_write(g,access,address,size,value,user)
 def mark(self,stage):
  self.stage_marks.append(dict(stage=stage,instruction=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',
   mode=self.u(0x480d98),scratch_record_hex=bytes(self.g.mem_read(0x480db0,0x28)).hex(),
   source_scratch_backing=f'{self.u(0x480db8):08x}',source_scratch_bytes=self.u(0x480dbc),
   fixed_registry=self.registry(),world=f'{self.u(0x526234):08x}',
   fallback_backing_producer_global=f'{self.u(0x518434):08x}'))
 def game_arena(self):
  from oracle_smg_firing_1_1 import STACK,STOP
  self.call('original_physical_arena_root',0x12cb70)
  self.active='original_game_arena_except_unprovided_savegame_io';self.g.reg_write(UC_X86_REG_ESP,STACK)
  self.g.emu_start(0x125280,0x1252ba,count=2000000);assert self.g.reg_read(UC_X86_REG_EIP)==0x1252ba
  # The exact original frame is preserved across the excluded single call.
  # No success return is substituted for the unavailable I/O routine217090.
  self.g.emu_start(0x1252bf,STOP,count=2000000);assert self.g.reg_read(UC_X86_REG_EIP)==STOP
  self.producers.append(dict(name=self.active,original_fragments=[['00125280','001252ba'],['001252bf','00125339']],
   excluded_original_call=['001252ba','001252bf'],excluded_callee='00217090',
   frame_preserved_across_excluded_call=True,completion='complete_fragments_and_original_return'))
  self.call('original_BSP_datum_table_constructor',0x1f15b0)
 def object_pool(self):
  self.fragment('original_object_header_pool_bootstrap',0xb76c2,0xb76e0)
  self.producers.append(dict(name=self.active,original_fragment=['000b76c2','000b76e0'],stop_is_exclusive=True,
   completion='complete_fragment',excluded_earlier_other_object_setup_and_later_dynamic_body_heap=True))
  self.call('original_object_header_pool_activation',0x16d210,regs=((UC_X86_REG_EAX,self.u(0x4e78d0)),))
  p=self.u(0x4e78d0);assert (self.u(p+0x20),self.u(p+0x24),self.u(p+0x38),self.u(p+0x3c))==(2048,12,0,0)
  assert self.u(p+0x40,2)==0xe26f
 def pool_snapshot(self):
  p=self.u(0x4e78d0);n,stride,data,bitmap=(self.u(p+x)for x in(0x20,0x24,0x44,0x48))
  return dict(pointer=f'{p:08x}',header_hex=bytes(self.g.mem_read(p,0x4c)).hex(),capacity=n,stride=stride,
   count=self.u(p+0x3c),high_water=self.u(p+0x38),next_salt=f'{self.u(p+0x40,2):04x}',
   data=f'{data:08x}',bitmap=f'{bitmap:08x}',
   data_sha256=hashlib.sha256(bytes(self.g.mem_read(data,n*stride))).hexdigest(),
   occupied_bitmap_hex=bytes(self.g.mem_read(bitmap,((n+31)//32)*4)).hex())
 def hook(self,g,a,size,user):
  # The trace records stages without changing any machine state.
  if a in(0x1c6096,0x1c60c6,0x1c60d8,0x1c60dd):self.mark(f'original_stage_{a:08x}')
  self.allocation_hook(g,a,size,user)

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix'))
 proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env)
 args.output.parent.mkdir(parents=True,exist_ok=True)
 try:
  data=json.loads(proc.stdout.readline());(args.output.parent/'raw-host-context.json').write_text(json.dumps(data,indent=2)+'\n')
  traces=[];rows=[]
  for ready,empty,block in((False,False,0x2dd621),(True,False,0x1da942),(True,True,None)):
   t=FixedTrace(data);t.heap_prefix(None)
   if ready:
    t.call('original_CRT_lock_initializer',0x32cffc)
    t.call('original_CRT_onexit_array_initializer',0x3274e4)
   t.register()
   if ready:
    t.call('original_game_allocator_CRT_constructor_and_atexit',0x37d8c0)
    assert (t.u(0x480db8),t.u(0x480dbc))==(0x480dd0,0x4000)
    out=t.u(0x56b930);assert t.u(out)==0x37fee0 and t.u(0x56b92c)==out+4
   t.mark('prepared_CRT_state')
   t.components()
   if empty:t.game_arena();t.object_pool()
   else:t.arena()
   t.call('original_impact_pool_initializer',0x2284c0);t.memory_backing()
   object_before=t.pool_snapshot()
   t.call('original_full_new_map_order',0x1c5ff0,blocked=block)
   assert t.u(0x526234)==0x6005470
   assert t.u(0x480d98)==(2 if ready else 1)
   if ready:assert not [r for r in t.allocations if r['requested_bytes']==0x2010]
   if empty:
    assert t.u(0x509998)==t.u(0x526248) and t.u(0x50999c)==t.u(0x5099a0)==0xffffffff
    assert t.u(0x4e78d0)==0x82d4828c+8000
    assert t.u(0x480db8)==0x480dd0 and t.u(0x480dbc)==0x4000
   t.mark('retained_after_full_new_map_attempt')
   row=dict(case='complete_full_new_map_with_source_scratch_and_source_empty_object_pool'if empty else
    'full_new_map_with_source_scratch_to_inherited_unfixed_object'if ready else'full_new_map_without_source_scratch_to_fixed_allocation_failure',
    producers=t.producers,original_calls=t.new_calls,fixed_allocation_calls=t.allocations,
    original_free_calls=t.free_calls,source_scratch_record_stores=t.scratch_stores,stage_marks=t.stage_marks,
    source_object_pool_before=object_before,source_object_pool_after=t.pool_snapshot(),
    retained_registry=t.registry(),whole_new_map_routine_completed=empty,ordinary_motion_or_contact_executed=False,
    **t.context_snapshot())
   rows.append(row);traces.append(t)

 finally:
  _,err=proc.communicate('x',timeout=60);(args.output.parent/'runtime-stderr.txt').write_text(err)
 assert proc.returncode==0,err
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);listings=[]
 t=traces[0];executed=set().union(*(x.executed for x in traces))
 for a in executed:
  i=t.code.at(a);assert t.image.read(a,len(i.bytes))==i.bytes;assert all(i.bytes==bytes(x.g.mem_read(a,len(i.bytes)))for x in traces)
 executed_listing=directory/'executed-original-instructions.asm'
 executed_listing.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in(t.code.at(a)for a in sorted(executed))))
 for entry in ENTRIES:
  routine=_trace(t.code,entry,t.starts|set(ENTRIES));ins=[t.code.at(a)for a in sorted(routine.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and all(i.bytes==bytes(x.g.mem_read(i.address,len(i.bytes)))for x in traces)
  p=directory/f'{entry:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=digest(p),instructions=len(ins),executed=sum(i.address in executed for i in ins),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in routine.sites if k=='call']))
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 static_evidence=[dict(address=f'{p:08x}',bytes=n,raw_original_hex=t.image.read(p,n).hex(),sha256=hashlib.sha256(t.image.read(p,n)).hexdigest())for p,n in((0x480db0,0x20),(0x45a8fc,44),(0x45e77c,44),(0x45e7a8,44),(0x468624,4),(0x468cc4,4),(0x468948,4))]
 d=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),frozen_pins=FROZEN,
  instruction_manifest_sha256=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
  instruction_listings=listings,executed_original_listing=dict(file=executed_listing.name,sha256=digest(executed_listing),rows=len(executed)),original_static_evidence=static_evidence,executed_original_addresses_verified=len(executed),instruction_patches=[],cases=rows,
  original_startup_provenance=dict(runtime_init_table_word='00468624',runtime_init_target='0032c7a6',source_runtime_lock_initializer='0032cffc',
   C_init_table_word='00468cc4',onexit_array_initializer='003274e4',Cplusplus_init_table_word='00468948',scratch_CRT_constructor='0037d8c0',
   static_backing='00480dd0',static_backing_bytes=0x4000,static_backing_initial_sha256=hashlib.sha256(t.image.read(0x480dd0,0x4000)).hexdigest()),
  summary=dict(cases=len(rows),fixed_allocator_failure_cases=1,full_new_map_core_returns=3,full_new_map_world_insertion_returns=2,
   complete_full_new_map_routine_cases=1,ordinary_motion_or_contact_cases=0),
  boundaries=[
   'Frozen whole compatibility KPCR/ETHREAD/backed host stack producer executable is read unchanged and a new real Wine32 worker remains alive. Original process-heap/raw memory and all earlier compatibility services/exclusions persist. No new raw host allocation service is introduced.',
   'Original CRT32CFFC lock initializer, C-table3274E4 exit-array initializer, selected original eight Havok registrations and CRT37D8C0 scratch constructor/atexit execute. Earlier CRT/TLS/runtime initialization and all other C/C++ constructors remain unproved; selected table ordering is recovered from original2D8155/2D80FD and exact table words.',
   'Original37D8C0 registers literal480DD0/4000 with original2DD590 at adapter480DB0. Original327544/32750C/327464 stores destructor37FEE0 into the source heap-backed exit array. No scratch field/backing/capacity/mode or allocation success is written by the fixture.',
   'The negative case preserves missing scratch producer and reproduces2010 failure at2DD621 with46 registered fixed allocations and maximum aligned16 hole3496. The source scratch producer prevents this request through unchanged313CD7 scratch-capacity branch.',
   'Inherited-object positive case retains preceding explicit SMG/unit fixture objects; full world creation and BSP insertion return before1C6370 stops at original1DA942 on an unfixed serialized tag shape. No inherited physical-component/pose fields are supplied to bypass it.',
   'Empty-object positive case executes original125280 prologue/clear and exact1252BF tail, preserving its frame but excluding only original217090 savegame/thread IO call. It obtains the original game-arena allocator. OriginalB76C2..B76E0 then constructs and16D210 activates actual2048x12 header pool; later body-heap and prior object-setup calls are excluded. This replaces inherited preconstructed fixture membership through source initialization, not manual bitmap edits.',
   'Selected arena/BSP table/object-pool producer ordering is explicit input preparation; complete authored process/map/world object allocation order is not established. Empty source object membership is an explicit bounded case, not a claim that real Lockout has no authored world objects.',
   'Unchanged complete1C5FF0 now returns in the source-empty-pool case, including complete1C6270 then1C66A0, mode2 assignment, fallback-record construction, BSP insertion1C7320, listener3123A0 and1C6370. This proves this routine completion with stated inputs; not whole title initialization, ordinary live biped construction, Havok motion/contact or Xbox execution.',
   'Fallback allocator1482F0 record exists but backing source global518434 remains NULL in the bounded prefix. No fallback allocation is requested here. Its original133760 backing producer and complete dynamic startup ownership remain separate unresolved work.',
   'All frozen earlier files and production remain unchanged.'])
 args.output.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
 (args.output.parent/'run-native-evidence-pins.json').write_text(json.dumps({str(CONTEXT_OUT/'generator.exe'):digest(CONTEXT_OUT/'generator.exe'),'raw-host-context.json':digest(args.output.parent/'raw-host-context.json'),'/usr/bin/wine':digest('/usr/bin/wine'),'/usr/lib/wine/i386-windows/ntdll.dll':digest('/usr/lib/wine/i386-windows/ntdll.dll')},indent=2,sort_keys=True)+'\n')
 print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),cases=len(rows),manifest=d['instruction_manifest_sha256'],executed=len(executed))))
if __name__=='__main__':main()
