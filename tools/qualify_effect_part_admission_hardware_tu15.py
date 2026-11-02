#!/usr/bin/env python3
"""Replay original part admission on physical i386 without callee substitution.

The first location call is a stop boundary, never a supplied iterator result.
This does not execute environment queries, sound admission or audible playback.
"""
import argparse,ctypes,hashlib,json,os,struct,subprocess
from pathlib import Path
from original_audio_cpu import OriginalAudioCpu
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_effect_part_admission_tu15 import initial,STACK,TABLE,ENTRY,ACCEPT,STOP,BANK,EXTENT


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 if a.output.exists():p.error('Preserve original physical reference')
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);ref=json.loads(a.reference.read_text());image=Xbe(NEW_DEFAULT)
 assert hashlib.sha256(image.data).hexdigest()==NEW_PIN==ref['original_xbe_sha256'];code=_Code(image,'.text')
 layout=scratch/'ptrace-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
 subprocess.run(['cc',str(layout),'-o',str(scratch/'ptrace-layout')],check=True);subprocess.run([str(scratch/'ptrace-layout')],check=True)
 host=OriginalAudioCpu(image,scratch/'original32.elf',0x27f,0x1f80);aliases=dict(eax='rax',ecx='rcx',edx='rdx',ebx='rbx',esp='rsp',ebp='rbp',esi='rsi',edi='rdi');names=tuple(aliases.values());visited=set();steps=stores=0
 try:
  host.trace(6,host.debug_offset+7*8,0)
  for row in ref['cases']:
   host.write(BANK,bytes(initial(row)));host.write(0x4f0414,struct.pack('<I',TABLE));host.prepare(ENTRY,STACK,dict(zip(names,row['gp'])))
   regs=host.registers();regs.eflags=row['eflags'];host.trace(13,0,ctypes.addressof(regs));trace=hashlib.sha256();writes=hashlib.sha256();count=written=0
   for _ in range(200):
    pc=regs.rip
    if pc in(ACCEPT,STOP):break
    ins=code.at(pc);assert ins and ins.bytes.hex()==ref['executed_original_instructions'][f'{pc:08x}']and host.read(pc,len(ins.bytes))==ins.bytes
    visited.add(pc);trace.update(struct.pack('<I',pc));count+=1
    def register(name):return 0 if not name else getattr(regs,aliases[name])&0xffffffff
    pending=[]
    for op in ins.operands:
     if op.type==3 and op.access&2:
      m=op.mem;pending.append(((register(ins.reg_name(m.base))+register(ins.reg_name(m.index))*m.scale+m.disp)&0xffffffff,op.size))
    if ins.mnemonic=='push':pending.append(((regs.rsp-4)&0xffffffff,4))
    before={address:int.from_bytes(host.read(address,size),'little')for address,size in pending}
    host.trace(9);regs=host.wait_stop()
    for address,size in pending:
     value=int.from_bytes(host.read(address,size),'little');writes.update(struct.pack('<5I',pc,address,size,before[address],value));written+=1
   else:raise RuntimeError('Original part admission instruction budget')
   assert regs.rip==row['end_pc']and (regs.rip==ACCEPT)==row['selected_for_location_iteration'],row['name']
   assert [getattr(regs,n)&0xffffffff for n in names]==row['expected_gp'],(row['name'],'GP')
   assert (regs.eflags&0xcd5)==(row['expected_eflags']&0xcd5),(row['name'],'flags')
   assert count==row['instruction_count']and trace.hexdigest()==row['instruction_sha256'],(row['name'],'original instruction order')
   assert written==row['ordered_store_count']and writes.hexdigest()==row['ordered_store_sha256'],(row['name'],'ordered original stores')
   assert hashlib.sha256(host.read(BANK,EXTENT)).hexdigest()==row['whole_declared_bank_sha256'],(row['name'],'complete declared bank')
   steps+=count;stores+=written
 finally:host.close()
 assert visited=={int(p,16)for p in ref['executed_original_instructions']}
 result=dict(passed=True,cases=len(ref['cases']),selected_iterator_boundaries=sum(r['selected_for_location_iteration']for r in ref['cases']),natural_rejections=sum(not r['selected_for_location_iteration']for r in ref['cases']),
  instruction_sites=len(visited),executed_instructions=steps,ordered_stores=stores,original_xbe_sha256=NEW_PIN,reference_sha256=hashlib.sha256(a.reference.read_bytes()).hexdigest(),
  backend='Physical Linux x86-64 child, CS23 i386 compatibility, PTRACE_SINGLESTEP',original_instruction_patches=[],substituted_callees=[],environment_voice_or_playback_result_supplied=False,
  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),retained_large_artifacts=[])
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))


if __name__=='__main__':main()
