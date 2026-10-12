#!/usr/bin/env python3
"""Pin this separate CPU startup checkpoint without replacing earlier evidence."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
from oracle_smg_audio_rng_startup_1_1 import FROZEN

OWNED=('include/halo2/sound_startup.hpp','src/sound_startup.cpp','tests/sound_startup_tests.cpp',
 'tools/research_sound_startup_u5.py','tools/oracle_sound_startup_native.py',
 'tools/export_sound_startup_fixtures.py','tools/audit_sound_startup_native.py',
 'tools/freeze_sound_startup_native.py','docs/MP_SOUND_STARTUP_NATIVE_INTEGRATION.md')

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve()
 audit=json.loads((d/'audit.json').read_text());assert audit['prior_pins_verified']==FROZEN
 for rel,pin in FROZEN.items():assert sha(ROOT/rel)==pin,rel
 for rel in('include/halo2/sound_startup.hpp','src/sound_startup.cpp','tests/sound_startup_tests.cpp'):
  assert (ROOT/rel).read_bytes()==(d/'native/source'/rel).read_bytes(),rel
 evidence={str(q.relative_to(ROOT)):dict(sha256=sha(q),bytes=q.stat().st_size)for q in d.rglob('*')if q.is_file()and q.name!='final-manifest.json'}
 source={rel:dict(sha256=sha(ROOT/rel),bytes=(ROOT/rel).stat().st_size)for rel in OWNED}
 inputs={str(q):dict(sha256=sha(q),bytes=q.stat().st_size)for q in(OLD,NEW)}
 packet=dict(schema='source-sound-startup-native-frozen-checkpoint-v1',owned_current_files=source,evidence_files=evidence,
     prior_pins=FROZEN,original_inputs=inputs,audit_sha256=sha(d/'audit.json'),
     scope='Observed standalone source/normal/GCC sanitizer artifacts only. No current shared archive, release or complete successful native cache/device/map startup is claimed. Canonical report is original whole-caller stop plus independently bounded original CPU commit fragment. Preliminary pre-pinning records stay separately preserved.',
     native_audio_connected=False,original_full_device_startup_accepted=False)
 save(d/'final-manifest.json',encoded(packet))
 for group in(source,evidence,inputs):
  for rel,entry in group.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==entry['sha256'],rel
 for rel,pin in FROZEN.items():assert sha(ROOT/rel)==pin
 print(json.dumps(dict(final_manifest_sha256=sha(d/'final-manifest.json'),owned_files=len(source),evidence_files=len(evidence),prior_pins=len(FROZEN),inputs=len(inputs),mismatches=0)))

if __name__=='__main__':main()
