#!/usr/bin/env python3
"""Original post-world cap with independently pending phases.

Complements oracle_round_lifecycle.py without changing that earlier evidence.
The declared empty world, NONE replication/local users and raw API banks have
the same boundary. Complete original calls execute without patched instructions
or supplied returns. This tests game-over-before-phase ordering, not live reset.
"""
import argparse,json
from pathlib import Path
from oracle_round_lifecycle import LifecycleTrace,OWNERS,HudCache,ROOT,compact_raw_records,sha,CACHE_PIN

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/round_lifecycle_cap_original.json')
 ap.add_argument('--verify-fixture',type=Path)
 a=ap.parse_args()
 if a.verify_fixture:assert a.fixture.resolve()!=a.verify_fixture.resolve()
 cache=HudCache(ROOT/'assets/disc/maps/lockout.map');assert sha(cache.path)==CACHE_PIN
 versions={}
 inputs=[dict(lifecycle=2,status=status,index=31,countdown=1)for status in(0,1,2,3,-1)]
 for image in OWNERS:
  t=LifecycleTrace(image,cache);records=[]
  for row in inputs:
   t.prepare(row);before=t.record();calls=t.invoke('post_world')
   records.append(dict(before=before,after=t.record(),calls=calls))
  versions[image]=dict(image_sha256=t.spec['pin'],owners=t.phase_owners,tick_rows=records)
 data=compact_raw_records(dict(schema='original-round-lifecycle-cap-v1',boundary=__doc__,tick_inputs=inputs,
  versions=versions,lockout_sha256=sha(cache.path),instruction_patches=[],supplied_returns=[]))
 serialized=json.dumps(data,sort_keys=True,separators=(',',':'))+'\n'
 if a.verify_fixture:assert serialized==a.verify_fixture.read_text()
 a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(serialized)
 print(json.dumps(dict(completed_cap_calls=15,fixture_sha256=sha(a.fixture))))
if __name__=='__main__':main()
