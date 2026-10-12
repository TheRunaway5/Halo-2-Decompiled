#!/usr/bin/env python3
"""Freeze the isolated map owner and stops without changing previous evidence."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
from oracle_sound_map_activation_native import CACHE
from audit_sound_map_activation_native import prior_pins
OWNED=('include/halo2/sound_map_activation.hpp','src/sound_map_activation.cpp','tests/sound_map_activation_tests.cpp','tools/research_sound_map_activation_u5.py','tools/oracle_sound_map_activation_native.py','tools/export_sound_map_activation_fixtures.py','tools/validate_sound_map_activation_native.py','tools/audit_sound_map_activation_native.py','tools/freeze_sound_map_activation_native.py','docs/MP_SOUND_MAP_ACTIVATION_NATIVE_INTEGRATION.md')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();pins=prior_pins();audit=json.loads((d/'audit.json').read_text());assert audit['prior_pins_verified']==pins
 for rel,pin in pins.items():assert sha(ROOT/rel)==pin,rel
 for rel in OWNED[:3]:assert(ROOT/rel).read_bytes()==(d/'native/source'/rel).read_bytes(),rel
 evidence={str(q.relative_to(ROOT)):dict(sha256=sha(q),bytes=q.stat().st_size)for q in d.rglob('*')if q.is_file()and q.name!='final-manifest.json'}
 source={rel:dict(sha256=sha(ROOT/rel),bytes=(ROOT/rel).stat().st_size)for rel in OWNED};inputs={str(q):dict(sha256=sha(q),bytes=q.stat().st_size)for q in(OLD,NEW,CACHE)}
 packet=dict(schema='sound-map-activation-native-frozen-checkpoint-v1',owned_current_files=source,evidence_files=evidence,prior_pins=pins,original_inputs=inputs,audit_sha256=sha(d/'audit.json'),scope='Fresh standalone observed source snapshots/binaries with exact original stopped126DE0 prefix comparison. Complete static CPU owner retains mandatory unsupported native routing/effects. No shared archive freshness, complete map activation, current-map publication, driver initialization, virtual voice admission, audible parity or live launcher claim.',whole_original_map_owner_returned=False,native_audio_connected=False)
 save(d/'final-manifest.json',encoded(packet))
 for group in(source,evidence,inputs):
  for rel,q in group.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==q['sha256'],rel
 print(json.dumps(dict(final_manifest_sha256=sha(d/'final-manifest.json'),owned_files=len(source),evidence_files=len(evidence),prior_pins=len(pins),inputs=len(inputs),mismatches=0)))
if __name__=='__main__':main()
