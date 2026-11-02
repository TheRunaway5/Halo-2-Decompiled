"""Preserve a newer qualified selection inside an active Workshop test owner.

Restore only the authenticated selected-source delta and corresponding archived
tests. Explicit Workshop overlays and the narrow test-module include survive.
No dirty checkout source, launcher, runtime or asset is adopted or replaced.
Invoke sequentially through the owning run_test.py command session after its
previous build/test command has completed. Temporary extraction is removed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tarfile
import tempfile

from build_launcher_engine import source_hashes


def digest(path):
    with path.open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def inventory(pins):return hashlib.sha256(json.dumps(pins,sort_keys=True).encode()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--preparation',type=Path,required=True)
    p.add_argument('--current-receipt',type=Path,required=True)
    p.add_argument('--selected-reference',type=Path,required=True)
    p.add_argument('--preservation-diff',type=Path,required=True)
    p.add_argument('--terminal-build',type=Path,required=True)
    p.add_argument('--test-module',required=True)
    p.add_argument('--resume-partial',action='store_true',help='Verify and finish the exact selected production delta after an interrupted test-reference refresh')
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    root=Path(__file__).resolve().parents[1];owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=owner/'source';assert source.is_dir()and not source.is_symlink()
    assert not a.output.resolve().is_relative_to(owner)
    prep=json.loads(a.preparation.read_text());prior=json.loads(a.current_receipt.read_text())
    assert Path(prep['owned_scratch'])==owner and Path(prep['source'])==source and prior['passed']
    terminal=json.loads(a.terminal_build.read_text())
    assert terminal['status']in('passed','failed')and all(r['exit_code']is not None for r in terminal['steps'])
    before=source_hashes(source)
    selection_path=root/'gameplay-runtime-selection.json';selection_bytes=selection_path.read_bytes();selection=json.loads(selection_bytes)
    reference=json.loads(a.selected_reference.read_text());assert reference['passed']and reference['source_inventory_matches_selected_package']
    assert selection['runtime']==reference['runtime']and selection['runtime_sha256']==reference['runtime_sha256']
    runtime=root/selection['runtime'];assert digest(runtime)==selection['runtime_sha256']
    selected=json.loads((runtime.parent/'source-hashes.json').read_text())
    assert inventory(selected)==reference['production_inventory_sha256']
    baseline=json.loads((root/prep['selected_runtime']).parent.joinpath('source-hashes.json').read_text())
    changes={name for name in set(baseline)|set(selected)if baseline.get(name)!=selected.get(name)}
    preservation=json.loads(a.preservation_diff.read_text())
    assert preservation['selected_runtime']==selection['runtime']
    assert changes=={r['path']for r in preservation['changed_files']}
    for row in preservation['changed_files']:
        assert row['baseline']==baseline.get(row['path'])and row['selected']==selected.get(row['path'])
    relative=Path(a.test_module);assert relative.parent==Path('cmake')and relative.suffix=='.cmake'
    cmake_name='cmake/PhysicsLiveWorldTu15.cmake'
    include='  include(${CMAKE_CURRENT_SOURCE_DIR}/'+a.test_module+')\n'
    current_cmake=(source/cmake_name).read_text();assert current_cmake.count(include)==1
    resumed=inventory(before)!=prior['production_inventory_sha256']
    if resumed:
        assert a.resume_partial
        assert hashlib.sha256(current_cmake.replace(include,'').encode()).hexdigest()==selected[cmake_name]
        recovered=dict(before)
        for name in changes-{cmake_name}:
            assert before.get(name)==selected.get(name),('Unexpected partial refresh',name)
            if name in baseline:recovered[name]=baseline[name]
            else:recovered.pop(name,None)
        recovered[cmake_name]=next(row['sha256']for row in prep['overlays']if row['path']==cmake_name)
        assert inventory(recovered)==prior['production_inventory_sha256']
        before=recovered
    else:
        assert not a.resume_partial
        assert hashlib.sha256(current_cmake.replace(include,'').encode()).hexdigest()==baseline[cmake_name]
    for name in changes-{cmake_name}:assert before.get(name)==baseline.get(name),('Unexpected overlapping source change',name)
    own={name:pin for name,pin in before.items()if name not in changes and baseline.get(name)!=pin}
    test_overlays={}
    for receipt in sorted(a.current_receipt.parent.glob('source-refresh-*.json'),key=lambda p:int(p.stem.rsplit('-',1)[1])):
        if int(receipt.stem.rsplit('-',1)[1])>int(a.current_receipt.stem.rsplit('-',1)[1]):continue
        data=json.loads(receipt.read_text());assert data['passed']
        for row in data['changes']:
            if row['path'].startswith('tests/'):test_overlays[row['path']]=row['sha256']
    assert all(digest(source/name)==pin for name,pin in test_overlays.items())
    archive=Path(reference['path']);assert digest(archive)==reference['sha256']
    copied_tests=[];written=[];materialized_references=[]
    with tempfile.TemporaryDirectory(prefix='selected-source-refresh-',dir=owner)as temporary:
        fresh=Path(temporary);count=total=0;seen=set()
        with tarfile.open(archive,'r|zst')as stream:
            for member in stream:
                name=Path(member.name);assert member.isfile()and not name.is_absolute()and'..'not in name.parts
                assert member.name not in seen;seen.add(member.name);count+=1;total+=member.size
                assert count<=24000 and total<=512*1024*1024
                stream.extract(member,fresh,filter='data')
        assert count==reference['files']and total==reference['archive_logical_bytes']
        assert source_hashes(fresh)==selected
        # A preparation may borrow this reference-only directory. Replace only
        # the owned link, never write through it into the shared checkout.
        helper=source/'tests/helpers'
        if helper.is_symlink():
            assert helper.resolve()==(root/'tests/helpers').resolve()
            assert not any(name.startswith('tests/helpers/')for name in test_overlays)
            materialized_references.append(dict(path='tests/helpers',borrowed_target=str(helper.resolve())))
            helper.unlink();helper.mkdir()
        for name in sorted(changes):
            target=source/name;assert target.parent.resolve().is_relative_to(source)and not target.is_symlink()
            if name in selected:
                target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(fresh/name,target)
                assert digest(target)==selected[name]
            else:target.unlink()
            written.append(dict(path=name,before=before.get(name),selected=selected.get(name)))
        target=source/cmake_name;text=target.read_text();anchor='  add_executable(script_clock_builtin_tests tests/script_clock_builtin_tests.cpp)\n'
        assert text.count(anchor)==1 and include not in text;target.write_text(text.replace(anchor,anchor+include))
        for path in sorted((fresh/'tests').rglob('*')):
            if not path.is_file():continue
            name=path.relative_to(fresh).as_posix()
            if name in test_overlays:continue
            target=source/name;assert target.parent.resolve().is_relative_to(source)and not target.is_symlink()
            pin=digest(path)
            if target.exists()and digest(target)==pin:continue
            old=digest(target)if target.exists()else None
            target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target);assert digest(target)==pin
            copied_tests.append(dict(path=name,before=old,sha256=pin))
        expected=dict(selected);expected.update(own);expected[cmake_name]=digest(source/cmake_name)
        after=source_hashes(source);assert after==expected
        assert all(digest(source/name)==pin for name,pin in test_overlays.items())
        assert digest(archive)==reference['sha256']and digest(runtime)==selection['runtime_sha256']
    assert not Path(temporary).exists()
    unchanged=selection_path.read_bytes()==selection_bytes
    report=dict(scope=__doc__,passed=unchanged,selection_unchanged=unchanged,runtime=selection['runtime'],
        runtime_sha256=selection['runtime_sha256'],baseline_runtime=prep['selected_runtime'],
        selected_reference=str(a.selected_reference),selected_changes=written,test_sources_refreshed=copied_tests,
        preserved_workshop_source_hashes=own,preserved_test_overlay_hashes=test_overlays,
        private_cmake_include=a.test_module,production_files=len(after),production_inventory_sha256=inventory(after),
        resumed_exact_partial_refresh=resumed,materialized_reference_directories=materialized_references,
        temporary_extraction_removed=True,owned_scratch=str(owner),runtime_published=False,retained_large_artifacts=[])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=unchanged,selected_changes=len(written),test_files_refreshed=len(copied_tests),production_files=len(after))))
    return 0 if unchanged else 1


if __name__=='__main__':raise SystemExit(main())
