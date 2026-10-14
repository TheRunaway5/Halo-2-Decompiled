#!/usr/bin/env python3
"""Extract already captured original state at every107B50 effect-service entry.

Inputs are the immutable constructor/update reports, not native expected values.
The generated fixture retains both52-byte barrels,12-byte trigger and six input/
charge bytes alongside heat/age/RNG/timers at each original callback boundary.
"""
import argparse,hashlib,json,struct
from pathlib import Path
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 lines=[];inputs=[]
 for case in json.loads(a.report.read_text())['cases']:
  path=Path(case['path']);d=json.loads(path.read_text());inputs.append(dict(path=str(path),sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
  assert inputs[-1]['sha256']==case['sha256']
  for row in d['rows']:
   effects=[e for e in d['events'][slice(*row['event_range'])] if e.get('service')=='00107b50']
   for ordinal,e in enumerate(effects):
    s=e['state'];words=[int(s['heat_bits'],16),int(s['spent_bits'],16),int(s['weapon_flags'],16)&15,int(s['seed'],16),*s['hand_timers'],s['blocking'],int(s['inhibitor_bits'],16)]
    words+=list(struct.unpack('<26I',bytes.fromhex(s['barrels_hex'])))+list(struct.unpack('<3I',bytes.fromhex(s['trigger_hex'])))
    analog=bytes.fromhex(s['analog_hex']);words+=[int.from_bytes(analog[:4],'little'),int.from_bytes(analog[4:],'little')]
    assert len(words)==39
    lines.append(' '.join([str(case['representation']),str(int(case['local'])),case['pattern'],str(row['tick']),str(ordinal),*(f'{v:08x}' for v in words)]))
 a.fixture.write_text('\n'.join(lines)+'\n');a.output.write_text(json.dumps(dict(scope=__doc__,requests=len(lines),inputs=inputs,
  source_report_sha256=hashlib.sha256(a.report.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
  fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest()),indent=2,sort_keys=True)+'\n')
 print('original callback states',len(lines),flush=True)
if __name__=='__main__':main()
