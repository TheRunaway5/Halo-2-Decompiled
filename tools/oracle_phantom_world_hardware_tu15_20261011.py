#!/usr/bin/env python3
"""Run the TU1.5 activation contracts on untouched i386 hardware instructions.

The complete graph also contains unexercised FS/SEH owners. The single-step
metadata describes the observed contract footprint only; an unexpected hardware
instruction fails the trace. No FS owner is substituted or reported covered.
"""
import argparse,copy,json
from pathlib import Path
from oracle_vehicle_motion_math_hardware_20261011 import execute
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from research_phantom_world_tu15_20261011 import sha

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--correspondence',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert sha(NEW_DEFAULT)==NEW_PIN
 paired=json.loads(a.correspondence.read_text());source=a.original/'u5.json';fixture=json.loads(source.read_text());assert fixture['image']=='1.5'
 seen={p for c in fixture['cases'] for p in c['instructions']};assert seen<=set(paired['instruction_map'].values())
 code=_Code(Xbe(NEW_DEFAULT),'.text')
 protected={b for q in paired['instruction_map'].values() for b in range(q,q+code.at(q).size)}
 for target in {c['exit_entry'] for c in fixture['cases'] if not c['complete']}:assert not protected&set(range(target,target+15))
 observed=copy.deepcopy(paired);observed['instruction_map']={p:q for p,q in paired['instruction_map'].items() if q in seen}
 a.output.mkdir(parents=True,exist_ok=False)
 result=execute(fixture,observed,NEW_DEFAULT,a.output/'u5-harness')
 result.update(input_fixture_sha256=sha(source),correspondence_sha256=sha(a.correspondence),wrapper_tool_sha256=sha(__file__),
  full_graph_sites=len(paired['instruction_map']),hardware_metadata_sites=len(seen),unexercised_FS_owners_claimed=False)
 target=a.output/'u5.json';target.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
 hardware={c['id']:c for c in result['cases']};combined=copy.deepcopy(fixture)
 for case in combined['cases']:
  for key in ('exit','final_banks','instructions','writes'):case[key]=hardware[case['id']][key]
  case['writes']=[dict(event,kind='W') for event in case['writes']]
 combined.update(oracle_backend='original_i386_hardware_single_step',hardware_fixture_sha256=sha(target),hardware_wrapper_sha256=sha(__file__),
  hardware_tool_sha256=result['tool_sha256'],unported_owner_capture_stubs=result['unported_owner_capture_stubs'],paired_instruction_bytes_changed=False,
  trace_observer_sha256=result['trace_observer_sha256'],single_stepped_original_instructions=result['single_stepped_original_instructions'])
 native=a.output/'u5-native.json';native.write_text(json.dumps(combined,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(wrapper_tool_sha256=sha(__file__),hardware_tool_sha256=result['tool_sha256'],fixture_sha256=sha(target),native_fixture_sha256=sha(native),
  cases=len(combined['cases']),single_stepped_original_instructions=result['single_stepped_original_instructions'],hardware_metadata_sites=len(seen),unexercised_FS_owners_claimed=False)
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
