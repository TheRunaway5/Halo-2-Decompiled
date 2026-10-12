#!/usr/bin/env python3
"""Append a new immutable source promotion checkpoint without prior edits."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
from audit_sound_promotion_native import prior_pins
OWNED=('include/halo2/sound_promotion.hpp','src/sound_promotion.cpp','tests/sound_promotion_tests.cpp','tools/research_sound_promotion_u5.py','tools/oracle_sound_promotion_native.py','tools/export_sound_promotion_fixtures.py','tools/validate_sound_promotion_native.py','tools/audit_sound_promotion_native.py','tools/freeze_sound_promotion_native.py','docs/MP_SOUND_PROMOTION_NATIVE_INTEGRATION.md')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();pins=prior_pins();audit=json.loads((d/'audit.json').read_text());assert audit['prior_pins_verified']==pins
    for rel,pin in pins.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
    for rel in OWNED[:3]:assert(ROOT/rel).read_bytes()==(d/'native/source'/rel).read_bytes(),rel
    evidence={str(p.relative_to(ROOT)):dict(sha256=sha(p),bytes=p.stat().st_size)for p in d.rglob('*')if p.is_file()and p.name!='final-manifest.json'}
    source={rel:dict(sha256=sha(ROOT/rel),bytes=(ROOT/rel).stat().st_size)for rel in OWNED}
    report=json.loads((d/'measured/report.json').read_text());paths={OLD,NEW}|{Path(q['map']['path'])for q in report['maps']}
    inputs={str(p):dict(sha256=sha(p),bytes=p.stat().st_size)for p in paths}
    packet=dict(schema='sound-promotion-native-frozen-checkpoint-v1',owned_current_files=source,evidence_files=evidence,prior_pins=pins,original_inputs=inputs,audit_sha256=sha(d/'audit.json'),scope='Complete128610 promotion-selector and128140 clock CPU ownership; actual23owned serialized banks, original constructor/map-prefix clock origin and real measured OS tick replay. Full ordinary clock and positive counter/selector/cooldown/aging outputs. No full map/device/admission/permutation/voice/playback or campaign-fade validation claim.',native_audio_connected=False)
    save(d/'final-manifest.json',encoded(packet))
    for group in (source,evidence,inputs):
        for rel,q in group.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==q['sha256']
    print(json.dumps(dict(final_manifest_sha256=sha(d/'final-manifest.json'),owned_files=len(source),evidence_files=len(evidence),prior_pins=len(pins),original_inputs=len(inputs),mismatches=0)))
if __name__=='__main__':main()
