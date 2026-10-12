#!/usr/bin/env python3
"""Retain actual external source calls and worker/file callback bindings."""
import hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
E=ROOT/'analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 from capstone import Cs,CS_ARCH_X86,CS_MODE_32
 p=E/'correspondence-v1/complete-report.json';assert sha(p)=='616bee89417916769418d9f2a70075ab036c2bba8a1decadd46698c5d57f1933'
 d=json.loads(p.read_text());assert sha(NEW_DEFAULT)==NEW_PIN
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(NEW_DEFAULT)
 md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;entries={int(p,16):q for p,q in d['entries'].items()}
 external={};indirect=[];callback=[]
 for routine in d['routines']:
  for row in routine['instructions']:
   oi,ni=[next(md.disasm(image.read(row[side],len(bytes.fromhex(row[side+'_bytes']))),row[side]))for image,side in ((old,'old'),(new,'later'))]
   if oi.mnemonic=='call':
    if oi.operands[0].type==2:
     target,later=oi.operands[0].imm,ni.operands[0].imm
     if target not in entries:
      key=f'{target:08x}';entry=external.setdefault(key,dict(later_target=later,calls=[]));assert entry['later_target']==later
      entry['calls'].append(dict(old_site=oi.address,later_site=ni.address,owner=routine['old']))
    else:indirect.append(dict(old_site=oi.address,later_site=ni.address,owner=routine['old'],later_operand=ni.op_str))
   for x,y in zip(oi.operands,ni.operands):
    if x.type==2 and x.imm in (0x122350,0x1a4250,0x1a4c20):
     assert entries[x.imm]==y.imm
     callback.append(dict(old_site=oi.address,later_site=ni.address,old_target=x.imm,later_target=y.imm))
 report=dict(schema=1,tool_sha256=sha(__file__),correspondence_sha256=sha(p),later_original_sha256=NEW_PIN,
  complete_bodies=17,compiled=False,source_sites=730,external_direct_calls=external,indirect_calls=indirect,callback_bindings=callback,
  native_platform_services_joined=False,complete_game_arena_startup=False)
 p=E/'external-owners.json';raw=json.dumps(report,sort_keys=True,indent=2)+'\n'
 if p.exists():assert p.read_text()==raw
 else:p.write_text(raw)
 print(json.dumps(dict(sha256=sha(p),external_direct_targets=len(external),indirect_calls=len(indirect),callback_bindings=len(callback))))
if __name__=='__main__':main()
