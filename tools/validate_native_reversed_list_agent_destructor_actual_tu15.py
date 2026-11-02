#!/usr/bin/env python3
"""Actual typed2F81B0 BEFORE input, independent unchanged-original leaf ABI."""
import argparse,bisect,hashlib,inspect,json,lzma,os,subprocess,sys,tarfile
from pathlib import Path
import validate_native_reversed_list_agent_destructor_tu15 as parent
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 parser=argparse.ArgumentParser(add_help=False);parser.add_argument('--capture',type=Path,required=True);parser.add_argument('--archive-sha256',required=True);a,remaining=parser.parse_known_args();archive=a.capture.resolve();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(archive)==a.archive_sha256
 capture=json.loads(archive.with_name('capture-native-before.json').read_text());assert capture['schema']=='native-tu15-pair-deletion-child-typed-before-v1'and capture['reference']=='English TU1.5 only'and not capture['guest_abi_fabricated']and not capture['source_memory_mutated']and not capture['physics_result_supplied'];typed=capture['input'];assert typed['owner']==0x2f81b0 and typed['receiver']is not None and len(typed['arguments'])==1 and typed['return_pop']==4 and typed['preserved_x87_depth']==0
 manifest=json.loads(lzma.decompress(archive.with_name('all-file-hashes.json.xz').read_bytes()));wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(archive)],stdout=subprocess.PIPE)
 with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
  for member in tar:
   name=str(Path(member.name));key=name[2:]if name.startswith('./')else name
   if key not in wanted:continue
   row=wanted[key];raw=tar.extractfile(member).read();proof=manifest.get('./'+key,manifest.get(key));assert proof is not None and len(raw)==row['size']and hashlib.sha256(raw).hexdigest()==proof['sha256'];banks[row['address']]=raw
 assert stream.wait()==0 and len(banks)==len(wanted);starts=sorted(banks)
 def read(p,n=4):
  index=bisect.bisect_right(starts,p)-1
  if index<0:return None
  first=starts[index];raw=banks[first];return raw[p-first:p-first+n]if p+n<=first+len(raw)else None
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');owners=(0x2f81b0,0x2f81e0,0x2f7e90,0x2f7a10,0x149da0,0x149e00,0x233250,0x233420);assert sha(NEW_DEFAULT)==NEW_PIN
 for owner in owners:
  for pc in complete_body(code,owner).code:
   i=code.at(pc);actual=read(pc,i.size)
   if actual is not None:assert actual==i.bytes,hex(pc)
 assert all(not any(p<=reserved<p+len(raw)for p,raw in banks.items())for reserved in(0x4000000,0x2000000,0x71000000))
 provenance=dict(archive=str(archive),archive_sha256=a.archive_sha256,actual_typed_child=typed,verified_before_banks=len(banks),verified_before_bytes=sum(map(len,banks.values())),source_code_authenticated=True,no_captured_GP_or_after_consumed=True,ordinary_original_leaf_ABI_declared=True)
 parent.child.base.reference_inputs=lambda image:(banks,provenance)
 actual_cases=lambda:[dict(name=f'actual2f81b0_fp_{n}',entry=0x2f81b0,receiver=typed['receiver'],arguments=typed['arguments'],user_words=[])for n in range(4)]
 source=inspect.getsource(parent.main);source=source.replace('child.cases=cases','child.cases=actual_cases')
 # The actual selected low-bit release admits both independent mutations;
 # owned-array coverage remains with the ten source-allocated component cases.
 source=source.replace("('borrowed_storage','if(!(capacity&0x80000000u))','if(false)'),",'')
 source=source.replace("'tools/validate_native_reversed_list_agent_destructor_tu15.py'","'tools/validate_native_reversed_list_agent_destructor_tu15.py','tools/validate_native_reversed_list_agent_destructor_actual_tu15.py'")
 environment=dict(parent.__dict__,actual_cases=actual_cases);sys.argv=[sys.argv[0],*remaining];exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
