#!/usr/bin/env python3
"""Owned Linux/Mac differential cohort for geometry-free world vertex shaders.

Invoke through tools/run_test.py. Actual original VS+GS outputs live only in
that owner and are streamed into a Mac run_test.py owner. Keep bounded reports;
no generated shader binaries, readbacks, validation layer or SDK is retained.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tarfile
import urllib.request

ROOT=Path(__file__).resolve().parents[2]
LAYER='vulkan-validation-layers-1.4.363.0-1-x86_64.pkg.tar.zst'
INPUTS={LAYER:'904f4eba2e708c9c96579900688a284309cd56dc09b5af3f78c563ffda13df9e',
        LAYER+'.sig':'e8f686fa461dd25015ea5a93a0cf79e0d98a4d246109e4536eba2900766e889d'}


def validation_environment(scratch:Path,output:Path):
    receipt={'scratch_owner':str(scratch)}
    for name,expected in INPUTS.items():
        url='https://geo.mirror.pkgbuild.com/extra/os/x86_64/'+name
        request=urllib.request.Request(url,headers={'User-Agent':'Halo2-port-validation'})
        with urllib.request.urlopen(request,timeout=90) as response:
            data=response.read();actual=hashlib.sha256(data).hexdigest()
            if actual!=expected:raise ValueError('Validation layer checksum mismatch: '+name)
            (scratch/name).write_bytes(data)
            receipt[name]={'url':response.url,'bytes':len(data),'sha256':actual}
    verify=subprocess.run(['pacman-key','--verify',str(scratch/(LAYER+'.sig')),str(scratch/LAYER)],capture_output=True,text=True)
    receipt['signature']={'exit_code':verify.returncode,'output':(verify.stdout+verify.stderr)[-8000:]}
    (output/'validation-layer-input.json').write_text(json.dumps(receipt,indent=2)+'\n');verify.check_returncode()
    subprocess.run(['tar','-xf',str(scratch/LAYER),'-C',str(scratch)],check=True)
    env=dict(os.environ);env['VK_LAYER_PATH']=str(scratch/'usr/share/vulkan/explicit_layer.d');env['LD_LIBRARY_PATH']=str(scratch/'usr/lib')+(':'+env['LD_LIBRARY_PATH'] if env.get('LD_LIBRARY_PATH') else '')
    return env


def source_hashes():
    paths=['tools/compile_menu_world_vertex_catalog.py','tools/world_triangle_vertex.py','tools/macos/test_world_triangles.py',
      'src/ui/menu_world_native_draw.cpp','src/ui/menu_world_source_storage.cpp','src/ui/menu_world_triangle_storage.cpp','src/ui/menu_world_vertex_catalog.cpp','src/rendering/renderer/renderer.cpp','tools/macos/graphics_probe.cpp',
      'include/halo2/menu_world_vertex_catalog.hpp','include/halo2/menu_world_native_draw.hpp','include/halo2/menu_world_source_storage.hpp','include/halo2/menu_world_triangle_storage.hpp','include/halo2/macos/vulkan_math.hpp',
      'shaders/source_world_vertex_fetch.glsl','shaders/world_depth_geometry.glsl','shaders/menu_world.frag','shaders/menu_world_fragment.glsl','shaders/source_depth_storage.glsl',
      'tests/macos/depth_storage/gpu.hpp','tests/menu_world_vertex_catalog_capture.geom']
    paths += [str(p.relative_to(ROOT)) for p in (ROOT/'tests/macos/world_triangles').glob('*') if p.is_file()]
    return {p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sorted(paths)}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--mac',default='codex-mac')
    parser.add_argument('--mac-root',default='/Users/eric/Developer/Halo2-macos-port')
    parser.add_argument('--mac-output',default='build/test-results/world-triangle-mac')
    parser.add_argument('--engine-only',action='store_true')
    parser.add_argument('--mac-fast-math',choices=('0','1','2'),help='Explicit MoltenVK compiler-policy diagnostic')
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
    env=validation_environment(scratch,args.output)
    if args.engine_only:
        command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--target','hologram_draw_gpu_tests','--target','menu_world_source_storage_tests','--filter','^(original_hologram_raw_gpu|original_hologram_triangle_carrier_gpu|retail_menu_world_source_storage)$','--test-jobs','1','--retain-report','hologram-draw/report.json','--retain-report','hologram-carrier/report.json','--output',str(args.output)]
        return subprocess.run(command,cwd=ROOT,env=env).returncode
    oracle=scratch/'oracle'
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(ROOT/'tests/macos/world_triangles'),'--target','world_triangle_tests','--target','vulkan_portability_tests','--filter','^(world_triangle_host|vulkan_portability)$','--cmake-arg=-DHALO2_WORLD_TRIANGLE_ORACLE='+str(oracle),'--test-jobs','1','--retain-report','world-triangle-host.json','--retain-report','catalog/compiled.json','--output',str(args.output)]
    result=subprocess.run(command,cwd=ROOT,env=env)
    if result.returncode:return result.returncode
    expected_sources=source_hashes()
    receipt={'sources':expected_sources,'files':{p.name:{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(oracle.glob('*.bin'))}}
    if len(receipt['files'])!=322:raise ValueError('Original-path output coverage incomplete')
    (args.output/'oracle-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    (oracle/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    # The remote owner checks every streamed original result and the exact
    # source snapshot before configuring its universal test build.
    remote='expected_sources='+repr(expected_sources)+'\n'+'''from pathlib import Path
import hashlib,json,os,subprocess,sys,tarfile
actual={p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in expected_sources}
assert actual==expected_sources,'Mac source admission mismatch: '+str([p for p in actual if actual[p]!=expected_sources[p]])
output=Path(sys.argv[1]);output.mkdir(parents=True,exist_ok=True)
(output/'source-admission.json').write_text(json.dumps(actual,indent=2)+'\\n')
oracle=Path(os.environ['HALO2_TEST_SCRATCH'])/'oracle';oracle.mkdir()
with tarfile.open(fileobj=sys.stdin.buffer,mode='r|') as archive:archive.extractall(oracle,filter='data')
receipt=json.loads((oracle/'receipt.json').read_text())
assert receipt['sources']==expected_sources and len(receipt['files'])==322
for name,metadata in receipt['files'].items():
    assert Path(name).name==name
    path=oracle/name
    assert path.stat().st_size==metadata['bytes'] and hashlib.sha256(path.read_bytes()).hexdigest()==metadata['sha256']
(output/'oracle-receipt.json').write_text(json.dumps(receipt,indent=2)+'\\n')
command=[sys.executable,'-B','tools/macos/test_port.py','--engine-configure-check','--source','tests/macos/world_triangles','--engine-target','world_triangle_tests','--engine-test-filter','^(world_triangle_arm64|world_triangle_x86_64|vulkan_portability)$','--cmake-arg=-DHALO2_WORLD_TRIANGLE_ORACLE='+str(oracle),'--retain-report','world-triangle-arm64.json','--retain-report','world-triangle-x86_64.json','--retain-report','catalog/compiled.json','--output',str(output)]
raise SystemExit(subprocess.run(command).returncode)
'''
    policy='' if args.mac_fast_math is None else 'MVK_CONFIG_FAST_MATH_ENABLED='+args.mac_fast_math+' '
    remote_command='cd '+shlex.quote(args.mac_root)+' && PATH=/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin '+policy+'caffeinate -i '+shlex.join(['python3','-B','tools/run_test.py','--','python3','-B','-c',remote,args.mac_output])
    process=subprocess.Popen(['ssh','-o','BatchMode=yes','-o','ConnectTimeout=15',args.mac,remote_command],stdin=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdin,mode='w|') as archive:
            for path in sorted(oracle.iterdir()):archive.add(path,arcname=path.name,recursive=False)
    finally:
        process.stdin.close()
    return process.wait()

if __name__=='__main__':raise SystemExit(main())
