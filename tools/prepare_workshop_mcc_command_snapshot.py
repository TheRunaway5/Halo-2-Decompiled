"""Prepare an owned MCC command integration cohort from the selected source.

Preserve the selected networking/rendering cohort, refresh only the declared
command changes, and opt into the command only in this private experiment.
No launcher is published. Run through run_test.py; --session keeps that owner
alive for official run_tests.py/build_launcher_engine.py workers and cleanup.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import runpy
import shutil
import sys
import tarfile


def digest(path):
    with path.open('rb')as file:return hashlib.file_digest(file,'sha256').hexdigest()


def replace(path,old,new):
    text=path.read_text();assert text.count(old)==1,(str(path),old)
    path.write_text(text.replace(old,new))


def body(source,name):
    first=source.index('static void '+name+'(WorldState &s) {')
    last=source.find('\nstatic void ',first+1)
    return source[first:last if last!=-1 else len(source)]


def borrow_upscaler_inputs(root,source,package):
    # The code-only archive deliberately excludes precompiled reference
    # libraries/licenses. Authenticate them against the selected package's
    # unchanged manifest and borrow only missing declared inputs.
    relative=Path('external/upscaling');manifest=root/relative/'manifest.json'
    assert manifest.read_bytes()==(package/'upscalers/manifest.json').read_bytes()
    records=[]
    for row in json.loads(manifest.read_text())['files']:
        path=Path(row['path']);assert not path.is_absolute() and '..' not in path.parts
        original=root/relative/path;target=source/relative/path
        assert original.stat().st_size==row['bytes'] and digest(original)==row['sha256']
        if target.exists():assert digest(target)==row['sha256']
        else:
            target.parent.mkdir(parents=True,exist_ok=True);target.symlink_to(original)
            records.append(dict(path=str(relative/path),bytes=row['bytes'],sha256=row['sha256']))
    target=source/relative/'manifest.json'
    if not target.exists():shutil.copyfile(manifest,target)
    assert digest(target)==digest(manifest)
    # CMake also bundles this license, which the historical upstream
    # manifest omits. Authenticate it directly against the sealed package.
    original=root/relative/'LICENSE.AMD.txt';target=source/relative/'LICENSE.AMD.txt'
    assert digest(original)==digest(package/'upscalers/LICENSE.AMD.txt')
    if not target.exists():
        target.symlink_to(original)
        records.append(dict(path=str(relative/'LICENSE.AMD.txt'),bytes=original.stat().st_size,sha256=digest(original)))
    return records


def freeze_vendor_inputs(root,source):
    # The delivery source inventory excludes these separate vendor trees.
    # Freeze their current regular files explicitly, preserving the original
    # reference owners and any already-authenticated precompiled aliases.
    pins={};excluded={'.git','__pycache__','target','build','CMakeFiles'}
    for parent in ('external','third_party'):
        for path in (root/parent).rglob('*'):
            relative=path.relative_to(root)
            if not path.is_file() or set(relative.parts).intersection(excluded):continue
            name=relative.as_posix();pin=digest(path);target=source/relative
            if target.exists():assert digest(target)==pin,name
            else:
                target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
            pins[name]=pin
    assert all(digest(root/name)==pin for name,pin in pins.items())
    return pins


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--reference',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--session',action='store_true')
    a=p.parse_args();root=Path(__file__).resolve().parents[1]
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not a.output.resolve().is_relative_to(scratch)
    reference=json.loads(a.reference.read_text());archive=Path(reference['path'])
    selected=root/'gameplay-runtime-selection.json';selection_before=selected.read_bytes()
    selection=json.loads(selection_before)
    assert root/selection['runtime']==Path(reference['runtime'])
    assert selection['runtime_sha256']==reference['runtime_sha256']==digest(root/selection['runtime'])
    assert digest(archive)==reference['sha256']
    source=scratch/'source';source.mkdir()
    total=0;count=0
    with tarfile.open(archive,'r|zst')as stream:
        for member in stream:
            path=Path(member.name)
            assert member.isfile() and not path.is_absolute() and '..' not in path.parts
            total+=member.size;count+=1
            assert total<=512*1024*1024 and count<=20000
            stream.extract(member,source,filter='data')
    assert count==reference['files'] and total==reference.get('archive_logical_bytes',reference['logical_source_bytes'])
    borrowed=borrow_upscaler_inputs(root,source,(root/selection['runtime']).parent)
    vendor_pins=freeze_vendor_inputs(root,source)
    # Borrow missing reference trees, never overwrite the archived code/data.
    for name in ('assets','resources','analysis','.tools','Halo 2 Vista installer'):
        if (root/name).exists() and not (source/name).exists():
            (source/name).symlink_to(root/name,target_is_directory=True)
    for parent in ('tools','tests','multiplayer'):
        for path in (root/parent).iterdir():
            target=source/parent/path.name
            if not path.is_dir() or path.name.startswith('.') or path.name=='__pycache__' or target.exists():continue
            if parent=='tests' and path.name=='fixtures':
                target.mkdir()
                for child in path.iterdir():
                    out=target/child.name
                    if child.is_file() and child.stat().st_size<=65536:shutil.copyfile(child,out)
                    else:out.symlink_to(child,target_is_directory=child.is_dir())
            else:target.symlink_to(path,target_is_directory=True)
    old_hooks=runpy.run_path(str(source/'tools/mcc_script_builtin_hooks.py'))
    new_hooks=runpy.run_path(str(root/'tools/mcc_script_builtin_hooks.py'))
    refreshed={};edits=[]
    for name in ('src/campaign/ai/source_ai_frame_tu15_generated.cpp',
                 'src/campaign/source_campaign_runtime_tu15_generated.cpp'):
        target=source/name;plain=old_hooks['remove_hooks'](target.read_text())
        current=new_hooks['remove_hooks']((root/name).read_text())
        for routine in ('run_211b10','run_211da0')+ (('run_212010',)if '/ai/' in name else ()):
            assert body(plain,routine)==body(current,routine),(name,routine)
        target.write_text(new_hooks['add_hooks'](plain));edits.append(dict(path=name,kind='shared command hook; exact original HS bodies match'))
    paths=[
        'include/halo2/native_mcc_script_builtin.hpp','include/halo2/script_mcc_legacy_command.hpp',
        'src/campaign/scripting/native_mcc_script_builtin.cpp','src/campaign/scripting/mcc_script_layout.inc',
        'tools/mcc-script-layout.json','tools/generate_mcc_script_layout.py','tools/mcc_script_builtin_hooks.py',
        'tools/build_launcher_engine.py','tests/test_build_launcher_engine.py',
        'tools/test_workshop_candidate_sweep.py','tools/validate_mcc_hs_commands.py',
        'tools/prepare_workshop_mcc_command_snapshot.py','tools/oracle_workshop_hs_signature_tu15.py',
        'tests/native_mcc_script_command_tests.cpp','tests/script_mcc_legacy_command_tests.cpp',
        'tests/native_mcc_script_world_tests.cpp',
        'tests/fixtures/workshop_mcc_blur_installed.json']
    for name in paths:
        target=source/name;assert not target.is_symlink(),name
        target.parent.mkdir(parents=True,exist_ok=True)
        refreshed[name]=digest(root/name);shutil.copyfile(root/name,target)
        assert digest(target)==refreshed[name]
    replace(source/'src/gameplay/simulation.cpp','#include "halo2/script_clock_builtin.hpp"',
        '#include "halo2/script_clock_builtin.hpp"\n#include "halo2/script_mcc_legacy_command.hpp"')
    replace(source/'src/gameplay/simulation.cpp','const std::vector<ScriptValue> &args) {\n  auto arity',
        'const std::vector<ScriptValue> &args) {\n  if(const auto command=mcc_script_legacy_command(name,args))return *command;\n  auto arity')
    cmake=(root/'cmake/PhysicsLiveWorldTu15.cmake').read_text()
    first=cmake.index('  add_executable(native_mcc_script_command_tests ')
    last=cmake.index('  add_executable(script_clock_builtin_tests ',first)
    additions=cmake[first:last];assert additions.count('add_executable(')==3
    anchor='  add_test(NAME native_mcc_script_clock_getters COMMAND native_mcc_script_builtin_tests)\n'
    replace(source/'cmake/PhysicsLiveWorldTu15.cmake',anchor,anchor+additions)
    private_admission=[
        ('src/physics/physics_live_world_tu15.cpp','mcc_script_profile.emplace(syntax,strings);','mcc_script_profile.emplace(syntax,strings,true);'),
        ('src/app/steam_workshop.cpp','NativeMccScriptBuiltinProfile owned(layout, strings);','NativeMccScriptBuiltinProfile owned(layout, strings,true);'),
        ('tools/probe_workshop_mod.cpp','NativeMccScriptBuiltinProfile profile(layout,strings);','NativeMccScriptBuiltinProfile profile(layout,strings,true);')]
    for name,old,new in private_admission:
        replace(source/name,old,new);edits.append(dict(path=name,kind='private command-admission experiment',old=old,new=new))
    assert all(digest(root/name)==pin for name,pin in refreshed.items())
    assert selected.read_bytes()==selection_before and digest(root/selection['runtime'])==selection['runtime_sha256']
    sys.path.insert(0,str(root/'tools'));from build_launcher_engine import source_hashes
    pins=source_hashes(source)
    a.output.mkdir(parents=True,exist_ok=True)
    (a.output/'vendor-inputs.json').write_text(json.dumps(vendor_pins,indent=2)+'\n')
    report=dict(scope=__doc__,status='prepared_not_tested',owned_scratch=str(scratch),source=str(source),
        reference=str(a.reference),archive_sha256=reference['sha256'],archive_files=count,
        borrowed_precompiled_inputs=borrowed,
        frozen_vendor_files=len(vendor_pins),
        refreshed_root_hashes=refreshed,private_edits=edits,production_source_files=len(pins),
        production_source_inventory_sha256=hashlib.sha256(json.dumps(pins,sort_keys=True).encode()).hexdigest(),
        selection_unchanged=True,runtime_unchanged=True,root_command_admission_enabled=False,
        retained_large_artifacts=[])
    (a.output/'preparation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status=report['status'],source=str(source),files=len(pins),logical_archive_bytes=total)),flush=True)
    if a.session:
        os.execv(sys.executable,[sys.executable,'-u','-B',str(root/'tools/macos/test_session.py'),
            '--output',str(a.output/'session')])


if __name__=='__main__':main()
