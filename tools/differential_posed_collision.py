#!/usr/bin/env python3
"""Validate real authored collision import and query against original execution.

Supplied explicit poses are original-oracle inputs, not live animation state.
Authored geometry and recorded outputs remain in ignored local analysis.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[1]
SUITE=ROOT/'analysis/multiplayer/map-pack/spartan-collision-suite/suite.json'
EDGES=ROOT/'analysis/multiplayer/map-pack/spartan-surface-edge-1.1-oracle.json'
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=ROOT/'build-multiplayer/posed_collision_tests')
    parser.add_argument('--fixtures',type=Path,default=ROOT/'analysis/multiplayer/integration/posed-collision-fixtures.tsv')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/posed-collision-differential.json')
    parser.add_argument('--export-only',action='store_true')
    args=parser.parse_args()
    if ROOT/'analysis' not in args.fixtures.resolve().parents: raise ValueError('Keep original fixtures in ignored analysis')
    suite=json.loads(SUITE.read_text());edges=json.loads(EDGES.read_text())
    for report in (suite,edges):
        for key,path in [('oracle_tool_sha256','tools/oracle_spartan_collision_1_1.py'),
                ('xbe_sha256','assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),
                ('cache_sha256','assets/disc/maps/lockout.map')]:
            if digest(ROOT/path)!=report[key]: raise ValueError(f'Original authority differs: {path}')
    rows=[]
    for index,pose in enumerate(suite['pose_inputs']):
        words=[f'{word:08x}' for matrix in pose['matrices_hex']
            for word in struct.unpack('<13I',bytes.fromhex(matrix))]
        rows.append(' '.join(map(str,['pose',index,len(pose['matrices_hex']),*words])))
    for case in suite['cases']:
        path=ROOT/case['report']
        if digest(path)!=case['sha256']: raise ValueError('Original case differs')
        row=json.loads(path.read_text());expected=row['decoded'];raw=bytes.fromhex(row['raw_result_hex'])
        if row['external_services_executed']: raise ValueError('Unexpected supplied collision service')
        fields=['query',case['name'],row['pose_index'],row['lower_flags'],int(row['upper16af90']),
            row['caller_previous_fraction_bits'] or '7f7fffff',len(row['permutation_bytes']),
            *row['permutation_bytes'],*row['origin']['bits'],*row['delta']['bits'],int(row['returned_hit'])]
        if row['returned_hit']:
            upper=row['upper16af90'];at=0x4c if upper else 0x10
            leaf,surface,plane=struct.unpack_from('<iii',raw,at)
            flags,breakable=struct.unpack_from('<BB',raw,at+12)
            fields += [expected['fraction_bits'],expected['packed_bsp'],expected['region'],
                expected['node'],leaf,surface,plane,expected['model_material_entry'],flags,breakable]
            if upper:fields += [*expected['normal']['bits'],expected['plane_distance_bits'],expected['global_material']]
        rows.append(' '.join(map(str,fields)))
    for index,row in enumerate(edges['cases']):
        if row['external_services_executed'] or row['breakable_bitset_pointer']!='00000000':
            raise ValueError('Unsupported primitive fixture boundary')
        rows.append(' '.join(map(str,['edge',index,row['region_index'],row['bsp_index'],row['surface'],
            row['projection_axis'],row['projection_orientation'],*row['point_bits'],int(row['returned_contains'])])))
    args.fixtures.write_text('\n'.join(rows)+'\n')
    if args.export_only:
        print(f'Exported {len(suite["cases"])} original queries and {len(edges["cases"])} edge cases');return
    run=subprocess.run([str(args.binary.resolve()),str(ROOT),str(args.fixtures.resolve())],
        text=True,capture_output=True)
    print(run.stdout,end='');print(run.stderr,end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode==0,queries=suite['case_count'],
        hits=suite['hit_count'],edge_cases=edges['case_count'],edge_accepts=edges['accepted_count'],
        native_binary_sha256=digest(args.binary),original_reports={str(p.relative_to(ROOT)):digest(p) for p in (SUITE,EDGES)},
        fixtures_sha256=digest(args.fixtures),tool_sha256=digest(Path(__file__)),
        scope=__doc__,stdout=run.stdout,stderr=run.stderr),indent=2)+'\n')
    raise SystemExit(run.returncode)

if __name__=='__main__':main()
