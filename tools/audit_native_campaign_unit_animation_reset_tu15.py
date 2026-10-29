#!/usr/bin/env python3
"""Authenticate116920 callers and the remaining actual action/animation frontier."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);a=p.parse_args();assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir();image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');sec=next(s for s in image.sections if s.name=='.text');raw=image.section_bytes(sec);calls=[]
 for i in range(len(raw)-4):
  if raw[i] in [0xe8,0xe9] and (sec.va+i+5+struct.unpack_from('<i',raw,i+1)[0])&0xffffffff==0x116920:
   ins=code.at(sec.va+i);assert ins.mnemonic=='call';calls.append(sec.va+i)
 assert calls==[0xcd3c8,0xd0594]
 def listing(start,stop):
  rows=[];pc=start
  while pc<stop:
   ins=code.at(pc);rows.append(dict(instruction=hex(pc),bytes=bytes(ins.bytes).hex(),operation=ins.mnemonic,operands=ins.op_str));pc+=ins.size
  assert pc==stop;return rows
 body=_trace(code,0x116920,{0x116920});assert not any(code.at(pc).mnemonic=='call' for pc in body.code)
 next_entries=[0xeb360,0xeb8f0,0xeb860,0x116570,0x1d6200,0x1ce7b0,0x1e3150,0x1e5c80,0x1e3270,0x1e5af0,0x126200,0x1264e0]
 frontier=[]
 for entry in next_entries:
  b=_trace(code,entry,set(next_entries));frontier.append(dict(entry=hex(entry),sites=len(b.code),calls=[dict(site=hex(pc),target=code.at(pc).op_str) for pc in sorted(b.code) if code.at(pc).mnemonic=='call']))
 absolute=[]
 for section in image.sections:
  data=image.section_bytes(section);needle=struct.pack('<I',0x116920);i=data.find(needle)
  while i>=0:absolute.append(dict(section=section.name,address=hex(section.va+i)));i=data.find(needle,i+1)
 assert not absolute
 receipt=dict(passed=True,original_xbe_sha256=NEW_PIN,entry='116920',sites=len(body.code),original_body_sha256=hashlib.sha256(b''.join(bytes(code.at(pc).bytes) for pc in sorted(body.code))).hexdigest(),direct_callers=[hex(pc) for pc in calls],absolute_function_pointer_references=absolute,caller_continuations={'CD3C8':listing(0xcd3cd,0xcd3d7),'D0594':listing(0xd0599,0xd059e),'ED060_after_D0520':listing(0xed09f,0xed0f9)},ordinary_contract='Void reset(EAX unit),RET0; caller stack and callee-savedGP remain owned by caller. No returned private extension-addressEAX,zeroEDX,minus-oneECX,flags or XMM0/1 scratch is reconstructed.',liveness={'direct_continuations':'Both CD3C8 and D0594 continuations contain only callee-saved pops, stack release and RET. No volatile GP/flags/SIMD is consumed there under their ordinary void contracts.','required_orders_path':'After D0520, ED060 selects its next branch from unit+AA. Both paths freshly setEAX (unitESI or constant2) before calls; required arguments are explicit GP/stack inputs. No reset private scratch is used.','FP':'116920 uses XORPS/MOVSS solely for raw zero/one stores, with no FP arithmetic or status change. XMM0/1 are dead private scratch; no FP result is exported. x87/MMX/FP controls/status remain caller-owned.'},boundaries=['Ordinary authenticated direct call contracts, not arbitrary computed/hidden-register consumers.','Pure reset qualification does not close D0520,ED060 or any action-begin admission.'],remaining_action_animation_frontier=frontier,shared_allocator=dict(entry='16E6A0',api='native_tu15_data_array_allocate(memory,pool)',header='include/halo2/native_tu15_data_array_allocate.hpp',proof='analysis/campaign-native-data-array-allocate-20261028/proof.json',needed_by='animation resource demand126200; not used by116920'),runtime_original_execution=False)
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(passed=True,sites=len(body.code),direct_callers=receipt['direct_callers'],native_entry_dependencies=0)))
if __name__=='__main__':main()
