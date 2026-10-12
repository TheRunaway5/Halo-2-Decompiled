#!/usr/bin/env python3
"""Execute original mode1/mode3/mode5 start/end response and queue equivalence.

Actual raw options modes are supplied independently of the separate original
simulation constructor proof. Original event/response/queue bodies execute;
the base oracle's explicit platform/caption/network services stay external.
"""
import hashlib,itertools,json
from pathlib import Path
from oracle_vista_announcer_events import Original,PcMap,CONTEXTS,event
ROOT=Path(__file__).resolve().parents[1]
def main():
 image=ROOT/'Halo 2 Vista installer/halo2.exe';cache_path=ROOT/'Halo 2 Vista installer/maps/shared.map'
 raw=image.read_bytes();original=Original(raw,PcMap(cache_path));rows=[]
 for (cw,mx),rate,language,kind,tick in itertools.product(CONTEXTS,(30,60),range(9),((2,0),(0,19)),(0,62)):
  pair=[]
  for mode in (1,3,5):
   original.reset(rate,cw,mx)
   q=dict(event=event(kind,0x12340000 if kind==(2,0) else 0xffffffff),language=language,
       mode=mode,over=int(kind==(0,19)),flags=9,user=0,tick=tick)
   result=original.run(q);result.update(cw=cw,mxcsr=mx,rate=rate);pair.append(result)
  for actual in pair[1:]:
   for field in ('before','after','calls','selected'):assert pair[0][field]==actual[field],(q,field)
  rows.extend(pair)
 output=ROOT/'analysis/frontend/vista-session-flow-audit/player-birth-original'
 evidence=dict(source_sha256=hashlib.sha256(raw).hexdigest(),asset_sha256=hashlib.sha256(cache_path.read_bytes()).hexdigest(),
  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=len(rows),instructions=original.visits,
  scope=__doc__,rows=rows)
 (output/'mode5-announcer-equivalence.json').write_text(json.dumps(evidence,indent=2)+'\n')
 print(f'PASS {len(rows)} original mode1/mode3/mode5 event-response-queue cases; {original.visits} instructions')
if __name__=='__main__':main()
