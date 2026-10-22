#!/usr/bin/env python3
"""Freeze complete admission CPU code with exact unavailable/query evidence."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
from audit_sound_admission_native import prior_pins
OWNED=('include/halo2/sound_admission.hpp','src/audio/sound_admission.cpp','tests/sound_admission_tests.cpp','tools/research_sound_admission_u5.py','tools/oracle_sound_admission_native.py','tools/export_sound_admission_fixtures.py','tools/validate_sound_admission_native.py','tools/validate_sound_admission_production.py','tools/audit_sound_admission_native.py','tools/freeze_sound_admission_native.py','docs/MP_SOUND_ADMISSION_NATIVE_INTEGRATION.md')
SELECTED=('paired-v2','paired-repeat','measured-v5','measured-repeat','fixtures','export-repeat','validation-v2','validation-v2-repeat','production')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();pins=prior_pins();audit=json.loads((d/'audit.json').read_text());assert audit['prior_pins_verified']==pins
 for rel,pin in pins.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
 for rel in OWNED[:3]:
  for group in('validation-v2/native/source','validation-v2-repeat/native/source','production/source'):assert(ROOT/rel).read_bytes()==(d/group/rel).read_bytes(),rel
 files=[p for group in SELECTED for p in(d/group).rglob('*')if p.is_file()]+[d/'audit.json',d/'measured/instructions/actual14cc00-preparation.asm']
 evidence={str(p.relative_to(ROOT)):dict(sha256=sha(p),bytes=p.stat().st_size)for p in files};owned={rel:dict(sha256=sha(ROOT/rel),bytes=(ROOT/rel).stat().st_size)for rel in OWNED}
 inputs={str(p):dict(sha256=sha(p),bytes=p.stat().st_size)for p in(OLD,NEW)}
 packet=dict(schema='sound-admission-frozen-v1',owned_current_files=owned,evidence_files=evidence,prior_pins=pins,original_inputs=inputs,audit_sha256=sha(d/'audit.json'),successful_full_admission=False,source_readiness_listener_voice_or_platform_results_supplied=[],cmake_registration_observed=True,scope='Complete128380 CPU owner/dependencies with mandatory actual24BE30 scene service;276 genuine unavailable full calls,138 explicit RNG/chance slices,3450 direct authored/numeric queries. Actual14CC00 storage constructor,46owned map/imagebanks; no live event consumer, admitted voice or audible reload claim.')
 save(d/'final-manifest.json',encoded(packet))
 for group in(owned,evidence,inputs):
  for rel,q in group.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==q['sha256']
 for p in files+[d/'final-manifest.json']:p.chmod(p.stat().st_mode&~0o222)
 print(json.dumps(dict(manifest_sha256=sha(d/'final-manifest.json'),owned_files=len(owned),evidence_files=len(evidence),prior_pins=len(pins),mismatches=0)))
if __name__=='__main__':main()
