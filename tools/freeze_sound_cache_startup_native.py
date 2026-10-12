#!/usr/bin/env python3
"""Pin this independent complete cache owner without changing older freezes."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
from audit_sound_cache_startup_native import prior_pins

OWNED=('include/halo2/sound_cache_startup.hpp','src/sound_cache_startup.cpp','tests/sound_cache_startup_tests.cpp',
 'tools/research_sound_cache_startup_u5.py','tools/oracle_sound_cache_startup_native.py',
 'tools/export_sound_cache_startup_fixtures.py','tools/validate_sound_cache_startup_native.py',
 'tools/audit_sound_cache_startup_native.py','tools/freeze_sound_cache_startup_native.py',
 'docs/MP_SOUND_CACHE_STARTUP_NATIVE_INTEGRATION.md')

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve()
 audit=json.loads((d/'audit.json').read_text());pins=prior_pins();assert audit['prior_pins_verified']==pins
 for rel,pin in pins.items():assert sha(ROOT/rel)==pin,rel
 for rel in('include/halo2/sound_cache_startup.hpp','src/sound_cache_startup.cpp','tests/sound_cache_startup_tests.cpp','include/halo2/sound_startup.hpp','src/sound_startup.cpp'):
  assert (ROOT/rel).read_bytes()==(d/'native/source'/rel).read_bytes(),rel
 evidence={str(q.relative_to(ROOT)):dict(sha256=sha(q),bytes=q.stat().st_size)for q in d.rglob('*')if q.is_file()and q.name!='final-manifest.json'}
 source={rel:dict(sha256=sha(ROOT/rel),bytes=(ROOT/rel).stat().st_size)for rel in OWNED}
 inputs={str(q):dict(sha256=sha(q),bytes=q.stat().st_size)for q in(OLD,NEW)}
 packet=dict(schema='source-sound-cache-startup-native-frozen-checkpoint-v1',owned_current_files=source,evidence_files=evidence,
   prior_pins=pins,original_inputs=inputs,audit_sha256=sha(d/'audit.json'),
   scope='Observed standalone exact source/normal/GCC sanitizer cache constructor and portable startup composition. Earlier source image/startup freezes remain unchanged. No current shared archive freshness, source activation/async worker/read completion, successful real platform callback, admitted voice or runtime audio claim.',
   native_audio_connected=False,source_device_constructor_completed=False)
 save(d/'final-manifest.json',encoded(packet))
 for group in(source,evidence,inputs):
  for rel,row in group.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==row['sha256'],rel
 for rel,pin in pins.items():assert sha(ROOT/rel)==pin,rel
 print(json.dumps(dict(final_manifest_sha256=sha(d/'final-manifest.json'),owned_files=len(source),evidence_files=len(evidence),prior_pins=len(pins),inputs=len(inputs),mismatches=0)))

if __name__=='__main__':main()
