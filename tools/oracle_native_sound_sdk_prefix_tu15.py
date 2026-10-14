#!/usr/bin/env python3
"""Compare compiled native SDK factory CPU intervals with unchanged original i386.

Real native pool/lock results and actual source FS context are interval inputs.
No setting, SDK-device success, expected output, instruction patch or byte mask
is supplied to original CPU execution. Kernel/DSP code is not executed here.
"""
import argparse,json
from pathlib import Path
from oracle_source_sound_startup_tu15 import sha,replay,initial_abi,differences
from generate_source_sound_sdk_tu15 import Code
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_sandbox_first_person_hardware_tu15 import execute

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 evidence=json.loads((a.capture/'report.json').read_text());assert evidence['original_sha256']==NEW_PIN and not evidence['fixture_callee_results_used']
 assert evidence['native_allocations']==evidence['native_capacity_queries']==2 and evidence['native_lock_acquisitions']==evidence['unavailable_setting_queries']==1
 assert evidence['native_allocations_after_abort']==evidence['native_locks_after_abort']==0 and evidence['partial_source_state_faulted'] and not evidence['source_device_output_published'] and not evidence['source_driver_initialized']
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=Code(image);cases=[];fragments=[];ends=[];start='before.json'
 for n in range(1,len(evidence['native_platform_calls'])+2):
  stop=f'call-{n}-before.json'if n<=len(evidence['native_platform_calls'])else'setting-before.json'
  before=json.loads((a.capture/start).read_text());after=json.loads((a.capture/stop).read_text());assert before['abi']['fs_base']==after['abi']['fs_base']==evidence['original_fs_base']
  assert any(r['address']<=evidence['original_fs_base']+0x24<r['address']+r['size']for r in before['regions'])
  case,fragment=replay(before,after,n,code)
  for pc in case['original_instructions']:
   i=code.at(pc);region=next(r for r in before['regions']if r['address']<=pc and pc+i.size<=r['address']+r['size']);assert bytes.fromhex(region['hex'])[pc-region['address']:pc-region['address']+i.size]==image.read(pc,i.size),'SDK instruction bytes differ from owned original'
  cases.append(case);fragments.append(fragment);ends.append(after);start=f'call-{n}-after.json'
 paired=dict(address_map={},instruction_map={f'{pc:08x}':pc for c in cases for pc in c['original_instructions']},entries={f'{c["entry"]:08x}':c['entry']for c in cases})
 raw=execute(dict(image='1.5',cases=cases),paired,(a.output/'hardware').resolve(),code_override=code);compared=[]
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
  original_code_patches=raw['original_instruction_patches'],native_expected_data_used=raw['native_expected_data_used'],native_fixture_callee_results_used=False,byte_masks_used=False,native_instruction_trace_compared=True,actual_source_fs_base=evidence['original_fs_base'],
  raw_fp_classification_encoding_equality=False,native_device_dsp_or_pcm_verified=False,tool_sha256=sha(Path(__file__)),original_results_sha256=sha(a.output/'original-results.json'),capture_sha256={p.name:sha(p)for p in a.capture.glob('*.json')})
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(status=report['status'],intervals=len(compared),instructions=sum(c['instructions']for c in compared),retained_bytes_per_interval=[c['exact_retained_bytes']for c in compared],byte_masks_used=False,native_instruction_trace_compared=True,source_fs_base=evidence['original_fs_base'])))
 return int(report['status']!='pass')
if __name__=='__main__':raise SystemExit(main())
