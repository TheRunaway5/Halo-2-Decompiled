#!/usr/bin/env python3
"""Export source prefix states, exact owned regions and ordered writes."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save,encoded

FIELDS=('current_matg_address','sound_system_address','current_ugh_address','driver_address','environment_address','options_address','runtime_environment_parameters_address','queue_budget')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--providers',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 r=json.loads(a.report.read_text());packet=json.loads(a.providers.read_text());c=packet['constants']
 lines=['# Whole126DE0 prefix only; no driver/plugin/effect return or completed map activation.']
 lines.append('constants\t'+'\t'.join(f'{int.from_bytes(bytes.fromhex(c[k]["raw4_hex"]),"little"):08x}'for k in('one_bits','ambient_transition_bits')))
 for case in r['cases']:
  name=case['name'];lines.append('case\t'+name)
  lines.append('state\t'+name+'\tentry\t'+'\t'.join(case['before'][k]for k in FIELDS))
  lines.append('state\t'+name+'\tstopped\t'+'\t'.join(case['after']['state'][k]for k in FIELDS))
  for region in case['regions']:lines.append('region\t'+name+'\t'+region['address']+'\t'+str(int(region['mutable']))+'\t'+region['raw_hex'])
  lines.append('output\t'+name+'\t'+case['after']['arena_hex'])
  for row in case['tag_table']:lines.append('tag\t'+name+'\t'+row['index']+'\t'+row['address'])
  for row in case['clock_calls']:lines.append('clock\t'+name+'\t'+f'{row["tick_ms_u32"]:08x}')
  for row in case['stores']:lines.append('store\t'+name+'\t'+'\t'.join(str(row[k])for k in('instruction','address','bytes','before','after')))
  b=case['boundary'];lines.append('stop\t'+name+'\t'+b['instruction']+'\t'+b['address']+'\t'+str(b['bytes'])+'\t'+str(b['access']))
 save(a.output,('\n'.join(lines)+'\n').encode())
 save(a.output.with_suffix('.json'),encoded(dict(schema='source-sound-map-activation-fixtures-v1',exporter_sha256=sha(__file__),report_sha256=sha(a.report),providers_sha256=sha(a.providers),fixture_sha256=sha(a.output),state_field_order=FIELDS,cases=len(r['cases']),stores=sum(len(c['stores'])for c in r['cases']),
   bounds='Literal owned Lockout bindings and entire prior source-created startup arena. Two explicitly changed metadata branch controls. Whole source owner stops before unavailable driver use. No full platform-callback, successful map/voice or runtime audio result.')))
 print(json.dumps(dict(fixture_sha256=sha(a.output),cases=len(r['cases']))))
if __name__=='__main__':main()
