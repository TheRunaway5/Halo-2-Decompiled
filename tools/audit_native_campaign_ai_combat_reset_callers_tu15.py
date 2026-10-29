#!/usr/bin/env python3
"""Authenticate the ordinary1EC7B0 caller contract beyond private scratch state."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);a=p.parse_args();assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir();image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');text=next(s for s in image.sections if s.name=='.text');raw=image.section_bytes(text);references=[]
 for i in range(len(raw)-4):
  if raw[i] in [0xe8,0xe9] and (text.va+i+5+struct.unpack_from('<i',raw,i+1)[0])&0xffffffff==0x1ec7b0:
   ins=code.at(text.va+i);assert ins.mnemonic=='call' and ins.op_str=='0x1ec7b0';references.append(text.va+i)
 assert references==[0x1ec890,0x2652e1]
 absolute=[]
 for section in image.sections:
  buf=image.section_bytes(section);needle=struct.pack('<I',0x1ec7b0);offset=buf.find(needle)
  while offset>=0:absolute.append(dict(section=section.name,address=hex(section.va+offset)));offset=buf.find(needle,offset+1)
 assert not absolute
 def listing(start,stop):
  out=[];pc=start
  while pc<stop:
   i=code.at(pc);out.append(dict(instruction=hex(pc),bytes=bytes(i.bytes).hex(),operation=i.mnemonic,operands=i.op_str));pc+=i.size
  assert pc==stop;return out
 listings={'1EC820_continuation':listing(0x1ec895,0x1ec8f7),'2652E1_continuation':listing(0x2652e6,0x265329),'1EB590_scratch_initialization':listing(0x1eb590,0x1eb5b0),'1B0E10_scratch_initialization':listing(0x1b0e10,0x1b0e40)}
 next_body=_trace(code,0x1b0e10,{0x1b0e10});next_fp=[hex(pc) for pc in sorted(next_body.code) if 'xmm' in code.at(pc).op_str or code.at(pc).mnemonic.startswith('f')];assert not next_fp
 result=dict(passed=True,original_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),relative_calls_and_tails=[hex(p) for p in references],absolute_entry_references=absolute,ordinary_contract='Void1EC7B0 takes actual EAX datum and signedDI16; RET0. Callee-savedGP,caller stack,FP policy/x87/live state are preserved. EAX/ECX/EDX/EFLAGS and mode0/1 privateXMM0 are not public outputs.',continuations=listings,listing_sha256={k:hashlib.sha256(json.dumps(v,separators=(',',':')).encode()).hexdigest() for k,v in listings.items()},liveness={'1EC890_EAX':'1EC895 overwritesAL before its byte-only store;1EC89F overwritesfullEAX before PUSH/use.1EC8A4 then supplies actorEAX to1EB590. No returned upperEAX is consumed.','1EC890_ECX_EDX_flags':'No ECX/EDX read before1EB590. Its entryMOVECX1EB590 andMOVEDX1EB596 define both;ANDEAX at1EB599 defines flags before any conditional branch. Callee input is EAX+stack, not old scratch.','2652E1_EAX_ECX_EDX':'2652E6 replacesEAX with clock pointer;2652EB replacesECX from clockfrequency before CVTSI2SS;26530F definesEDX from the newly converted stack integer before PUSH. Old return packet values do not flow to a persistent store.','2652E1_flags':'No conditional branch reads incoming flags before1B0E10. Its ANDECX at1B0E16 and subsequent integer initialization define flags before firstJcc1B0E56.','XMM0':'Only267290 writes privateXMM0 viaXORPS in modes0/1.1EC820 continuation does not readXMM0 or return anFP result; reached1EB590 memoryMOVSS definesXMM0 before its use.2652EF CVTSI2SS defineslow32 before MULSS/MOVSS; preserved upper96 are never consumed. The next full1B0E10 owner contains no x87/SIMD operands.','FP_status':'The five-owner combat closure has no FP arithmetic. XORPS does not changeMXCSR/x87 policy/sticky status; other SIMD/MMX/x87 state remains caller-owned. Native transport preserves it.'},boundaries=['Authenticated original direct-reference inventory and ordinary void contract; arbitrary computed/hidden-register callers are not claimed.','1EC820 unit-NONE path returns no publicFP result; dead scratchXMM0 is not reconstructed.','Actual campaign passage remains a separate coherent runtime qualification.'],production_binary_execution=False,core_changes_required=False,retained_large_artifacts=[])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(passed=True,call_sites=[hex(x) for x in references],absolute_references=0,next_callee_fp_uses=0,core_changes_required=False)))
if __name__=='__main__':main()
