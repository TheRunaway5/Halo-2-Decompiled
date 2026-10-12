#!/usr/bin/env python3
"""Compare native stock SMG FP clocks against frozen original phase outputs."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/map-pack/smg-fp-clock'

def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def word(value): return f'{value & 0xffffffff:08x}'

def state(snapshot):
    hand=snapshot['hands'][0];b=bytes.fromhex(hand['manager_hex'])
    return [word(struct.unpack_from(fmt,b,offset)[0]) for fmt,offset in
            [('<I',0),('<I',4),('<B',0x10),('<B',0x11),('<H',0x12),
             ('<H',0x14),('<H',0x16),('<I',0x18),('<I',0x1c),('<I',0x7c)]]+[
                word(struct.unpack_from('<h',bytes.fromhex(hand['scalars_e4_104_hex']),14)[0])]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=ROOT/'build-multiplayer/fp_clock_tests')
    parser.add_argument('--fixtures',type=Path,default=ROOT/'analysis/multiplayer/integration/fp-clock-fixtures.tsv')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/fp-clock-differential.json')
    parser.add_argument('--export-only',action='store_true')
    args=parser.parse_args()
    if ROOT/'analysis' not in args.fixtures.resolve().parents:raise ValueError('Original fixture must remain ignored')
    pins={BASE/'source-fixture.json':'4f87bca8cda0b5b5652a0dadf9c48a5e957ae1c8e99caadd7db524624e48c2df',
          BASE/'audit.json':'f403d5d4c62d38897f39bcab4582ead272aca2f333e917fc9f5676ec8676d1f5',
          BASE/'frozen/suite.json':'d831472491423ff7a2fe9db7a20daabc2727e29a1a340a70739efed07e4ccebb',
          ROOT/'tools/oracle_smg_fp_clock_1_1.py':'7d5548e26d3503ea1057aef3202a9285d7e9fa431d0a943c372788b64282b5d6',
          ROOT/'tools/oracle_smg_fp_ready_producer_1_1.py':'634e30e563558ffeeb42669ecf8c9fbeab72a3d64db67a6f87a3f62f7e259797',
          ROOT/'tools/audit_smg_fp_clock_1_1.py':'25acb7ecf0d040b8e7c7b1b59be1d091e830b3bd0539fa28b82a6787970bbe52',
          ROOT/'assets/disc/maps/lockout.map':'14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9',
          ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03':'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'}
    for path,pin in pins.items():
        if digest(path)!=pin:raise ValueError(f'Frozen clock authority changed: {path}')
    source=json.loads((BASE/'source-fixture.json').read_text());lines=[]
    for case in source['cases']:
        report=ROOT/case['source_report_path']
        if digest(report)!=case['source_report_sha256']:raise ValueError('Original case changed')
        repeat=report.with_name(report.stem+'-reexecution.json')
        if digest(repeat)!=case['source_report_sha256']:raise ValueError('Original repeat changed')
    # Metadata comes from exact recorded graph bytes; no native expected output.
    refs=source['authored']['graph_sound_references']
    for activity,index in [(0x0400000c,21),(0x06000006,20),(0x05000024,29),(0x0c000064,30),(0x0b000065,31)]:
        clip=source['authored']['clips'][index];events=[]
        for e in clip['timing_events']:
            kind,frame=struct.unpack('<hh',bytes.fromhex(e['raw_hex']))
            events.append([0,frame,0xffffffff,kind,0,0x0600008a])
        for e in clip['sound_events']:
            kind,frame,marker=struct.unpack('<hhI',bytes.fromhex(e['raw_hex']))
            ref=bytes.fromhex(refs[kind]['raw_hex'])
            events.append([1,frame,struct.unpack_from('<I',ref,4)[0],0xffffffff,struct.unpack_from('<H',ref,8)[0],marker])
        if clip['effect_events']:raise ValueError('Stock clip unexpectedly contains effects')
        lines.append(' '.join(['D',word(activity),word(index),word(clip['frames']),word(clip['loop_frame']),word(len(events)),
                               *[word(v) for event in events for v in event]]))
    ticks=0
    for case in source['cases']:
        lines.append(' '.join(['I',case['name'],*state(case['initial'])]))
        for row in case['rows']:
            buttons=struct.unpack_from('<H',bytes.fromhex(row['control']['supplied_input_record_hex']),0x1c)[0]
            if buttons not in (0,1):raise ValueError('Unhandled original action buttons')
            fields=['T',word(row['tick']),word(row['supplied_held']),word(buttons==1),
                    word(case['name']=='normal_wrapper_ready' and row['tick']==0)]
            for phase in ['before','after_unit','after_weapon','after_postpass','after_fp']:fields+=state(row[phase])
            events=[e for e in case['source_event_callbacks'] if e['tick']==row['tick']]
            fields.append(word(len(events)))
            for event in events:
                fields += [word(v) for v in struct.unpack('<HhIIII',bytes.fromhex(event['source_event_hex']))]
            lines.append(' '.join(fields));ticks+=1
    args.fixtures.write_text('\n'.join(lines)+'\n')
    if args.export_only:print(f'Exported {ticks} original FP ticks and {ticks*5} phase states');return
    run=subprocess.run([str(args.binary.resolve()),str(ROOT/'assets/disc/maps/lockout.map'),str(args.fixtures.resolve())],text=True,capture_output=True)
    print(run.stdout,end='');print(run.stderr,end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode==0,scope=__doc__,original_ticks=ticks,
        primary_channel_phase_states=ticks*5,source_event_callbacks=4,local_match_ticks=516,
        timed_ready_producer_native=False,full_camera_aim_audio_parity=False,xbox_matrix_bit_parity=False,
        source_fixture_sha256=digest(BASE/'source-fixture.json'),native_binary_sha256=digest(args.binary),
        fixture_sha256=digest(args.fixtures),tool_sha256=digest(Path(__file__)),stdout=run.stdout,stderr=run.stderr),indent=2)+'\n')
    raise SystemExit(run.returncode)

if __name__=='__main__':main()
