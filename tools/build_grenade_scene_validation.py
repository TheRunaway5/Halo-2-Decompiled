#!/usr/bin/env python3
"""Compile scene controls against explicitly observed matching frozen Cache ABI.

This is an isolated validation build, not a claim that the current complete
production tree or every renderer archive was rebuilt. Scene, geometry and
test are compiled here; the retained renderer helpers are observed libraries.
"""
import argparse, hashlib, json, os, subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--sanitizer',action='store_true');p.add_argument('--production',action='store_true');p.add_argument('--bank',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 prior=ROOT/'analysis/multiplayer/integration/live-authored-audio-20261010/frozen'; observed=ROOT/'build-sandbox-20261010'
 selected=prior/('sanitizer' if a.sanitizer else 'normal')
 headers=prior/'source/include'
 if a.production:
  observed=ROOT/'build-sandbox-release-20261010';selected=observed;headers=ROOT/'include'
 if a.bank:
  observed=a.bank.resolve()/'archives';selected=observed;headers=a.bank.resolve()/'include'
 sources=[ROOT/x for x in ('tests/source_grenade_scene_tests.cpp','src/source_grenade_scene.cpp','src/source_object_geometry.cpp')]
 renderer=[observed/x for x in ('libhalo2_renderer.a','libhalo2_imgui.a','libhalo2_shadow_render.a','libhalo2_menu_world_storage.a')]
 middle=[selected/x for x in ('libhalo2_systems.a','libhalo2_sound_platform.a')]
 last=[observed/'libhalo2_particle_renderer.a',selected/'libhalo2_cache.a',selected/'libhalo2_native_synchronization.a']
 deps=renderer+middle+last
 before={str(f):digest(f) for f in deps}
 flags=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'] if a.sanitizer else ['-O2']
 binary=a.output.resolve()/'source_grenade_scene_tests'
 cmd=['c++','-std=c++20',*flags,'-ffp-contract=off','-fno-fast-math','-I'+str(headers),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),*[str(f) for f in sources],'-o',str(binary),*[str(f) for f in renderer+middle],'-lSDL2','-lavformat','-lavcodec','-lswresample','-lswscale','-lavutil',str(last[0]),'-lvulkan',str(last[1]),'-lz','-lglm',str(last[2])]
 run=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True);(a.output/'compiler.log').write_text(run.stdout+run.stderr)
 after={str(f):digest(f) for f in deps};assert before==after,'observed link dependency changed during build'
 report=dict(schema='grenade-scene-isolated-build-v1',scope=__doc__,command=cmd,cwd=str(ROOT),environment={k:os.environ.get(k) for k in ('CXX','CXXFLAGS','ASAN_OPTIONS','UBSAN_OPTIONS')},returncode=run.returncode,tool_sha256=digest(Path(__file__)),source_sha256={str(f):digest(f) for f in sources},observed_archive_sha256=before,matching_cache_headers=str(headers),compiler=subprocess.run(['c++','--version'],capture_output=True,text=True).stdout,limits='New scene/geometry/test compiled with stated instrumentation; retained libraries are observed prebuilt archives. Frozen mode uses matching Cache headers and archives; production mode observes the root consistent release build. No complete fresh production-tree or all-library sanitizer claim.')
 if run.returncode==0:report['binary_sha256']=digest(binary)
 (a.output/'build.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');assert run.returncode==0
 print(binary,flush=True)
if __name__=='__main__':main()
