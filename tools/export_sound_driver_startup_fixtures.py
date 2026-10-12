#!/usr/bin/env python3
"""Export untouched paired driver prefixes and actual boundary requests."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded
from oracle_sound_driver_startup_native import FIELDS
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--providers',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=json.loads(a.report.read_text());packet=json.loads(a.providers.read_text());c=packet['constants'];lines=['# Actual original allocation-prefix outputs; mandatory real device request has no supplied return.']
 for tag in('old','u5'):
  lines.append('constants\t'+tag+'\t'+f'{int.from_bytes(bytes.fromhex(c["one_bits"]["raw4_hex"]),"little"):08x}'+'\t'+c['basis_forward']['raw12_hex']+'\t'+c['basis_up']['raw12_hex']+'\t'+packet['source_selected_plugin_identity'][tag])
 for q in r['cases']:
  name=q['name'];lines.append('case\t'+name+'\t'+q['image']+'\t'+q['policy16_hex'])
  for phase in('before','after'):lines.append('state\t'+name+'\t'+phase+'\t'+'\t'.join(q[phase][k]for k in FIELDS))
  lines.append('input\t'+name+'\t'+q['initial_arena_hex']);lines.append('output\t'+name+'\t'+q['output_arena_hex'])
  for w in q['stores']:lines.append('store\t'+name+'\t'+'\t'.join(str(w['canonical'][k])for k in('instruction','address','bytes','before','after')))
  lines.append('request\t'+name+'\t'+'\t'.join(q['native_platform_boundary']['actual_stack_arguments']))
 save(a.output,('\n'.join(lines)+'\n').encode());save(a.output.with_suffix('.json'),encoded(dict(schema='source-sound-driver-startup-fixtures-v1',exporter_sha256=sha(__file__),source_report_sha256=sha(a.report),providers_sha256=sha(a.providers),fixture_sha256=sha(a.output),cases=len(r['cases']),stores=sum(len(q['stores'])for q in r['cases']),state_field_order=FIELDS,bounds=r['bounds'])))
 print(json.dumps(dict(fixture_sha256=sha(a.output),cases=len(r['cases']))))
if __name__=='__main__':main()
