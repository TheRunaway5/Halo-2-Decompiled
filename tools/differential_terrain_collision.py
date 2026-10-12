#!/usr/bin/env python3
"""Compare native raw terrain traversal with frozen original pickup segments.

This validates authored sbsp import and the lower collision query. Outer
locations, endpoint correction, pickup visibility, dynamic shapes and live
breakable-state producers remain separate integration work.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[1]
REPORT=ROOT/'analysis/multiplayer/integration/inventory-source/spatial/report.json'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-multiplayer/terrain_collision_tests')
    p.add_argument('--fixtures',type=Path,default=ROOT/'analysis/multiplayer/integration/terrain-collision-fixtures.tsv')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/terrain-collision-differential.json')
    p.add_argument('--export-only',action='store_true')
    args=p.parse_args()
    if ROOT/'analysis' not in args.fixtures.resolve().parents:raise ValueError('Keep original exports in ignored analysis')
    for path,pin in {
        REPORT:'6b03f62ec351afeb0760a9052305a08a1cbc3d15a3175f80edfd672c76320b59',
        ROOT/'tools/oracle_mp_inventory_spatial_1_1.py':'18a3bc331e8ec2f92032725a5b711e37ee9d5acd48ff3cb12be70ce8696b7bed',
    }.items():
        if digest(path)!=pin:raise ValueError(f'Original terrain authority changed: {path}')
    report=json.loads(REPORT.read_text())
    for relative,pin in report['source_tool_dependencies'].items():
        if digest(ROOT/'tools'/relative)!=pin:raise ValueError(f'Original dependency changed: {relative}')
    for relative,pin in {
        'assets/disc/maps/lockout.map':report['cache_sha256'],
        'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03':report['xbe_sha256'],
    }.items():
        if digest(ROOT/relative)!=pin:raise ValueError(f'Original input changed: {relative}')
    for row in report['instructions']:
        if digest(ROOT/row['listing'])!=row['sha256']:raise ValueError(f'Original listing changed: {row["entry"]}')
    metadata=report['terrain_metadata']
    counts=[row['count'] for row in metadata['blocks'].values()]
    materials=bytes.fromhex(metadata['collision_materials']['raw_hex'])
    global_materials=[struct.unpack_from('<h',materials,at+8)[0] for at in range(0,len(materials),0x14)]
    lines=['I '+' '.join(map(str,counts+[len(global_materials)]+global_materials))]
    hits=0
    plane_base=int(metadata['blocks']['planes']['data']['address'],16)
    for index,row in enumerate(report['collision_queries']):
        lower=row['lower'];raw=bytes.fromhex(lower['result_hex']);desc=bytes.fromhex(lower['descriptor_after_hex'])
        assert lower['range_bits']==['00000000','3f800000'] and lower['root_node']=='00000000'
        assert lower['initial_best_fraction_bits']=='7f7fffff'
        assert lower['breakable_count']==256 and lower['visited_count']<=256
        hit=bool(int(lower['return_eax'],16)&255)
        assert hit==(row['collision_type']==1)
        values=['Q',index,int(lower['flags'],16),*lower['origin']['bits'],*lower['delta']['bits'],int(hit)]
        if hit:
            fraction,plane_pointer,leaf,surface,plane=struct.unpack_from('<IIiii',raw)
            flags,breakable,material=struct.unpack_from('<BBh',raw,0x14)
            values += [f'{fraction:08x}',(plane_pointer-plane_base)//16,leaf,surface,plane,material,flags,breakable]
            hits+=1
        values += [struct.unpack_from('<i',desc,0x1c)[0],desc[0x20],
            struct.unpack_from('<i',desc,0x24)[0],lower['visited_count'],*lower['visited_leaves']]
        lines.append(' '.join(map(str,values)))
    args.fixtures.write_text('\n'.join(lines)+'\n')
    if args.export_only:
        print(f'Exported {len(lines)-1} original terrain queries / {hits} hits');return
    run=subprocess.run([str(args.binary.resolve()),str(ROOT),str(args.fixtures.resolve())],text=True,capture_output=True)
    print(run.stdout,end='');print(run.stderr,end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode==0,queries=len(lines)-1,hits=hits,
        scope=__doc__,native_binary_sha256=digest(args.binary),original_report_sha256=digest(REPORT),
        fixtures_sha256=digest(args.fixtures),tool_sha256=digest(Path(__file__)),stdout=run.stdout,stderr=run.stderr),indent=2)+'\n')
    raise SystemExit(run.returncode)
if __name__=='__main__':main()
