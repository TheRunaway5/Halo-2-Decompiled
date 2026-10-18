#!/usr/bin/env python3
"""Authenticate the ordinary TU1.5 World camera flag and producer ordering."""
import argparse,hashlib,json
from pathlib import Path
from generate_lens_query_tu15 import QueryCode
from oracle_smg_firing_1_1 import Xbe,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--directory',type=Path,required=True);args=parser.parse_args();args.directory.mkdir(parents=True,exist_ok=False)
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=QueryCode(image)
 chain=[(0x229ef0,0x22a124,0x2b5b0),(0x2b5b0,0x2b694,0x2b770),(0x2b770,0x2b94f,0x2b9f0),(0x2b9f0,0x2bbfb,0x132b0),(0x132b0,0x13315,0x12f80),(0x2b9f0,0x2bc57,0x13660),(0x13660,0x13768,0x2bcb0),(0x2bcb0,0x2bcea,0x13430)]
 caller=code.at(0x229c73);assert caller.mnemonic=='call'and caller.operands[0].imm==0x229ef0
 bodies={}
 for owner,site,target in chain:
  body=bodies.setdefault(owner,_trace(code,owner,{owner}));assert site in body.code
  i=code.at(site);assert i.mnemonic=='call'and i.operands[0].imm==target
 for owner,body in bodies.items():
  (args.directory/f'{owner:08x}.txt').write_text('\n'.join(f'{pc:08x} {code.at(pc).mnemonic} {code.at(pc).op_str}'for pc in sorted(body.code))+'\n')
 def instruction(pc,mnemonic,operand):
  i=code.at(pc);assert i.mnemonic==mnemonic and i.op_str==operand,(hex(pc),i.mnemonic,i.op_str);return dict(pc=pc,bytes=bytes(i.bytes).hex(),instruction=mnemonic+' '+operand)
 checks=[instruction(0x2b691,'push','0'),instruction(0x2b693,'push','esi'),instruction(0x2b770,'sub','esp, 0x16c'),instruction(0x2b934,'mov','edx, dword ptr [esp + 0x1b4]'),instruction(0x2b940,'push','edx'),instruction(0x2b9f4,'sub','esp, 0x298'),instruction(0x2bb18,'mov','bl, byte ptr [esp + 0x2b4]'),instruction(0x2bb6f,'mov','byte ptr [esp + 0x17], bl'),instruction(0x2bbf4,'lea','esi, [esp + 0x10]'),instruction(0x2bbf9,'rep movsd','dword ptr es:[edi], dword ptr [esi]'),instruction(0x132c3,'rep movsd','dword ptr es:[edi], dword ptr [esi]'),instruction(0x132f3,'mov','dl, byte ptr [ebp + 7]'),instruction(0x13314,'push','edx')]
 # These completed child ABIs restore the original2B770 caller stack before
 # its17-argument call. No native replacement return is used in this audit.
 cleanup={}
 for entry,expected in [(0x2b700,4),(0x14de90,0),(0x1310f0,16),(0x2f950,0),(0x156e40,0)]:
  body=_trace(code,entry,{entry});returns={int(code.at(pc).operands[0].imm)if code.at(pc).operands else 0 for pc in body.code if code.at(pc).mnemonic=='ret'};assert returns=={expected},(hex(entry),returns);cleanup[f'{entry:08x}']=expected
 pushes=[pc for pc in sorted(bodies[0x2b770].code)if 0x2b8ff<=pc<0x2b94f and code.at(pc).mnemonic=='push'];assert len(pushes)==17 and pushes[14]==0x2b940
 before_load=sum(pc<0x2b934 for pc in pushes);assert before_load==12
 source_offset=-0x16c-4*4-before_load*4+0x1b4;assert source_offset==8
 target_arg=len(pushes)-pushes.index(0x2b940);assert target_arg==3
 packet_arg_offset=-0x298-4*4+0x2b4;assert packet_arg_offset==12
 # Packet starts atESP+10; its+7 byte is copied to491E27 by132B0.
 assert 0x17-0x10==7
 report=dict(passed=True,authority='OG Xbox TU1.5',original_sha256=NEW_PIN,chain=chain,checks=checks,child_stack_cleanup=cleanup,ordinary_parent_second_argument=0,source_argument_entry_sp_offset=source_offset,target_argument_number=target_arg,target_argument_entry_sp_offset=packet_arg_offset,view_packet_scaled_byte=7,ordinary_world_scaled_projection=False,camera_publisher_precedes_view_initializer=True,ordering_call_sites=[0x2bbfb,0x2bc57],scope='Ordinary type0 view branch in2B5B0 and main2B9F0 path; foreground, alternate render types and timed-boundary execution remain separate.',game_returns_supplied=[],gpu_returns_supplied=[],full_goal_complete=False,tool_sha256=sha(Path(__file__)))
 (args.directory/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS original normal World flag0 and12F80-before13430 ordering')
if __name__=='__main__':main()
