#!/usr/bin/env python3
"""Append an immutable submit checkpoint, preserving preceding source freezes."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
from audit_sound_submit_native import prior_pins
OWNED=('include/halo2/sound_submit.hpp','src/sound_submit.cpp','tests/sound_submit_tests.cpp','tools/research_sound_submit_u5.py','tools/oracle_sound_submit_native.py','tools/export_sound_submit_fixtures.py','tools/validate_sound_submit_native.py','tools/audit_sound_submit_native.py','tools/freeze_sound_submit_native.py','docs/MP_SOUND_SUBMIT_NATIVE_INTEGRATION.md')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();pins=prior_pins();audit=json.loads((d/'audit.json').read_text());assert audit['prior_pins_verified']==pins
 for rel,pin in pins.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
 for rel in OWNED[:3]:assert(ROOT/rel).read_bytes()==(d/'native/source'/rel).read_bytes(),rel
 evidence={str(p.relative_to(ROOT)):dict(sha256=sha(p),bytes=p.stat().st_size)for p in d.rglob('*')if p.is_file()and p.name!='final-manifest.json'};source={rel:dict(sha256=sha(ROOT/rel),bytes=(ROOT/rel).stat().st_size)for rel in OWNED};inputs={str(p):dict(sha256=sha(p),bytes=p.stat().st_size)for p in(OLD,NEW)}
 packet=dict(schema='sound-submit-native-frozen-checkpoint-v1',owned_current_files=source,evidence_files=evidence,prior_pins=pins,original_inputs=inputs,audit_sha256=sha(d/'audit.json'),scope='Complete raw submit/effect/controller/pool CPU bodies; source-produced cold startup arena + explicit literal current-map/raw160 ABI composition. Exact original naturally rejected outputs and actual effect allocate/bind/reject/release. No initialized/admitted voice/factory/platform success or full active-map/raw-caller/live playback claim.',native_audio_connected=False)
 save(d/'final-manifest.json',encoded(packet))
 for group in(source,evidence,inputs):
  for rel,q in group.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==q['sha256']
 print(json.dumps(dict(final_manifest_sha256=sha(d/'final-manifest.json'),owned_files=len(source),evidence_files=len(evidence),prior_pins=len(pins),inputs=len(inputs),mismatches=0)))
if __name__=='__main__':main()
