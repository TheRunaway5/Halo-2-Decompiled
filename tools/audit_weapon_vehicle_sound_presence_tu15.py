#!/usr/bin/env python3
"""Compare declared weapon/vehicle audio dependencies with MCC tag namespaces.

Offline Xbox metadata is the reference. This follows explicit references in
the listed audio-bearing schemas; implicit material/global/script owners and
actual event admission, playback and waveforms remain separate requirements.
Run through tools/run_test.py; no assets are changed.
"""
import argparse
from functools import lru_cache
import json
import os
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

from inspect_multiplayer import Cache, ROOT
from mcc_content import MccCache
from original_xbox_pose_case import BASE, sha

PIN = 'fd14ef3c46135e9422966c6a64cd926fb0bd23fb'
GROUPS = {'weap','vehi','proj','hlmt','jmad','effe','lsnd','jpt!','foot'}
SCHEMAS = {}


@lru_cache(None)
def schema(group):
    path = f'src/Assembly/Plugins/Halo2/{group}.xml'
    data = subprocess.check_output(['git','-C',str(ROOT/'tools/reference/assembly'),
                                    'show',f'{PIN}:{path}'])
    import hashlib
    SCHEMAS[path] = hashlib.sha256(data).hexdigest()
    return ET.fromstring(data)


def references(cache, node, base, depth=0, trail=''):
    if depth>32:raise ValueError('Excessive nested authored tag blocks')
    for field in node:
        if 'offset' not in field.attrib:continue
        at=base+int(field.attrib['offset'],0)
        kind=field.tag.lower()
        if kind=='tagref':
            datum=cache.u(at+(0 if field.get('withGroup','true').lower()=='false' else 4))
            if datum!=0xffffffff:yield datum
        elif kind=='tagblock':
            if not any(child.tag.lower()=='tagref' for child in field.iter()):continue
            # Predicted Resources is an index-based prefetch table, not a
            # semantic full-salt dependency edge or sound-event producer.
            if field.get('name')=='Predicted Resources':continue
            try:rows=cache.block(at,int(field.attrib['elementSize'],0))
            except AssertionError as error:
                raise ValueError(f'Unresolved compiled block {trail}/{field.get("name")} at {at:08x}, count={cache.u(at)}, pointer={cache.u(at+4):08x}') from error
            for row in rows:
                yield from references(cache,field,row,depth+1,trail+'/'+field.get('name',''))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--maps',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--map',action='append',default=[])
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    scope=BASE/'authored-scope.json'
    source=json.loads(scope.read_text())
    owners={};maps=[];issues=[]
    for item in source['maps']:
        path=Path(item['path'])
        if args.map and path.stem not in args.map:continue
        if sha(path)!=item['sha256']:raise ValueError('Reference map differs from pinned scope: '+str(path))
        c=Cache(path)
        mcc_path=args.maps/path.name
        try:
            present=None
            if mcc_path.is_file():
                mcc=MccCache(mcc_path)
                try:present={(tag['group'],tag['name']) for tag in mcc.tags.values()}
                finally:mcc.close()
            else:issues.append(dict(map=path.name,error='Matching MCC map filename is absent; namespace not compared'))
            edges={};leaves={};outside=set()
            def closure(datum,visited):
                if datum in visited:return
                visited.add(datum)
                tag=c.tags.get(datum)
                if tag is None:raise ValueError(f'Authored reference has no full-salt owner: {datum:08x}')
                group=tag['group']
                if group=='snd!':leaves[datum]=tag;return
                if group not in GROUPS:outside.add(group);return
                if tag['offset'] is None:raise ValueError(f'Nonresident {group} dependency: {tag["name"]}')
                if datum not in edges:edges[datum]=tuple(references(c,schema(group),tag['offset']))
                for child in edges[datum]:closure(child,visited)
            count=0;missing=set()
            for datum,tag in c.tags.items():
                if tag['group'] not in ('weap','vehi'):continue
                key=(tag['group'],tag['name'])
                owner=owners.setdefault(key,dict(group=key[0],name=key[1],maps=[],sounds=set(),loops=set(),missing=set()))
                owner['maps'].append(path.name)
                visited=set()
                try:closure(datum,visited)
                except (ValueError,AssertionError,KeyError) as error:
                    issues.append(dict(map=path.name,owner=tag['name'],error=str(error)));continue
                audio=[c.tags[d] for d in visited if c.tags[d]['group'] in ('snd!','lsnd')]
                for sound in audio:
                    owner['sounds' if sound['group']=='snd!' else 'loops'].add(sound['name'])
                    if present is not None and (sound['group'],sound['name']) not in present:
                        owner['missing'].add(sound['name']);missing.add(sound['name'])
                count+=1
            maps.append(dict(name=path.name,reference_sha256=item['sha256'],mcc_sha256=sha(mcc_path) if present is not None else None,
                             weapon_vehicle_rows=count,missing_sound_paths=sorted(missing),
                             unfollowed_reference_groups=sorted(outside)))
            print(json.dumps(dict(map=path.name,owners=count,missing=len(missing),issues=[i for i in issues if i['map']==path.name][:2])),flush=True)
        finally:c.data.close();c.stream.close()
    rows=[]
    for owner in owners.values():
        rows.append({key:sorted(value) if isinstance(value,set) else value for key,value in owner.items()})
    missing=sorted({name for row in rows for name in row['missing']})
    report=dict(scope=__doc__,passed=not issues and not missing and len(maps)==len(source['maps']),
                reference_scope_sha256=sha(scope),schema_commit=PIN,schema_sha256=SCHEMAS,
                followed_reference_groups=sorted(GROUPS),maps=maps,owners=rows,issues=issues,
                weapon_paths=sum(row['group']=='weap' for row in rows),
                vehicle_paths=sum(row['group']=='vehi' for row in rows),
                missing_sound_paths=missing,implicit_dependencies_verified=False,
                decoded_payload_equality_verified=False,full_tu15_audio_parity=False,
                runner_scratch=str(scratch),large_artifacts_retained=[],tool_sha256=sha(Path(__file__)))
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key] for key in ('passed','weapon_paths','vehicle_paths','missing_sound_paths')}|{'issue_count':len(issues)}),flush=True)
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
