"""Prepare an owned exact selected source cohort for further native mod work.

Restore the qualified code archive, authenticate separate vendor/reference
inputs, and apply only explicitly named source overlays. No runtime is selected
or published. Run through run_test.py; --session preserves its active owner.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import tarfile

from prepare_workshop_mcc_command_snapshot import borrow_upscaler_inputs, freeze_vendor_inputs


def digest(path):
    with path.open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--overlay',action='append',default=[])
    parser.add_argument('--test-module',action='append',default=[])
    parser.add_argument('--session',action='store_true')
    args=parser.parse_args();root=Path(__file__).resolve().parents[1]
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(scratch)
    reference=json.loads(args.reference.read_text());archive=Path(reference['path'])
    selection_path=root/'gameplay-runtime-selection.json';selection_bytes=selection_path.read_bytes()
    selected=json.loads(selection_bytes)
    runtime=Path(reference['runtime']);runtime=runtime if runtime.is_absolute()else root/runtime
    assert runtime==root/selected['runtime']
    assert digest(runtime)==selected['runtime_sha256']==reference['runtime_sha256']
    assert digest(archive)==reference['sha256']
    source=scratch/'source';source.mkdir();files=0;total=0
    with tarfile.open(archive,'r|zst')as stream:
        for member in stream:
            name=Path(member.name)
            assert member.isfile()and not name.is_absolute()and '..'not in name.parts
            files+=1;total+=member.size;assert files<=24000 and total<=512*1024*1024
            stream.extract(member,source,filter='data')
    assert files==reference['files']and total==reference['archive_logical_bytes']
    manifest=json.loads((source/'QUALIFIED_SOURCE_ARCHIVE.json').read_text())
    from build_launcher_engine import source_hashes,tool_source_path
    sealed=json.loads((runtime.parent/'source-hashes.json').read_text())
    manifest_status=reference.get('archive_manifest_status','current')
    if manifest_status=='stale_ancestor_not_used':
        # An upstream archive carried its ancestor's bookkeeping JSON while
        # preserving the newer source files. Authenticate the complete actual
        # production inventory independently; never accept the stale receipt
        # as evidence for either runtime or test qualification. The pinned
        # whole-archive digest still fixes every nonproduction input byte.
        assert reference['passed']and reference['source_inventory_matches_selected_package']
        assert reference['complete_stream_roundtrip_verified']
        assert digest(source/'QUALIFIED_SOURCE_ARCHIVE.json')==reference['ancestor_manifest_sha256']
        assert manifest['runtime']==reference['ancestor_manifest_runtime']
        assert hashlib.sha256(json.dumps(sealed,sort_keys=True).encode()).hexdigest()==reference['production_inventory_sha256']
    else:
        assert manifest_status=='current'
        for name,row in manifest['files'].items():
            path=source/name;assert path.stat().st_size==row['bytes']and digest(path)==row['sha256'],name
    assert source_hashes(source)==sealed
    borrowed=borrow_upscaler_inputs(root,source,runtime.parent)
    vendors=freeze_vendor_inputs(root,source)
    for name in ('assets','resources','analysis','.tools','Halo 2 Vista installer'):
        if (root/name).exists()and not(source/name).exists():
            (source/name).symlink_to(root/name,target_is_directory=True)
    reference_aliases=set();omitted_aliases={}
    for parent in ('tools','tests','multiplayer'):
        for path in (root/parent).iterdir():
            target=source/parent/path.name
            if not path.is_dir()or path.name.startswith('.')or path.name=='__pycache__'or target.exists():continue
            if parent=='tools'or(parent=='multiplayer'and path.name=='master_server'):
                unsealed=[str(child.relative_to(root))for child in path.rglob('*')
                    if child.is_file()and tool_source_path(child.relative_to(path))]
                if unsealed:
                    omitted_aliases[str(target.relative_to(source))]=sorted(unsealed);continue
            if parent=='tests'and path.name=='fixtures':
                target.mkdir()
                for child in path.iterdir():
                    out=target/child.name
                    if child.is_file()and child.stat().st_size<=65536:shutil.copyfile(child,out)
                    else:out.symlink_to(child,target_is_directory=child.is_dir())
            else:
                target.symlink_to(path,target_is_directory=True);reference_aliases.add(target)
    # Newly created reference directories in a concurrently dirty checkout
    # must not silently add production generator/service inputs to this cohort.
    # Archive contents already contain its complete declared production code.
    borrowed_pins=source_hashes(source)
    for name in sorted(set(borrowed_pins)-set(sealed)):
        aliases=[parent for parent in (source/name).parents if parent in reference_aliases]
        assert len(aliases)==1,('Undeclared production input after reference borrowing',name)
        alias=aliases[0];omitted_aliases.setdefault(str(alias.relative_to(source)),[]).append(name)
    for name in omitted_aliases:
        alias=source/name
        if alias in reference_aliases:
            assert alias.is_symlink();alias.unlink()
        else:assert not alias.exists()and not alias.is_symlink()
    assert source_hashes(source)==sealed,'Reference borrowing changed the sealed production cohort'
    overlays=[]
    for name in args.overlay:
        relative=Path(name);assert not relative.is_absolute()and '..'not in relative.parts
        original=root/relative;target=source/relative
        assert original.is_file()and not target.is_symlink()
        assert target.parent.resolve().is_relative_to(source)
        before=digest(target)if target.exists()else None
        pin=digest(original);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(original,target)
        assert digest(original)==digest(target)==pin
        overlays.append(dict(path=name,before=before,sha256=pin))
    for name in args.test_module:
        relative=Path(name);assert relative.parent==Path('cmake')and relative.suffix=='.cmake'
        assert (source/relative).is_file()
        target=source/'cmake/PhysicsLiveWorldTu15.cmake';text=target.read_text()
        anchor='  add_executable(script_clock_builtin_tests tests/script_clock_builtin_tests.cpp)\n'
        insertion='  include(${CMAKE_CURRENT_SOURCE_DIR}/'+name+')\n'
        assert text.count(anchor)==1 and insertion not in text
        before=digest(target);target.write_text(text.replace(anchor,anchor+insertion))
        overlays.append(dict(path='cmake/PhysicsLiveWorldTu15.cmake',before=before,sha256=digest(target),
            change='test-only include '+name))
    assert selection_path.read_bytes()==selection_bytes and digest(runtime)==selected['runtime_sha256']
    pins=source_hashes(source);args.output.mkdir(parents=True,exist_ok=True)
    (args.output/'vendor-inputs.json').write_text(json.dumps(vendors,indent=2)+'\n')
    report=dict(scope=__doc__,status='prepared_not_validated',owned_scratch=str(scratch),source=str(source),
        reference=str(args.reference),selected_runtime=selected['runtime'],runtime_sha256=selected['runtime_sha256'],
        archive_files=files,archive_logical_bytes=total,archive_sha256=reference['sha256'],
        sealed_inventory_verified=True,archive_manifest_status=manifest_status,
        borrowed_precompiled_inputs=borrowed,
        unsealed_reference_aliases_omitted=omitted_aliases,
        frozen_vendor_files=len(vendors),overlays=overlays,production_files=len(pins),
        production_inventory_sha256=hashlib.sha256(json.dumps(pins,sort_keys=True).encode()).hexdigest(),
        selection_unchanged=True,retained_large_artifacts=[])
    (args.output/'preparation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status=report['status'],source=str(source),production_files=len(pins))),flush=True)
    if args.session:
        os.execv(sys.executable,[sys.executable,'-u','-B',str(root/'tools/macos/test_session.py'),
            '--output',str(args.output/'session')])


if __name__=='__main__':main()
