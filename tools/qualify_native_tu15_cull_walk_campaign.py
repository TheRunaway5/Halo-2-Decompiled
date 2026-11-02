#!/usr/bin/env python3
"""Finite authored campaign original/native44710 GPU execution diagnostics.

Natural campaign timing is not a pixel-parity reference. Known real script
faults remain visible and prevent a full campaign acceptance claim. Use run_test.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from perf_4k60.parity import validate_image
from physics_test_cache import runtime_cache,runtime_map

ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--mcc-root',type=Path,required=True);p.add_argument('--map',default='03a_oldmombasa')
    p.add_argument('--frames',type=int,default=180);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if not 2<=a.frames<=180:raise ValueError('Finite2..180frame exercise required')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);runtime=a.runtime.resolve(strict=True)
    maps,receipt=runtime_cache(a.mcc_root);map_path=runtime_map(maps,maps/(a.map+'.map'))
    report=dict(passed=False,runtime_sha256=sha(runtime),map_sha256=sha(map_path),mcc_inputs=receipt,runs=[],
        scope='Authored real campaign views with explicit original/native44710 parent selection and positive native invocation census. Natural timing/image hashes are diagnostics. Existing script faults prevent full campaign acceptance.',
        campaign_completion_verified=False,whole_renderer_xbox_visual_parity_verified=False,owned_scratch=str(scratch),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True)
    for role in ('reference','candidate'):
        image=scratch/(role+'.ppm');log=scratch/(role+'.log')
        cmd=['xvfb-run','-a','-s','-screen 0 960x540x24',str(runtime),'--campaign','--map',str(map_path),
            '--shared-maps',str(maps),'--graphics-mode','PathTracing','--frames',str(a.frames),'--viewport','960','540',
            '--render-size','960','540','--screenshot',str(image),'--mute','--hidden']
        env={k:v for k,v in os.environ.items()if not k.startswith('HALO2_')and k!='WAYLAND_DISPLAY'}
        env.update(HALO2_TEST_SCRATCH=str(scratch),HALO2_NATIVE_CULL_WALK_STATS='1',SDL_AUDIODRIVER='dummy',SDL_VIDEODRIVER='x11',
            HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch/'pipeline'),MESA_SHADER_CACHE_DIR=str(scratch/'mesa'),__GL_SHADER_DISK_CACHE_PATH=str(scratch/'nvidia'))
        for key in ('XDG_CONFIG_HOME','XDG_DATA_HOME','XDG_CACHE_HOME'):env[key]=str(scratch/key.lower())
        if role=='reference':env['HALO2_REFERENCE_MAIN_CULL']='1'
        driver='import subprocess,sys\ntry:r=subprocess.run(sys.argv[1:],timeout=180)\nexcept subprocess.TimeoutExpired:raise SystemExit(124)\nraise SystemExit(r.returncode)\n'
        command=[sys.executable,str(ROOT/'tools/run_test.py'),'--scratch-root',str(scratch),'--',sys.executable,'-c',driver,*cmd]
        with log.open('w')as output:r=subprocess.run(command,cwd=runtime.parent,env=env,stdout=output,stderr=subprocess.STDOUT)
        with log.open('rb')as stream:stream.seek(max(0,log.stat().st_size-4*1024*1024));text=stream.read().decode(errors='replace')
        calls=re.findall(r'Native44710 stats: calls=(\d+) frustum_calls=(\d+) sphere_calls=(\d+) native_entries=(\d+) residual_entries=(\d+)',text)
        entry=dict(role=role,command=cmd,exit_code=r.returncode,valid_image=False,native_calls=sum(int(c[0])for c in calls),
            frustum_calls=sum(int(c[1])for c in calls),script_faults=list(dict.fromkeys(line for line in text.splitlines()if 'Script fault:'in line))[:64],
            runtime_errors=[line for line in text.splitlines()if line.startswith('error:')][-16:])
        if image.is_file()and image.stat().st_size<=960*540*3+4096:
            data=image.read_bytes();entry.update(valid_image=validate_image(data,(960,540))is None,image_sha256=sha(image))
        report['runs'].append(entry);a.report.write_text(json.dumps(report,indent=2)+'\n');print(role,entry['valid_image'],entry['native_calls'],'native walks',flush=True)
    ref,native=report['runs'];report['inputs_unchanged']=sha(runtime)==report['runtime_sha256']and sha(map_path)==report['map_sha256']
    report['renderer_repro_passed']=all(r['valid_image']and not r['runtime_errors']for r in report['runs'])and ref['native_calls']==0 and native['native_calls']>0 and native['frustum_calls']>0 and report['inputs_unchanged']
    report['passed']=report['renderer_repro_passed']and all(r['exit_code']==0 and not r['script_faults']for r in report['runs'])
    a.report.write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
