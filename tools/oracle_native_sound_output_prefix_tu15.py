#!/usr/bin/env python3
"""Compare native output-device caller intervals with unchanged original i386.

The output identity is produced by an actual SDL device owner. No recorded
callee return or expected final native output is supplied to original execution.
Original kernel/DSP code is not executed by this CPU-only interval comparison.
"""
import argparse,json
from pathlib import Path
from oracle_source_sound_startup_tu15 import sha,replay,initial_abi,differences
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_sandbox_first_person_hardware_tu15 import execute

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 evidence=json.loads((a.capture/'report.json').read_text());assert evidence['original_sha256']==NEW_PIN and not evidence['fixture_callee_results_used']
 assert evidence['native_output_creations']==evidence['unavailable_capability_requests']==1 and evidence['device_closed_after_abort'] and evidence['output_withdrawn_after_abort']
 assert evidence['native_backend'] not in ('dummy','disk') and evidence['native_callback_observed_playing'] and not evidence['source_driver_initialized']
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');cases=[];fragments=[];ends=[]
 for n,(start,stop)in enumerate((('before.json','create-before.json'),('create-after.json','capabilities-before.json')),1):
  before=json.loads((a.capture/start).read_text());after=json.loads((a.capture/stop).read_text());text=image.section('.text');original_text=next(r for r in before['regions']if r['address']==text.va);assert bytes.fromhex(original_text['hex'])==image.read(text.va,text.vsize),'Native input contains modified original code';case,fragment=replay(before,after,n,code);cases.append(case);fragments.append(fragment);ends.append(after)
 paired=dict(address_map={},instruction_map={f'{pc:08x}':pc for c in cases for pc in c['original_instructions']},entries={f'{c["entry"]:08x}':c['entry']for c in cases})
 raw=execute(dict(image='1.5',cases=cases),paired,(a.output/'hardware').resolve());compared=[]
 for c,end,f in zip(raw['cases'],ends,fragments):
  expected=initial_abi(end['abi']);errors=differences(c['exit'],expected)
  for region,bank in zip(end['regions'],c['final_banks']):
   assert region['address']==bank['address']
   if region['hex']!=bank['hex']:errors.append(dict(kind='exact_memory',address=region['address']))
  if c['instructions']!=f['instructions']:errors.append(dict(kind='original_unicorn_trace'))
  if c['instructions']!=end['instructions']:errors.append(dict(kind='native_instruction_trace'))
  compared.append(dict(id=c['id'],status='pass'if not errors else'fail',errors=errors,instructions=len(c['instructions']),exact_retained_bytes=f['retained_bytes'],raw_fp_encodings=[dict(field=k,host=c['exit'][k],native=expected[k])for k in ('x87_control_word','x87_tag')if c['exit'][k]!=expected[k]]))
 (a.output/'original-results.json').write_text(json.dumps(raw,separators=(',',':'))+'\n')
 report=dict(scope=__doc__,status='pass'if all(c['status']=='pass'for c in compared)else'fail',original_sha256=NEW_PIN,cases=compared,fragments=fragments,
  original_code_patches=raw['original_instruction_patches'],native_expected_data_used=raw['native_expected_data_used'],native_fixture_callee_results_used=False,byte_masks_used=False,native_instruction_trace_compared=True,
  raw_fp_classification_encoding_equality=False,native_output_creation_verified=True,native_dsp_voices_or_pcm_verified=False,tool_sha256=sha(Path(__file__)),original_results_sha256=sha(a.output/'original-results.json'),capture_sha256={p.name:sha(p)for p in a.capture.glob('*.json')})
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(status=report['status'],intervals=len(compared),instructions=sum(c['instructions']for c in compared),byte_masks_used=False,native_instruction_trace_compared=True,native_resource_input=True)))
 return int(report['status']!='pass')
if __name__=='__main__':raise SystemExit(main())
