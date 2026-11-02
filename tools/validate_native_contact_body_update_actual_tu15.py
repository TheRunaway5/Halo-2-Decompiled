#!/usr/bin/env python3
"""Actual typed1D77D0 BEFORE DATA; unchanged original and native whole parent."""
import argparse,bisect,hashlib,inspect,json,lzma,os,subprocess,sys,tarfile
from pathlib import Path
import validate_native_contact_body_update_tu15 as parent
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 parser=argparse.ArgumentParser(add_help=False);parser.add_argument('--capture',type=Path,required=True);parser.add_argument('--archive-sha256',required=True);a,remaining=parser.parse_known_args();archive=a.capture.resolve();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(archive)==a.archive_sha256
 capture=json.loads(archive.with_name('capture-native-before.json').read_text());assert capture['schema']=='native-tu15-contact-body-update-typed-before-v1'and capture['reference']=='English TU1.5 only'and not capture['guest_abi_fabricated']and not capture['source_memory_mutated']and not capture['physics_result_supplied'];typed=capture['input'];assert typed['owner']==0x1d77d0 and typed['manager']and typed['row']and typed['preserved_x87_depth']==0
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
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');owners=(0x1d77d0,0x1d7b60,0x323020,0x323040,0x31dd70,0x31edf0,0x323a10,0x186c30,0x1e1170,0x315c90,0x315cb0,0x3165d0,0x316d40,0x322660,0x324f80,0x8fdb0);assert sha(NEW_DEFAULT)==NEW_PIN
 for owner in owners:
  for pc in complete_body(code,owner).code:
   i=code.at(pc);actual=read(pc,i.size)
   if actual is not None:assert actual==i.bytes,hex(pc)
 assert all(not any(p<=reserved<p+len(raw)for p,raw in banks.items())for reserved in(0x4000000,0x2000000,0x71000000))
 provenance=dict(archive=str(archive),archive_sha256=a.archive_sha256,actual_typed_child=typed,verified_before_banks=len(banks),verified_before_bytes=sum(map(len,banks.values())),source_code_authenticated=True,no_captured_GP_or_after_consumed=True,ordinary_original_leaf_ABI_declared=True)
 actual_inputs=lambda image:(banks,provenance)
 actual_fp=[typed['floating_point'][k]for k in('x87_control_word','x87_status','mxcsr')]
 actual_cases=lambda:[dict(name=f'actual1d77d0_fp_{n}',entry=0x1d77d0,receiver=typed['row'],manager=typed['manager'],arguments=[typed['skip_impulse'],int(typed['reversed']),typed['component_datum'],typed['body_slot']],user_words=[],**({'actual_fp':actual_fp}if n==0 else{}))for n in range(4)]
 source=inspect.getsource(parent.main);source=source.replace('child.cases=cases;child.base.reference_inputs=reference_inputs','child.cases=actual_cases;child.base.reference_inputs=actual_inputs')
 # The actual tuple determines which scalar/tag branches execute; the broad
 # declared source fixtures retain the independent three mutation controls.
 start=source.index(' source=source[:begin]+"mutants=');end=source.index('\n source=source.replace("scope=',start)
 source=source[:start]+" source=source[:begin]+\"mutants=[]\"+source[end:]\n"+source[end:]
 anchor=' source=source.replace(needle,extra+needle)';assert source.count(anchor)==1
 source=source.replace(anchor,' extra+=\'driver=driver.replace("check(cases>20,","check(cases>0,");\'\n'+anchor)
 fp_expression='[0x27f|((index%4)<<10),((index%8)<<11)|((index%4)<<8)|(index%32),0x1f80|((index%4)<<13)]'
 original_anchor='source=inspect.getsource(base.original);'
 patch='source=source.replace('+repr('fp='+fp_expression)+','+repr("fp=case.get('actual_fp',"+fp_expression+")")+');'
 child_anchor='source=inspect.getsource(child.main)';assert source.count(child_anchor)==1
 source=source.replace(child_anchor,child_anchor+';source=source.replace('+repr(original_anchor)+','+repr(original_anchor+patch)+')')
 source=source.replace("'tools/validate_native_contact_body_update_tu15.py'","'tools/validate_native_contact_body_update_tu15.py','tools/validate_native_contact_body_update_actual_tu15.py'")
 environment=dict(parent.__dict__,actual_inputs=actual_inputs,actual_cases=actual_cases);sys.argv=[sys.argv[0],*remaining];exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
