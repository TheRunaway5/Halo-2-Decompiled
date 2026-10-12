#!/usr/bin/env python3
"""Export captured original zoom outputs, without native expected-value generation."""
import argparse,json
from pathlib import Path
def fields(s):
 return [s['parent'],s['action_word'],str(s['weapon_count']),s['weapon_flags'],s['weapon_minimum'],s['weapon_maximum'],
  str(s['binocular_count']),s['binocular_minimum'],s['binocular_maximum'],str(s['magazine_modes'][0])]
def main():
 p=argparse.ArgumentParser();p.add_argument('report',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
 r=json.loads(a.report.read_text());rows=[]
 s=r['initial'];rows.append(['I','constructor',str(s['current']),str(s['requested']),s['motion']])
 for q in r['leaf_cases']:
  kind={'000c9920':'A','000c9cc0':'C','000c9be0':'M'}[q['entry']]
  b=q.get('supplied_boundary') or {};current=b.get('incoming_current_signed16',-1)
  rows.append([kind,q['name'],*fields(q['before']),str(current),q['return_eax'],q['return_xmm0_low']])
 phases=[q['unit_phase']for q in r['composition']]+r['unit_update_controls']
 for q in phases:
  s=q['before'];z=q['after'];admission=[c for c in q['calls']if c['entry']=='000c9920']
  enabled=not(int(s['unit_definition_flags'],16)&1024)and not(s['unit_object_flags_10a']&4)
  assert len(admission)==int(enabled)
  rows.append(['U',q['name'],*fields(s),str(s['current']),str(s['requested']),s['motion'],str(int(enabled)),
   str(z['current']),str(z['requested']),z['motion'],f'{z["input_flags"]:04x}',
   admission[0]['return_eax']if admission else 'NA'])
 a.output.write_text('\n'.join(' '.join(q)for q in rows)+'\n')
 print(json.dumps({'fixture_rows':len(rows),'leaf_cases':len(r['leaf_cases']),'unit_phases':len(phases)}))
if __name__=='__main__':main()
