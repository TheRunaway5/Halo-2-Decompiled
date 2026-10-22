#!/usr/bin/env python3
"""Freeze bounded live kill-trigger momentum regression and source generations."""
import json
from pathlib import Path
import freeze_mp_spartan_death_physical as archive
ROOT=archive.ROOT;BASE=ROOT/'analysis/multiplayer/integration/kill-trigger-momentum-native-20261010'
def main():
 r=json.loads((BASE/'validation-report.json').read_text());negative=json.loads((BASE/'normal-red-report.json').read_text())
 assert negative['cases'][0]['returncode']==0 and negative['cases'][1]['returncode']==1
 assert all(r[k]for k in('production_sanitizer_flags_verified','actual_asan_and_ubsan_symbols_present','asan_u5_direct_pass','asan_map_pack_11_CTest_pass',
  'normal_production_archives_pass','isolated_old_clear_red','root_coherent_revision4_production_u5_pass','concurrent_source_generations_kept_separate'))
 assert archive.sha(BASE/'input/src/gameplay/multiplayer/multiplayer.cpp')==r['normal_and_root_revision4_source_sha256']
 assert archive.sha(BASE/'sanitizer-source/src/gameplay/multiplayer/multiplayer.cpp')==r['later_sanitizer_source_sha256']
 assert archive.sha(BASE/'asan-test')==r['sanitizer_binary_sha256']
 assert archive.sha(ROOT/'tests/multiplayer_kill_trigger_momentum_tests.cpp')==r['test_sha256']
 paths=[ROOT/'tools/freeze_mp_kill_trigger_momentum_native.py',ROOT/'docs/MP_KILL_TRIGGER_MOMENTUM_NATIVE_RESEARCH.md',ROOT/'tests/multiplayer_kill_trigger_momentum_tests.cpp',
  ROOT/'analysis/multiplayer/integration/spartan-kill-trigger-sound-20261010/frozen/manifest.json']
 for folder in('input','sanitizer-source'):paths += [p for p in(BASE/folder).rglob('*')if p.is_file()and p.suffix not in('.a','.o')]
 paths += [p for p in BASE.iterdir()if p.is_file()and p.suffix in('.log','.json','.txt','.diff','.cpp')]
 paths += [BASE/n for n in('normal-test','old-clear-test','asan-test','root-revision4-test')]
 archive.freeze(BASE/'frozen',paths,r)
if __name__=='__main__':main()
