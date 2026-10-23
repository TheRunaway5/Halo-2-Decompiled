#!/usr/bin/env python3
"""Run the existing strict authored weapon replays on prepared MCC assets.

These exercise actual SDL inputs and submissions, together with their existing
pickup/pose/projectile assertions. This is handheld action coverage, not every
campaign-only definition, Xbox PCM equality or full sound-driver acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from run_test import stop_process_tree

CASES={**{name:'lockout' for name in ('magnum','needler','plasma_pistol','battle_rifle',
    'plasma_rifle','shotgun','sniper_rifle','sword')},'beam_rifle':'burial_mounds',
    'covenant_carbine':'burial_mounds','rocket_launcher':'burial_mounds',
    'brute_shot':'backwash','brute_plasma_rifle':'turf'}


def sha(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('binary','maps','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--case',action='append',choices=tuple(CASES))
    parser.add_argument('--representation',type=int,choices=(2,3),action='append')
    parser.add_argument('--action',choices=('reload','ammo','heat','charge'))
    parser.add_argument('--timeout',type=float,default=300)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.binary=args.binary.resolve(strict=True);args.maps=args.maps.resolve(strict=True)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
    cases=args.case or tuple(CASES)
    if args.action:
        allowed={'reload':{'shotgun','magnum','needler','battle_rifle','sniper_rifle','covenant_carbine','brute_shot','rocket_launcher'},
                 'ammo':{'needler'},'heat':{'plasma_rifle','brute_plasma_rifle','beam_rifle','plasma_pistol'},
                 'charge':{'plasma_pistol'}}[args.action]
        if not args.case or not set(cases)<=allowed:parser.error('Select explicit weapons supported by this authored action replay')
    maps={str(args.maps/(CASES[name]+'.map')):sha(args.maps/(CASES[name]+'.map'))for name in cases}
    for path in maps:
        with Path(path).open('rb') as stream:
            if stream.read(8)!=bytes.fromhex('646165680d000000'):
                parser.error('Handheld runtime requires a prepared MCC version13 map')
    report=dict(scope=__doc__,passed=False,cases=[],binary_sha256=sha(args.binary),maps=maps,
        tool_sha256=sha(Path(__file__)),full_tu15_audio_parity=False,large_artifacts_retained=[])
    def save():(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    save()
    with tempfile.TemporaryDirectory(prefix='all-weapon-audio-',dir=scratch) as owned:
        work=Path(owned)
        for name in cases:
            for representation in args.representation or (2,3):
                label=f'{name}-'+('spartan' if representation==2 else 'elite')+(('-'+args.action) if args.action else '')
                raw=work/(label+'.json')
                env=dict(os.environ,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
                    HALO2_TEST_REPRESENTATION=str(representation),HALO2_TEST_LOCAL_PLAYERS='1',
                    HALO2_TEST_WEAPON_STATE_REPORT=str(raw),
                    HALO2_TEST_VERIFY_WEAPON_AUDIO_REQUESTS='1',
                    HALO2_PIPELINE_CACHE_DIRECTORY=str(work/'vulkan-cache'),
                    MESA_SHADER_CACHE_DIR=str(work/'mesa-cache'),__GL_SHADER_DISK_CACHE_PATH=str(work/'nvidia-cache'))
                command=['xvfb-run','-a','-s','-screen 0 800x600x24',str(args.binary),
                    str(args.maps/(CASES[name]+'.map')),str(args.maps),'native-mcc',name]
                if args.action:command+=['',args.action]
                started=time.monotonic();print('START '+label,flush=True)
                process=subprocess.Popen(command,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                    text=True,start_new_session=True)
                try:
                    output,_=process.communicate(timeout=args.timeout);code=process.returncode;timed_out=False
                except subprocess.TimeoutExpired:
                    stop_process_tree(process);output,_=process.communicate();code=None;timed_out=True
                finally:stop_process_tree(process)
                lines=output.splitlines()
                observations=[line for line in lines if line.startswith(('PASS ','Ordinary firing observations:',
                    'Needler live effects:','Pickup gate observations:','Multiplayer frames=','Attack observations:',
                    'Ready observations:','Native SDL video driver:','Original weapon audio requests:','Staged reload:','Magazine reload:'))]
                errors=[line for line in lines if not line.startswith('Multiplayer SDL audio:') and
                    any(key in line.lower()for key in ('audio:','sound:',
                    'regression failed','first-person audio:','weapon event sound:'))]
                row=dict(case=label,command=command,exit_code=code,timed_out=timed_out,
                    passed=code==0 and not errors and any(line.startswith('PASS '+name)for line in lines),
                    seconds=round(time.monotonic()-started,3),observations=observations,
                    audio_errors=errors[-32:],output_tail=lines[-12:])
                if raw.is_file():
                    data=json.loads(raw.read_text())
                    row['counts']={key:data[key]for key in ('legacy_fired_rounds','firing_sound_submissions',
                        'picked_tick','maximum_heat','maximum_spent','render_loop_completed')if key in data}
                    row['source_owner']={key:data[key]for key in ('tested_weapon_definition','tested_weapon_path',
                        'tested_weapon_owner','tested_action','runtime_cache_source','native_world_constructor')if key in data}
                    row['raw_diagnostic_bytes_removed']=raw.stat().st_size
                    raw.unlink()
                delivery=Path(str(raw)+'.audio.json')
                if delivery.is_file():
                    row['audio_request_comparison']=json.loads(delivery.read_text())
                    row['raw_diagnostic_bytes_removed']=row.get('raw_diagnostic_bytes_removed',0)+delivery.stat().st_size
                    delivery.unlink()
                elif row['passed']:
                    row['passed']=False
                    row['audio_errors'].append('Successful replay supplied no original-to-SDL audio comparison receipt')
                if row['passed'] and not row.get('source_owner',{}).get('tested_weapon_path'):
                    row['passed']=False
                    row['audio_errors'].append('Successful replay supplied no actual weapon-definition path')
                report['cases'].append(row);save();print(json.dumps(row),flush=True)
        report['passed']=bool(report['cases']) and all(row['passed']for row in report['cases'])
        if sha(args.binary)!=report['binary_sha256']:raise RuntimeError('Handheld executable changed during replay')
        for path,pin in maps.items():
            if sha(Path(path))!=pin:raise RuntimeError('Handheld map changed during replay')
    report['execution_scratch_removed']=not work.exists();save()
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
