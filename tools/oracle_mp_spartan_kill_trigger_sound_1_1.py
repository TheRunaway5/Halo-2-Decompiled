#!/usr/bin/env python3
"""Selected source sound storage/current-ugh continuation of held kill fatal call.

The frozen diagnostic kill admission is first reproduced exactly. Source storage
constructors then run on a separate external caller stack. The held CPU context
and original stack are retained. No completed map/audio/device success is added.
"""
import argparse,copy,ctypes,json,os,struct,subprocess
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_spartan_kill_trigger_1_1 as frozen
from oracle_smg_firing_1_1 import ROOT,STOP,PIN,CACHE_PIN
from oracle_mp_spawn_biped_physics import PLACEMENT
from oracle_mp_spawn_process_heap_world import KernelBoundary
from functions import _trace
prior=frozen.prior
BASE=ROOT/'analysis/multiplayer/integration/spartan-kill-trigger-20261010'
PREFIX_PIN='e43e73718e3a327842ad1b3e30dfb3101bafd59492e71997fc1a76861b0dcb5d'
NEW={0x20e8b0,0x20e920,0x20e960,0x126de0,0x20ef90,0x21b540,0x13fb30,0x33785c,0x1656a0}
CALLER_STACK=0xc000000
class SoundTrace(frozen.KillTrace):
 def __init__(self,data):
  self.sound_extension=False;self.raw_providers=[];self.host_backings=[]
  self.libc=ctypes.CDLL(None,use_errno=True)
  self.libc.mmap.restype=ctypes.c_void_p
  self.libc.mmap.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_long]
  super().__init__(data)
  self.g.mem_map(CALLER_STACK,0x20000)
 def physical_snapshot(self,backing=False):
  r=super().physical_snapshot(backing)
  if self.sound_extension:
   vals={f'{p:08x}':f'{self.u(p):08x}'for p in(0x4ed950,0x526464,0x52627c,0x526280,0x526284,0x4ed650,0x4ed654,0x4ed65c)}
   r['selected_sound_storage']=dict(globals=vals,bit_bank1880_hex=bytes(self.g.mem_read(self.u(0x52627c),0x1880)).hex()if self.u(0x52627c)else None,
    arena_slice1880_hex=bytes(self.g.mem_read(self.u(0x526280),0x1880)).hex()if self.u(0x526280)else None)
  return r
 def hook(self,g,a,n,user):
  if self.sound_extension and a==0x33785c:
   self.death_executed.add(a);self.returns_at(a)
   sp=g.reg_read(reg.UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(4)]
   assert args==[0,0x1880,0x101000,4],args
   cap=0x2000;ctypes.set_errno(0);p=self.libc.mmap(None,cap,3,0x22,-1,0);err=ctypes.get_errno()
   assert p not in(None,ctypes.c_void_p(-1).value),(p,err)
   alias=0xc100000;assert all(alias+cap<=lo or alias>hi for lo,hi,_ in g.mem_regions())
   g.mem_map_ptr(alias,cap,3,p);self.host_backings.append(dict(actual_host_address=f'{p:x}',capacity=cap))
   self.raw_providers.append(dict(phase=self.active,entry=f'{a:08x}',return_site=f'{self.u(sp):08x}',arguments=[f'{v:08x}'for v in args],
    returned_alias=f'{alias:08x}',requested_bytes=args[1],rounded_bytes=cap,actual_provider='libc mmap anonymous private PROT_READ|PROT_WRITE',
    actual_errno=err,original_VirtualAlloc_implementation_executed=False,scope='Actual raw OS memory only; source constructor owns every sound field. No retail Xbox address/status claim.'))
   g.reg_write(reg.UC_X86_REG_EAX,alias);g.reg_write(reg.UC_X86_REG_ESP,sp+20);g.reg_write(reg.UC_X86_REG_EIP,self.u(sp));return
  super().hook(g,a,n,user)
 def death_read(self,g,access,address,size,value,user):
  before=len(self.death_reads);super().death_read(g,access,address,size,value,user)
  if self.sound_extension and len(self.death_reads)>before:self.death_reads[-1]['phase']=self.active
 def selected(self,name,entry):
  self.active=name;self.new_invalid=None;before=self.physical_snapshot(True);start=len(self.events)
  pending=self.death_pending;self.death_pending=[]
  sp=CALLER_STACK+0x1f000;self.g.mem_write(sp,struct.pack('<I',STOP));self.g.reg_write(reg.UC_X86_REG_ESP,sp)
  try:
   self.g.emu_start(entry,STOP,count=1000000);assert self.g.reg_read(reg.UC_X86_REG_EIP)==STOP
   self.returns_at(STOP);complete=True
  except(unicorn.UcError,KernelBoundary):assert self.new_invalid is not None;complete=False
  self.death_stages.append(dict(name=name,entry=f'{entry:08x}',arguments=[],input_registers={},before=before,after=self.physical_snapshot(True),completed=complete,
   stopped_eip=f'{self.g.reg_read(reg.UC_X86_REG_EIP):08x}',invalid=self.new_invalid,used_services=self.events[start:],selected_external_caller_stack=f'{sp:08x}'))
  print(json.dumps(dict(stage=name,completed=complete,invalid=self.new_invalid)),flush=True)
  self.death_pending=pending
  return complete

def prepare(data):
 prior.motion.CommandPoseTrace=SoundTrace;t,correlation=prior.motion.prepare(data)
 for lo,hi,permissions,raw in t.source_fixture:t.g.mem_write(lo,raw)
 t.g.context_restore(t.source_fixture_cpu)
 movement=prior.motion.probe(t,'authored_heading_forward',0x3f800000,0,True)
 old=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/command-locomotion-pose/measured/report.json').read_text())
 want=next(c for c in old['cases']if c['case']=='authored_heading_forward')
 for left,right in zip(want['stages'],movement['stages']):
  for key in('before_state','after_state'):
   assert {k:v for k,v in old['states'][left[key]].items()if k!='id'}=={k:v for k,v in t.states[right[key]].items()if k!='id'}
 t.selected_physics_fixup(0xe9f2086d);t.death_enabled=True;prior.ENTRIES.update(t.starts|frozen.ENTRIES)
 for name,e in(('original_effect_storage_constructor',0x1775d0),('original_effect_map_reset',0x177690)):assert t.phase(name,e)
 s=t.u(0x4e7920);assert t.u(s+0x230)==1;index=t.u(t.u(s+0x234),2);v=t.u(s+0x10c)+index*0x44
 raw=bytes(t.g.mem_read(v,0x44));assert int.from_bytes(raw[4:6],'little')==0xffff
 o=struct.unpack_from('<3f',raw,0x24);extent=struct.unpack_from('<3f',raw,0x30);f=struct.unpack_from('<3f',raw,0xc);u=struct.unpack_from('<3f',raw,0x18)
 right=(u[1]*f[2]-u[2]*f[1],u[2]*f[0]-u[0]*f[2],u[0]*f[1]-u[1]*f[0])
 p=PLACEMENT+0x1000;t.g.mem_write(p,struct.pack('<3f',*(o[i]+.5*(f[i]*extent[0]+right[i]*extent[1]+u[i]*extent[2])for i in range(3))))
 stages=[('actual_moving_root_kill_check',0x1efef0,(),((reg.UC_X86_REG_EDI,t.spawned_unit),)),
  ('explicit_authored_interior_point_check',0x11da40,(),((reg.UC_X86_REG_EAX,index),(reg.UC_X86_REG_EDI,p))),
  ('explicit_original_diagnostic_position_delivery',0xb8490,(t.spawned_unit,0,0),((reg.UC_X86_REG_EAX,p),(reg.UC_X86_REG_ECX,0),(reg.UC_X86_REG_EDX,0))),
  ('actual_retained_root_kill_check_after_position_delivery',0x1efef0,(),((reg.UC_X86_REG_EDI,t.spawned_unit),)),
  ('explicit_original_current_biped_motion_owner',0xdefc0,(t.spawned_unit,),())]
 for name,e,args,regs in stages:
  if not t.phase(name,e,args,regs):break
 assert prior.digest(BASE/'probe/report.json')==PREFIX_PIN
 old=json.loads((BASE/'probe/report.json').read_text())
 for key,current in(('stages',t.death_stages),('calls',t.death_calls),('reads',t.death_reads),('stores',t.death_stores),('events',t.events)):
  assert old[key]==current,('frozen_prefix',key)
 assert old['final']==t.physical_snapshot(True)
 return t,correlation

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();frozen.ctor.verify();a.output.parent.mkdir(parents=True,exist_ok=True)
 err=(a.output.parent/'runtime-stderr.txt').open('w');proc=subprocess.Popen(['wine',str(prior.player.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(prior.player.CONTEXT_OUT/'wine-prefix')));data=json.loads(proc.stdout.readline())
 try:
  t,correlation=prepare(data);print(json.dumps(dict(entire_frozen_kill_prefix_byte_identical=True)),flush=True)
  prefix=dict(stages=len(t.death_stages),calls=len(t.death_calls),reads=len(t.death_reads),stores=len(t.death_stores),events=len(t.events),order=t.death_order)
  held=t.g.context_save();held_regs=t.registers();held_sp=t.g.reg_read(reg.UC_X86_REG_ESP)
  stack_region=next((lo,hi)for lo,hi,_ in t.g.mem_regions()if lo<=held_sp<=hi);stack=bytes(t.g.mem_read(stack_region[0],stack_region[1]-stack_region[0]+1))
  t.sound_extension=True;prior.ENTRIES.update(NEW);initial=t.physical_snapshot(True)
  assert t.selected('selected_original_sound_bit_storage_constructor',0x20e8b0)
  assert t.selected('selected_original_sound_bit_storage_reset',0x20e960)
  assert not t.selected('selected_original_current_ugh_map_prefix_cold_system',0x126de0)
  assert t.new_invalid['instruction']=='00126e22'and t.u(0x4ed950)==0 and t.u(0x526464)!=0
  assert bytes(t.g.mem_read(stack_region[0],len(stack)))==stack
  t.g.context_restore(held);assert t.registers()==held_regs
  t.active='resume_held_original_kill_fatal_after_selected_storage';t.new_invalid=None;before=t.physical_snapshot(True);events=len(t.events)
  try:
   t.g.emu_start(t.g.reg_read(reg.UC_X86_REG_EIP),STOP,count=1000000);assert t.g.reg_read(reg.UC_X86_REG_EIP)==STOP;t.returns_at(STOP);complete=True
  except(unicorn.UcError,KernelBoundary):assert t.new_invalid is not None;complete=False
  t.death_stages.append(dict(name=t.active,entry=held_regs['eip']if'eip'in held_regs else'0020efc2',arguments=[],input_registers=held_regs,before=before,after=t.physical_snapshot(True),completed=complete,
   stopped_eip=f'{t.g.reg_read(reg.UC_X86_REG_EIP):08x}',invalid=t.new_invalid,used_services=t.events[events:]))
  print(json.dumps(dict(resumed_fatal_complete=complete,stop=t.new_invalid)),flush=True)
  assert not complete and t.new_invalid['instruction']=='0020efc2'
  # The faulting instruction consumes ECX loaded before the missing owner was
  # initialized. Retry only its preceding original global load in this frame.
  assert t.image.read(0x20efbc,6)==bytes.fromhex('8b0d64645200')
  t.active='retry_held_frame_original_current_ugh_load';t.new_invalid=None;before=t.physical_snapshot(True);events=len(t.events)
  try:
   t.g.emu_start(0x20efbc,STOP,count=1000000);assert t.g.reg_read(reg.UC_X86_REG_EIP)==STOP;t.returns_at(STOP);complete=True
  except(unicorn.UcError,KernelBoundary):assert t.new_invalid is not None;complete=False
  t.death_stages.append(dict(name=t.active,entry='0020efbc',arguments=[],input_registers={},before=before,after=t.physical_snapshot(True),completed=complete,
   stopped_eip=f'{t.g.reg_read(reg.UC_X86_REG_EIP):08x}',invalid=t.new_invalid,used_services=t.events[events:],
   scope='Re-execute only preceding unchanged global load in retained frame; no gameplay field or service result assigned.'))
  print(json.dumps(dict(original_global_reload_fatal_complete=complete,stop=t.new_invalid)),flush=True)
  final=t.physical_snapshot(True)
 finally:
  proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60);proc.stdout.close();err.close();assert proc.returncode==0
 prior.write(a.output.parent/'raw-host-context.json',prior.encoded(data));prior.write(a.output.parent/'raw-host-allocations.json',prior.encoded(t.host_backings));listings=[]
 roots={int(c['entry'],16)for c in t.death_calls}|NEW|frozen.ENTRIES
 for e in sorted(roots):
  f=_trace(t.code,e,t.starts|roots);rows=[t.code.at(p)for p in sorted(f.code)]
  for r in rows:assert r.bytes==t.image.read(r.address,len(r.bytes))==bytes(t.g.mem_read(r.address,len(r.bytes)))
  path=a.output.parent/'instructions'/f'{e:08x}.asm';prior.write(path,''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in rows).encode())
  listings.append(dict(entry=f'{e:08x}',file=str(path.relative_to(a.output.parent)),sha256=prior.digest(path),rows=len(rows)))
 rows=[t.code.at(p)for p in sorted(t.death_executed)]
 for r in rows:assert r.bytes==t.image.read(r.address,len(r.bytes))==bytes(t.g.mem_read(r.address,len(r.bytes)))
 path=a.output.parent/'instructions/executed-kill-sound.asm';prior.write(path,''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in rows).encode())
 listings.append(dict(kind='all_execution',file=str(path.relative_to(a.output.parent)),sha256=prior.digest(path),rows=len(rows)))
 report=dict(schema=1,oracle_sha256=prior.digest(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,frozen_prefix_sha256=PREFIX_PIN,entire_frozen_prefix_byte_identical=True,prefix_counts=prefix,
  prior_pins=frozen.ctor.PINS,prior_runtime_correlation=correlation,initial=initial,final=final,stages=t.death_stages,calls=t.death_calls,reads=t.death_reads,stores=t.death_stores,events=t.events,
  raw_os_providers=t.raw_providers,held_registers=held_regs,held_stack_pointer=f'{held_sp:08x}',held_stack_region=[f'{v:08x}'for v in stack_region],held_stack_sha256=__import__('hashlib').sha256(stack).hexdigest(),
  original_stack_unchanged_during_selected_roots=True,instructions=listings,instruction_patches=[],new_supplied_gameplay_or_audio_success=[],boundaries=[
   'Selected constructors/reset/current-ugh prefix are separate roots, not naturally reached map startup. Callback44 remains stopped before reset45 in natural ascending map order.',
   '20E8B0 uses actual pre-existing source125280 game arena/cursor/CRC. Exact raw1880 request uses actual mmap backing; no device or sound record is supplied.',
   'Cold126DE0 publishes actual current-ugh then stops on absent source126B20 sound system. Its side effects are retained as an explicitly selected prefix; no completed callback or successful map activation is claimed.',
   'Held original CPU and stack are preserved while selected roots run on external caller stack. Direct fault retry retains cachedNULL negative; the next explicit resume re-executes only preceding20EFBC original global load in that frame, without re-running admission or patching its return.',
   'Inherited diagnostic placement and command/clock/platform/service boundaries remain those of the immutable kill-trigger freeze; this is not natural falling, complete title startup, Xbox hardware or live ragdoll proof.'])
 frozen.ctor.verify();prior.write(a.output,prior.encoded(report));print(json.dumps(dict(report=str(a.output),sha256=prior.digest(a.output),calls=len(t.death_calls),stages=len(t.death_stages))),flush=True)
if __name__=='__main__':main()
