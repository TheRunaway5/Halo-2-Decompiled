#!/usr/bin/env python3
"""Compare every authored zoom and firing-selector contract with prepared MCC.

Read-only offline Xbox metadata and actual MCC directories. This does not
establish execution, placement, particles, PCM, or framebuffer equality.
Run through tools/run_test.py; only the bounded explicit report survives.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from inspect_multiplayer import Cache
from mcc_content import MccCache, directory

NONE=0xffffffff

def sha(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mcc-root',type=Path,required=True)
    p.add_argument('--scope',type=Path,default=Path('analysis/weapons-tu15-goal-20261013/authored-scope.json'))
    p.add_argument('--report',type=Path,required=True)
    p.add_argument('--fixture',type=Path)
    args=p.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    scope=json.loads(args.scope.read_text());maps=directory(args.mcc_root)/'h2_maps_win64_dx11'
    inputs=[];differences=[];missing=[];rows=[];definitions=set();zoom_names=set();effects=set()
    contracts=[];contract_indices={};fixture_maps=[]
    def u(c,at):return struct.unpack_from('<I',c.data,at)[0]
    def ref(c,value):
        if value==NONE:return None
        t=c.tags[value];return [t['group'],t['name']]
    def contract(c,t):
        q=t['offset']
        if q is None:raise ValueError('Nonresident contract')
        # C7DD0 consumes signed count and exact authored level range, then
        # +270/+278. 107C40 selects count-bounded normal/misfire/empty rows.
        zoom=dict(levels=struct.unpack_from('<h',c.data,q+0x1fe)[0],
            range_bits=bytes(c.data[q+0x200:q+0x208]).hex(),
            sound_in=ref(c,u(c,q+0x270)),sound_out=ref(c,u(c,q+0x278)))
        barrels=[]
        for b in c.block(q+0x2d0,0xec):
            firing=[]
            for e in c.block(b+0xe4,0x34):
                firing.append(dict(shot_bounds=bytes(c.data[e:e+4]).hex(),
                    normal=ref(c,u(c,e+8)),misfire=ref(c,u(c,e+0x10)),empty=ref(c,u(c,e+0x18))))
            barrels.append(firing)
        triggers=[]
        for t in c.block(q+0x2c8,0x40):
            triggers.append(dict(charging=ref(c,u(c,t+0x34)),charging_damage=ref(c,u(c,t+0x3c))))
        return dict(zoom=zoom,firing=barrels,triggers=triggers,ready=ref(c,u(c,q+0x144)))
    # Shared lookup is used only for an actual nonresident map identity, with
    # its complete salted datum/group/name matched, mirroring native admission.
    shared=[MccCache(maps/n) for n in ['shared.map','single_player_shared.map']]
    for c in shared:inputs.append(dict(role='MCC shared',path=str(c.path),sha256=sha(c.path)))
    try:
        for item in scope['maps']:
            original=Path(item['path'])
            if sha(original)!=item['sha256']:raise ValueError('Original scope pin differs')
            filename='highplains.map' if original.name=='highplains2.map' else original.name
            candidate=maps/filename
            if not candidate.is_file():missing.append(dict(map=original.name,reason='MCC map absent'));continue
            a=Cache(original);b=MccCache(candidate)
            inputs.extend([dict(role='Original',path=str(original),sha256=item['sha256']),dict(role='MCC',path=str(candidate),sha256=sha(candidate))])
            try:
                by_name={(t['group'],t['name']):(d,t) for d,t in b.tags.items()}
                fixture_rows=[]
                for source in item['weapons']:
                    t=source['tag'];name=t['name'];definitions.add(name)
                    old=contract(a,a.tags[int(t['datum'],16)])
                    key=json.dumps(old,sort_keys=True)
                    if key not in contract_indices:
                        contract_indices[key]=len(contracts);contracts.append(old)
                    fixture_rows.append(dict(weapon=name,contract=contract_indices[key]))
                    matched=by_name.get(('weap',name))
                    if matched is None:missing.append(dict(map=original.name,weapon=name,reason='MCC identity absent'));continue
                    datum,tag=matched;owner=b
                    if tag['offset'] is None:
                        residents=[(c,c.tags[datum]) for c in shared if datum in c.tags and
                            c.tags[datum]['offset'] is not None and (c.tags[datum]['group'],c.tags[datum]['name'])==('weap',name)]
                        if len(residents)!=1:raise ValueError('Nonresident weapon lacks unique matching shared owner')
                        owner,tag=residents[0]
                    new=contract(owner,tag)
                    if old['zoom']['levels']>0:zoom_names.add(name)
                    for key in ['zoom','firing','triggers','ready']:
                        if old[key]!=new[key]:differences.append(dict(map=original.name,weapon=name,field=key,original=old[key],mcc=new[key]))
                    for barrel in old['firing']:
                        for e in barrel:
                            for k in ['normal','misfire','empty']:
                                if e[k]:effects.add(tuple(e[k]))
                    rows.append(dict(map=original.name,weapon=name,matched=old==new,zoom_levels=old['zoom']['levels'],
                        zoom_in=old['zoom']['sound_in'],zoom_out=old['zoom']['sound_out'],barrels=len(old['firing'])))
                # Binocular fallback is map-global, independent of weap count.
                ga=next(t for t in a.tags.values() if t['group']=='matg')
                gb=b.tags[b.u32(b.index+16)];globals_owner=b
                if gb['offset'] is None:
                    found=[(c,c.tags[gb['datum']]) for c in shared if gb['datum'] in c.tags and c.tags[gb['datum']]['offset'] is not None and c.tags[gb['datum']]['name']==gb['name']]
                    if len(found)!=1:raise ValueError('Globals has no unique actual shared owner')
                    globals_owner,gb=found[0]
                def binocular(c,t):
                    block=c.block(t['offset']+0x130,0x11c)
                    if len(block)!=1:raise ValueError('Player info count differs')
                    q=block[0]
                    return dict(levels=u(c,q+0xb8),sound_in=ref(c,u(c,q+0xc8)),sound_out=ref(c,u(c,q+0xd0)))
                ba,bb=binocular(a,ga),binocular(globals_owner,gb)
                fixture_maps.append(dict(name=filename,original=str(original),original_sha256=item['sha256'],
                    mcc_sha256=sha(candidate),binocular=ba,weapons=fixture_rows))
                if ba!=bb:differences.append(dict(map=original.name,field='binocular',original=ba,mcc=bb))
            finally:a.data.close();a.stream.close();b.close()
    finally:
        for c in shared:c.close()
    report=dict(schema='H2_TU15_ALL_WEAPON_ZOOM_SHOOTING_CONTRACTS_V1',passed=not missing,
        raw_contracts_equivalent=not differences,native_conversion_verified=False,
        scope_sha256=sha(args.scope),maps=len(rows) and len({r['map'] for r in rows}),weapons=len(definitions),
        weapon_rows=len(rows),optic_weapon_paths=sorted(zoom_names),firing_reference_identities=len(effects),
        differences=differences,missing=missing,rows=rows,inputs=inputs,
        runtime_execution_verified=False,full_output_parity=False,retained_large_artifacts=[],
        schema_commit='fd14ef3c46135e9422966c6a64cd926fb0bd23fb')
    data=json.dumps(report,indent=2)+'\n'
    if len(data)>1024*1024:raise ValueError('Report exceeds bounded storage')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(data)
    print(json.dumps({k:report[k] for k in ['passed','maps','weapons','weapon_rows','optic_weapon_paths','firing_reference_identities','missing']},indent=2))
    print('differences',len(differences))
    if args.fixture:
        fixture=json.dumps(dict(schema='H2_TU15_ALL_WEAPON_PRESENTATION_CONTRACTS_V1',
            scope_sha256=sha(args.scope),contracts=contracts,maps=fixture_maps),separators=(',',':'))+'\n'
        if len(fixture)>1024*1024:raise ValueError('Fixture exceeds bounded storage')
        args.fixture.write_text(fixture)
        print('fixture bytes',len(fixture),'contracts',len(contracts))
    return 0 if not missing else 1

if __name__=='__main__':raise SystemExit(main())
