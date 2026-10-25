#!/usr/bin/env python3
"""Link the existing ambience metadata test to the library that owns its BSP reader."""
import hashlib,json,os
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    follow=ROOT/'analysis/combat-presentation-audit-20261022/repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins
    path=source/'CMakeLists.txt';before=path.read_text()
    old='    target_link_libraries(mcc_looping_sound_tests PRIVATE halo2_cache)'
    new='    target_link_libraries(mcc_looping_sound_tests PRIVATE halo2_systems)'
    assert before.count(old)==1 and (ROOT/'CMakeLists.txt').read_text().count(new)==1
    after=before.replace(old,new);path.write_text(after)
    updated=source_hashes(source);assert {p for p in pins if pins[p]!=updated[p]}=={'CMakeLists.txt'}
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    receipt=dict(scope=__doc__,before_sha256=hashlib.sha256(before.encode()).hexdigest(),after_sha256=hashlib.sha256(after.encode()).hexdigest(),
        original_failure='Undefined reference to Cache::load_multiplayer_audio_bsp; no omission verdict from that failed link')
    (follow.parent/'repair-scenery-audio/detail-test-link-owner-correction.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
if __name__=='__main__':main()
