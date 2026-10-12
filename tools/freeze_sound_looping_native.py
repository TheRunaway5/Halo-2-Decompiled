#!/usr/bin/env python3
"""Freeze bounded complete loop storage/registration ownership evidence."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
from audit_sound_looping_native import prior_pins
OWNED=('include/halo2/sound_looping.hpp','src/sound_looping.cpp','tests/sound_looping_tests.cpp','tools/research_sound_looping_u5.py','tools/oracle_sound_looping_preparation.py','tools/oracle_sound_looping_native.py','tools/export_sound_looping_fixtures.py','tools/validate_sound_looping_native.py','tools/validate_sound_looping_production.py','tools/audit_sound_looping_native.py','tools/freeze_sound_looping_native.py','docs/MP_SOUND_LOOPING_NATIVE_INTEGRATION.md')
SELECTED=('paired-v5','paired-repeat','preparation-v3','preparation-repeat','measured-v5','measured-repeat-v2','fixtures-v3','export-repeat','validation-v2','validation-repeat','production')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();pins=prior_pins();audit=json.loads((d/'audit.json').read_text());assert audit['prior_pins_verified']==pins
 for rel,pin in pins.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
 files=[p for name in SELECTED for p in(d/name).rglob('*')if p.is_file()]+[d/'audit.json'];evidence={str(p.relative_to(ROOT)):dict(sha256=sha(p),bytes=p.stat().st_size)for p in files};owned={rel:dict(sha256=sha(ROOT/rel),bytes=(ROOT/rel).stat().st_size)for rel in OWNED};inputs={str(p):dict(sha256=sha(p),bytes=p.stat().st_size)for p in(OLD,NEW)}
 report=dict(schema='sound-looping-native-frozen-v1',owned_current_files=owned,evidence_files=evidence,prior_pins=pins,original_inputs=inputs,audit_sha256=sha(d/'audit.json'),actual_grenade_registration=True,driver_reset_naturally_returned=False,voice_or_platform_admission_supplied=False,cmake_registration_observed=True,scope='Complete original loop/class storage and registration CPU owners; actual one-tick grenade continuation has held frag/debit. Positive driver-reset case is explicit independently produced raw ABI backing. No live Q/reload/playback/launcher claim.')
 save(d/'final-manifest.json',encoded(report))
 for group in(owned,evidence,inputs):
  for rel,q in group.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==q['sha256']
 for p in files+[d/'final-manifest.json']:p.chmod(p.stat().st_mode&~0o222)
 print(json.dumps(dict(manifest_sha256=sha(d/'final-manifest.json'),owned_files=len(owned),evidence_files=len(evidence),prior_pins=len(pins),mismatches=0)))
if __name__=='__main__':main()
