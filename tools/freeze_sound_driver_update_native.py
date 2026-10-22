#!/usr/bin/env python3
"""Freeze exact driver-update stop contracts and observed production linkage."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
from audit_sound_driver_update_native import prior_pins
OWNED=('include/halo2/sound_driver_update.hpp','src/audio/sound_driver_update.cpp','tests/sound_driver_update_tests.cpp','tools/research_sound_driver_update_u5.py','tools/oracle_sound_driver_update_native.py','tools/export_sound_driver_update_fixtures.py','tools/validate_sound_driver_update_native.py','tools/validate_sound_driver_update_production.py','tools/audit_sound_driver_update_native.py','tools/freeze_sound_driver_update_native.py','docs/MP_SOUND_DRIVER_UPDATE_NATIVE_INTEGRATION.md')
SELECTED=('paired','paired-repeat','measured-v2','repeated-v2','validation-v2','validation-v2-repeat','production-v2','export-repeat')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();pins=prior_pins();audit=json.loads((d/'audit.json').read_text());assert audit['prior_pins_verified']==pins
    for rel,pin in pins.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
    for rel in OWNED[:3]:
        for name in('validation-v2','validation-v2-repeat','production-v2'):assert(ROOT/rel).read_bytes()==(d/name/('source'if name=='production-v2'else'native/source')/rel).read_bytes(),rel
    files=[p for name in SELECTED for p in(d/name).rglob('*')if p.is_file()]+[d/name for name in('fixtures.tsv','fixtures.json','audit.json')]
    evidence={str(p.relative_to(ROOT)):dict(sha256=sha(p),bytes=p.stat().st_size)for p in files};owned={rel:dict(sha256=sha(ROOT/rel),bytes=(ROOT/rel).stat().st_size)for rel in OWNED};inputs={str(p):dict(sha256=sha(p),bytes=p.stat().st_size)for p in(OLD,NEW)}
    packet=dict(schema='sound-driver-update-frozen-v1',owned_current_files=owned,evidence_files=evidence,prior_pins=pins,original_inputs=inputs,audit_sha256=sha(d/'audit.json'),platform_results=[],successful_active_update_or_status_supplied=False,cmake_registration_observed_in_snapshot=True,scope='Complete220D10/193020 CPU caller contracts, mandatory real section/DSP/device/status and source callees.16 actual original1.1/u5 driver-prefix rawABI compositions stop before first persistent store; focused productionnormal+ASanUBSan includes frozen lifecycle. No fabricated startup/admission/voice or whole-tree freshness claim.')
    save(d/'final-manifest.json',encoded(packet))
    for group in(owned,evidence,inputs):
        for rel,q in group.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==q['sha256']
    for p in files+[d/'final-manifest.json']:p.chmod(p.stat().st_mode&~0o222)
    print(json.dumps(dict(manifest_sha256=sha(d/'final-manifest.json'),owned_files=len(owned),evidence_files=len(evidence),prior_pins=len(pins),original_inputs=len(inputs),mismatches=0)))
if __name__=='__main__':main()
