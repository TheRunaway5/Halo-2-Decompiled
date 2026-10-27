#!/usr/bin/env python3
"""Authenticate original geometry callers and narrow native owner seams.

This is an ABI frontier report, not a global volatile-register liveness proof.
"""
import argparse,bisect,csv,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');section=image.section('.text');raw=image.section_bytes(section);registry=ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv';starts=sorted(int(r['address'],16)for r in csv.DictReader(registry.open(),delimiter='\t'));bodies={}
 # The decompiler registry omits this genuine virtual method. Its authored
 # pointer in the actual original vtable proves the additional callable entry.
 additional=[(0x323900,0x41f220),(0x323b20,0x41f22c)]
 for entry,pointer in additional:assert image.read(pointer,4)==struct.pack('<I',entry)
 starts=sorted(set(starts)|{entry for entry,_ in additional})
 def body(entry):
  if entry not in bodies:bodies[entry]={pc:code.at(pc)for pc in complete_body(code,entry).code}
  return bodies[entry]
 calls={}
 for target,expected in((0x2e44d0,17),(0x2e47b0,8),(0x2e4690,23)):
  hits=[];index=raw.find(b'\xe8')
  while index>=0 and index+5<=len(raw):
   pc=section.va+index
   if(pc+5+struct.unpack_from('<i',raw,index+1)[0])&0xffffffff==target:
    nearest=bisect.bisect_right(starts,pc);owners=[]
    for entry in reversed(starts[max(0,nearest-8):nearest]):
     try:owner=body(entry)
     except Exception:continue
     i=owner.get(pc)
     if i is not None and i.mnemonic=='call'and i.operands[0].type==2 and i.operands[0].imm==target:owners.append(entry);break
    assert owners,f'Unauthenticated instruction boundary at{pc:x}'
    hits.append(dict(pc=hex(pc),full_cfg_owner=hex(owners[0]),bytes=code.at(pc).bytes.hex()))
   index=raw.find(b'\xe8',index+1)
  assert len(hits)==expected,(hex(target),len(hits));calls[hex(target)]=hits
 selected=(0x2e44d0,0x2e47b0,0x2e4690,0x2e4650,0x2e8870,0x2e9600,0x304350)
 listings={hex(e):[dict(pc=hex(pc),bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for pc,i in sorted(body(e).items())]for e in selected};(a.output/'listings.json').write_text(json.dumps(dict(image_sha256=NEW_PIN,bodies=listings),indent=2)+'\n')
 assert body(0x2e8870)[0x2e8888].operands[0].imm==0x2e44d0 and body(0x2e8870)[0x2e889f].mnemonic=='call'
 assert all(body(0x304350)[pc].operands[0].imm==0x2e47b0 for pc in(0x3045c1,0x304904))
 result=dict(completed=True,image_sha256=NEW_PIN,registry_sha256=sha(registry),additional_callable_entries=[dict(entry=hex(entry),authored_pointer=hex(pointer),bytes=image.read(pointer,4).hex())for entry,pointer in additional],source_sha256=sha(__file__),listings_sha256=sha(a.output/'listings.json'),direct_call_count=sum(map(len,calls.values())),direct_calls=calls,geometry_abi=dict(destination='ECX',arguments=['basis pointer at entryESP+4','source pointer at entryESP+8'],return_cleanup_bytes=8,semantic_outputs=['destination bytes','MXCSR arithmetic status'],original_volatile_postimages_provided_by_native_api=False),candidate_native_owners=[dict(entry='0x2e8870',sites=len(body(0x2e8870)),geometry_call='0x2e8888',downstream_call='0x2e889f',downstream_owner='actual child vtable slot0x18',boundary='Convert whole typed shape-bounds wrapper and authenticate actual child ABI; composed64-byte local must remain caller-owned across child call.'),dict(entry='0x304350',sites=len(body(0x304350)),geometry_calls=['0x3045c1','0x304904'],boundary='A completely typed native contact owner may directly consume semantic memory+FP results. Do not substitute legacy geometry callee register postimages.')],legacy_machine_entry_bindings_added=[],all_direct_or_indirect_caller_liveness_proven=False,volatile_outputs_frontier=['2E44D0 EAX/source+64, EDX/basis, XMM0..3 and flags','2E47B0 EAX/source, XMM0..7 and flags','2E4690 EAX/basis, XMM0..3 and flags'],owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],remaining=['Actual child-vtable input ABI for2E8870','Whole304350 native contact owner','Any direct or indirect legacy caller volatile result contract','Live geometry/native multiplayer proof','Xbox CPU bit identity'])
 (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(completed=True,direct_calls=result['direct_call_count'],typed_candidates=2)))
if __name__=='__main__':main()
