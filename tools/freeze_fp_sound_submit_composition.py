#!/usr/bin/env python3
"""Freeze the isolated composed FP/world/raw-submit checkpoint once."""
import json,hashlib,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/fp-sound-submit-composition'
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(8<<20),b''):h.update(b)
 return h.hexdigest()
def encoded(v):return(json.dumps(v,sort_keys=True,indent=2)+'\n').encode()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,('Existing freeze file differs',str(p))
 else:p.write_bytes(b)
def main():
 final=BASE/'final-manifest.json';assert not final.exists(),'Already frozen'
 audit=json.loads((BASE/'audit.json').read_text());assert(BASE/'audit.json').read_bytes()==(BASE/'audit-repeat.json').read_bytes()
 owned=['include/halo2/fp_sound_submit.hpp','src/gameplay/weapons/fp_sound_submit.cpp','tests/fp_sound_submit_tests.cpp','tools/oracle_fp_sound_submit_composition.py','tools/export_fp_sound_submit_composition.py','tools/validate_fp_sound_submit_composition.py','tools/audit_fp_sound_submit_composition.py','tools/freeze_fp_sound_submit_composition.py','docs/MP_FP_SOUND_RAW_SUBMIT_COMPOSITION.md']
 reviewed=['include/halo2/simulation.hpp','include/halo2/multiplayer.hpp','src/gameplay/multiplayer/multiplayer.cpp','src/gameplay/multiplayer/multiplayer_runtime.cpp','include/halo2/sdl_sound_platform.hpp','src/audio/sdl_sound_platform.cpp','include/halo2/sound_driver_startup.hpp','src/audio/sound_driver_startup.cpp','analysis/multiplayer/map-pack/smg-audio-startup/reference/cxbx-585c49a/XbDSoundTypes.h','analysis/multiplayer/map-pack/smg-audio-startup/reference/cxbx-585c49a/DirectSound/DirectSound.cpp']
 observed={}
 for name in reviewed:
  source=ROOT/name;before=sha(source);target=BASE/'reviewed-source'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target);assert sha(source)==sha(target)==before;observed[name]=dict(sha256=before,snapshot=str(target.relative_to(ROOT)))
 save(BASE/'reviewed-source.json',encoded(dict(schema='observed-individual-source-snapshots-v1',files=observed,scope='Individual stable read-only source observations supporting live/provider limits. These are snapshots, not immutable pins on future shared source and not a coherent full-tree build claim.')))
 reproduction=dict(schema='fp-raw-submit-reproduction-v1',original=['.tools/python/bin/python','tools/oracle_fp_sound_submit_composition.py','--output','<new-output-directory>'],export=['.tools/python/bin/python','tools/export_fp_sound_submit_composition.py','<new-report.json>','<new-fixtures.tsv>'],native_test=['<copied normal or sanitizer fp_sound_submit_tests>','analysis/multiplayer/integration/fp-sound-submit-composition/fixtures-final/fixtures.tsv','assets/disc/maps/lockout.map','assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'],sanitizer_environment=dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1'),exact_build_and_archive_commands=['validation-final/commands.json','validation-repeat-final/commands.json'],instruction_authority='Previous pinned complete original old/u5 bodies; fresh unchanged 1.1 schedule execution; no native expected-output generator.',limits='Inherited ready/local/world/clock fixture boundaries remain explicit. Native raw attention positive path remains mandatory and unexecuted. No live audio admission/playback, source startup completion or Xbox/SDL capability equivalence.')
 save(BASE/'reproduction.json',encoded(reproduction))
 evidence={str(p.relative_to(ROOT)):dict(sha256=sha(p),bytes=p.stat().st_size)for p in sorted(BASE.rglob('*'))if p.is_file()and p!=final}
 owned_pins={name:sha(ROOT/name)for name in owned};prior=audit['protected_prior_pins']
 for name,h in prior.items():assert sha(ROOT/name)==h,('Prior file changed during freeze',name)
 result=dict(schema='fp-world-raw-submit-composition-final-v1',owned_current_files=owned_pins,evidence_files=evidence,protected_prior_pins=prior,audit_sha256=sha(BASE/'audit.json'),cases=3,complete_world_stores=30,raw_constructor_stores=99,source_reply_substitutions=0,normal_and_actual_asan_ubsan=True,shared_build_or_cmake_edits=False,native_live_audio_connected=False)
 save(final,encoded(result))
 for path in BASE.rglob('*'):
  if path.is_file():path.chmod(path.stat().st_mode&~0o222)
 for name,h in owned_pins.items():assert sha(ROOT/name)==h
 for name,v in evidence.items():assert sha(ROOT/name)==v['sha256']
 print(json.dumps(dict(manifest_sha256=sha(final),owned_files=len(owned_pins),evidence_files=len(evidence),prior_pins=len(prior))))
if __name__=='__main__':main()
