#!/usr/bin/env python3
"""Actual1470 before DATA plus source-built ordinary event -> native281D40.

The retained capture is a typed322F70 entry, not a captured original callback
GP frame. The preceding selected world callbacks are authenticated RET4 stubs;
source322F70/31FEE0 therefore build the shown event from unchanged current
manager/body/id DATA before the selected first-body281D40 call.
"""
import argparse,bisect,hashlib,inspect,json,os,struct,sys,subprocess,tarfile
from pathlib import Path
import validate_native_gameplay_contact_removal_tu15 as parent
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(add_help=False);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);a,rest=p.parse_known_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN
 capture=json.loads((a.archive.parent/'capture-native-before.json').read_text());assert capture['schema']=='native-tu15-contact-point-typed-before-v1'and capture['input']['owner']==0x322f70 and capture['input']['preserved_x87_depth']==0 and not capture['guest_abi_fabricated']and not capture['physics_result_supplied'];wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
 with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
  for member in tar:
   name=Path(member.name).name
   if name in wanted:r=wanted[name];raw=tar.extractfile(member).read();assert len(raw)==r['size'];banks[r['address']]=raw
 assert stream.wait()==0 and len(banks)==len(wanted);starts=sorted(banks)
 def get(p,size):
  n=bisect.bisect_right(starts,p)-1
  if n<0:return None
  first=starts[n];return banks[first][p-first:p-first+size]if first<=p and p+size<=first+len(banks[first])else None
 def word(p,size=4):
  value=get(p,size);assert value is not None,(hex(p),size);return int.from_bytes(value,'little')
 typed=capture['input'];manager=typed['manager'];first=word(manager+0x1c);second=word(manager+0x20);world=word(manager+8)
 assert word(manager)==0x41f124 and word(manager+0x10)==0x41f478
 data,count=word(world+0xb4),word(world+0xb8);assert count<=16
 for n in range(count):
  listener=word(data+n*4)
  if listener:assert word(word(listener)+4)==0x2e4d60
 listeners,listener_count=word(first+0x60),word(first+0x64);assert listener_count==1 and word(word(word(listeners))+4)==0x281d40
 assert not any(p<parent.U+0x10000 and p+len(raw)>parent.U for p,raw in banks.items()),'Ordinary event DATA would alias an actual captured owner'
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 for owner in parent.OWNERS:
  for pc in complete_body(code,owner).code:
   i=code.at(pc);before=get(pc,i.size)
   if before is not None:assert before==i.bytes,(hex(pc),'Actual source code differs')
 fp=typed['floating_point'];selected=[]
 for padding in(0,0xa5,0x5a):
  selected.append(dict(name='actual1470_first_removed_padding_'+str(padding),entry=0x281d40,receiver=word(listeners),incoming_eax=0,arguments=[parent.EVENT],user_words=[(parent.EVENT-parent.U,typed['point']|((padding*0x101)<<16)),(parent.EVENT-parent.U+4,manager),(parent.EVENT-parent.U+8,first),(parent.EVENT-parent.U+12,second),(parent.EVENT-parent.U+16,first)],before_fp_input=[fp['x87_control_word'],fp['x87_status'],fp['mxcsr']]))
 source=inspect.getsource(parent.main);needle="banks={0x52be5c:struct.pack('<I',OWNER),0x52d68c:struct.pack('<I',CONSTRAINT_OWNER)}";assert source.count(needle)==1;source=source.replace(needle,'banks=actual_banks')
 source=source.replace("all_cases=cases()","all_cases=selected_cases")
 source=source.replace("environment=dict(base.__dict__,observer_path=observer_path);", "source=source.replace(\"case['before_fp']=fp\",\"fp=case.get('before_fp_input',fp);case['before_fp']=fp\");environment=dict(base.__dict__,observer_path=observer_path);")
 source=source.replace("driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','');", "driver=driver.replace('check(cases>20,','check(cases==3,').replace('NativeTu15ArrayGrowthActivity activity;','');")
 needle="mutants=(('wrong_point_width'";start=source.index(needle);end=source.index('\n  for name,before,after in mutants:',start)
 source=source[:start]+"mutants=(('wrong_point_width','m.word(event,2)','m.word(event)'),('wrong_count','last=count-1u','last=count'),('skip_callback','if(m.word(component+4u,1)&1u)','if(false)'))"+source[end:]
 source=source.replace("'tools/validate_native_gameplay_contact_removal_tu15.py','tools/validate_native_array_growth_tu15.py'", "'tools/validate_native_gameplay_contact_removal_tu15.py','tools/validate_native_gameplay_contact_actual_before_tu15.py','tools/validate_native_array_growth_tu15.py'")
 source=source.replace('actual_live_input_claimed=False','actual_live_input_claimed=True,captured_original_callback_gp_frame=False,actual_before_provenance=actual_provenance')
 source=source.replace("scope='Declared valid bodies/properties/component/72B rows. Strict DATA/ordered reads/stores/preimages/banks/FP; native22E1B0 ref2 path and genuine1E0A60 entry prefixes. No source GP postimages or event padding reads. Runtime constraint removal and part-contact child remain explicit requirements; no full live claim.'", "scope='Genuine1470 typed322F70 before banks plus source-built ordinary first-body callback event. Every captured bank/strict operands/preimages/FP compared under three event padding controls. No captured original callback GP frame or expectedafter consumed. Actual row has no effect/part child; no full live point-parent/world/frame claim.'")
 provenance=dict(archive=str(a.archive),archive_sha256=a.archive_sha256,actual_before_banks=len(banks),actual_before_bytes=sum(map(len,banks.values())),typed_capture=typed,event_source_writer='322F70 fields +31FEE0 subject',preceding_world_callbacks='actual2E4D60 only; no public mutation',expected_after_consumed=False)
 environment=dict(parent.__dict__,actual_banks=banks,selected_cases=selected,actual_provenance=provenance);sys.argv=[sys.argv[0],*rest];exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
