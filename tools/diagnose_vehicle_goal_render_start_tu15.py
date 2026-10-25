#!/usr/bin/env python3
"""Capture a bounded native startup stack with GDB as the owned parent.

This is a diagnostic stop, never gameplay acceptance. No source instruction,
register, memory, pose, admission or function return is replaced. Temporary
display, debugger commands, log and caches belong to tools/run_test.py.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time

from validate_weapon_runtime_tu15 import private_display
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','cohort','refresh','validation','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--stop-after',type=int,default=90)
    args=parser.parse_args()
    if not 15<=args.stop_after<=180:parser.error('Declare15..180 seconds before the diagnostic stop')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    prepared=json.loads(args.prepared.read_text());cohort=json.loads(args.cohort.read_text())
    refresh=json.loads(args.refresh.read_text());validation=json.loads(args.validation.read_text())
    borrowed=Path(cohort['owner']);frozen=borrowed/'source';build=borrowed/'build'
    if (owner==borrowed or Path(prepared['runner_scratch'])!=borrowed or
            Path(refresh['owner'])!=borrowed or Path(validation['build'])!=build):
        raise ValueError('Separate authenticated native build owner required')
    if not all(any(row['name']==name and row['exit_code']==0 for row in validation['steps'])
            for name in ('configure','build')):raise ValueError('Native build must finish first')
    expected=dict(prepared['registered_snapshot_source_files_sha256'])
    for row in cohort['changes']:
        if expected[row['path']]!=row['before']:raise ValueError('Cohort preimage differs')
        expected[row['path']]=row['after']
    expected.update({'data/'+name:pin for name,pin in cohort['private_data_sha256'].items()})
    expected.update(refresh['refreshed_source_sha256'])
    if any(sha(frozen/name)!=pin for name,pin in expected.items()):raise ValueError('Frozen source drifted')
    runtime=build/'vehicle_weapon_hud_runtime_tests';pin=sha(runtime)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    script=owner/'diagnostic.gdb'
    script.write_text('''set pagination off
set confirm off
set print frame-arguments none
set disable-randomization off
starti
python
import gdb,os,signal,threading
pid=gdb.selected_inferior().pid
timer=threading.Timer(STOP_AFTER,lambda:os.kill(pid,signal.SIGINT))
timer.daemon=True
timer.start()
try:gdb.execute('continue')
finally:timer.cancel()
alive=bool(gdb.selected_inferior().threads())
print('[VEHICLE-STACK] stopped_alive='+str(alive),flush=True)
if alive:
 gdb.execute('thread 1')
 gdb.execute('bt 64')
 frame=gdb.newest_frame()
 for depth in range(96):
  if frame is None:break
  name=frame.name() or ''
  if 'halo2::' in name:
   print('[VEHICLE-NATIVE-FRAME] depth='+str(depth)+' '+gdb.execute('info symbol '+str(frame.pc()),to_string=True).strip(),flush=True)
  frame=frame.older()
 gdb.execute('thread apply all bt 4')
 gdb.execute('kill')
end
'''.replace('STOP_AFTER',str(args.stop_after)))
    maps=Path(prepared['runtime_cache_inputs']['maps_directory'])
    command=['gdb','-q','-nx','-batch','-x',str(script),'--args',str(runtime),
        str(maps/'coagulation.map'),str(maps),'native-mcc',str(owner/'frame.ppm')]
    environment={key:value for key,value in os.environ.items() if not key.startswith(
        ('HALO2_REFERENCE_','HALO2_CAPTURE_','HALO2_TEST_')) or key=='HALO2_TEST_SCRATCH'}
    environment.update(SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
        XDG_CACHE_HOME=str(owner/'graphics-cache'),HALO2_TEST_TURRET_AUDIO='1',
        HALO2_TEST_VEHICLE_WEAPON_HUD_CASE='wraith',HALO2_VEHICLE_HUD_GPU_NAME='wraith',
        HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=str(owner/'gameplay.json'))
    started=time.monotonic();log=owner/'runtime.log'
    with private_display(owner) as display,log.open('w') as stream:
        environment['DISPLAY']=display
        try:code=subprocess.run(command,env=environment,stdout=stream,
            stderr=subprocess.STDOUT,timeout=args.stop_after+90).returncode
        except subprocess.TimeoutExpired:code=124
    raw=log.read_text(errors='replace');at=raw.rfind('[VEHICLE-STACK]')
    stack=raw[at:] if at>=0 else raw[-20000:]
    if len(stack.encode())>48000:stack=stack[:48000]
    unchanged=sha(runtime)==pin and all(sha(frozen/name)==value for name,value in expected.items())
    report=dict(diagnostic_completed=code==0 and '[VEHICLE-STACK] stopped_alive=True' in stack and unchanged,
        passed=False,diagnostic_only=True,full_vehicle_parity=False,owner=str(owner),
        borrowed_owner=str(borrowed),runtime_sha256=pin,command=command,
        stop_after_seconds=args.stop_after,elapsed_seconds=time.monotonic()-started,
        returncode=code,debugger_is_owned_parent=True,source_register_or_memory_writes=False,
        controlled_SIGINT_and_termination=True,original_opcode_execution=False,
        stack=stack,borrowed_inputs_unchanged=unchanged,large_artifacts_retained=False,
        tool_sha256=sha(Path(__file__)))
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+chr(10))
    print(json.dumps({key:report[key] for key in ('diagnostic_completed','returncode','elapsed_seconds')}),flush=True)
    return 0 if report['diagnostic_completed'] else 1


if __name__=='__main__':raise SystemExit(main())
