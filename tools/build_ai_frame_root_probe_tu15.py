#!/usr/bin/env python3
"""Build a strict AI-frame replay with one retained current-root memory/header ABI.

The standalone link uses current source memory, snapshot storage and the
complete AI module. No frozen game libraries or supplied game-service returns
are linked. Original input banks and independent hardware remain required.
"""
from source_layout import source_file
import argparse,json,shlex,shutil,subprocess
from pathlib import Path
from audit_rendering import file_sha

ROOT=Path(__file__).resolve().parents[1]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--reuse-build',type=Path,help='Reuse objects only after exact compiler/dependency admission')
    ap.add_argument('--resume-failed-link',action='store_true',help='Resume this owned immutable source snapshot with the same API/compiler flags after a linker failure')
    ap.add_argument('--support-module',type=Path,help='Retained complete native subroutine module for explicit replay owner dispatch')
    ap.add_argument('--api',choices=('frame','actor','object-material','damage-feedback','effect-particle-frame','effect-particle-draw','vehicle-director','recorded-animation','level-handles-new-map','resource-cache-new-map','campaign-activation','object-functions'),default='frame')
    ap.add_argument('--module',type=Path,required=True);a=ap.parse_args()
    out=a.output.resolve();resuming=a.resume_failed_link
    assert (out.is_dir() and (out/'source').is_dir() and not (out/'build.json').exists()) if resuming else not out.exists()
    source=out/'source';source.mkdir(parents=True,exist_ok=resuming)
    originals=[p for p in (ROOT/'include').rglob('*') if p.is_file()]
    originals+=list((ROOT/'src').rglob('*.hpp'))
    originals+=[source_file(ROOT, p) for p in ('source_runtime_memory.cpp','source_runtime_snapshot_bytes.cpp','cache_file_snapshot.cpp','source_runtime_write_protection.inc')]
    originals+=[ROOT/'tools/replay_native_ai_frame_tu15.cpp',ROOT/'src/campaign/scripting/native_mcc_script_builtin.cpp']
    effect_support_sources=('source_projectile_effect_observer.cpp','source_effect_world_locations.cpp','raw_datum_pool.cpp','raw_datum_pool_identity.cpp','mcc_sound_tag_layout.cpp','source_combat_action_tu15_generated.cpp') if a.api in ('effect-particle-frame','effect-particle-draw') and a.support_module else ()
    if a.api=='effect-particle-draw':
        effect_support_sources+=('source_effect_particle_frame_tu15_generated.cpp','source_vehicle_director_tu15_generated.cpp')
    originals+=[source_file(ROOT, name) for name in effect_support_sources]
    snapshot={}
    for p in originals:
        q=source/p.relative_to(ROOT);q.parent.mkdir(parents=True,exist_ok=True)
        if not (resuming and q.exists()):shutil.copy2(p,q)
        snapshot[str(p.relative_to(ROOT))]=file_sha(q)
    module=a.module.resolve()
    if resuming:assert file_sha(module)==file_sha(source/'module.cpp')
    else:shutil.copy2(module,source/'module.cpp')
    support=a.support_module.resolve() if a.support_module else None
    if support:
        if resuming:assert file_sha(support)==file_sha(source/'support.cpp')
        else:shutil.copy2(support,source/'support.cpp')
        snapshot['support.cpp']=file_sha(source/'support.cpp')
    common=['c++','-std=c++20','-O3','-DNDEBUG','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(source/'include'),'-I'+str(source/'src')]
    files=[source/'module.cpp',source/'tools/replay_native_ai_frame_tu15.cpp',*[source_file(source, p) for p in ('source_runtime_memory.cpp','source_runtime_snapshot_bytes.cpp','cache_file_snapshot.cpp')]]
    if support:files.append(source/'support.cpp')
    files += [source_file(source, name) for name in effect_support_sources]
    if '#include "halo2/native_mcc_script_builtin.hpp"' in module.read_text():
        files.append(source/'src/campaign/scripting/native_mcc_script_builtin.cpp')
    commands=[common+['-c',str(p),'-o',str(out/(p.stem+'.o'))] for p in files]
    # The standalone support owner uses the real layout-requirement exception;
    # unused cache-conversion functions must not pull in the entire Cache graph.
    if effect_support_sources:
        for p,command in zip(files,commands):
            if p.name=='mcc_sound_tag_layout.cpp':command[1:1]=['-ffunction-sections','-fdata-sections']
    if a.api=='actor':commands[1].insert(1,'-DHALO2_REPLAY_AI_ACTOR_TU15=1')
    if a.api=='object-material':commands[1].insert(1,'-DHALO2_REPLAY_OBJECT_MATERIAL_TU15=1')
    if a.api=='damage-feedback':commands[1].insert(1,'-DHALO2_REPLAY_DAMAGE_FEEDBACK_TU15=1')
    if a.api=='effect-particle-frame':commands[1].insert(1,'-DHALO2_REPLAY_EFFECT_PARTICLE_FRAME_TU15=1')
    if a.api=='effect-particle-draw':commands[1].insert(1,'-DHALO2_REPLAY_EFFECT_PARTICLE_DRAW_TU15=1')
    if a.api=='vehicle-director':commands[1].insert(1,'-DHALO2_REPLAY_VEHICLE_DIRECTOR_TU15=1')
    if a.api in ('effect-particle-frame','effect-particle-draw') and support:commands[1].insert(1,'-DHALO2_REPLAY_EFFECT_PARTICLE_SUPPORT_TU15=1')
    if a.api=='recorded-animation':commands[1].insert(1,'-DHALO2_REPLAY_RECORDED_ANIMATION_TU15=1')
    if a.api=='level-handles-new-map':commands[1].insert(1,'-DHALO2_REPLAY_LEVEL_HANDLES_NEW_MAP_TU15=1')
    if a.api=='resource-cache-new-map':commands[1].insert(1,'-DHALO2_REPLAY_RESOURCE_CACHE_NEW_MAP_TU15=1')
    if a.api=='campaign-activation':commands[1].insert(1,'-DHALO2_REPLAY_CAMPAIGN_ACTIVATION_TU15=1')
    if a.api=='campaign-activation' and support:commands[1].insert(1,'-DHALO2_REPLAY_SUPPORT_RENDERER_TAIL_TU15=1')
    if a.api=='object-functions':commands[1].insert(1,'-DHALO2_REPLAY_OBJECT_FUNCTIONS_TU15=1')
    commands.append(['c++',*[str(out/(p.stem+'.o')) for p in files],'-pthread','-o',str(out/'replay-native')])
    if effect_support_sources:commands[-1].insert(1,'-Wl,--gc-sections')
    previous_path=a.reuse_build.resolve() if a.reuse_build else None
    previous=json.loads((previous_path/'build.json').read_text()) if previous_path else None
    dependencies={};dependency_commands=[];executed=[];reused=[]
    def normalize(command,directory):
        return [v.replace(str(directory/'source'),'${SOURCE}').replace(str(directory),'${BUILD}') for v in command]
    resumed=[]
    if resuming:shutil.copy2(out/'build.log',out/'failed-link.log')
    with (out/'build.log').open('a' if resuming else 'w') as log:
        for p,command in zip(files,commands):
            depfile=out/(p.stem+'.d');object_=out/(p.stem+'.o')
            prior_dependencies=shlex.split(depfile.read_text().replace('\\\n',' ').partition(':')[2]) if resuming and depfile.exists() and object_.exists() else None
            flags=command[:command.index('-c')]
            dependency_command=flags+['-MM','-MF',str(depfile),'-MT',str(object_),str(p)]
            dependency_commands.append(dependency_command)
            subprocess.run(dependency_command,stdout=log,stderr=subprocess.STDOUT,check=True)
            paths=shlex.split(depfile.read_text().replace('\\\n',' ').partition(':')[2])
            required={}
            for name in paths:
                dependency=Path(name).resolve();relative=str(dependency.relative_to(source))
                digest=file_sha(dependency);required[relative]=digest
                assert digest==(file_sha(source/'module.cpp') if relative=='module.cpp' else snapshot[relative])
            dependencies[p.stem]=required
            if prior_dependencies is not None:
                assert set(prior_dependencies)==set(paths),'Interrupted object dependency set changed'
                assert max(Path(name).stat().st_mtime_ns for name in paths)<=object_.stat().st_mtime_ns
                for name,digest in required.items():
                    root_file=module if name=='module.cpp' else support if name=='support.cpp' else ROOT/name
                    assert root_file and file_sha(root_file)==digest,'Interrupted object source/header differs from current root'
                resumed.append(dict(object=object_.name,sha256=file_sha(object_),same_owned_snapshot=True,dependency_set_and_mtimes_revalidated=True));continue
            admitted=previous is not None
            if admitted:
                for name,digest in required.items():
                    prior=(previous_path/'source'/name)
                    expected=previous['module_sha256'] if name=='module.cpp' else previous['source_snapshot_sha256'].get(name)
                    if not prior.is_file() or expected!=digest or file_sha(prior)!=digest:
                        admitted=False;break
            if admitted:
                prior_commands=[c for c in previous['commands'] if '-c' in c and Path(c[c.index('-o')+1]).name==object_.name]
                admitted=len(prior_commands)==1 and normalize(prior_commands[0],previous_path)==normalize(command,out)
            if admitted:
                prior=previous_path/object_.name
                assert file_sha(prior)==previous['objects_sha256'][object_.name]
                shutil.copy2(prior,object_);reused.append(dict(object=object_.name,build=str(previous_path),sha256=file_sha(prior)))
            else:
                subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True);executed.append(command)
        subprocess.run(commands[-1],stdout=log,stderr=subprocess.STDOUT,check=True);executed.append(commands[-1])
    # Only used headers and sources can change this replay's actual ABI. Keep
    # all other shared-root drift as observations without rejecting the build.
    copied_drift=[p for p,h in snapshot.items() if p!='support.cpp' and file_sha(ROOT/p)!=h]
    required_root={p for required in dependencies.values() for p in required if p!='module.cpp'}
    drift=sorted(set(copied_drift)&required_root)
    for required in dependencies.values():
        for p,digest in required.items():assert file_sha(source/p)==digest
    report=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),commands=commands,api=a.api,
        resumed_after_link_failure=resuming,resumed_objects=resumed,
        original_module=str(module),module_sha256=file_sha(source/'module.cpp'),source_snapshot_sha256=snapshot,
        support_module=str(support) if support else None,support_module_sha256=file_sha(source/'support.cpp') if support else None,
        root_drift_after_compile=drift,copied_root_drift_after_compile=copied_drift,
        compiler_dependency_sha256=dependencies,dependency_commands=dependency_commands,
        dependency_files_sha256={p.name:file_sha(p) for p in out.glob('*.d')},
        executed_commands=executed,reused_objects=reused,
        reuse_build_report_sha256=file_sha(previous_path/'build.json') if previous_path else None,
        objects_sha256={p.name:file_sha(p) for p in out.glob('*.o')},
        binary_sha256=file_sha(out/'replay-native'),passed=not drift)
    (out/'build.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    assert not drift,drift
    print('Built current-root AI frame',report['binary_sha256'])

if __name__=='__main__':main()
