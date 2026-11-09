#!/usr/bin/env python3
"""Finite authored campaign original/native44710 execution diagnostics.

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
from validate_weapon_runtime_tu15 import private_display

ROOT=Path(__file__).resolve().parents[1]
def sha(p):
    with Path(p).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--mcc-root',type=Path,required=True);p.add_argument('--map',default='03a_oldmombasa')
    p.add_argument('--xbe',type=Path)
    p.add_argument('--require-model-cull',action='store_true')
    p.add_argument('--timeout',type=int,default=180,help='Maximum seconds for each role')
    p.add_argument('--frames',type=int,default=180);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if not(2<=a.frames<=180 and 1<=a.timeout<=900):raise ValueError('Finite frame and timeout bounds required')
    if a.require_model_cull and not a.xbe:p.error('The model cull gate requires an explicit TU1.5 XBE')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);runtime=a.runtime.resolve(strict=True)
    maps,receipt=runtime_cache(a.mcc_root);map_path=runtime_map(maps,maps/(a.map+'.map'))
    xbe=a.xbe.resolve(strict=True)if a.xbe else None
    pins={str(p):sha(p)for p in (runtime,map_path,Path(__file__),*(p for p in (xbe,maps/'shared.map',maps/'single_player_shared.map',maps/'mainmenu.map')if p and p.is_file()))}
    report=dict(passed=False,runtime_sha256=sha(runtime),map_sha256=sha(map_path),mcc_inputs=receipt,runs=[],
        source_runtime_map_sha256=pins,required_model_cull=a.require_model_cull,
        scope='Authored campaign views with original/native44710 selection. The model gate compares full unit/mask/FP per eligible entry on the same World. Natural timing and image hashes are diagnostics.',
        campaign_completion_verified=False,whole_renderer_xbox_visual_parity_verified=False,owned_scratch=str(scratch),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True)
    for role in (('reference','oracle','candidate')if a.require_model_cull else ('reference','candidate')):
        image=scratch/(role+'.ppm');log=scratch/(role+'.log')
        cmd=[str(runtime),'--campaign','--map',str(map_path),
            '--shared-maps',str(maps),'--graphics-mode','PathTracing','--frames',str(a.frames),'--viewport','960','540',
            '--render-size','960','540','--screenshot',str(image),'--mute','--hidden']
        if xbe:cmd+=['--gameplay-xbe',str(xbe)]
        env={k:v for k,v in os.environ.items()if not k.startswith('HALO2_')and k!='WAYLAND_DISPLAY'}
        env.update(HALO2_TEST_SCRATCH=str(scratch),HALO2_NATIVE_CULL_WALK_STATS='1',SDL_AUDIODRIVER='dummy',SDL_VIDEODRIVER='x11',
            HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch/'pipeline'),MESA_SHADER_CACHE_DIR=str(scratch/'mesa'),__GL_SHADER_DISK_CACHE_PATH=str(scratch/'nvidia'))
        for key in ('XDG_CONFIG_HOME','XDG_DATA_HOME','XDG_CACHE_HOME'):env[key]=str(scratch/key.lower())
        if role=='reference':env['HALO2_REFERENCE_MAIN_CULL']='1'
        if role=='oracle':env['HALO2_MAIN_CULL_ORACLE']='1'
        driver='import subprocess,sys\ntry:r=subprocess.run(sys.argv[2:],timeout=int(sys.argv[1]))\nexcept subprocess.TimeoutExpired:raise SystemExit(124)\nraise SystemExit(r.returncode)\n'
        command=[sys.executable,str(ROOT/'tools/run_test.py'),'--scratch-root',str(scratch),'--',sys.executable,'-c',driver,str(a.timeout),*cmd]
        with private_display(scratch)as display,log.open('w')as output:
            env['DISPLAY']=display;r=subprocess.run(command,cwd=runtime.parent,env=env,stdout=output,stderr=subprocess.STDOUT)
        with log.open('rb')as stream:stream.seek(max(0,log.stat().st_size-4*1024*1024));text=stream.read().decode(errors='replace')
        calls=re.findall(r'Native44710 stats: calls=(\d+) frustum_calls=(\d+) sphere_calls=(\d+) native_entries=(\d+) residual_entries=(\d+)',text)
        entry=dict(role=role,command=cmd,exit_code=r.returncode,valid_image=False,native_calls=sum(int(c[0])for c in calls),
            frustum_calls=sum(int(c[1])for c in calls),script_faults=list(dict.fromkeys(line for line in text.splitlines()if 'Script fault:'in line))[:64],
            runtime_errors=[line for line in text.splitlines()if line.startswith('error:')][-16:])
        stats=re.findall(r'Native main cull: views=(\d+) native_entries=(\d+) residual_entries=(\d+) fallbacks=(\d+) '
            r'unit_mismatches=(\d+) mask_mismatches=(\d+) fp_mismatches=(\d+) native_model_entries=(\d+) residual_model_entries=(\d+) scope=campaign',text)
        for i,key in enumerate(('views','native_entries','residual_entries','fallbacks','unit_mismatches','mask_mismatches',
            'fp_mismatches','native_model_entries','residual_model_entries')):entry[key]=sum(int(s[i])for s in stats)
        model_counts={}
        for datum,count in re.findall(r'Native main model cull: scope=campaign datum=(\d+) entries=(\d+)',text):
            model_counts[datum]=model_counts.get(datum,0)+int(count)
        entry['model_datum_entries']=model_counts
        if image.is_file()and image.stat().st_size<=960*540*3+4096:
            data=image.read_bytes();entry.update(valid_image=validate_image(data,(960,540))is None,image_sha256=sha(image))
        report['runs'].append(entry);a.report.write_text(json.dumps(report,indent=2)+'\n');print(role,entry['valid_image'],entry['native_calls'],'native walks',flush=True)
    ref,native=report['runs'][0],report['runs'][-1];report['inputs_unchanged']=all(sha(p)==h for p,h in pins.items())
    report['model_cull_passed']=not a.require_model_cull or ref['native_model_entries']==0 and all(
        r['native_model_entries']>0 and sum(r['model_datum_entries'].values())==r['native_model_entries'] and
        not(r['residual_model_entries']or r['unit_mismatches']or r['mask_mismatches']or r['fp_mismatches'])
        for r in report['runs'][1:])
    report['renderer_repro_passed']=all(r['valid_image']and not r['runtime_errors']for r in report['runs'])and ref['native_calls']==0 and native['native_calls']>0 and native['frustum_calls']>0 and report['inputs_unchanged']
    report['passed']=report['renderer_repro_passed']and report['model_cull_passed']and all(r['exit_code']==0 and not r['script_faults']for r in report['runs'])
    a.report.write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
