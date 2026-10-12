#!/usr/bin/env python3
"""Freeze the independently audited lifecycle checkpoint without old rewrites."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
from audit_sound_cache_lifecycle_native import prior_pins
OWNED=('include/halo2/sound_cache_lifecycle.hpp','src/sound_cache_lifecycle.cpp','tests/sound_cache_lifecycle_tests.cpp','tools/research_sound_cache_lifecycle_u5.py','tools/oracle_sound_cache_lifecycle_native.py','tools/export_sound_cache_lifecycle_fixtures.py','tools/validate_sound_cache_lifecycle_native.py','tools/audit_sound_cache_lifecycle_native.py','tools/freeze_sound_cache_lifecycle_native.py','docs/MP_SOUND_CACHE_LIFECYCLE_NATIVE_INTEGRATION.md')
SELECTED=('paired','paired-repeat','measured','repeated','validation-v2','validation-v2-repeat','export-repeat')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();pins=prior_pins();audit=json.loads((d/'audit.json').read_text());assert audit['prior_pins_verified']==pins
    for rel,pin in pins.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
    for rel in OWNED[:3]:
        for name in('validation-v2','validation-v2-repeat'):assert(ROOT/rel).read_bytes()==(d/name/'native/source'/rel).read_bytes(),rel
    files=[p for name in SELECTED for p in(d/name).rglob('*')if p.is_file()]+[d/name for name in('fixtures.tsv','fixtures.json','audit.json')]
    evidence={str(p.relative_to(ROOT)):dict(sha256=sha(p),bytes=p.stat().st_size)for p in files}
    owned={rel:dict(sha256=sha(ROOT/rel),bytes=(ROOT/rel).stat().st_size)for rel in OWNED}
    inputs={str(p):dict(sha256=sha(p),bytes=p.stat().st_size)for p in(OLD,NEW,ROOT/'assets/disc/maps/lockout.map',ROOT/'assets/disc/maps/shared.map')}
    packet=dict(schema='sound-cache-lifecycle-frozen-v1',owned_current_files=owned,evidence_files=evidence,prior_pins=pins,original_inputs=inputs,audit_sha256=sha(d/'audit.json'),source_availability_assignments=[],active_audio_or_concurrent_claim=False,scope='Complete21A920/13E9A0/21AA60/21A800 CPU lifecycle and boundedcold127290; actual cached unpinned state and measured real OS time, no active-driver or live admission shortcut.')
    save(d/'final-manifest.json',encoded(packet))
    for group in(owned,evidence,inputs):
        for rel,q in group.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==q['sha256']
    for p in files+[d/'final-manifest.json']:p.chmod(p.stat().st_mode&~0o222)
    print(json.dumps(dict(manifest_sha256=sha(d/'final-manifest.json'),owned_files=len(owned),evidence_files=len(evidence),prior_pins=len(pins),original_inputs=len(inputs),mismatches=0)))
if __name__=='__main__':main()
