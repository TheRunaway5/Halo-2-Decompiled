#!/usr/bin/env python3
"""Regress Mac graphics/fetch/depth/sky with the production compiler policy.

Run through tools/run_test.py. One remote scratch owner holds the authenticated
SDK and streamed sky references across disposable CMake runs, then removes both.
"""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tarfile

ROOT=Path(__file__).resolve().parents[2]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mac',default='codex-mac')
    parser.add_argument('--mac-root',default='/Users/eric/Developer/Halo2-macos-port')
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--mac-output',default='build/test-results/shader-math-regressions')
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output.mkdir(parents=True,exist_ok=True)
    spec=importlib.util.spec_from_file_location('sky_references',ROOT/'tests/macos/sky_stencil/prepare_references.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    reference=scratch/'sky-references';receipt=module.prepare(reference)
    sources={'include/halo2/macos/vulkan_math.hpp','tools/macos/graphics_probe.cpp','tools/macos/test_port.py','tools/macos/test_shader_math.py',
      'src/rendering/world/source_sky_stencil.cpp','src/rendering/weather/source_weather_depth_alias.cpp','include/halo2/source_sky_stencil.hpp','include/halo2/source_weather_depth_alias.hpp',
      'include/halo2/source_depth_storage.hpp','cmake/SourceSkyStencil.cmake','cmake/SourceSkyStencilReference.cmake','cmake/SourceWeatherDepthAlias.cmake',
      'shaders/source_world_vertex_fetch.glsl','shaders/world_depth_geometry.glsl','shaders/world_depth_fragment.glsl','shaders/source_depth_storage.glsl'}
    for directory in ('tests/macos/depth_storage','tests/macos/vertex_fetch','tests/macos/sky_stencil'):
        sources.update(str(p.relative_to(ROOT)) for p in (ROOT/directory).iterdir() if p.is_file())
    sources.update(str(p.relative_to(ROOT)) for pattern in ('source_sky_stencil*','source_weather_depth_alias*','source_depth_color_writeback*') for p in (ROOT/'shaders').glob(pattern) if p.is_file())
    expected={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sorted(sources)}
    (args.output/'sources.json').write_text(json.dumps(expected,indent=2)+'\n')
    (args.output/'references.json').write_text(json.dumps(receipt,indent=2)+'\n')
    remote='expected='+repr(expected)+'\n'+'''from pathlib import Path
import hashlib,json,os,subprocess,sys,tarfile
actual={p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in expected}
assert actual==expected,'Source admission differs: '+str([p for p in actual if actual[p]!=expected[p]])
output=Path(sys.argv[1]);output.mkdir(parents=True,exist_ok=True)
(output/'sources.json').write_text(json.dumps(actual,indent=2)+'\\n')
reference=Path(os.environ['HALO2_TEST_SCRATCH'])/'sky-references';reference.mkdir()
with tarfile.open(fileobj=sys.stdin.buffer,mode='r|') as archive:archive.extractall(reference,filter='data')
receipt=json.loads((reference/'reference-receipt.json').read_text());assert len(receipt)==12
for item in receipt:
    path=reference/item['path'];assert path.is_relative_to(reference)
    assert path.stat().st_size==item['bytes'] and hashlib.sha256(path.read_bytes()).hexdigest()==item['sha256']
(output/'references.json').write_text(json.dumps(receipt,indent=2)+'\\n')
commands=[]
first=[sys.executable,'-B','tools/macos/test_port.py','--engine-configure-check','--source','tests/macos/depth_storage','--engine-target','depth_storage_tests','--engine-test-filter','^(depth_storage_arm64|depth_storage_x86_64|vulkan_portability)$','--retain-report','depth-storage-arm64.json','--retain-report','depth-storage-x86_64.json','--output',str(output/'depth')]
result=subprocess.run(first);commands.append({'command':first,'exit_code':result.returncode})
configuration=output/'depth/report.json'
if not configuration.exists():raise SystemExit(result.returncode or 1)
configured=json.loads(configuration.read_text())['steps'][0]['command']
common=[value for value in configured if value.startswith(('-D','-G'))]
common=[value for value in common if not value.startswith(('-DBUILD_TESTING=','-DCMAKE_BUILD_TYPE='))]
cohorts=[('fetch','tests/macos/vertex_fetch',['vertex_fetch_tests','vulkan_portability_tests'],'^(macos_vertex_fetch_arm64|macos_vertex_fetch_x86_64|vulkan_portability)$',['vertex-fetch-arm64.json','vertex-fetch-x86_64.json'],[]),
 ('sky','tests/macos/sky_stencil',['macos_sky_stencil_tests','vulkan_portability_tests'],'^(macos_sky_stencil_arm64|macos_sky_stencil_x86_64|vulkan_portability)$',['sky-stencil-arm64.json','sky-stencil-x86_64.json','reference-receipt.json'],['-DHALO2_SKY_REFERENCE_ROOT='+str(reference)]),
 ('graphics','.', ['halo2_macos_graphics_probe','vulkan_portability_tests'],'^(macos_moltenvk_|macos_bundle$|vulkan_portability$)',['macos-arm64.json','macos-x86_64.json','Halo 2 Graphics Check.bundle.json','Halo 2 Graphics Check.app/Contents/Resources/bundle-manifest.json'],['-DHALO2_MACOS_BRINGUP_ONLY=ON'])]
for name,source,targets,filter,reports,extra in cohorts:
    command=[sys.executable,'-B','tools/run_tests.py','--source',source,'--output',str(output/name),'--jobs','2','--test-jobs','1','--filter',filter]
    command += [value for target in targets for value in ('--target',target)]
    command += [value for report in reports for value in ('--retain-report',report)]
    command += ['--cmake-arg='+value for value in common+extra]
    status=subprocess.run(command).returncode;commands.append({'command':command,'exit_code':status})
(output/'cohorts.json').write_text(json.dumps(commands,indent=2)+'\\n')
raise SystemExit(1 if any(row['exit_code'] for row in commands) else 0)
'''
    command='cd '+shlex.quote(args.mac_root)+' && PATH=/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin MVK_CONFIG_FAST_MATH_ENABLED=1 caffeinate -i '+shlex.join(['python3','-B','tools/run_test.py','--','python3','-B','-c',remote,args.mac_output])
    process=subprocess.Popen(['ssh','-o','BatchMode=yes','-o','ConnectTimeout=15',args.mac,command],stdin=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdin,mode='w|') as archive:
            for path in sorted(reference.rglob('*')):
                if path.is_file():archive.add(path,arcname=str(path.relative_to(reference)),recursive=False)
    finally:process.stdin.close()
    status=process.wait()
    (args.output/'execution.json').write_text(json.dumps({'command':command,'exit_code':status},indent=2)+'\n')
    return status

if __name__=='__main__':raise SystemExit(main())
