#!/usr/bin/env python3
"""Export only independently measured original section bytes, never native output."""
import argparse,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'analysis/multiplayer/sandbox-source/live-prop/death-measured/report.json'
PIN='cc2475ab1311310587fa83f358c791d5b036b2e5e9bbafe20beac583dfa42d19'
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
 raw=SOURCE.read_bytes();assert hashlib.sha256(raw).hexdigest()==PIN
 report=json.loads(raw);rows=[]
 for index in (0,1,2,3,4,5,6,8):
  state=report['states'][index];obj=next(o for o in state['world']['objects']if o['datum']=='e26f0000')
  body=bytes.fromhex(obj['raw_hex']);offset=struct.unpack_from('<h',body,0x122)[0]
  assert offset==0x130;rows.append(body[offset:offset+16])
 blob=b'PDTH1101'+struct.pack('<II',2,len(rows))+b''.join(rows)
 args.output.parent.mkdir(parents=True,exist_ok=True)
 if args.output.exists():assert args.output.read_bytes()==blob
 else:args.output.write_bytes(blob)
 info=dict(schema=1,source_report=str(SOURCE.relative_to(ROOT)),source_sha256=PIN,
  source_state_ids=[0,1,2,3,4,5,6,8],sections=2,stride=8,fields=dict(destroyed=0,damage=2,pending=4),
  original_direct_death_request_not_projectile_admission=True,
  final_row='actual DB710 destroyed-bit store before unavailable effect allocation',
  fixture_sha256=hashlib.sha256(blob).hexdigest(),native_implementation_expected_data=False)
 manifest=args.output.with_suffix('.json');encoded=(json.dumps(info,indent=2,sort_keys=True)+'\n').encode()
 if manifest.exists():assert manifest.read_bytes()==encoded
 else:manifest.write_bytes(encoded)
 print(json.dumps(info))
if __name__=='__main__':main()
