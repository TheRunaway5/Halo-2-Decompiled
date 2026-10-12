#!/usr/bin/env python3
"""Compare bounded native first-person registration with the frozen original producer."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/map-pack/smg-fp-registration'


def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def word(value): return f'{value & 0xffffffff:08x}'


def binding(state):
    fields=[]
    for view in state['views']:
        fields += [view['unit'],view['selector']]
        for hand in view['hands']:
            fields += [hand['flags'],hand['weapon'],word(hand['weapon_node_count']),word(hand['arms_node_count'])]
            for key in ('weapon_node_map_hex','arms_node_map_hex'):
                fields += [word(v) for v in struct.unpack('<64I',bytes.fromhex(hand[key]))]
            channel=hand['channel']
            fields += [channel[k] for k in ('graph','animation_identifier','flags_11','flags_12','rate_18_bits','frame_1c_bits')]
    return fields


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=ROOT/'build-multiplayer/fp_registration_tests')
    parser.add_argument('--fixtures',type=Path,default=ROOT/'analysis/multiplayer/integration/fp-registration-fixtures.tsv')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/fp-registration-differential.json')
    parser.add_argument('--export-only',action='store_true')
    args=parser.parse_args()
    if ROOT/'analysis' not in args.fixtures.resolve().parents:raise ValueError('Original-data fixtures must remain ignored')
    pins={
        BASE/'report.json':'fe7c7ea7ef620b526ab6a9debda83068055a4e02ee700bdd1ed3dea9cb375ca2',
        BASE/'source-fixture.json':'bf4a515f17d1f66d2bfe243893c3060c0c4e175733d8ed14d68fee0e30a0d0eb',
        BASE/'audit.json':'04d8237948359fad338fd67319bc61e77e6e6f0505a088d7030dc3ac9e033f30',
        BASE/'instructions/inventory.json':'e4fb2c631cbc68ee2799ad5226bba8a7f72abcb77044711c1bf04d1f618ddf82',
        ROOT/'tools/oracle_smg_fp_registration_1_1.py':'06c0ddfece364a4361b60219b4f2ab26e98acc693aacccbe16eb89f8d0535e92',
        ROOT/'tools/audit_smg_fp_registration_1_1.py':'e215f575f5eb23e15aa71f39cf04d2fba7432d74ae938b6b9871253f515a57c9',
        ROOT/'assets/disc/maps/lockout.map':'14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9',
        ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03':'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
    }
    for path,pin in pins.items():
        if digest(path)!=pin:raise ValueError(f'Frozen first-person authority changed: {path}')
    source=json.loads((BASE/'source-fixture.json').read_text());report=json.loads((BASE/'report.json').read_text())
    meta=source['metadata'];fp=bytes.fromhex(meta['weapon_first_person_record']['raw_hex'])
    model,graph=struct.unpack_from('<I',fp,4)[0],struct.unpack_from('<I',fp,12)[0]
    models={int(m['tag']['datum'],16):[n['string_id'] for n in m['nodes']] for m in meta['models']}
    graph_nodes=[n['string_id'] for n in meta['graph_nodes']];idle=meta['selected_clips'][0]['index']
    lines=[]
    for selector in (0,2):
        arms=struct.unpack_from('<I',bytes.fromhex(meta['representations'][selector]['record']['raw_hex']),4)[0]
        fields=['D',word(selector),meta['weapon']['datum'],word(arms),word(model),word(graph),word(idle)]
        for nodes in (models.get(arms,[]),models[model],graph_nodes):fields += [word(len(nodes)),*nodes]
        lines.append(' '.join(fields))
    phases=0
    for step in source['steps'][1:]: # Complete arena allocation is outside the native represented fields.
        name=step['name'];entry=step.get('entry');before=step['before']
        if entry=='001677e5':fields=['I',name]
        elif entry=='001678c1':
            indices=bytes.fromhex(before['unit_current_indices_hex'])
            weapons=[before['unit_inventory_datums'][i] if i!=255 else 'ffffffff' for i in indices]
            fields=['O',name,before['user_player_datums'][0],before['player_current_unit_2c'],word(before['player_selector_88']),*weapons]
        elif entry=='00169d7a':fields=['R',name,word(step['arguments'][0])]
        elif entry=='00167d32':
            call=next(c for c in report['calls'] if c['entry']==entry and c['phase']==name)
            fields=['L',name,word(step['arguments'][0]),call['return_eax']]
        elif entry=='0014d600':fields=['C',name] # Native profile storage is outside this consumer.
        elif entry is None:
            fields=['F',name,word(len(step['hand_timer_phases']))]
            for phase in step['hand_timer_phases']:
                for key in ('before','after_unit','after_weapon','after_postpass'):fields += [word(t) for t in phase[key]]
                fields.append(word(step['request_ticks'].count(phase['tick'])))
                phases+=4
        else:raise ValueError('Unhandled frozen registration step')
        lines.append(' '.join([*fields,*binding(step['after'])]))
    args.fixtures.write_text('\n'.join(lines)+'\n')
    if args.export_only:print(f'Exported {len(source["steps"])-1} source steps and {phases} hand-timer phases');return
    run=subprocess.run([str(args.binary.resolve()),str(ROOT/'assets/disc/maps/lockout.map'),str(args.fixtures.resolve())],text=True,capture_output=True)
    print(run.stdout,end='');print(run.stderr,end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode==0,scope=__doc__,source_steps=len(source['steps'])-1,
        hand_timer_phases=phases,node_pairs=41,source_report_sha256=digest(BASE/'report.json'),
        native_binary_sha256=digest(args.binary),fixture_sha256=digest(args.fixtures),tool_sha256=digest(Path(__file__)),
        stdout=run.stdout,stderr=run.stderr),indent=2)+'\n')
    raise SystemExit(run.returncode)


if __name__=='__main__':main()
