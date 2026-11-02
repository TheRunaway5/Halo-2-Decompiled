#!/usr/bin/env python3
"""Repeat original wet voice CPU calls on physical i386 compatibility mode.

Original instructions are single-stepped without patches. The fixture's guard
ABI boundaries and adversarial source-header inputs are replayed explicitly;
original kernels, DMA packing, playback and feedback time are not established.
"""
import argparse,ctypes,hashlib,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_sound_sdk_commit import FragmentCpu


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 if a.output.exists():p.error('Preserve existing original physical results')
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);ref=json.loads(a.reference.read_text());image=Xbe(NEW_DEFAULT)
 assert hashlib.sha256(image.data).hexdigest()==NEW_PIN==ref['original_xbe_sha256'];code=_Code(image,'DSOUND')
 canonical={int(k,16):v for k,v in ref['canonical_store_PC_map'].items()};entries={name:next(k for k,v in canonical.items()if v==base)for name,base in [('initialize',0x37cef1),('format',0x38094b),('attach',0x382be3)]}
 layout=scratch/'ptrace-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
 subprocess.run(['cc',str(layout),'-o',str(scratch/'ptrace-layout')],check=True);subprocess.run([str(scratch/'ptrace-layout')],check=True)
 host=FragmentCpu(image,scratch/'original32.elf',0x27f,0x1f80);guards={int(k):v for k,v in ref['declared_guard_ABI_boundaries'].items()};visited=set();calls=stores_count=0
 try:
  host.trace(6,host.debug_offset+7*8,0);stack,stop=0x100e000,0x100ff00;read=lambda at:int.from_bytes(host.read(at,4),'little')
  for row in ref['cases']:
   host.control_word,host.mxcsr=row['control'],row['mxcsr'];regions=[(r['address'],len(bytes.fromhex(r['hex'])))for r in row['regions']]
   for region in row['regions']:host.write(region['address'],bytes.fromhex(region['hex']))
   for phase,call in enumerate(row['calls']):
    host.write(stack-0x800,bytes([0xa5])*0x900);host.write(stack,struct.pack('<'+'I'*(1+len(call['arguments'])),stop,*call['arguments']));host.continuing=False
    entry=entries[call['kind']];inputs={k:0x56781234 for k in ('rax','rdx','rbx','rbp','rsi','rdi')};inputs['rcx']=call['self'];stores=[];boundaries=[]
    for _ in range(32):
     result,changed,executed=host.run_traced(entry,stack,code,0,0x02000000,stop_addresses=(*guards,stop),**inputs)
     inputs={};visited.update(executed)
     for pc,address,size,before,value in changed:
      if any(at<=address and address+size<=at+n for at,n in regions):stores.append([canonical[pc],address,size,before,value])
     if result.rip==stop:break
     assert result.rip in guards;boundaries.append(guards[result.rip]);result.rip=read(result.rsp);result.rsp+=4
     host.trace(13,0,ctypes.addressof(result));host.continuing=True;entry=result.rip
    else:raise RuntimeError('Original CPU boundary limit')
    assert stores==call['stores'],(row['name'],phase,'Original ordered physical stores differ')
    assert boundaries==call['guards']and result.rsp==stack+4+len(call['arguments'])*4
    if call['kind']!='attach':assert result.rax&0xffffffff==call['result']
    calls+=1;stores_count+=len(stores)
    for action in row['actions']:
     if action['after_phase']==phase:host.write(action['address'],bytes.fromhex(action['hex']))
   assert all(host.read(region['address'],len(bytes.fromhex(region['hex']))).hex()==region['hex']for region in row['final_regions'])
  assert visited==set(canonical)
 finally:host.close()
 out=dict(passed=True,cases=len(ref['cases']),whole_original_CPU_calls=calls,ordered_stores=stores_count,original_instruction_sites=len(visited),original_xbe_sha256=NEW_PIN,
  reference_sha256=hashlib.sha256(a.reference.read_bytes()).hexdigest(),backend='Physical Linux x86-64 child, CS23 i386 compatibility, PTRACE_SINGLESTEP',original_instruction_patches=[],
  guard_ABI_inputs_and_explicit_adversarial_headers_replayed=True,kernels_DMA_or_playback_executed=False,feedback_clock_verified=False,retained_large_artifacts=[])
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))


if __name__=='__main__':main()
