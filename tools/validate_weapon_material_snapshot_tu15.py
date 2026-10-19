#!/usr/bin/env python3
"""Validate weapon material changes against one disposable source snapshot.

Use tools/run_test.py. CMake validation still runs through tools/run_tests.py.
Large original assets/reference fixtures are borrowed; changed code and selected
fixtures are copied and hashed so concurrent header/library edits cannot mix ABIs.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
from original_xbox_pose_case import ROOT,sha
from research_mp_sandbox_update_functions import NEW_DEFAULT


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--application',action='store_true')
    parser.add_argument('--cmake-arg',action='append',default=[])
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    output=args.output.absolute();output.mkdir(exist_ok=False)
    source=scratch/'source';source.mkdir()
    copied=[]
    def copy_file(path):
        relative=path.relative_to(ROOT);destination=source/relative
        destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(path,destination);copied.append(relative)
    def copy_tree(name):
        base=ROOT/name
        for path in sorted(base.rglob('*')):
            if any(part in ('__pycache__','.git') for part in path.relative_to(base).parts):continue
            if path.is_file():copy_file(path)
    copy_file(ROOT/'CMakeLists.txt')
    for name in ('src','include','cmake','shaders','third_party','external'):
        copy_tree(name)
    for path in sorted((ROOT/'tools').iterdir()):
        if path.is_file() and path.suffix in ('.py','.cpp','.hpp','.h','.c','.S','.sh','.txt'):
            copy_file(path)
    for name in ('data','reference','pstencil'):
        if (ROOT/'tools'/name).exists():(source/'tools'/name).symlink_to(ROOT/'tools'/name,target_is_directory=True)
    for path in sorted((ROOT/'tests').iterdir()):
        if path.is_file():copy_file(path)
        elif path.is_dir() and path.name not in ('fixtures','__pycache__'):
            (source/'tests'/path.name).symlink_to(path,target_is_directory=True)
    fixtures=source/'tests/fixtures';fixtures.mkdir()
    selected={'weapon_material_builtins_tu15.json','weapon_heat_shader_original.tsv',
        'weapon_primary_charge_tu15_original.json','retail_source_weapon_shader_input.tsv'}
    for path in (ROOT/'tests/fixtures').iterdir():
        if path.name in selected:copy_file(path)
        else:(fixtures/path.name).symlink_to(path,target_is_directory=path.is_dir())
    for name in ('assets','analysis','resources','data','multiplayer','.tools'):
        if (ROOT/name).exists():(source/name).symlink_to(ROOT/name,target_is_directory=True)
    pins={str(relative):sha(source/relative) for relative in copied}
    drift=[str(relative) for relative in copied if sha(ROOT/relative)!=pins[str(relative)]]
    manifest=dict(source=str(source),source_files_sha256=pins,source_drift_during_copy=drift,
        changed_code_and_selected_fixtures_are_private_copies=True,
        borrowed_large_original_inputs_are_not_copied=True,runner_scratch=str(scratch))
    (output/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    assert not drift,drift[:10]
    tests=['original_tu15_weapon_material_inputs','retail_source_weapon_shader_input',
        'original_weapon_heat_shader','original_tu15_weapon_primary_charge']
    targets=['source_weapon_material_input_tests','source_weapon_shader_input_tests',
        'weapon_heat_shader_tests','weapon_primary_charge_tu15_tests']
    if args.application:
        # Only validation registrations are added to this private CMake copy.
        # Each finite CLI invocation has its own standard scratch wrapper.
        with (source/'CMakeLists.txt').open('a') as stream:
            for players in (1,4):
                name=f'weapon_material_application_{players}_players';tests.append(name)
                stream.write(f'''\nadd_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:halo2_native>"
    --multiplayer --map "${{CMAKE_CURRENT_SOURCE_DIR}}/assets/disc/maps/lockout.map"
    --shared-maps "${{CMAKE_CURRENT_SOURCE_DIR}}/assets/disc/maps"
    --gameplay-xbe "{NEW_DEFAULT}" --mp-starting-weapon 5 --local-players {players}
    --simulate 120 --mute)
set_tests_properties({name} PROPERTIES TIMEOUT 120)\n''')
    commands=[];steps=[]
    report=dict(passed=False,status='running',runner_scratch=str(scratch),private_source=str(source),
        source_manifest_sha256=sha(output/'source-manifest.json'),commands=commands,steps=steps,
        source_cleanup_pending_runner_exit=True,large_artifacts_retained=[])
    def save():(output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    save()
    for sanitizer in (False,True):
        label='sanitizers' if sanitizer else 'release'
        command=[str(Path(os.sys.executable).absolute()),str(ROOT/'tools/run_tests.py'),
            '--source',str(source),'--scratch-root',str(scratch/'cmake-scratch'),
            '--output',str(output/label),'--cmake-arg=-DPython3_EXECUTABLE='+str(Path(os.sys.executable).absolute()),
            '--filter','^('+'|'.join(tests[:4] if sanitizer else tests)+')$',
            '--retain-report','weapon-material-input-acceptance.json']
        for target in targets+(['halo2_native'] if args.application and not sanitizer else []):
            command+=['--target',target]
        if sanitizer:command+=['--sanitizers']
        else:command+=['--cmake-arg=-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG']
        command+=['--cmake-arg='+value for value in args.cmake_arg]
        commands.append(command);save()
        with (output/(label+'-runner.log')).open('w') as log:
            result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
        child=json.loads((output/label/'report.json').read_text())
        removed=not Path(child['build']).parent.exists()
        steps.append(dict(name=label,exit_code=result.returncode,private_build_removed=removed,
            report_sha256=sha(output/label/'report.json')))
        save()
        assert removed
        if result.returncode:
            report['status']='failed';save();return result.returncode
    report.update(passed=True,status='passed',snapshot_source_files_unchanged=all(sha(source/path)==pin
        for path,pin in pins.items() if path!='CMakeLists.txt'),private_test_builds_removed=True,
        full_source_tree_immutable_during_root_work=False)
    save()
    print(json.dumps(dict(passed=True,checks=[step['name'] for step in steps],runner_scratch=str(scratch))),flush=True)
    return 0


if __name__=='__main__':
    raise SystemExit(main())
