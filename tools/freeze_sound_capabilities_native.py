#!/usr/bin/env python3
"""Freeze the isolated SDK accounting checkpoint without changing old freezes."""
import json,shutil
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded
BASE=ROOT/'analysis/multiplayer/integration/sound-capabilities-native'
OWNED=('include/halo2/sound_capabilities.hpp','src/sound_capabilities.cpp','tests/sound_capabilities_tests.cpp','tools/research_sound_capabilities_u5.py','tools/oracle_sound_capabilities_native.py','tools/export_sound_capabilities_fixtures.py','tools/validate_sound_capabilities_native.py','tools/audit_sound_capabilities_native.py','tools/freeze_sound_capabilities_native.py','docs/MP_SOUND_CAPABILITIES_ACCOUNTING_RESEARCH.md')
def run():
 final=BASE/'final-manifest.json';assert not final.exists(),'Already frozen'
 assert(BASE/'audit.json').read_bytes()==(BASE/'audit-repeat.json').read_bytes();audit=json.loads((BASE/'audit.json').read_text())
 save(BASE/'reproduction.json',encoded(dict(schema='sound-capabilities-reproduction-v1',paired=['.tools/python/bin/python','tools/research_sound_capabilities_u5.py','--output','<new-paired-directory>'],original=['.tools/python/bin/python','tools/oracle_sound_capabilities_native.py','--output','<new-original-directory>'],export=['.tools/python/bin/python','tools/export_sound_capabilities_fixtures.py','--report','<new-original-directory>/report.json','--providers','analysis/multiplayer/integration/sound-capabilities-native/paired-v2/providers.json','--output','<new-fixture-directory>'],native=['.tools/python/bin/python','tools/validate_sound_capabilities_native.py','--output','<new-validation-directory>','--fixtures','analysis/multiplayer/integration/sound-capabilities-native/fixtures-v2/fixtures.tsv'],test_cli=['<copied normal or sanitizer sound_capabilities_tests>','analysis/multiplayer/integration/sound-capabilities-native/fixtures-v2/fixtures.tsv'],sanitizer_environment=dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'),canonical_inputs=dict(original='measured-v2/report.json',repeat='repeat-v2/report.json',paired='paired-v2/report.json',providers='paired-v2/providers.json',fixtures='fixtures-v2/fixtures.tsv',native='validation-final/report.json',native_repeat='validation-final-repeat/report.json'),limits='Normal source startup remains stopped at XC_AUDIO; separate raw-ABI CPU construction is not a natural DirectSoundCreate return. Recorded context/clock inputs and actual host pool rounding are explicit. Platform stores are separately retained, not claimed as native mutex representation.')))
 snapshots={}
 for name in OWNED:
  source=ROOT/name;h=sha(source);target=BASE/'owned-source'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target);assert sha(source)==sha(target)==h;snapshots[name]=h
 evidence={str(p.relative_to(ROOT)):dict(sha256=sha(p),bytes=p.stat().st_size)for p in sorted(BASE.rglob('*'))if p.is_file()and p!=final}
 for name,h in audit['protected_prior_pins'].items():assert sha(ROOT/name)==h,('Prior pin changed',name)
 result=dict(schema='sound-capabilities-accounting-final-v1',owned_current_files=snapshots,evidence_files=evidence,protected_prior_pins=audit['protected_prior_pins'],audit_sha256=sha(BASE/'audit.json'),cases=10,ordered_source_stores=20537,whole_output_bytes=450648,complete_u5_bodies=27,actual_OS_allocations_per_native_run=2,real_recursive_mutex_requests_per_native_run=14,normal_and_actual_asan_ubsan=True,source_initialized_78=0,shared_build_cmake_originaldata_runtime_edits=False,native_live_audio_connected=False)
 save(final,encoded(result))
 for p in BASE.rglob('*'):
  if p.is_file():p.chmod(p.stat().st_mode&~0o222)
 for name,h in snapshots.items():assert sha(ROOT/name)==h
 for name,v in evidence.items():assert sha(ROOT/name)==v['sha256']
 print(json.dumps(dict(manifest_sha256=sha(final),owned_files=len(snapshots),evidence_files=len(evidence),prior_pins=len(audit['protected_prior_pins']))))
if __name__=='__main__':run()
