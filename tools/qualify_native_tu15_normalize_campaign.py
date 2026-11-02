#!/usr/bin/env python3
"""Bounded campaign projected-camera exercise for native30BD0.

This checks actual GPU execution and increasing per-site camera admissions.
Campaign frame timing is natural; image hashes are diagnostics, not parity.
Use tools/run_test.py. All images/logs/caches remain scratch-owned.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

from perf_4k60.parity import validate_image
from physics_test_cache import runtime_cache,runtime_map
from qualify_native_tu15_normalize_renderer import census

ROOT=Path(__file__).resolve().parents[1]
SITES={0x166ded,0x166edc,0x166fd2,0x1670c2}


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--mcc-root',type=Path,required=True);p.add_argument('--map',default='00a_introduction')
    p.add_argument('--mode',choices=('pStencil','PathTracing'),default='pStencil');p.add_argument('--report',type=Path,required=True)
    p.add_argument('--frames',type=int,default=30);a=p.parse_args()
    if not 2<=a.frames<=180:raise ValueError('Use a finite2..180frame exercise')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);runtime=a.runtime.resolve(strict=True)
    maps,receipt=runtime_cache(a.mcc_root);map_path=runtime_map(maps,maps/(a.map+'.map'))
    report=dict(passed=False,runtime=str(runtime),runtime_sha256=sha(runtime),map=str(map_path),map_sha256=sha(map_path),
        graphics_mode=a.mode,mcc_inputs=receipt,runs=[],helper_sha256=sha(__file__),scratch_owner=str(scratch),retained_large_artifacts=[],
        scope='Natural-timing finite campaign GPU/camera execution and increasing native166C90 call-site admissions; image hashes do not establish pixel parity.',
        whole_renderer_xbox_visual_parity_verified=False)
    a.report.parent.mkdir(parents=True,exist_ok=True)
    for role,frames in (('reference',a.frames),('candidate-single',1),('candidate',a.frames)):
        name=role+'-'+str(frames);image=scratch/(name+'.ppm');log=scratch/(name+'.log')
        command=['xvfb-run','-a','-s','-screen 0 960x540x24',str(runtime),'--campaign','--map',str(map_path),
            '--shared-maps',str(maps),'--graphics-mode',a.mode,'--frames',str(frames),'--viewport','960','540',
            '--render-size','960','540','--screenshot',str(image),'--mute','--hidden']
        env={k:v for k,v in os.environ.items()if not k.startswith('HALO2_')and k!='WAYLAND_DISPLAY'}
        env.update(HALO2_TEST_SCRATCH=str(scratch),HALO2_NATIVE_NORMALIZE_STATS='1',SDL_AUDIODRIVER='dummy',SDL_VIDEODRIVER='x11',
            HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch/'pipeline'),MESA_SHADER_CACHE_DIR=str(scratch/'mesa'),
            __GL_SHADER_DISK_CACHE_PATH=str(scratch/'nvidia'))
        for key in ('XDG_CONFIG_HOME','XDG_DATA_HOME','XDG_CACHE_HOME'):env[key]=str(scratch/key.lower())
        if role=='reference':env['HALO2_REFERENCE_NORMALIZE_VECTOR']='1'
        # The nested owner kills only this invocation's process group on a
        # timeout, before its Xvfb/process storage is removed.
        driver='import subprocess,sys\ntry:r=subprocess.run(sys.argv[1:],timeout=180)\nexcept subprocess.TimeoutExpired:raise SystemExit(124)\nraise SystemExit(r.returncode)\n'
        owned=[sys.executable,str(ROOT/'tools/run_test.py'),'--scratch-root',str(scratch),'--',sys.executable,'-c',driver,*command]
        start=time.monotonic()
        with log.open('w')as output:r=subprocess.run(owned,cwd=runtime.parent,env=env,stdout=output,stderr=subprocess.STDOUT)
        entry=dict(role=role,frames_requested=frames,command=command,exit_code=r.returncode,elapsed_seconds=time.monotonic()-start,
            valid_image=False,native_census=census(log))
        if image.is_file()and image.stat().st_size<=960*540*3+4096:
            data=image.read_bytes();entry.update(valid_image=validate_image(data,(960,540))is None,image_sha256=hashlib.sha256(data).hexdigest())
        with log.open('rb')as stream:stream.seek(max(0,log.stat().st_size-1800));entry['log_tail']=stream.read().decode(errors='replace')
        with log.open('rb')as stream:
            stream.seek(max(0,log.stat().st_size-4*1024*1024));lines=stream.read().decode(errors='replace').splitlines()
        # Teardown's detailed census can exceed the tail budget. Retain real
        # script/runtime failures separately instead of losing their cause.
        entry['script_faults']=list(dict.fromkeys(line for line in lines if 'Script fault:'in line))[:64]
        entry['runtime_errors']=[line for line in lines if line.startswith('error:')][-16:]
        entry['scene_status']=[line for line in lines if line.startswith('tick=')][-1:]
        if image.is_file()and entry['valid_image']:
            pixels=image.read_bytes().split(b'\n',3)[3]
            entry['image_rgb_range']=[min(pixels),max(pixels)]
        entry['projected_camera_admissions']=sum(s['admitted']for w in entry['native_census']['worlds']for s in w['renderer_sites']if s['pc']in SITES)
        report['runs'].append(entry);a.report.write_text(json.dumps(report,indent=2)+'\n')
        print(role,frames,'frames',r.returncode,'camera admissions',entry['projected_camera_admissions'],flush=True)
        # A rendered scene may still report a real script/runtime failure at
        # shutdown. Collect the candidate diagnostics too, but keep every
        # nonzero exit in the final failure condition below.
        if not entry['valid_image']:break
    if len(report['runs'])==3:
        reference,single,candidate=report['runs']
        report['literal_reference_exercised']=reference['native_census']['admitted']==0 and reference['native_census']['refused'][0]>0
        report['camera_admissions_increased']=candidate['projected_camera_admissions']>single['projected_camera_admissions']+a.frames-1
        report['census_consistent']=all(r['native_census']['census_consistent']for r in report['runs'])
        report['inputs_unchanged']=sha(runtime)==report['runtime_sha256']and sha(map_path)==report['map_sha256']
        report['passed']=all(r['exit_code']==0 and r['valid_image']for r in report['runs'])and all(report[k]for k in
            ('literal_reference_exercised','camera_admissions_increased','census_consistent','inputs_unchanged'))
    a.report.write_text(json.dumps(report,indent=2)+'\n')
    return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
