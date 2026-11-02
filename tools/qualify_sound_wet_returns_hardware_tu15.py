#!/usr/bin/env python3
"""Repeat wet-return CPU boundaries on physical Linux i386 compatibility mode.

Single-step the unchanged original fragment in an owned child, stopping before
declared SDK boundaries. The same explicit external ABI controls are replayed;
physical audio, the SDK callees and whole startup remain outside this check.
"""
import argparse,ctypes,hashlib,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_sound_sdk_commit import FragmentCpu


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);reference=json.loads(a.reference.read_text())
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN==reference['original_xbe_sha256'];code=_Code(image,'.text')
 layout=scratch/'ptrace-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
 subprocess.run(['cc',str(layout),'-o',str(scratch/'ptrace-layout')],check=True);subprocess.run([str(scratch/'ptrace-layout')],check=True)
 host=FragmentCpu(image,scratch/'original32.elf',0x27f,0x1f80);visited=set();calls=0
 try:
  host.trace(6,host.debug_offset+7*8,0);bank=reference['bank'];drivers=reference['drivers'];stack=bank+0xe000
  write=lambda address,value:host.write(address,struct.pack('<I',value&0xffffffff))
  read=lambda address:struct.unpack('<I',host.read(address,4))[0]
  boundaries={int(k,16):(v[0],v[1])for k,v in reference['declared_sdk_abi_boundaries'].items()}
  for row in reference['cases']:
   for i,d in enumerate(drivers):host.write(d,bytes([0x5b+i])*0x2ad8);write(d+0x2ab0,bank+0x18000+i*0x100)
   write(reference['global_driver'],drivers[0]);host.write(stack-0x100,bytes([0xa5])*0x200);host.continuing=False
   entry=reference['fragment_entry'];ordinal=0
   for _ in range(64):
    result,stores,executed=host.run_traced(entry,stack,code,bank,0x10000,stop_addresses=(*boundaries,reference['fragment_end']))
    visited.update(executed);pc=result.rip
    if pc==reference['fragment_end']:break
    kind,count=boundaries[pc];expected=row['calls'][ordinal];ordinal+=1;calls+=1;assert kind==expected['kind']
    args=[read(result.rsp+4+4*i)for i in range(count)]
    if kind=='create':
     assert [args[i]for i in(0,2,3)]==[expected['args'][i]for i in(0,2,3)]
     assert list(struct.unpack('<6I',host.read(args[1],24)))==expected['descriptor']
    elif kind=='bins':
     assert args[0]==expected['args'][0] and read(args[1])==expected['count']
     assert list(struct.unpack('<'+'I'*(2*expected['count']),host.read(read(args[1]+4),8*expected['count'])))==expected['pairs']
    else:assert args==expected['args']
    for address,value in expected['actions']:write(address,value)
    result.rax=expected.get('result',0);result.rip=read(result.rsp);result.rsp+=4+count*4
    host.trace(13,0,ctypes.addressof(result));host.continuing=True;entry=result.rip
   else:raise RuntimeError('Original fragment boundary limit')
   assert ordinal==len(row['calls']) and result.rsp==stack and read(reference['global_driver'])==row['final_driver']
   assert [[read(d+0x2a70+j*4)for j in range(8)]for d in drivers]==row['final_slots']
  assert visited=={int(k,16)for k in reference['executed_original_instructions']}
 finally:host.close()
 out=dict(passed=True,cases=len(reference['cases']),external_abi_calls=calls,original_instruction_sites=len(visited),original_xbe_sha256=NEW_PIN,
  reference_sha256=hashlib.sha256(a.reference.read_bytes()).hexdigest(),backend='Physical x86-64 Linux child, CS23 i386 compatibility, PTRACE_SINGLESTEP',
  original_instruction_patches=[],declared_external_ABI_inputs_replayed=True,SDK_callees_or_audio_device_executed=False,whole_startup_or_frame_clock_verified=False,
  retained_large_artifacts=[])
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))


if __name__=='__main__':main()
