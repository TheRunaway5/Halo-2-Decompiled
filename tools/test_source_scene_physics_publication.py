#!/usr/bin/env python3
"""Freeze, build and test source physics publication independently of live edits."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/source_scene_physics_tests.cpp','src/runtime/source_scene_physics.cpp','src/rendering/models/source_world_pose_wire.cpp',
    'src/runtime/source_runtime_memory.cpp','src/gameplay/objects/raw_datum_pool_identity.cpp','src/gameplay/objects/raw_datum_pool.cpp',
    'src/gameplay/objects/raw_datum_pool_startup.cpp','src/core/datum_pool.cpp','src/gameplay/objects/native_object_lifetimes.cpp',
    'src/runtime/scene.cpp','src/gameplay/objects/object_initialization.cpp','src/rendering/renderer/render_interpolation.cpp',
    'src/core/transform.cpp','src/rendering/shadows/source_shadow.cpp']
CONSUMERS=['src/gameplay/multiplayer/multiplayer.cpp','src/gameplay/objects/source_prop_scene.cpp','src/gameplay/objects/source_scenery_scene.cpp',
    'src/gameplay/objects/source_machine_scene.cpp','src/gameplay/objects/source_device_world.cpp','src/physics/collision/static_scenery_collision.cpp',
    'src/gameplay/objects/prop_sandbox.cpp','src/gameplay/multiplayer/multiplayer_runtime.cpp','src/network/multiplayer_network.cpp']


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--sanitizer',action='store_true');a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=False);snapshot=a.output/'source';snapshot.mkdir()
    shutil.copytree(ROOT/'include',snapshot/'include')
    for relative in SOURCES+CONSUMERS:
        path=snapshot/relative;path.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/relative,path)
    # Source files with private local headers consume these frozen headers too.
    for path in (ROOT/'src').rglob('*.hpp'):
        destination=snapshot/path.relative_to(ROOT)
        destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(path,destination)
    archive=a.output/'libhalo2_cache.a'
    shutil.copy2(ROOT/'analysis/multiplayer/sandbox-source/physics-live-world-tu15-20261012/dev/lib/libhalo2_cache.a',archive)
    manifest={str(path.relative_to(a.output)):digest(path) for path in sorted(snapshot.rglob('*'))if path.is_file()}
    manifest['libhalo2_cache.a']=digest(archive)
    (a.output/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    report=dict(passed=False,sanitizer=a.sanitizer,source_manifest_sha256=digest(a.output/'source-manifest.json'),
        limits='Explicit original-shaped input bank and source transform presentation tests. The reused cache archive is uninstrumented. No live solved-body gameplay acceptance is inferred.')
    flags=['-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-ffunction-sections','-fdata-sections','-I'+str(snapshot/'include'),'-I'+str(snapshot/'src'),'-I/usr/include/SDL2']
    if a.sanitizer:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    commands=[]
    def run(name,args):
        commands.append(args);result=subprocess.run(args,cwd=ROOT,capture_output=True,text=True)
        (a.output/(name+'.log')).write_text(result.stdout+result.stderr)
        (a.output/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        print(name,'PASS' if result.returncode==0 else 'FAIL',flush=True)
        if result.returncode:print(result.stdout+result.stderr);raise RuntimeError(name+' failed')
    binary=a.output/'source_scene_physics_tests'
    run('build',['g++',*flags,*[str(snapshot/s)for s in SOURCES],str(archive),'-Wl,--gc-sections','-lz','-o',str(binary)])
    run('tests',[str(binary.resolve())])
    for source in CONSUMERS:
        run('syntax-'+Path(source).stem,['g++',*flags,'-fsyntax-only',str(snapshot/source)])
    for relative,sha in manifest.items():assert digest(a.output/relative)==sha
    report['passed']=True;report['binary_sha256']=digest(binary)
    (a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
