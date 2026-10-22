#!/usr/bin/env python3
"""Audit all authored weapon WORLD models against the native presentation filter.

Run through run_test.py. Tag presence does not establish whether an AI, vehicle,
objective or player owner may drop/equip an object. No eligibility is inferred.
"""
import argparse
from collections import Counter
import json
import os
from pathlib import Path

from inspect_multiplayer import Cache
from original_xbox_pose_case import BASE, ROOT, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    scope_path=BASE/'authored-scope.json';scope=json.loads(scope_path.read_text())
    weapons={};counts=Counter();maps=[]
    for item in scope['maps']:
        path=Path(item['path']);assert sha(path)==item['sha256']
        cache=Cache(path)
        try:
            map_counts=Counter()
            for authored in item['weapons']:
                assert authored['resident']
                tag=cache.tags[int(authored['tag']['datum'],16)]
                assert tag['group']=='weap' and tag['offset']==authored['tag']['offset']
                root=tag['offset']
                assert bytes(cache.data[root:root+0x31c]).hex()==authored['root_hex']
                fp=[(cache.u(at+4),cache.u(at+12)) for at in cache.block(root+0x2a8,16)]
                selected_by_native_body_filter=bool(fp and fp[0][0]!=0xffffffff and fp[0][1]!=0xffffffff)
                model=cache.u(root+0x38);render=animation=0xffffffff
                if model!=0xffffffff:
                    owner=cache.tags[model];assert owner['group']=='hlmt'
                    assert owner['offset'] is not None
                    render=cache.u(owner['offset']+4);animation=cache.u(owner['offset']+0x14)
                    if render!=0xffffffff:
                        assert cache.tags[render]['group']=='mode' and cache.tags[render]['offset'] is not None
                category=('world_in_native_body_filter' if selected_by_native_body_filter else
                    'world_excluded_by_native_body_filter') if render!=0xffffffff else 'no_authored_world_render_model'
                counts[category]+=1;map_counts[category]+=1
                entry=weapons.setdefault(tag['name'],dict(name=tag['name'],variants={}))
                key=(model,render,animation,tuple(fp),category)
                variant=entry['variants'].setdefault(key,dict(model=cache.reference(model),
                    render=cache.reference(render),world_animation=cache.reference(animation),
                    first_person=[dict(model=cache.reference(m),animation=cache.reference(a))for m,a in fp],
                    selected_by_native_body_filter=selected_by_native_body_filter,category=category,
                    original_owner_applicability='unverified',occurrences=[]))
                variant['occurrences'].append(dict(map=str(path),datum=tag['datum']))
            maps.append(dict(path=str(path),sha256=item['sha256'],rows=len(item['weapons']),counts=dict(map_counts)))
        finally:
            cache.data.close();cache.stream.close()
    rows=[dict(name=entry['name'],variants=list(entry['variants'].values()))for entry in weapons.values()]
    assert len(rows)==63 and len(maps)==38 and sum(counts.values())==1110
    excluded=[entry['name']for entry in rows if any(v['category']=='world_excluded_by_native_body_filter'for v in entry['variants'])]
    report=dict(authored_metadata_audit_completed=True,weapon_paths=63,authored_rows=1110,maps=maps,
        counts=dict(counts),world_model_paths_excluded_by_first_person_filter=excluded,weapons=rows,
        filter_source='src/gameplay/multiplayer/multiplayer_runtime.cpp: body_guns first_person_models.front() model/animation predicate',
        filter_source_sha256=sha(ROOT/'src/gameplay/multiplayer/multiplayer_runtime.cpp'),scope_sha256=sha(scope_path),
        native_content_import_admission_verified=False,owning_game_rules_verified=False,
        dropping_equipping_or_visual_behavior_verified=False,full_weapon_parity=False,fully_certified_weapons=0,
        runner_scratch=os.environ['HALO2_TEST_SCRATCH'],tool_sha256=sha(Path(__file__)),large_artifacts_retained=[])
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(weapon_paths=63,authored_rows=1110,counts=dict(counts),excluded_paths=excluded)),flush=True)


if __name__=='__main__':main()
