#!/usr/bin/env python3
"""Run actual MCC shot/reload regressions while a disposable build is alive.

Run through tools/run_test.py. Copies only the completed test executable and
its shader inputs into that owner's scratch, then removes them on exit. The
parent build remains owned by tools/run_tests.py. No installed runtime changes.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

from run_test import stop_process_tree
from validate_disposable_weapon_runtime_tu15 import has_writer, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-report', type=Path)
    parser.add_argument('--build-current', action='store_true')
    parser.add_argument('--mcc-root', type=Path)
    parser.add_argument('--jobs', type=int, default=8)
    parser.add_argument('--maps', type=Path, required=True)
    parser.add_argument('--image', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--wait-seconds', type=float, default=2400)
    parser.add_argument('--case', action='append', choices=('coagulation1','coagulation2','coagulation4','lockout1'))
    parser.add_argument('--cycles', type=int, choices=range(1,11), default=3)
    parser.add_argument('--timeout', type=float, default=300)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    report = dict(passed=False, status='waiting', cases=[], runner_scratch=str(scratch),
                  scope='Actual local weapon shots/reloads and BSP ambient; walking acceptance separate',
                  walking_acceptance_verified=False, full_tu15_audio_parity=False,
                  large_artifacts_retained=[], tool_sha256=sha(Path(__file__)))
    execution=tempfile.TemporaryDirectory(prefix='weapon-audio-',dir=scratch)
    work=Path(execution.name)
    report['execution_scratch']=str(work)
    def save():
        (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    save()
    try:
        if args.build_current:
            if not args.mcc_root:raise ValueError('--build-current requires --mcc-root')
            root=Path(__file__).resolve().parents[1]
            prepare=[sys.executable,str(root/'tools/validate_weapon_material_snapshot_tu15.py'),
                     '--mcc-root',str(args.mcc_root.resolve()),'--output',str((args.output/'source-preparation').resolve()),
                     '--owned-build-worker','--prepare-only','--release-only']
            report['prepare_command']=prepare;save()
            with (args.output/'prepare.log').open('w') as log:
                subprocess.run(prepare,stdout=log,stderr=subprocess.STDOUT,check=True)
            source=scratch/'source'
            # The release bundler intentionally rejects symlinked authored
            # data. Freeze this small input tree instead of borrowing it.
            if (source/'data').is_symlink():
                (source/'data').unlink()
                shutil.copytree(root/'data',source/'data')
            report['additional_frozen_inputs']={str(path.relative_to(source)):sha(path)
                for path in sorted((source/'data').rglob('*')) if path.is_file()}
            args.build_report=args.output/'cmake/report.json'
            command=[sys.executable,str(root/'tools/run_tests.py'),'--build-worker',
                     '--source',str(source),'--output',str(args.output/'cmake'),
                     '--target','multiplayer_audio_tests',
                     '--target','multiplayer_audio_runtime_tests','--target','multiplayer_all_sound_resources_tests',
                     '--target','halo2_native',
                     '--filter','^(multiplayer_authored_audio|owned_multiplayer_(all|latest_dlc)_sound_resources)$',
                     '--jobs',str(args.jobs),'--test-jobs','1']
            report.update(status='building',build_command=command);save()
            print('Building frozen weapon audio source and runtime checks',flush=True)
            with (args.output/'cmake-driver.log').open('w') as log:
                result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
            if result.returncode:raise RuntimeError('Frozen CMake validation failed; inspect cmake/report.json and build.log')
        if not args.build_report:raise ValueError('Supply --build-report or --build-current')
        deadline = time.monotonic()+args.wait_seconds
        previous = None
        while time.monotonic()<deadline:
            parent = json.loads(args.build_report.read_text())
            if parent['status'] in ('failed', 'interrupted'):
                raise RuntimeError('Parent build failed before runtime validation')
            build = Path(parent['build'])
            source = build/'multiplayer_audio_runtime_tests'
            if source.is_file() and os.access(source, os.X_OK):
                stat = source.stat(); sample = stat.st_size, stat.st_mtime_ns
                if sample==previous and stat.st_size and not has_writer(source):
                    break
                previous=sample
            time.sleep(1)
        else:
            raise TimeoutError('Runtime test did not link within the declared wait')
        binary=work/'multiplayer_audio_runtime_tests'; shaders=work/'shaders'
        before=sha(source);shutil.copy2(source,binary)
        if sha(binary)!=before:raise RuntimeError('Runtime test changed while copying')
        shutil.copytree(build/'shaders',shaders)
        report.update(status='running', binary_sha256=before, image_sha256=sha(args.image))
        save()
        env=dict(os.environ,SDL_AUDIODRIVER='dummy',SDL_VIDEODRIVER='x11',
                 HALO2_TEST_SCRATCH=str(work),TMPDIR=str(work),TMP=str(work),TEMP=str(work))
        for name,map_name,flags in [('coagulation1','coagulation',[]),
                                    ('coagulation2','coagulation',['--split2']),
                                    ('coagulation4','coagulation',['--split4']),
                                    ('lockout1','lockout',[])]:
            if args.case and name not in args.case:continue
            command=['xvfb-run','-a','-s','-screen 0 800x600x24',str(binary),
                     str((args.maps/(map_name+'.map')).resolve()),str(args.maps.resolve()),
                     str(args.image.resolve()),*flags,'--weapons','--shaders',str(shaders),'--cycles',str(args.cycles)]
            process=subprocess.Popen(command,env=env,stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT,text=True,start_new_session=True)
            try:
                output,_=process.communicate(timeout=args.timeout);code=process.returncode
            except subprocess.TimeoutExpired:
                stop_process_tree(process);output,_=process.communicate();code=None
            finally:
                stop_process_tree(process)
            lines=output.splitlines()
            selected=[line for line in lines if any(s in line.lower() for s in ('audio:', 'sound:', 'sound ', 'actual sdl audio:', 'pass actual', 'differs', 'regression failed'))]
            row=dict(name=name,command=command,exit_code=code,passed=code==0,
                     audio_lines=selected[-100:],tail=lines[-8:])
            report['cases'].append(row);save();print(json.dumps(row),flush=True)
        report.update(passed=all(row['passed'] for row in report['cases']),status='complete')
    except Exception as error:
        report.update(status='failed',error=str(error));print(str(error),flush=True)
    finally:
        execution.cleanup()
        report['execution_scratch_removed']=not work.exists()
    save()
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
