#!/usr/bin/env python3
"""Observe authored object-loop scheduling in an owned TU1.5 source world.

Each command is a real declared-birth match, never a supplied sound admission.
This diagnostic distinguishes loop registration from original requests. Native
The normal-loop packet mode additionally consumes actual manual SDL PCM.
Complete loop scheduling and launcher split-screen remain separate acceptance.
"""
import argparse,hashlib,json,os,subprocess,tempfile
from pathlib import Path
from build_launcher_engine import source_hashes

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/combat-presentation-audit-20261022'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map',choices=('coagulation','lockout'),default='coagulation')
    parser.add_argument('--owner',choices=('banshee','teleporter','sound-scenery','extra-rocks','skittery-bugs','flies','birds-insects','powerhaus','under-glass','green-hum'))
    parser.add_argument('--mode',choices=('baseline','source-frame','listeners-and-source-frame','selected-loop','nonspatial-control','native-views-frame','native-views-empty','native-views-far','native-loop-packet','native-loop-packet-converted','native-object-loop','native-object-loop-omit','native-object-loop-retire','native-object-loop-maneuvers'))
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    followup=BASE/'repair-followup-20261023';source=owner/'source'
    pins=json.loads((followup/'source-hashes.json').read_text())
    admission=json.loads((followup/'diagnostic-admission.json').read_text())
    assert source_hashes(source)==pins
    assert sha(source/'tests/object_loop_audio_runtime_tu15_tests.cpp')==admission['object_loop_fixture_sha256']
    exe=owner/'build/object_loop_audio_runtime_tu15_tests';pin=sha(exe)
    assert admission.get('build_completed') and admission['executables'][exe.name]==pin,'Current diagnostic build has not passed'
    maps=(ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11').resolve()
    out=BASE/'repair-scenery-audio/runtime';attempt=2
    while out.exists():out=BASE/'repair-scenery-audio'/('runtime-'+str(attempt));attempt+=1
    out.mkdir(parents=True)
    results=[]
    with tempfile.TemporaryDirectory(prefix='scenery-audio-',dir=owner) as temporary:
        scratch=Path(temporary)
        for kind in ((args.owner,) if args.owner else ('banshee','teleporter','sound-scenery')):
            for mode in ((args.mode,) if args.mode else ('baseline','source-frame','listeners-and-source-frame')):
                name=kind+'-'+mode;path=out/(name+'.json')
                cmd=[str(exe),str(maps/(args.map+'.map')),str(maps),str(path),mode,kind]
                with (scratch/'run.log').open('w') as log:
                    try:code=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,timeout=420).returncode
                    except subprocess.TimeoutExpired:code=124
                report=json.loads(path.read_text()) if path.exists() else {}
                result=dict(case=name,command=cmd,exit_code=code,completed=bool(report.get('ticks')),
                    passed=bool(report.get('passed')),
                    loop_records=len(report.get('initial_loops',[])),original_request_count=report.get('request_count'),
                    target_original_request_count=report.get('target_original_request_count'),
                    error=report.get('error'),native_PCM_verified=bool(report.get('native_PCM_verified')),
                    native_loop_packet_peak=report.get('native_loop_packet_peak'),
                    completed_native_loop_scheduler_verified=bool(report.get('completed_native_loop_scheduler_verified')))
                (out/(name+'.log')).write_text((scratch/'run.log').read_text(errors='replace')[-8192:])
                results.append(result);print(json.dumps(result),flush=True)
    receipt=dict(scope=__doc__,cases=results,executable_sha256=pin,executable_unchanged=sha(exe)==pin,
        source_snapshot_role=admission.get('source_snapshot_role'),
        production_unchanged=source_hashes(source)==pins,scratch_removed=not scratch.exists())
    (out/'cohort.json').write_text(json.dumps(receipt,indent=2)+'\n')
    native_output=(args.mode or '').startswith(('native-loop-packet','native-object-loop'))
    return 0 if all(r['completed'] and (not native_output or r['passed']) for r in results) and receipt['executable_unchanged'] and receipt['production_unchanged'] else 1

if __name__=='__main__':raise SystemExit(main())
