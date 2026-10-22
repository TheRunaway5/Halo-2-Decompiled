#!/usr/bin/env python3
"""Append a separately pinned creation-owner checkpoint; no prior rewrites."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded
from audit_sound_create_native import prior_pins
OWNED=('include/halo2/sound_create.hpp','src/audio/sound_create.cpp','tests/sound_create_tests.cpp','tools/research_sound_create_u5.py','tools/oracle_sound_create_native.py','tools/export_sound_create_fixtures.py','tools/validate_sound_create_native.py','tools/reproduce_sound_create_native.py','tools/audit_sound_create_native.py','tools/freeze_sound_create_native.py','docs/MP_SOUND_CREATE_NATIVE_INTEGRATION.md')
SELECTED=('paired-v2','paired-repeat','measured','repeated','native','native-repeat-run','export-repeat','reproduction-logs')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();pins=prior_pins();audit=json.loads((d/'audit.json').read_text());assert audit['prior_pins_verified']==pins
 for rel,pin in pins.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
 for rel in OWNED[:3]:assert(ROOT/rel).read_bytes()==(d/'native/source'/rel).read_bytes(),rel
 files=[p for name in SELECTED for p in(d/name).rglob('*')if p.is_file()]+[d/name for name in('fixtures.tsv','fixtures.json','audit.json','reproduction.json')]
 evidence={str(p.relative_to(ROOT)):dict(sha256=sha(p),bytes=p.stat().st_size)for p in files}
 owned={rel:dict(sha256=sha(ROOT/rel),bytes=(ROOT/rel).stat().st_size)for rel in OWNED}
 r=json.loads((d/'measured/report.json').read_text());inputs={str(p):dict(sha256=sha(p),bytes=p.stat().st_size)for p in{OLD,NEW}|{Path(q['map']['path'])for q in r['maps']}}
 packet=dict(schema='sound-create-native-frozen-checkpoint-v1',owned_current_files=owned,evidence_files=evidence,prior_pins=pins,original_inputs=inputs,audit_sha256=sha(d/'audit.json'),
  scope='Whole127750 CPU creation implementation with mandatory named cache/effect/controller services;138 actually executed directrawABI prefixes across23ownedmaps/twoimages. Whole returned creation, original128380admission, driver/cache synchronization/file completion/voice/playback are not established.',native_audio_connected=False)
 save(d/'final-manifest.json',encoded(packet))
 for group in(owned,evidence,inputs):
  for rel,q in group.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==q['sha256']
 for p in files+[d/'final-manifest.json']:p.chmod(p.stat().st_mode&~0o222)
 print(json.dumps(dict(final_manifest_sha256=sha(d/'final-manifest.json'),owned_files=len(owned),evidence_files=len(evidence),prior_pins=len(pins),original_inputs=len(inputs),mismatches=0)))
if __name__=='__main__':main()
