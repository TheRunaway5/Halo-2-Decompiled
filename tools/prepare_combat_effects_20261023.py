#!/usr/bin/env python3
"""Prepare an owned, hash-pinned live combat-effects investigation.

Use inside tools/run_test.py, optionally its finite command coordinator.
Production source remains unchanged; diagnostic clones observe completed
per-view spans and effect-producer inputs. Compiler products stay in their owner.
"""
import hashlib,json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import freeze_source,source_hashes
from validate_weapon_runtime_tu15 import private_display

ROOT=Path(__file__).resolve().parents[1]

def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=owner/'source';out=ROOT/'analysis/combat-presentation-audit-20261022/repair-followup-20261023'
    out.mkdir(parents=True,exist_ok=True)
    attempt=1
    while (out/f'diagnostic-admission-{attempt}.json').exists():attempt+=1
    if (out/'diagnostic-admission.json').exists():
        (out/f'diagnostic-admission-{attempt}.json').write_bytes((out/'diagnostic-admission.json').read_bytes())
    label='validation-retry' if '--reuse' in sys.argv else 'validation'
    count=2;validation=out/label
    while validation.exists():validation=out/(label+'-'+str(count));count+=1
    if '--reuse' in sys.argv:
        pins=json.loads((out/'source-hashes.json').read_text())
        assert source_hashes(source)==pins,'Owned source changed outside its declared refresh'
        for name in ('audit_combat_presentation_20261022.cpp','audit_joined_effects_20261022.cpp'):
            p=source/'tools'/name;data=(ROOT/'tools'/name).read_bytes()
            if p.read_bytes()!=data:p.write_bytes(data)
        test=source/'tests/object_loop_audio_runtime_tu15_tests.cpp'
        data=(ROOT/'tests/object_loop_audio_runtime_tu15_tests.cpp').read_bytes()
        if test.read_bytes()!=data:test.write_bytes(data)
    else:pins=freeze_source(ROOT,source)
    (out/'source-hashes.json').write_text(json.dumps(pins,indent=2,sort_keys=True)+'\n')
    runtime=source/'src/gameplay/multiplayer/multiplayer_runtime.cpp'
    clone=source/'tools/joined_effects_runtime.cpp';text=runtime.read_text()
    needle='      view.lens_draws=lens_draws[i];';assert text.count(needle)==1
    text=text.replace(needle,needle+'''\n      std::cout<<"[COMBAT-JOIN-LENS] tick="<<current.tick<<" view="<<i<<" queries="<<view.lens_queries.size()<<" flares="<<view.lens_draws.size()<<'\\n';''')
    text=text.replace(needle,needle+'''\n      if(i==0 && native_lens)for(const auto &completed:native_lens->completed_queries())
        std::cout<<"[COMBAT-REPLICA-GPU] id="<<completed.id<<" generation="<<completed.generation<<" samples="<<completed.samples<<'\\n';''')
    needle='\n          for(const auto &request:source_standalone_light_lenses_tu15(effect_light_views[i]))';assert text.count(needle)==1
    text=text.replace(needle,'\n          if(!std::getenv("HALO2_AUDIT_OMIT_STANDALONE_LENS"))'+needle)
    if not clone.exists() or clone.read_text()!=text:clone.write_text(text)
    effects=source/'src/rendering/effects/world_effects.cpp'
    effect_clone=source/'tools/joined_effects_world.cpp';effect_text=effects.read_text()
    needle='    const auto &e=*effect;const auto &source=*e.replica_render;';assert effect_text.count(needle)==1
    effect_text='#include <iostream>\n'+effect_text.replace(needle,needle+'''
    std::cout<<"[COMBAT-REPLICA-EFFECT] tick="<<i.tick<<" identity="<<source.identity
      <<" clock_flags="<<e.clock.flags<<" event="<<e.clock.event<<" elapsed="<<e.clock.elapsed
      <<" target="<<e.clock.target<<" source_flags="<<source.flags
      <<" static_world="<<source_effect_replica_static_world_tu15(source)
      <<" locations="<<e.source_locations->size()<<" parts="
      <<((e.clock.event>=0&&std::size_t(e.clock.event)<e.definition->events.size())?e.definition->events[e.clock.event].lenses.size():0)<<'\\n';''')
    needle='      if(found==e.source_locations->end())continue;';assert effect_text.count(needle)==1
    effect_text=effect_text.replace(needle,'''      std::cout<<"[COMBAT-REPLICA-PART] lens="<<part.definition<<" location="<<part.location
        <<" found="<<(found!=e.source_locations->end())<<'\\n';
'''+needle)
    needle='        out.push_back({source,part.definition,ordinal++,frame.position,frame.forward,opacity});';assert effect_text.count(needle)==1
    effect_text=effect_text.replace(needle,'''        std::cout<<"[COMBAT-REPLICA-REQUEST] lens="<<part.definition<<" opacity="<<opacity
          <<" position="<<frame.position.x<<','<<frame.position.y<<','<<frame.position.z
          <<" forward="<<frame.forward.x<<','<<frame.forward.y<<','<<frame.forward.z<<'\\n';
'''+needle)
    needle='    frames[impl_->cache.string_id(location.name)].push_back(frame);';assert effect_text.count(needle)==1
    effect_text=effect_text.replace(needle,'''    std::cout<<"[COMBAT-SOURCE-LOCATION] effect="<<effect<<" name="<<location.name
      <<" decoded="<<impl_->cache.string_id(location.name)<<'\\n';
'''+needle)
    if not effect_clone.exists() or effect_clone.read_text()!=effect_text:effect_clone.write_text(effect_text)
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    role_path=out/'snapshot-role.json'
    role=json.loads(role_path.read_text()) if role_path.exists() else {'role':'Frozen production source with read-only diagnostic runtime clone'}
    (out/'diagnostic-admission.json').write_text(json.dumps(dict(
        build_completed=False,standalone_lens_omission_control=True,source_snapshot_role=role,production_runtime_sha256=sha(runtime),diagnostic_clone_sha256=sha(clone),
        production_world_effects_sha256=sha(effects),diagnostic_world_effects_sha256=sha(effect_clone),
        joined_fixture_sha256=sha(source/'tools/audit_joined_effects_20261022.cpp'),
        combat_fixture_sha256=sha(source/'tools/audit_combat_presentation_20261022.cpp'),
        object_loop_fixture_sha256=sha(source/'tests/object_loop_audio_runtime_tu15_tests.cpp'),
        scope=role['role']+'. Diagnostic clones observe completed lens spans and effect-producer inputs. No original admission, projectile, visibility or GPU result is supplied.'),indent=2)+'\n')
    cmake=source/'CMakeLists.txt';saved=cmake.read_bytes()
    with cmake.open('a') as f:f.write('''
add_executable(combat_presentation_audit EXCLUDE_FROM_ALL tools/audit_combat_presentation_20261022.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
add_executable(joined_effects_audit EXCLUDE_FROM_ALL tools/audit_joined_effects_20261022.cpp tools/joined_effects_runtime.cpp tools/joined_effects_world.cpp)
foreach(combat_target IN ITEMS combat_presentation_audit joined_effects_audit)
  target_include_directories(${combat_target} PRIVATE src/gameplay/multiplayer)
  target_link_libraries(${combat_target} PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
  target_compile_definitions(${combat_target} PRIVATE HALO2_SHADER_DIR=\\"${CMAKE_CURRENT_BINARY_DIR}/shaders\\")
  add_dependencies(${combat_target} halo2_shaders)
endforeach()
add_custom_target(combat_effects_cohort DEPENDS combat_presentation_audit joined_effects_audit
    projectile_glow_gpu_tests camouflage_material_gpu_tests source_model_build_tests
    object_loop_audio_runtime_tu15_tests)
''')
    try:
        display_root=owner/'validation-display';display_root.mkdir(exist_ok=True)
        with private_display(display_root) as display:
            env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'graphics-cache'))
            cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
                 '--target','combat_effects_cohort',
                 '--filter','^(original_model_render_build|projectile_glow_material_banks|camouflage_object_owner_isolation)$',
                 '--cmake-arg=-DHALO2_ROCKET_GHOST_MCC_MAP='+str(ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b/halo2/h2_maps_win64_dx11/coagulation.map'),
                 '--jobs','2','--test-jobs','1','--output',str(validation)]
            with (out/(validation.name+'-command.log')).open('w') as log:r=subprocess.run(cmd,env=env,stdout=log,stderr=subprocess.STDOUT)
    finally:cmake.write_bytes(saved)
    assert source_hashes(source)==pins
    admission_path=out/'diagnostic-admission.json'
    admission=json.loads(admission_path.read_text())
    admission['build_completed']=r.returncode==0
    if r.returncode==0:
        admission['executables']={name:sha(owner/'build'/name) for name in
            ('combat_presentation_audit','joined_effects_audit','object_loop_audio_runtime_tu15_tests')}
    admission_path.write_text(json.dumps(admission,indent=2)+'\n')
    print(json.dumps(dict(exit_code=r.returncode,source_files=len(pins),production_source_restored=True)),flush=True)
    return r.returncode

if __name__=='__main__':raise SystemExit(main())
