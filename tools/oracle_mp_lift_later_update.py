#!/usr/bin/env python3
"""Execute the owned later-update scalar with explicit frozen contact inputs.

The older report is input configuration and comparison only, never a supplied
original return. All outputs are obtained independently from untouched later
instructions. The real contact/world/time producers remain unexecuted.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from research_mp_sandbox_update_functions import ROOT,NEW_DEFAULT,NEW_PIN,sha
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
OLD=ROOT/'analysis/multiplayer/sandbox-source/lift-scalar/measured/report.json'
OLD_PIN='df8b9921a0ab60bf8ee7bfb1168b365ef8cb4123cbd0984e7111d952d02e89b8'
TIME,OUT,STACK,STOP=0x1000000,0x1001000,0x200d000,0x3000000
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);args=ap.parse_args();assert sha(OLD)==OLD_PIN and sha(NEW_DEFAULT)==NEW_PIN
 mapping=ROOT/'analysis/multiplayer/sandbox-source/later-update-functions/report.json';m=json.loads(mapping.read_text());mr=next(r for r in m['routines']if r['disc_1_1_entry']=='001d5c70');assert mr['later_entry']=='001da530'and mr['normalized_instruction_and_internal_branch_topology_equal']and mr['all_paired_memory_bytes_equal']
 source=json.loads(OLD.read_text());x=Xbe(NEW_DEFAULT);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 for p in(TIME,STACK&~0xffff,STOP):g.mem_map(p,0x10000)
 def w(p,v):g.mem_write(p,struct.pack('<I',v&0xffffffff))
 w(0x51df54,TIME);executed=set();g.hook_add(UC_HOOK_CODE,lambda _g,p,_s,_u:executed.add(p));cases=[]
 for n,row in enumerate(source['cases']):
  inputs=[int(s,16)for s in row['argument_words']];assert inputs[-1]==OUT
  w(TIME+4,int(row['clock_step_bits'],16));w(STACK,STOP)
  for i,v in enumerate(inputs):w(STACK+4+4*i,v)
  w(OUT,0xcdcdcdcd)
  for r,v in((UC_X86_REG_ESP,STACK),(UC_X86_REG_EBX,row['allow_reverse_bl']),(UC_X86_REG_FPCW,0x027f),(UC_X86_REG_MXCSR,0x1f80),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):g.reg_write(r,v)
  g.emu_start(0x1da530,STOP,count=5000);assert g.reg_read(UC_X86_REG_EIP)==STOP
  result=f'{g.reg_read(UC_X86_REG_XMM0)&0xffffffff:08x}';ratio=bytes(g.mem_read(OUT,4))[::-1].hex();cleanup=g.reg_read(UC_X86_REG_ESP)-STACK
  cases.append({'case_index':n,'input_case':row,'later_original_result_xmm0_bits':result,'later_original_ratio_bits':ratio,'later_original_stack_cleanup':cleanup,'disc_1_1_comparison_equal':result==row['result_xmm0_bits']and ratio==row['ratio_bits']and cleanup==row['stack_cleanup']})
 assert all(r['disc_1_1_comparison_equal']for r in cases)
 code=_Code(x,'.text');fn=_trace(code,0x1da530,{0x1da530});instructions=[code.at(p)for p in sorted(fn.code)]
 args.output.parent.mkdir(parents=True,exist_ok=True);listing=args.output.parent/'001da530.asm';listing.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in instructions))
 for p in executed:
  if p==STOP:continue
  i=code.at(p);assert i.bytes==x.read(p,len(i.bytes))==bytes(g.mem_read(p,len(i.bytes)))
 r={'tool_sha256':sha(__file__),'owned_later_xbe_sha256':NEW_PIN,'disc_1_1_input_fixture_sha256':OLD_PIN,'source_mapping_sha256':sha(mapping),'source_entry':'001da530','original_time_pointer_slot':'0051df54','cases':cases,'executed_addresses':[f'{p:08x}'for p in sorted(executed)],'instruction_listing_sha256':sha(listing),'original_epsilon_46a180_hex':x.read(0x46a180,4).hex(),'boundaries':source['boundaries'],'scope':'512 unchanged later original scalar executions against explicitly supplied old authored rule/contact/velocity/mass/30Hz clock inputs, with full bit comparisons. No original contact activation, whole vector/Havok/player lift, source time owner or latest-ever version proof.','native_gameplay_update_applied':False}
 assert sha(OLD)==OLD_PIN;args.output.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({'cases':len(cases),'bit_equal':sum(v['disc_1_1_comparison_equal']for v in cases),'report_sha256':sha(args.output)}))
if __name__=='__main__':main()
