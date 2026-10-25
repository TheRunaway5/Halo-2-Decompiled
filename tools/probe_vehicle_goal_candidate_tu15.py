#!/usr/bin/env python3
"""Exercise an exact rebuilt native candidate without changing the active build.

The executable copy and graphics/capture files belong to run_test.py. Original
assets, current build products and launcher selections remain read-only inputs.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from weapon_test_cache import sha
from validate_weapon_runtime_tu15 import private_display

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','cohort','refresh','validation','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--label',required=True)
    parser.add_argument('--timeout',type=float,default=180)
    parser.add_argument('--warm-gameplay',action='store_true',
                        help='First run actual SDL Banshee firing as an independent gameplay witness')
    parser.add_argument('--warm-runtime',action='store_true',
                        help='Run the ordinary native executable to export its real driver pipeline bank')
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    prepared=json.loads(args.prepared.read_text());cohort=json.loads(args.cohort.read_text())
    refresh=json.loads(args.refresh.read_text());validation=json.loads(args.validation.read_text())
    source=Path(cohort['source']);build=Path(validation['build'])
    if source.parent!=Path(cohort['owner']) or build!=source.parent/'build':raise ValueError('Foreign build')
    if not all(any(row['name']==name and row['exit_code']==0 for row in validation['steps'])
               for name in ('configure','build')):raise ValueError('Current native rebuild has not completed')
    expected=dict(prepared['registered_snapshot_source_files_sha256'])
    for change in cohort['changes']:
        if expected[change['path']]!=change['before']:raise ValueError('Cohort preimage differs')
        expected[change['path']]=change['after']
    expected.update({'data/'+name:pin for name,pin in cohort['private_data_sha256'].items()})
    expected.update(refresh['refreshed_source_sha256'])
    if any(sha(source/name)!=pin for name,pin in expected.items()):raise ValueError('Frozen candidate source drifted')
    args.output.mkdir(parents=True,exist_ok=False);candidate=owner/'candidate';candidate.mkdir()
    original=build/'halo2_native';runtime=candidate/'halo2_native';pin=sha(original)
    shutil.copy2(original,runtime)
    if sha(runtime)!=pin or sha(original)!=pin:raise ValueError('Candidate changed during its private copy')
    (candidate/'source-hashes.json').write_text(json.dumps(expected,indent=2)+'\n')
    warm=None;warm_runtime=None;pipeline=None;gpu=None;gpu_pin=None
    maps=Path(prepared['runtime_cache_inputs']['maps_directory'])
    graphics=owner/'warm-pipeline'
    if args.warm_gameplay or args.warm_runtime:graphics.mkdir()
    if args.warm_gameplay:
        gpu=build/'vehicle_weapon_hud_runtime_tests';gpu_pin=sha(gpu)
        maps=Path(prepared['runtime_cache_inputs']['maps_directory'])
        warm_owner=owner/'warm-gameplay';warm_owner.mkdir()
        raw_report=warm_owner/'report.json';raw_frame=warm_owner/'frame.ppm'
        command=[str(gpu),str(maps/'coagulation.map'),str(maps),'native-mcc',str(raw_frame)]
        with private_display(warm_owner) as display,(warm_owner/'runtime.log').open('w') as log:
            environment={key:value for key,value in os.environ.items() if not key.startswith('HALO2_REFERENCE_')}
            environment.update(DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
                XDG_CACHE_HOME=str(warm_owner/'graphics-cache'),HALO2_PIPELINE_CACHE_DIRECTORY=str(graphics),
                HALO2_TEST_VEHICLE_WEAPON_HUD_CASE='banshee',HALO2_VEHICLE_HUD_GPU_NAME='banshee',
                HALO2_TEST_TURRET_AUDIO='1',HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=str(raw_report))
            environment.pop('HALO2_TEST_BANSHEE_AIRBORNE',None)
            try:code=subprocess.run(command,env=environment,stdout=log,stderr=subprocess.STDOUT,timeout=args.timeout).returncode
            except subprocess.TimeoutExpired:code=124
        data=json.loads(raw_report.read_text()) if raw_report.exists() else {}
        warm=dict(exit_code=code,passed=code==0 and data.get('passed') is True,
            executable_sha256=gpu_pin,command=command,normal_presentation_defaults=True,
            natural_process_shutdown=code==0)
        if raw_report.exists():
            if raw_report.stat().st_size>1024*1024:raise ValueError('Warm gameplay report exceeds its bounded budget')
            shutil.copyfile(raw_report,args.output/'warm-gameplay.json')
            warm['report_sha256']=sha(args.output/'warm-gameplay.json')
        (args.output/'warm-gameplay.log').write_bytes((warm_owner/'runtime.log').read_bytes()[-65536:])
        if raw_frame.exists():
            from PIL import Image
            image=args.output/'warm-gameplay.png'
            with Image.open(raw_frame) as frame:frame.convert('RGB').save(image)
            if image.stat().st_size>1024*1024:raise ValueError('Required native reference image exceeds its bounded budget')
            warm['retained_reference_image']=dict(path=str(image),bytes=image.stat().st_size,sha256=sha(image))
        print(json.dumps(dict(warm_gameplay_passed=warm['passed'])),flush=True)
    if args.warm_runtime:
        warm_owner=owner/'warm-runtime';warm_owner.mkdir()
        command=[str(runtime),'--multiplayer','--map',str(maps/'coagulation.map'),
            '--shared-maps',str(maps),'--local-players','2','--frames','120','--mute','--viewport','800','450']
        with private_display(warm_owner) as display,(warm_owner/'runtime.log').open('w') as log:
            environment={key:value for key,value in os.environ.items() if not key.startswith('HALO2_REFERENCE_')}
            environment.update(DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
                XDG_CACHE_HOME=str(warm_owner/'graphics-cache'),HALO2_PIPELINE_CACHE_DIRECTORY=str(graphics),
                HALO2_PIPELINE_CACHE_TRACE='1')
            try:code=subprocess.run(command,env=environment,stdout=log,stderr=subprocess.STDOUT,timeout=args.timeout).returncode
            except subprocess.TimeoutExpired:code=124
        tail=(warm_owner/'runtime.log').read_text(errors='replace')[-65536:]
        (args.output/'warm-runtime.log').write_text(tail)
        frames=re.search(r'Multiplayer frames=(\d+) ticks=(\d+)',tail)
        warm_runtime=dict(passed=code==0 and frames is not None and int(frames[1])==120 and int(frames[2])>0,
            exit_code=code,command=command,normal_presentation_defaults=True,
            submitted_frames=int(frames[1]) if frames else 0,simulation_ticks=int(frames[2]) if frames else 0,
            natural_process_shutdown=code==0,neutral_inputs_only=True,vehicle_actions_certified=False)
        caches=list(graphics.glob('*.bin'))
        if warm_runtime['passed'] and len(caches)==1:
            pipeline=caches[0]
            warm_runtime['pipeline_reference']=dict(path=str(pipeline),bytes=pipeline.stat().st_size,sha256=sha(pipeline))
        (args.output/'warm-runtime.json').write_text(json.dumps(warm_runtime,indent=2)+'\n')
        print(json.dumps(dict(normal_runtime_completed=warm_runtime['passed'],pipeline_reference_ready=pipeline is not None)),flush=True)
    cases=[]
    for serial in (False,True):
        if args.warm_runtime and pipeline is None:
            cases.append(dict(gameplay_completed=False,not_run=True,serial_copy_reference_control=False,
                reason='Ordinary runtime did not produce the required valid native pipeline reference'))
            break
        label=args.label+('-serial-control' if serial else '-default')
        environment=dict(os.environ,HALO2_COMBAT_INSTALLED_LABEL=label,
            HALO2_COMBAT_INSTALLED_RUNTIME=str(runtime),HALO2_COMBAT_INSTALLED_SHA256=pin,
            HALO2_COMBAT_INSTALLED_TIMEOUT=str(args.timeout),HALO2_COMBAT_INSTALLED_AUDIO='1')
        for key in ('HALO2_COMBAT_INSTALLED_LAUNCHER','HALO2_COMBAT_INSTALLED_PIPELINE_REFERENCE',
                    'HALO2_REFERENCE_SERIAL_PRESENTATION_COPY'):
            environment.pop(key,None)
        if pipeline:
            case_owner=owner/label;case_owner.mkdir()
            environment.update(HALO2_TEST_SCRATCH=str(case_owner),HALO2_COMBAT_INSTALLED_PIPELINE_REFERENCE=str(pipeline))
        if serial:environment['HALO2_REFERENCE_SERIAL_PRESENTATION_COPY']='1'
        command=[sys.executable,'-B',str(ROOT/'tools/probe_installed_banshee_gameplay_20261022.py')]
        result=subprocess.run(command,env=environment,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT)
        path=ROOT/'analysis/combat-presentation-audit-20261022/repair-installed-banshee'/label/'report.json'
        data=json.loads(path.read_text()) if path.exists() else {}
        cases.append(dict(serial_copy_reference_control=serial,returncode=result.returncode,
            report=str(path),report_sha256=sha(path) if path.exists() else None,
            gameplay_completed=data.get('gameplay_completed',False),
            retained_reference_image=data.get('retained_reference_image')))
        print(json.dumps(dict(case=label,exit_code=result.returncode,
            gameplay_completed=cases[-1]['gameplay_completed'],report=str(path))),flush=True)
        if not serial and cases[-1]['gameplay_completed']:break
    unchanged=sha(original)==pin and sha(runtime)==pin and all(sha(source/name)==value for name,value in expected.items())
    if gpu:unchanged &= sha(gpu)==gpu_pin
    report=dict(passed=unchanged and cases[0]['gameplay_completed'],cases=cases,
        runtime_sha256=pin,source_unchanged=unchanged,source_manifest_sha256=sha(candidate/'source-hashes.json'),
        source_refresh_sha256=sha(args.refresh),compiled_native_candidate=True,
        default_path_passed=cases[0]['gameplay_completed'],
        serial_control_passed=any(row['serial_copy_reference_control'] and row['gameplay_completed'] for row in cases),
        copied_executable_bytes=runtime.stat().st_size,raw_owner=str(owner),
        executable_and_raw_captures_retained=False,full_vehicle_parity=False,
        warm_gameplay=warm,warm_runtime=warm_runtime,launcher_promoted=False,tool_sha256=sha(Path(__file__)))
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
