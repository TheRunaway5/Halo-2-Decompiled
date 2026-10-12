#!/usr/bin/env python3
"""Freeze source CPU-owner evidence without altering earlier checkpoints."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
from audit_sound_driver_owners_native import prior_pins
OWNED=('include/halo2/sound_driver_owners.hpp','src/sound_driver_owners.cpp','tests/sound_driver_owners_tests.cpp','tools/research_sound_driver_owners_u5.py','tools/oracle_sound_driver_owners_native.py','tools/export_sound_driver_owners_fixtures.py','tools/validate_sound_driver_owners_native.py','tools/audit_sound_driver_owners_native.py','tools/freeze_sound_driver_owners_native.py','docs/MP_SOUND_DRIVER_OWNERS_NATIVE_INTEGRATION.md')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();pins=prior_pins();audit=json.loads((d/'audit.json').read_text());assert audit['prior_pins_verified']==pins
 for rel,pin in pins.items():assert sha(ROOT/rel)==pin,rel
 for rel in OWNED[:3]:assert(ROOT/rel).read_bytes()==(d/'native/source'/rel).read_bytes(),rel
 evidence={str(q.relative_to(ROOT)):dict(sha256=sha(q),bytes=q.stat().st_size)for q in d.rglob('*')if q.is_file()and q.name!='final-manifest.json'}
 source={rel:dict(sha256=sha(ROOT/rel),bytes=(ROOT/rel).stat().st_size)for rel in OWNED};inputs={str(q):dict(sha256=sha(q),bytes=q.stat().st_size)for q in(OLD,NEW)}
 packet=dict(schema='sound-driver-owners-native-frozen-checkpoint-v1',owned_current_files=source,evidence_files=evidence,prior_pins=pins,original_inputs=inputs,audit_sha256=sha(d/'audit.json'),scope='Complete statically paired CPU flush/queue/packet/static-buffer owners; genuine empty returns and explicitly controlled unavailable-storage/request boundaries. Real frozen NativeSoundStorage source directly composed. No successful nonempty voice/cache-pin/device/DSP/platform callback, shared archive freshness, audible parity or launcher claim.',whole_empty_helpers_returned=True,whole_source_driver_initialized=False,native_audio_connected=False)
 save(d/'final-manifest.json',encoded(packet))
 for group in(source,evidence,inputs):
  for rel,q in group.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==q['sha256'],rel
 print(json.dumps(dict(final_manifest_sha256=sha(d/'final-manifest.json'),owned_files=len(source),evidence_files=len(evidence),prior_pins=len(pins),inputs=len(inputs),mismatches=0)))
if __name__=='__main__':main()
