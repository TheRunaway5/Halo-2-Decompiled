#!/usr/bin/env python3
"""Freeze independently audited cache ownership without prior rewrites."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
from audit_sound_cache_request_native import prior_pins
OWNED=('include/halo2/sound_cache_request.hpp','src/audio/sound_cache_request.cpp','tests/sound_cache_request_tests.cpp','tools/research_sound_cache_request_u5.py','tools/oracle_sound_cache_request_native.py','tools/export_sound_cache_request_fixtures.py','tools/validate_sound_cache_request_native.py','tools/audit_sound_cache_request_native.py','tools/freeze_sound_cache_request_native.py','docs/MP_SOUND_CACHE_REQUEST_NATIVE_INTEGRATION.md')
SELECTED=('paired','paired-repeat','measured','repeated','validation-v2','validation-v2-repeat','export-repeat')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();pins=prior_pins();audit=json.loads((d/'audit.json').read_text());assert audit['prior_pins_verified']==pins
 for rel,pin in pins.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
 for rel in OWNED[:3]:assert(ROOT/rel).read_bytes()==(d/'validation-v2/native/source'/rel).read_bytes(),rel
 files=[p for name in SELECTED for p in(d/name).rglob('*')if p.is_file()]+[d/name for name in('fixtures.tsv','fixtures.json','audit.json')]
 evidence={str(p.relative_to(ROOT)):dict(sha256=sha(p),bytes=p.stat().st_size)for p in files};owned={rel:dict(sha256=sha(ROOT/rel),bytes=(ROOT/rel).stat().st_size)for rel in OWNED}
 shared=Path(json.loads((d/'measured/report.json').read_text())['shared_file']['path']);inputs={str(p):dict(sha256=sha(p),bytes=p.stat().st_size)for p in(OLD,NEW,ROOT/'assets/disc/maps/lockout.map',shared)}
 packet=dict(schema='sound-cache-request-native-frozen-v1',owned_current_files=owned,evidence_files=evidence,prior_pins=pins,original_inputs=inputs,audit_sha256=sha(d/'audit.json'),native_platform_adaptation=True,source_admission_voice_or_thread=False,
  scope='Complete21A960 CPU/cache/page owner and mandatory async_read.3 original actual127750 frame compositions, real mutex/semaphore, actual owned shared.map IO, source worker done. No original thread/TLS/admission/device/voice/playback return supplied; executed1.1, fullbodyu5staticcorrespondence.')
 save(d/'final-manifest.json',encoded(packet))
 for group in(owned,evidence,inputs):
  for rel,q in group.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==q['sha256']
 for p in files+[d/'final-manifest.json']:p.chmod(p.stat().st_mode&~0o222)
 print(json.dumps(dict(final_manifest_sha256=sha(d/'final-manifest.json'),owned_files=len(owned),evidence_files=len(evidence),prior_pins=len(pins),original_inputs=len(inputs),mismatches=0)))
if __name__=='__main__':main()
