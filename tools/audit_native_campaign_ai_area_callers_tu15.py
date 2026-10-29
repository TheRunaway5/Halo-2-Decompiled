#!/usr/bin/env python3
"""Authenticate area candidate registry and actual low16 caller continuations."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);a=p.parse_args();assert 'HALO2_TEST_SCRATCH'in os.environ;image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text')
 def listing(begin,end):
  out=[];pc=begin
  while pc<end:
   i=code.at(pc);assert i and pc+i.size<=end;out.append(dict(instruction=pc,bytes=i.bytes.hex(),operation=i.mnemonic,operands=i.op_str));pc+=i.size
  assert pc==end;return out
 references=[]
 for sec in image.sections:
  raw=bytes(image.section_bytes(sec));offset=0
  while(offset:=raw.find(struct.pack('<I',0x1b6f20),offset))>=0:references.append(dict(section=sec.name,address=sec.va+offset));offset+=1
 assert references==[dict(section='.data',address=0x48a5e4)],references
 assert int.from_bytes(image.read(0x48a5d4+0x10,4),'little')==0x1b6f20
 assert code.at(0x1b0128).op_str=='dword ptr [edi + 0x10]';assert code.at(0x1b012b).op_str=='ax, ax';assert code.at(0x1b0130).op_str=='ax, 0x83';assert code.at(0x1b013c).op_str=='edx, ax';assert code.at(0x1b0178).op_str=='word ptr [edx], ax';assert code.at(0x1b017f).op_str=='word ptr [eax], 3'
 # The parent has a void-style output-parameter contract. After all candidate
 # continuations, it reads only newly definedAX; every fullEAX use defines it.
 assert code.at(0x1b01a9).op_str=='ax, word ptr [ebp + 4]';assert code.at(0x1b01be).op_str=='ecx, ax';assert code.at(0x1b01c1).op_str=='eax, dword ptr [edx + 8]'
 fixed={}
 for entry in(0x1b6f20,0x1c5870,0x20d180,0x20d2d0):
  body=complete_body(code,entry);fixed[hex(entry)]=dict(sites=len(body.code),calls=[hex(x)for x in sorted(body.calls)],listing_sha256=hashlib.sha256(''.join(f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(body.code)).encode()).hexdigest())
 report=dict(passed=True,original_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),literal_entry_references=references,callback_record=dict(address=0x48a5d4,slot=0x10),candidate_consumer=listing(0x1b0122,0x1b0186),common_continuation=listing(0x1b01a9,0x1b01d2),query_caller=listing(0x1b6f20,0x1b6f6b),fixed_profile=listing(0x1c5896,0x1c58b7),owners=fixed,
  owed_contract='1B6F20 returns candidate behavior ID inAX16 and consumesRET8;1C5870 typed query returns0/1 low16 and usesflags5/mode(actor26C!=NONE). No score3 is returned: caller writes score3 only after candidate validation.',
  liveness={'negative_or_large':'TESTAX at1B012B, CMPAX83 reject values, then1B01A9 overwritesAX. ReturnedupperEAX is never consumed.',
   'accepted':'MOVSXEDX,AX at1B013C definesfullEDX fromowedlow16; ECX isdefinedat1B013F; acceptedID writesAX at1B0178, thenEAX isfullydefined1B017B before writing score3.',
   'common':'1B01A9 readsnewAX from[EBP+4]; laterAXtests precede MOVSXECX,AX. FullEDX defined1B01B8 and fullEAX defined1B01C1 before fullwidth use. Earlyreturn1B01CB makes nofullwidthreturnedEAX use inthis void-style output-parameter owner.',
   'flags':'1B012B TESTAX defines flags before any conditional continuation; returnedflags aredead.',
   'SIMD_MMX_FP':'Original four-owner closure has only integer/pointer/control instructions; caller activeMMX, live x87 and SIMD/policy survive exactly. Native transport preserves all provided state.'},
  boundaries=['Ordinary authenticated registry callback contract; arbitrary computed/hidden register callers are not qualified','Fixedflags5 and mode0|1 derived from outer1C5870, not general20D180/mode2 API qualification'],retained_large_artifacts=[])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print('Authenticated candidateAX16, negative/positive/common caller paths, flags5/mode0|1')
if __name__=='__main__':main()
