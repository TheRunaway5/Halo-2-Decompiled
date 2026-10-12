#!/usr/bin/env python3
"""Compare capsule/visibility consumers with independently measured original outputs."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
from differential_item_point import verify_authority as verify_item_point_authority, BASE as ITEM_BASE

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/inventory-source'
MEASURED_SHA='4161d66409fa9779302f6cbc75fa41b458f46f0a461866336b95be833cc2fcfe'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def require(value,message):
    if not value:raise ValueError(message)
def read_lines(path):return [json.loads(line) for line in path.read_text().splitlines()]
def original_double(normalizer):
    # Literal representation conversion, not a native recomputation of length.
    stage=next(s for s in normalizer['stages'] if s['instruction']=='00030bf1')
    state=stage['fpu_state'];require(state['control']=='027f','Unrecovered x87 control')
    register=state['registers'][state['top']]
    mantissa=int(register['mantissa_hex'],16);exponent=int(register['exponent_hex'],16)
    require(exponent&0x8000==0 and mantissa&2047==0,'Length is not measured positive53-bit value')
    require(0<exponent-16383+1023<2047,'Unmeasured double exponent range')
    bits=((exponent-16383+1023)<<52)|((mantissa>>11)&((1<<52)-1))
    return f'{bits:016x}'
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=ROOT/'build-multiplayer/pickup_visibility_tests')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/pickup-consumer-differential.json')
    parser.add_argument('--item-points',action='store_true',help='Compose source FBFA0 points with visibility and ordinary terrain')
    args=parser.parse_args()
    measured_path=BASE/'pickup-consumer-measured/report.json'
    require(bool(MEASURED_SHA),'Independent measured authority has not frozen')
    pins={measured_path:MEASURED_SHA,
        BASE/'pickup-consumer-measured/manifest.json':'a26f5cfe698881f815a06404e46583d9ee3906d0adffdbf2b36c3b3602e2ce7d',
        ROOT/'docs/MP_PICKUP_CONSUMER_MEASURED_RESEARCH.md':'fc2cac108bcab3800478fa234be93f6c7cda0c699b98a4f76a6fa62826057b8c',
        ROOT/'tools/oracle_mp_pickup_consumer_measured_1_1.py':'0fd18d4f19876552180d2cf6149c9961c5ff1d7a557bfb09b9e9fafa15918da7',
        ROOT/'tools/oracle_mp_pickup_consumer_measured_audit_1_1.py':'1ddf6b8fd586e09022e629c1137158d263806dbf70cbf8b117ae02e88b905fb4',
        BASE/'pickup-consumer-fixtures/manifest.json':'01bd542b423cf260e2efcc2aa72b2252c6c00216907906fc7838415272cd554c',
        ROOT/'tools/export_mp_pickup_consumer_fixtures_1_1.py':'1840130981199a9ed7cf7bedbfe803734c16aa180391a8379a3916f9a7e933a6',
        ROOT/'docs/MP_PICKUP_CONSUMER_FIXTURES.md':'a2bf192b0b63ffff6419d9c72155fa99fc3d313a4bbf55cb93de39abfa9f156b'}
    for path,sha in pins.items():require(digest(path)==sha,f'Changed frozen authority: {path}')
    measured=json.loads(measured_path.read_text())
    measured_manifest=json.loads((BASE/'pickup-consumer-measured/manifest.json').read_text())
    for file,record in measured_manifest['payloads'].items():
        require(digest(BASE/'pickup-consumer-measured'/file)==record['sha256'],f'Changed measured payload: {file}')
    manifest=json.loads((BASE/'pickup-consumer-fixtures/manifest.json').read_text())
    for file,sha in {**manifest['source_pins'],**measured['source_pins']}.items():
        require(digest(ROOT/file)==sha,f'Changed original dependency: {file}')
    for file,record in manifest['payloads'].items():
        require(digest(BASE/'pickup-consumer-fixtures'/file)==record['sha256'],f'Changed observed payload: {file}')
    if args.item_points:verify_item_point_authority()
    cases=read_lines(BASE/'pickup-consumer-fixtures/cases.jsonl')
    casts=read_lines(BASE/'pickup-consumer-fixtures/casts.jsonl')
    current={c['global_id']:c for c in measured['cases']}
    rows=[]
    for c in cases:
        m=current[c['id']];cap=m['capsule'];metadata=struct.unpack('<4I',bytes.fromhex(cap['metadata_fields_hex']))
        require(m['unit_input']['parent']==m['item_input']['parent']=='ffffffff','Unrecovered parent producer')
        require(m['final_al']==c['final_al'] and cap['bottom_bits']==c['capsule']['bottom_bits'],'Original case correlation differs')
        geometry={g['instruction']:g['vectors'] for g in m['geometry']}
        row=['C',c['id'],m['unit_input']['datum'],m['item_input']['datum'],m['unit_input']['header_type'],m['item_input']['header_type'],
            cap['unit_input']['definition'],*[f'{v:08x}' for v in metadata],cap['current_crouch_bits'],*cap['source_root_bits'],
            *cap['bottom_bits'],cap['extent_bits'],cap['radius_bits'],*m['item_point_bits'],*geometry['000c5845']['top'],m['final_al']]
        rows.append(row)
        n=m['normalizer']
        if n:
            require(n['world_up_bits']==['00000000','00000000','3f800000'] and
                n['fallback_bits']==['00000000','bf800000','00000000'] and n['threshold_bits']=='38d1b717','Changed original constants')
            reciprocal=next(s for s in n['stages'] if s['instruction']=='00030c17')
            require(reciprocal['fpu_state']['mxcsr']=='00001f80','Unrecovered SSE control')
            rows.append(['N',c['id'],*n['input_bits'],original_double(n),n['length_spill_bits'],*n['output_bits'],n['return_xmm0_bits'],
                *geometry['000c59ab']['chosen_side'],*geometry['000c5bb2']['positive_target'],*geometry['000c5bb2']['negative_target'],reciprocal['xmm1_bits']])
    for q in casts:
        rows.append(['Q',q['case_id'],q['ordinal'],q['mask'],*q['ignored'],*q['start_bits'],*q['delta_bits'],q['return_al'],
            q['fraction_bits'],*q['result_point_bits'],q['initial_location8'],q['final_location8']])
    fixture=ROOT/'analysis/multiplayer/integration/pickup-consumer-fixtures.tsv'
    fixture.write_text('\n'.join(' '.join(map(str,row)) for row in rows)+'\n')
    command=[str(args.binary.resolve()),str(ROOT),str(fixture)]
    if args.item_points:command.append('--item-points')
    run=subprocess.run(command,text=True,capture_output=True)
    print(run.stdout,end='');print(run.stderr,end='')
    report=dict(passed=run.returncode==0,scope=__doc__,capsules=207,normalizers=187,casts=1513,visible=21,blocked=186,
        clear_target_clips=374,source_sqrt_length_spill_reciprocal_compared=True,small_length_fallback_executed=False,
        hit_driven_target_replacement_executed=False,live_pickup_integration=False,
        source_item_point_composed=args.item_points,
        limits=('Native source item points from literal original current nodes/contacts; supplied capsule roots/crouch and configured FPCW027F/MXCSR1F80; full world/Havok/current-pose producers remain open.' if args.item_points else
            'Inherited supplied roots/item points and configured FPCW027F/MXCSR1F80; full world/Havok/current-pose producers remain open.'),
        measured_report_sha256=digest(measured_path),fixture_sha256=digest(fixture),
        native_binary_sha256=digest(args.binary),tool_sha256=digest(Path(__file__)),stdout=run.stdout,stderr=run.stderr)
    if args.item_points:
        report.update(original_item_points=207,item_root_fallbacks=205,item_marker_projections=2,
            item_point_fixture_sha256=digest(ITEM_BASE/'fixtures.tsv'),item_point_manifest_sha256=digest(ITEM_BASE/'manifest.json'),
            item_point_pin_verifier_sha256=digest(ROOT/'tools/differential_item_point.py'))
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    raise SystemExit(run.returncode)
if __name__=='__main__':main()
