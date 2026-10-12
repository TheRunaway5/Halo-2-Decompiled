#!/usr/bin/env python3
"""Compare native four-entry damage history with frozen original source writes.

Only initialization and history mutation are validated here. Live hit finding,
death attribution, counters, animation and ownership scheduling are separate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/spawn-source'
REPORT=BASE/'smg-damage-report.json'
SLAYER=BASE/'slayer-accounting-report.json'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def hexword(value):return f'{value & 0xffffffff:08x}'
def entries(history):
    return [v for row in history for v in (hexword(row['timestamp']),row['damage_bits'],row['object'],row['player'],hexword(row['cause']))]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-multiplayer/damage_history_tests')
    p.add_argument('--fixtures',type=Path,default=ROOT/'analysis/multiplayer/integration/damage-history-fixtures.tsv')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/damage-history-differential.json')
    p.add_argument('--export-only',action='store_true')
    args=p.parse_args()
    if ROOT/'analysis' not in args.fixtures.resolve().parents:raise ValueError('Keep original exports in ignored analysis')
    for path,pin in {
        REPORT:'2b7d072e92dde4ad6af7a92f301cf5b734410df44745c988bbcf6b5b372e9aca',
        ROOT/'tools/oracle_mp_spawn_smg_damage.py':'4d20d8c2e7229dd2bf78a601482d1862535c0208ab7a57b86f47192337c1a378',
        SLAYER:'284cbcd6bb293bb8e9edc74e62dc9ffe3b3a2929877983884abc93d05b66f128',
    }.items():
        if digest(path)!=pin:raise ValueError(f'Original damage-history authority changed: {path}')
    report=json.loads(REPORT.read_text());slayer=json.loads(SLAYER.read_text())
    for data in (report,slayer):
        for relative,pin in data['dependencies'].items():
            if digest(ROOT/relative)!=pin:raise ValueError(f'Original dependency changed: {relative}')
    for path,pin in {
        ROOT/report['instruction_listing']:report['instruction_listing_sha256'],
        ROOT/slayer['source_listing']:slayer['source_listing_sha256'],
        ROOT/'assets/disc/maps/lockout.map':report['cache_sha256'],
        ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03':report['executable_sha256'],
    }.items():
        if digest(path)!=pin:raise ValueError(f'Original input changed: {path}')
    lines=[];writes=0
    for index,row in enumerate(report['source_history_initialization']):
        values=[]
        for field in ('before_hex','after_hex'):
            raw=bytes.fromhex(row[field]);assert len(raw)==74
            for i in range(4):
                tick,amount,object_datum,player=struct.unpack_from('<4I',raw,6+i*16)
                values += [hexword(tick),hexword(amount),hexword(object_datum),hexword(player),hexword(raw[70+i])]
        lines.append(' '.join(['I',str(index),*values]))
    def emit(tick,amount,object_datum,player,cause,before,after):
        nonlocal writes
        lines.append(' '.join(['H',str(writes),hexword(tick),hexword(amount),object_datum,player,
            hexword(cause),*entries(before),*entries(after)]));writes+=1
    for row in slayer['damage_history_writer']:
        for sample in row['snapshots']:
            emit(sample['tick'],struct.unpack('<I',struct.pack('<f',sample['damage']))[0],sample['object'],
                sample['full_player'],sample['cause'],sample['before'],sample['after'])
    hits=list(report['complete_damage_entry_hits'])+[h for sequence in report['damage_entry_sequences'] for h in sequence['hits']]
    consumers=0
    def consume(raw,tick,before,after):
        nonlocal consumers
        body,shield=struct.unpack_from('<2I',raw,0x44)
        player,object_datum=struct.unpack_from('<2I',raw,0xc)
        lines.append(' '.join(['C',str(consumers),hexword(tick),hexword(body),hexword(shield),
            hexword(object_datum),hexword(player),hexword(raw[0]),*entries(before),*entries(after)]));consumers+=1
    for row in hits:
        raw=bytes.fromhex(row['generated_result_hex'])
        consume(raw,row['tick'],row['before']['history'],row['after']['history'])
    for row in report['zero_amount_identity_cases']:
        consume(bytes.fromhex(row['result_hex']),200,row['before']['history'],row['after']['history'])
    args.fixtures.write_text('\n'.join(lines)+'\n')
    if args.export_only:
        print(f'Exported {len(report["source_history_initialization"])} resets, {writes} writes and {consumers} consumers');return
    run=subprocess.run([str(args.binary.resolve()),str(args.fixtures.resolve())],text=True,capture_output=True)
    print(run.stdout,end='');print(run.stderr,end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode==0,resets=3,writes=writes,consumers=consumers,
        scope=__doc__,native_binary_sha256=digest(args.binary),original_report_sha256=digest(REPORT),
        fixtures_sha256=digest(args.fixtures),tool_sha256=digest(Path(__file__)),stdout=run.stdout,stderr=run.stderr),indent=2)+'\n')
    raise SystemExit(run.returncode)
if __name__=='__main__':main()
