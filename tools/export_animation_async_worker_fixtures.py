#!/usr/bin/env python3
"""Export complete raw source queue/file buffers, stores and provider ordering."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import sha,save

def main():
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();d=json.loads(a.source.read_text())
 rows=['# Complete original raw-state and explicit native-provider contract; no source kernel/TLS/worker creation claim.','SOURCE '+sha(a.source)]
 for c in d['cases']:
  rows.append('CASE '+c['name']+' '+c['entry']+' '+f'{c["argument"]:08x}'+' '+c['kind']+' '+f'{(c["callback_control"] or 0):08x}')
  for phase in('before','after'):
   for name,raw in c[phase].items():rows.append(phase.upper()+' '+name+' '+raw)
  for e in c['events']:
   if 'store'in e:
    q=e['store'];rows.append('STORE '+q['instruction']+' '+q['address']+' '+str(q['bytes'])+' '+q['before']+' '+q['written'])
   else:
    q=e['call'];q=q[:-1]if q[0]=='read'else q
    rows.append('CALL '+q[0]+' '+' '.join(f'{v&0xffffffff:08x}'for v in q[1:]))
  rows.append('RETURN '+f'{c["returned_eax"]:08x}'+' '+str(bool(c['stop'])).lower());rows.append('END')
 save(a.output,('\n'.join(rows)+'\n').encode());print(json.dumps(dict(output=str(a.output),sha256=sha(a.output),cases=len(d['cases']))))
if __name__=='__main__':main()
