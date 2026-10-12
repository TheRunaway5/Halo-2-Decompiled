#!/usr/bin/env python3
"""Independently retain original sound CPU constructors and commit fragment.

The whole source caller stops at the real2205C0 request, without a return.
Its post-device CPU tail is a separately declared fragment, not an accepted
Xbox constructor or a forged device/availability result.
"""
import argparse,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_smg_audio_rng_startup_1_1 import RngTrace,AUDIO_ARENA,AUDIO_CAPACITY,ROOT
from oracle_mp_title_update_display import OLD,Xbe,_Code,sha,save,encoded

PRIOR=ROOT/'analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement'
PRIOR_PINS={'report.json':'43cf5c78f8fd6b2af8fd6e9de8c65ac249564ecda13463a340a1a333599b2075'}
DEPENDENCY=ROOT/'tools/oracle_smg_audio_rng_startup_1_1.py'
DEPENDENCY_PIN='c67ddaa11d18fb50a67eea73e213dd926b37f343dee1845d15f37ba702ef9cc6'
GLOBALS={'references':0x4ed944,'arena_base':0x517ff0,'arena_used':0x517ff4,
 'arena_crc':0x517ff8,'allocator_identity':0x46fbfc,'system':0x4ed950,
 'sources':0x4ed94c,'static_voices':0x4ed948,'controllers':0x526468,
 'controller_root':0x52646c,'looping':0x5098e8,'effects':0x526470,'shared_voices':0x5098ec}
PHASES={0x126b20:'entry',0x126b88:'before_cache',0x126b8d:'after_cache',
 0x126c52:'before_controllers',0x126c57:'after_controllers',0x126c64:'after_looping',
 0x126c71:'after_effects',0x126c83:'after_shared_voices',0x2205c0:'platform_request'}

class StartupTrace(RngTrace):
 def __init__(self,data,replay):
  self.startup_active=False;self.startup_stages=[];self.startup_stores=[];self.startup_executed=set()
  super().__init__(data,replay)
 def snapshot(self):
  values={name:self.u(va)for name,va in GLOBALS.items()}
  arena=bytes(self.g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY))
  return dict(globals={name:f'{v:08x}'for name,v in values.items()},arena_hex=arena.hex())
 def hook(self,g,address,size,user):
  if address==0x126b20:self.startup_active=True
  if self.startup_active:
   if self.code.at(address):self.startup_executed.add(address)
   if address in PHASES:self.startup_stages.append(dict(phase=PHASES[address],instruction=f'{address:08x}',state=self.snapshot()))
  super().hook(g,address,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.startup_active and (AUDIO_ARENA<=address<AUDIO_ARENA+AUDIO_CAPACITY or address in GLOBALS.values()):
   self.startup_stores.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',bytes=size,
       before=f'{self.u(address,size):0{size*2}x}',after=f'{value&((1<<(size*8))-1):0{size*2}x}'))
  super().mem_write(g,access,address,size,value,user)

def tail(prefix,out):
 x=Xbe(OLD);code=_Code(x,'.text');g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for sec in x.sections:g.mem_write(sec.va,x.section_bytes(sec))
 g.mem_map(AUDIO_ARENA,AUDIO_CAPACITY);g.mem_write(AUDIO_ARENA,bytes.fromhex(prefix['arena_hex']))
 for name,va in GLOBALS.items():g.mem_write(va,struct.pack('<I',int(prefix['globals'][name],16)))
 stack=0x200e000;g.mem_map(0x2000000,0x10000);g.reg_write(UC_X86_REG_ESP,stack)
 g.reg_write(UC_X86_REG_EBX,0);g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
 executed=set();stores=[]
 def ins(_g,a,n,u):executed.add(a)
 def store(_g,k,a,n,v,u):
  if AUDIO_ARENA<=a<AUDIO_ARENA+AUDIO_CAPACITY or a in GLOBALS.values():
   stores.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',bytes=n,
       before=f'{int.from_bytes(bytes(g.mem_read(a,n)),"little"):0{n*2}x}',after=f'{v&((1<<(n*8))-1):0{n*2}x}'))
 g.hook_add(UC_HOOK_CODE,ins);g.hook_add(UC_HOOK_MEM_WRITE,store)
 g.emu_start(0x126c9d,0x126d39,count=100000);assert g.reg_read(UC_X86_REG_EIP)==0x126d39
 output=dict(globals={name:f'{int.from_bytes(bytes(g.mem_read(va,4)),"little"):08x}'for name,va in GLOBALS.items()},
       arena_hex=bytes(g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY)).hex())
 rows=[code.at(a)for a in sorted(executed)]
 for z in rows:assert z.bytes==x.read(z.address,len(z.bytes))==bytes(g.mem_read(z.address,len(z.bytes)))
 path=out/'instructions/original-commit-fragment.asm'
 save(path,''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in rows).encode())
 return dict(entry='00126c9d',end_exclusive='00126d39',declared_ebx='00000000',input=prefix,output=output,stores=stores,
     platform_constructor_excluded_no_return_supplied=True,whole_original_available_startup_not_claimed=True)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 for name,pin in PRIOR_PINS.items():assert sha(PRIOR/name)==pin
 assert sha(DEPENDENCY)==DEPENDENCY_PIN
 prior=json.loads((PRIOR/'report.json').read_text());context=json.loads((PRIOR/'raw-host-context.json').read_text())
 assert sha(PRIOR/'raw-host-context.json')==prior['context_snapshot_sha256']
 assert sha(OLD)==prior['source_sha256']
 t=StartupTrace(context,prior['clock_calls']);steps=t.execute()
 assert t.u(t.u(0x4ed950)+0x78,1)==0 and t.u(0x4ed944)==0
 prefix=t.snapshot();fragment=tail(prefix,a.output)
 ins=[t.code.at(va)for va in sorted(t.startup_executed)]
 for z in ins:assert z.bytes==t.image.read(z.address,len(z.bytes))==bytes(t.g.mem_read(z.address,len(z.bytes)))
 save(a.output/'instructions/original-constructor-prefix.asm',''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in ins).encode())
 report=dict(schema='original-sound-startup-cpu-owners-v1',oracle_sha256=sha(__file__),source_sha256=sha(OLD),
   original_rng_constructor_tool_sha256=DEPENDENCY_PIN,
   prior_report_sha256=sha(PRIOR/'report.json'),recorded_context_sha256=sha(PRIOR/'raw-host-context.json'),
   initial_static_arena_protocol=dict(alias=f'{AUDIO_ARENA:08x}',bytes=AUDIO_CAPACITY,used=0,
       scope='Same independently declared raw static arena protocol as the prior source cache/RNG study; normal title reservation owner/allocation order not promoted.'),
   stages=t.startup_stages,stores=t.startup_stores,clock_calls=t.clock_calls,
   controller_calls=[row for row in t.rng_calls if row['entry']in('0021bc50','0032803e','00328505','00337a40')],
   stopped_original_caller=prefix,commit_fragment=fragment,
   bounds=['Full126B20 executes genuine cache/pool/controller/loop/effect/shared-voice constructors and stops at actual2205C0 request without a supplied result.',
       'Whole prior recorded compatibility context and real clock inputs replayed; this is not another live Wine/Xbox thread observation.',
       'The separate126C9D..126D39 original CPU commit fragment consumes the actual retained prefix; its source+78 writer is not successful device initialization or accepted original submit.',
       'No readiness, listener, selected permutation, capability, device or callback result is supplied to the whole caller.'],
   instruction_patches=[],new_platform_success_returns=[],native_audio_connected=False)
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),stages=len(t.startup_stages),constructor_stores=len(t.startup_stores),commit_stores=len(fragment['stores']))))

if __name__=='__main__':main()
