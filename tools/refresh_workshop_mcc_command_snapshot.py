"""Refresh the active command cohort after its first consumer tests pass.

Preserve the newer selected physics cohort and its matching tests, then add
the real World command gate and qualified standalone reset component. This
never changes a launcher or an original/reference owner. Run inside the same
active run_test.py owner; all refreshed source gets fresh timestamps.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import tarfile


def digest(path):
    with path.open('rb')as file:return hashlib.file_digest(file,'sha256').hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--consumers',type=Path,required=True)
    p.add_argument('--selected-reference',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    root=Path(__file__).resolve().parents[1]
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=scratch/'source'
    assert source.is_dir() and not source.is_symlink() and not a.output.resolve().is_relative_to(scratch)
    consumer=json.loads(a.consumers.read_text())
    assert consumer['passed'] and consumer['status']=='passed'
    assert Path(consumer['build']).resolve()==scratch/'build'
    reference=json.loads(a.selected_reference.read_text());archive=Path(reference['archive'])
    assert digest(archive)==reference['archive_sha256']
    selection_path=root/'gameplay-runtime-selection.json';selection_bytes=selection_path.read_bytes()
    selection=json.loads(selection_bytes);package=(root/selection['runtime']).parent
    assert selection['runtime']==reference['selected_runtime']
    assert selection['runtime_sha256']==reference['selected_runtime_sha256']
    assert selection['runtime_sha256']==digest(root/selection['runtime'])
    published=json.loads((package/'source-hashes.json').read_text())
    wanted={row['path']:row for row in reference['production']}
    assert all(published[name]==row['sha256']for name,row in wanted.items())
    wanted.update({row['path']:row for row in reference['changed_tests']
        if row['path']!='tests/test_build_launcher_engine.py'})
    extracted=scratch/'new-selected-refresh';extracted.mkdir()
    seen=set()
    with tarfile.open(archive,'r|zst')as stream:
        for member in stream:
            if member.name not in wanted:continue
            assert member.isfile() and not Path(member.name).is_absolute() and '..'not in Path(member.name).parts
            row=wanted[member.name];assert member.size==row['bytes']
            stream.extract(member,extracted,filter='data');assert digest(extracted/member.name)==row['sha256']
            seen.add(member.name)
    assert seen==set(wanted)
    changes=[]
    def copy(original,target):
        assert not target.is_symlink(),str(target)
        before=digest(target)if target.is_file()else None;target.parent.mkdir(parents=True,exist_ok=True)
        pin=digest(original);shutil.copyfile(original,target);assert digest(target)==pin
        changes.append(dict(path=target.relative_to(source).as_posix(),before=before,sha256=pin))
    for name in sorted(wanted):copy(extracted/name,source/name)
    paths=[
        'tools/build_launcher_engine.py','tests/test_build_launcher_engine.py',
        'tools/prepare_workshop_mcc_command_snapshot.py','tools/refresh_workshop_mcc_command_snapshot.py',
        'tools/validate_mcc_hs_commands.py','tests/native_mcc_script_world_tests.cpp',
        'src/campaign/scripting/source_workshop_unit_controls_tu15_generated.cpp',
        'include/halo2/source_workshop_unit_controls_tu15.hpp','cmake/WorkshopUnitControls.cmake',
        'tests/workshop_unit_controls_tests.cpp','tests/workshop_unit_controls_hs_tests.cpp',
        'tests/fixtures/workshop_player_action_reset_tu15.json',
        'tests/fixtures/workshop_player_action_reset_tu15.json.zlib',
        'tools/oracle_workshop_unit_controls_tu15.py','tools/validate_workshop_unit_controls_tu15.py',
        'tools/verify_workshop_unit_controls_host_x86.py']
    for name in paths:copy(root/name,source/name)
    cmake=source/'cmake/PhysicsLiveWorldTu15.cmake';text=cmake.read_text();current=(root/'cmake/PhysicsLiveWorldTu15.cmake').read_text()
    start='  add_executable(native_mcc_script_command_tests '
    end='  add_executable(script_clock_builtin_tests '
    first=text.index(start);last=text.index(end,first)
    i=current.index(start);j=current.index(end,i);block=current[i:j]
    assert block.count('add_executable(')==3
    before=digest(cmake);cmake.write_text(text[:first]+block+text[last:])
    changes.append(dict(path=str(cmake.relative_to(source)),before=before,sha256=digest(cmake)))
    assert selection_path.read_bytes()==selection_bytes
    assert all(digest(root/name)==digest(source/name)for name in paths)
    assert all(digest(source/name)==row['sha256']for name,row in wanted.items())
    # These stricter controls were added after the newer runtime's source
    # snapshot; replacing them with its older test file would lose coverage.
    assert 'test_zoom_delivery_is_separate_only_in_an_explicit_selected_protocol_cohort'in (source/'tests/test_build_launcher_engine.py').read_text()
    sys.path.insert(0,str(root/'tools'));from build_launcher_engine import source_hashes
    pins=source_hashes(source)
    report=dict(scope=__doc__,status='refreshed_not_validated',owned_scratch=str(scratch),
        consumer_report=str(a.consumers),selected_runtime=selection['runtime'],
        selected_runtime_sha256=selection['runtime_sha256'],reference=str(a.selected_reference),
        changes=changes,production_source_files=len(pins),
        production_inventory_sha256=hashlib.sha256(json.dumps(pins,sort_keys=True).encode()).hexdigest(),
        selection_unchanged=True,unit_reset_runtime_enabled=False,retained_large_artifacts=[])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    shutil.rmtree(extracted)
    print(json.dumps(dict(status=report['status'],refreshed_files=len(changes))))


if __name__=='__main__':main()
