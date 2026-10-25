#!/usr/bin/env python3
"""Inventory literal scenery audio owners without claiming runtime playback.

Read-only prepared MCC and corresponding original Xbox metadata. Run through
tools/run_test.py. The output identifies real placements for live regressions;
it does not turn a dependency reference into a sound request.
"""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import struct
import sys

from audit_weapon_vehicle_sound_presence_tu15 import references, schema as xbox_schema
from inspect_multiplayer import Cache
from original_xbox_pose_case import BASE
from validate_mcc_weapon_vehicle_sound_resources import Bank, merge, schema, SCHEMAS

ROOT=Path(__file__).resolve().parents[1]
PLACEMENTS=((6,0x50,0x5c),(7,0xa8,0x48),(8,0xb8,0x44),
            (9,0xc8,0x54),(10,0xd8,0x50),(11,0x328,0x4c))
FOLLOW={'scen','mach','ctrl','lifi','ssce','bloc','hlmt','effe','lsnd'}

def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def compact(path):
    raw=path.read_bytes()
    if len(raw)<=1024*1024:return
    report=json.loads(raw);packed=lzma.compress(raw);assert len(packed)<=1024*1024
    assert lzma.decompress(packed)==raw
    archive=path.with_suffix(path.suffix+'.xz');archive.write_bytes(packed)
    summary={k:v for k,v in report.items() if k!='maps'};summary['maps']=[]
    for row in report['maps']:
        value={k:v for k,v in row.items() if k not in ('mcc','xbox')}
        for side in ('mcc','xbox'):
            if side not in row:continue
            data=row[side];placed=data['audio_or_effect_placements']
            value[side]=dict(placement_counts=data['placement_counts'],issues=data['issues'],
                audio_placements=sum(bool(p['audio']) for p in placed),
                effect_placements=sum(bool(p['effects']) for p in placed),
                unique_audio_owner_paths=sorted({p['name'] for p in placed if p['audio']}))
        summary['maps'].append(value)
    summary['lossless_full_inventory']=dict(path=str(archive),bytes=len(packed),sha256=sha(archive),
        uncompressed_bytes=len(raw),uncompressed_sha256=hashlib.sha256(raw).hexdigest(),decompression_verified=True)
    summary['compaction_tool_sha256']=sha(Path(__file__))
    path.write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps(dict(compacted=str(path),archive_bytes=len(packed),original_bytes=len(raw))),flush=True)

def inventory(cache,tags,scenario,schema_reader):
    rows=[];issues=[];counts={}
    for kind,offset,stride in PLACEMENTS:
        choices=cache.block(scenario+offset+8,0x28)
        placed=cache.block(scenario+offset,stride)
        counts[str(kind)]=len(placed)
        for number,at in enumerate(placed):
            index=struct.unpack_from('<h',cache.data,at)[0]
            if index==-1:continue
            if index<0 or index>=len(choices):raise ValueError('Placement palette outside authored extent')
            datum=cache.u(choices[index]+4);bank,root=tags[datum]
            visited=set();audio=[];effects=[]
            def visit(current):
                if current in visited:return
                visited.add(current)
                owner,tag=tags[current]
                if tag['offset'] is None:raise ValueError('Unresolved resident dependency: '+tag['name'])
                if tag['group'] in {'snd!','lsnd'}:
                    audio.append(dict(datum=f'{current:08x}',group=tag['group'],name=tag['name']))
                if tag['group']=='effe':effects.append(tag['name'])
                if tag['group'] in FOLLOW:
                    for child in references(owner,schema_reader(tag['group']),tag['offset']):visit(child)
            try:visit(datum)
            except (AssertionError,ValueError,KeyError) as error:
                issues.append(dict(type=kind,placement=number,name=root['name'],error=str(error)))
            if audio or effects or 'teleport' in root['name']:
                rows.append(dict(type=kind,placement=number,datum=f'{datum:08x}',name=root['name'],
                    position=list(struct.unpack_from('<3f',cache.data,at+8)),
                    audio=audio,effects=sorted(set(effects))))
    return dict(placement_counts=counts,audio_or_effect_placements=rows,issues=issues)

def main():
    if len(sys.argv)==3 and sys.argv[1]=='--compact-existing':
        Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
        path=Path(sys.argv[2]).resolve(strict=True)
        assert path.is_relative_to(ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio')
        compact(path);return 0
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--maps',type=Path,required=True)
    ap.add_argument('--map',action='append',required=True)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=json.loads((BASE/'authored-scope.json').read_text())
    originals={Path(row['path']).stem:row for row in source['maps']}
    names=sorted(name for name in originals if (args.maps/(name+'.map')).is_file()) if args.map==['all'] else args.map
    report=dict(scope=__doc__,tool_sha256=sha(Path(__file__)),maps=[],
        metadata_only=True,runtime_playback_verified=False,full_tu15_parity=False,
        runner_scratch=str(scratch),large_artifacts_retained=[])
    shared=Bank((args.maps/'shared.map').resolve(strict=True))
    try:
        report['shared_sha256']=sha(shared.path)
        for name in names:
            local=Bank((args.maps/(name+'.map')).resolve(strict=True));original=None
            try:
                tags=merge(local,shared) if local.u(local.index+12)!=0xffffffff and any(t['offset'] is None for t in local.tags.values()) else {d:(local,t) for d,t in local.tags.items()}
                scenario=local.tag(local.u(local.index+12),'scnr')[1]
                row=dict(map=name,mcc_sha256=sha(local.path),mcc=inventory(local,tags,scenario,schema))
                if name in originals:
                    item=originals[name];path=Path(item['path']);assert sha(path)==item['sha256']
                    original=Cache(path)
                    sc=[t for t in original.tags.values() if t['group']=='scnr'];assert len(sc)==1
                    row['xbox_sha256']=item['sha256']
                    row['xbox']=inventory(original,{d:(original,t) for d,t in original.tags.items()},sc[0]['offset'],xbox_schema)
                report['maps'].append(row)
                print(json.dumps(dict(map=name,mcc_counts=row['mcc']['placement_counts'],
                    mcc_audio_owners=sum(bool(p['audio']) for p in row['mcc']['audio_or_effect_placements']),
                    issues=row['mcc']['issues'][:2])),flush=True)
            finally:
                local.close()
                if original is not None:original.data.close();original.stream.close()
        report['schema_sha256']=SCHEMAS
        report['metadata_complete']=all(not side.get('issues') for row in report['maps'] for key,side in row.items() if key in ('mcc','xbox'))
    finally:shared.close()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    compact(args.output)
    return 0 if report['metadata_complete'] else 1

if __name__=='__main__':raise SystemExit(main())
