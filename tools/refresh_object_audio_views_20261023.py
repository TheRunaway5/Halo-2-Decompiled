#!/usr/bin/env python3
"""Refresh only the declared native camera input seam in the active owned cohort."""
import hashlib,json,os
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/combat-presentation-audit-20261022'
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    follow=BASE/'repair-followup-20261023';pins=json.loads((follow/'source-hashes.json').read_text())
    assert source_hashes(source)==pins
    changes={}
    def save(rel,data):
        path=source/rel;before=path.read_bytes() if path.exists() else b''
        if before==data:return
        path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
        changes[rel]=dict(before=sha(before) if before else None,after=sha(data))
    for rel in ('include/halo2/source_object_audio_views_tu15.hpp',
                'include/halo2/source_object_loop_audio_tu15.hpp',
                'src/audio/source_object_loop_audio_tu15_generated.cpp',
                'tools/generate_object_loop_audio_tu15.py','tools/object_audio_views_tu15.py',
                'include/halo2/source_loop_track_transition_tu15.hpp',
                'src/audio/source_loop_track_transition_tu15.cpp'):
        save(rel,(ROOT/rel).read_bytes())
    rel='include/halo2/physics_live_world_tu15.hpp';old=(source/rel).read_text();root=(ROOT/rel).read_text()
    needle='#include "halo2/physics_world_runtime.hpp"';assert old.count(needle)==1
    old=old.replace(needle,needle+'\n#include "halo2/source_object_audio_views_tu15.hpp"')
    start=root.index('    // Native presentation\'s actual local camera inputs.')
    end=root.index('    void observe_effect_light_births(',start)
    needle='    void observe_effect_light_births(';assert old.count(needle)==1
    old=old.replace(needle,root[start:end]+needle);save(rel,old.encode())
    rel='src/physics/physics_live_world_tu15.cpp';old=(source/rel).read_text();root=(ROOT/rel).read_text()
    start=root.index('void PhysicsLiveWorldTu15::step_native_object_audio(')
    end=root.index('\n}\n',start)+3
    needle='void PhysicsLiveWorldTu15::observe_loop_sound_requests('
    anchor=old.index('\n}\n',old.index(needle))+3
    assert 'void PhysicsLiveWorldTu15::step_native_object_audio(' not in old
    save(rel,(old[:anchor]+root[start:end]+old[anchor:]).encode())
    rel='CMakeLists.txt';old=(source/rel).read_text();root=(ROOT/rel).read_text()
    start=root.index('target_sources(halo2_systems PRIVATE src/audio/source_loop_track_transition_tu15.cpp)')
    end=root.index('\nendif()',start)+len('\nendif()')
    anchor='target_sources(halo2_systems PRIVATE src/ui/menu_music.cpp src/audio/sound_fade.cpp)'
    assert old.count(anchor)==1;save(rel,old.replace(anchor,anchor+'\n'+root[start:end]).encode())
    for rel in ('tests/source_loop_track_transition_tu15_tests.cpp',
                'tests/fixtures/loop_track_transition_tu15.json'):
        save(rel,(ROOT/rel).read_bytes())
    updated=source_hashes(source)
    assert {p for p in updated if updated[p]!=pins.get(p)}=={p for p in changes if p in updated}
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    out=BASE/'repair-scenery-audio/native-view-input-refresh.json';assert not out.exists()
    receipt=dict(scope=__doc__,changes=changes,original_readiness_or_admission_supplied=False,
                 native_PCM_verified=False,runtime_installed=False)
    out.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
if __name__=='__main__':main()
