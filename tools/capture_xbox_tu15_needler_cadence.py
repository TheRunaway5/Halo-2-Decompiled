#!/usr/bin/env python3
"""Observe two natural Needler ammunition decrements with a hardware watchpoint.

The private normally played idle snapshot is restored, and the real configured
controller trigger is held. No guest code, register, clock or game result is
written. This compares one firing interval, not a complete weapon contract.
"""
import argparse
import datetime
import json
from pathlib import Path
import resource
import socket
import subprocess
import time

from audit_rendering import file_sha
from capture_xbox_tu15_grenade_reference import qmp, wait_for, x11_window, TEXT_SHA


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--native-flow', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    args.output.mkdir(parents=True)
    launch = json.loads((args.session / 'launch.json').read_text())
    assert launch['display'] != ':0' and launch['xvfb_pid'] > 0
    command = launch['command']
    address = command[command.index('-qmp') + 1]
    assert address.startswith('unix:')
    qp = Path(address[5:].split(',')[0])
    idle_path = args.session / 'needler-idle-census-v1/report.json'
    idle = json.loads(idle_path.read_text())
    assert idle['whole_live_text_sha256'] == TEXT_SHA and len(idle['players']) == 1
    unit = idle['players'][0]['current_unit']
    owner = next(o for o in idle['objects'] if o['datum'] == unit)
    # In this actual normally played snapshot, the starting Needler has full
    # identity E2830014. Resolve its body from the retained census, then
    # independently authenticate its live header and parent in the debugger.
    weapon = next(o for o in idle['objects'] if o['datum'] == 0xe2830014)
    assert weapon['definition'] == 3813343702 and owner['type'] == 0
    restored = qmp(qp, [{'execute': 'stop'}, {'execute': 'human-monitor-command',
                         'arguments': {'command-line': 'loadvm tu15-needler-idle'}},
                        {'execute': 'stop'}])
    assert not restored[2]['return'].strip(), restored
    with socket.socket() as reservation:
        reservation.bind(('127.0.0.1', 0))
        port = reservation.getsockname()[1]
    server = qmp(qp, [{'execute': 'human-monitor-command',
                       'arguments': {'command-line': f'gdbserver tcp:127.0.0.1:{port}'}}])
    script = args.output / 'observe.gdb'
    observed = (args.output / 'original.json').absolute()
    script.write_text(f'''set pagination off
set confirm off
set architecture i386
set remotetimeout 15
target remote 127.0.0.1:{port}
python
import gdb,hashlib,json,pathlib,struct
inf=gdb.selected_inferior()
def read(p,n):return bytes(inf.read_memory(p,n))
def word(p):return struct.unpack('<I',read(p,4))[0]
assert hashlib.sha256(read(0x12000,3611948)).hexdigest()=={TEXT_SHA!r}
pool=word(0x4ecbd0);headers=word(pool+0x44)
header=read(headers+({weapon['datum']}&65535)*12,12)
assert struct.unpack_from('<H',header)[0]=={weapon['datum']}>>16
body=struct.unpack_from('<I',header,8)[0]
assert body=={weapon['body']} and word(body)==3813343702
assert word(body+0x14)=={unit} and read(body+0xaa,1)==b'\\2'
clock=word(0x51df54);rate=struct.unpack('<H',read(clock+2,2))[0];assert rate==30
assert struct.unpack('<h',read(body+0x22c,2))[0]==30
watch_description=gdb.execute('watch -location *(unsigned short*)'+hex(body+0x22c),to_string=True)
assert 'Hardware watchpoint' in watch_description,watch_description
initial_tick=word(clock+8);events=[]
gdb.write('WATCH_ARMED\\n');gdb.flush()
for expected in (29,28):
 gdb.execute('continue')
 loaded=struct.unpack('<h',read(body+0x22c,2))[0];assert loaded==expected,loaded
 events.append(dict(tick=word(clock+8),loaded=loaded,reserve=struct.unpack('<h',read(body+0x22a,2))[0],pc=int(gdb.parse_and_eval('$eip'))))
assert hashlib.sha256(read(0x12000,3611948)).hexdigest()=={TEXT_SHA!r}
pathlib.Path({str(observed)!r}).write_text(json.dumps(dict(original_tick_rate=rate,initial_tick=initial_tick,events=events,weapon={weapon['datum']},unit={unit},body=body,whole_live_text_sha256={TEXT_SHA!r},watchpoint_description=watch_description,guest_code_patches=[],guest_memory_patches=[],guest_register_writes=[]),indent=2)+'\\n')
gdb.execute('delete')
end
detach
quit
''')
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    process = None
    log_path = args.output / 'observe.log'
    try:
        with log_path.open('w') as log:
            process = subprocess.Popen(['gdb', '-q', '-nx', '-batch', '-x', str(script)],
                                       stdout=log, stderr=subprocess.STDOUT)
            wait_for(lambda: 'WATCH_ARMED' in log_path.read_text(), 10, [process])
            x11_window(launch['display'], press=True, key='e', duration=1)
            assert process.wait(timeout=20) == 0
    finally:
        qmp(qp, [{'execute': 'stop'}])
        if process is not None and process.poll() is None:
            process.terminate()
            process.wait(timeout=10)
        qmp(qp, [{'execute': 'human-monitor-command',
                  'arguments': {'command-line': 'gdbserver none'}}, {'execute': 'stop'}])
    original = json.loads(observed.read_text())
    flow_path = args.native_flow / 'report.json'
    flow = json.loads(flow_path.read_text())
    drops = [row for previous, row in zip(flow['ticks'], flow['ticks'][1:])
             if row['loaded'] < previous['loaded']]
    assert len(drops) >= 2 and [row['loaded'] for row in drops[:2]] == [29, 28]
    native_interval = drops[1]['tick'] - drops[0]['tick']
    original_interval = original['events'][1]['tick'] - original['events'][0]['tick']
    assert original_interval == native_interval
    assert all(row['reserve'] == 60 for row in original['events'])
    result = dict(timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        passed=True, scope=__doc__, reference='OG Xbox TU1.5',
        original=original, original_shot_interval_ticks=original_interval,
        native_shot_interval_ticks=native_interval,
        native_first_two_events=[dict(tick=r['tick'], loaded=r['loaded'], reserve=r['reserve']) for r in drops[:2]],
        controller_input=dict(key='e', configured_binding='rtrigger = 8', host_hold_seconds=1),
        native_flow_report_sha256=file_sha(flow_path), idle_census_sha256=file_sha(idle_path),
        launch_sha256=file_sha(args.session / 'launch.json'), restore=restored, gdbserver=server,
        debugger_script_sha256=file_sha(script), tool_sha256=file_sha(Path(__file__)),
        original_observation_sha256=file_sha(observed), physical_xbox_verified=False,
        complete_cadence_sequence_verified=False, matched_framebuffers_verified=False,
        full_weapon_parity=False)
    (args.output / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(passed=True, original_interval=original_interval,
                         native_interval=native_interval, original_events=original['events'])))


if __name__ == '__main__':
    main()
