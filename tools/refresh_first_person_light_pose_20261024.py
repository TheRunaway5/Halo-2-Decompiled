#!/usr/bin/env python3
"""Admit the scoped original first-person light-pose repair into the owned cohort."""
import hashlib,json,os
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    follow=ROOT/'analysis/combat-presentation-audit-20261022/repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins
    paths=['include/halo2/source_first_person_light_pose_tu15.hpp',
        'src/rendering/effects/source_first_person_light_pose_tu15_generated.cpp',
        'src/physics/physics_live_world_tu15.cpp','include/halo2/source_bsp_effect_lights.hpp',
        'src/rendering/renderer/native_tu15_render_object_startup_data.inc',
        'src/rendering/world/source_bsp_effect_lights.cpp',
        'src/rendering/effects/world_effects.cpp']
    changes={}
    for name in paths:
        path=source/name;before=path.read_bytes() if path.exists() else None;after=(ROOT/name).read_bytes()
        if name=='src/physics/physics_live_world_tu15.cpp':
            # The live checkout also has a concurrent multiplayer event
            # observer. Project only these two light-owner insertions into
            # this frozen diagnostic cohort; preserve that unrelated work.
            text=before.decode();root=after.decode()
            line='#include "halo2/source_first_person_light_pose_tu15.hpp"\n'
            assert line in root
            if line not in text:
                anchor='#include "halo2/source_effect_light_view_tu15.hpp"\n'
                assert text.count(anchor)==1;text=text.replace(anchor,anchor+line)
            block='''        if(source_first_person_light_pose_tu15_owns_entry(c.target)) {
            source_run_first_person_light_pose_tu15(m,c.target,a,fp,services(),o);return;
        }
'''
            assert block in root
            if block not in text:
                anchor='        if(source_model_render_tu15_owns_entry(c.target)) {'
                assert text.count(anchor)==1;text=text.replace(anchor,block+anchor)
            after=text.encode()
        if before==after:continue
        if path.exists():path.unlink()
        path.write_bytes(after);changes[name]=dict(before=sha(before) if before else None,after=sha(after))
    path=source/'CMakeLists.txt';before=path.read_text()
    line='target_sources(halo2_physics_world_runtime PRIVATE src/rendering/effects/source_first_person_light_pose_tu15_generated.cpp)\n'
    assert line in (ROOT/'CMakeLists.txt').read_text()
    if line not in before:
        anchor='target_sources(halo2_physics_world_runtime PRIVATE src/rendering/models/source_model_render_tu15_generated.cpp)\n'
        assert before.count(anchor)==1
        path.write_text(before.replace(anchor,anchor+line))
    text=path.read_text();root=(ROOT/'CMakeLists.txt').read_text()
    start=root.index('if(BUILD_TESTING)\n    add_executable(first_person_light_up_tu15_tests ')
    end=root.index('endif()\n',start)+len('endif()\n');block=root[start:end]
    if 'add_executable(first_person_light_up_tu15_tests ' not in text:
        anchor='target_sources(halo2_systems PRIVATE src/rendering/models/source_live_model_draws_tu15.cpp)\n'
        assert text.count(anchor)==1;path.write_text(text.replace(anchor,anchor+'\n'+block))
    elif block not in text:
        begin=text.index('if(BUILD_TESTING)\n    add_executable(first_person_light_up_tu15_tests ')
        finish=text.index('endif()\n',begin)+len('endif()\n')
        path.write_text(text[:begin]+block+text[finish:])
    if path.read_text()!=before:changes['CMakeLists.txt']=dict(before=sha(before.encode()),after=sha(path.read_bytes()))
    for name in ['tests/first_person_light_up_tu15_tests.cpp','tests/fixtures/first_person_light_up_tu15.json']:
        path=source/name;data=(ROOT/name).read_bytes()
        if path.exists() and path.read_bytes()==data:continue
        if path.exists():path.unlink()
        path.write_bytes(data)
    updated=source_hashes(source);assert {p for p in updated if updated[p]!=pins.get(p)}==set(changes)
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    path=follow/'diagnostic-admission.json';d=json.loads(path.read_text());d.update(build_completed=False,invalidated_by=__doc__);path.write_text(json.dumps(d,indent=2)+'\n')
    base=ROOT/'analysis/combat-presentation-audit-20261022/repair-native-attached-light-collection/first-person-pose'
    out=base/'refresh.json';index=2
    while out.exists():out=base/f'refresh-{index}.json';index+=1
    result=dict(scope=__doc__,changes=changes,source_ready_or_pose_result_supplied=False,
        installed_runtime_unchanged=True,gameplay_verified=False)
    out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
if __name__=='__main__':main()
