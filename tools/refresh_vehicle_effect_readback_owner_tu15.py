#!/usr/bin/env python3
"""Refresh a proved compilation-ownership split after the actual link failure."""
import argparse
import hashlib
import json
import os
from pathlib import Path
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','validation','regression','split','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    base=json.loads(args.prepared.read_text());v=json.loads(args.validation.read_text())
    failure=json.loads(args.regression.read_text());split=json.loads(args.split.read_text())
    binding=v.get('vehicle_choice_source_binding',{});source=owner/'source';pins=dict(base['source_hashes'])
    if (Path(base['owner'])!=owner or not v['passed'] or binding.get('preparation_sha256')!=sha(args.prepared) or
        not binding.get('source_unchanged_after_run') or failure['passed'] or
        not any(s['name']=='build' and s['exit_code']==2 for s in failure['steps']) or
        not failure['candidate_vehicle_regression']['incoming_CMake_restored']):
        raise ValueError('Require this unchanged native epoch and its actual failed link')
    log=(args.regression.parent/'build.log').read_text()
    if 'undefined reference to `halo2::source_effect_replica_render_tu15(' not in log:
        raise ValueError('The failed build has another cause')
    if any(sha(source/name)!=pin for name,pin in pins.items()):raise ValueError('Frozen source changed')
    old=split['old_path'];new=split['new_path'];cmake='cmake/PhysicsLiveWorldTu15.cmake'
    if sha(source/old)!=split['old_sha256'] or (source/new).exists():raise ValueError('Readback split preimage differs')
    marker='namespace halo2 {\n';original=(source/old).read_text();point=original.index('float source_effect_part_lens_opacity_tu15(')
    original_readback=original.split(marker,1)[1][:original.split(marker,1)[1].index('float source_effect_part_lens_opacity_tu15(')]
    current_readback=(ROOT/new).read_text().split(marker,1)[1].removesuffix('}\n')
    if (current_readback!=original_readback or (ROOT/old).read_text().split(marker,1)[1]!=original[point:] or
        hashlib.sha256(current_readback.encode()).hexdigest()!=split['readback_body_sha256']):
        raise ValueError('A complete readback or lens-opacity body changed')
    line='target_sources(halo2_physics_world_runtime PRIVATE src/rendering/effects/source_effect_world_locations.cpp)\n'
    addition='target_sources(halo2_physics_world_runtime PRIVATE '+new+')\n'
    before=(source/cmake).read_text()
    if before.count(line)!=1 or before.replace(line,line+addition)!=(ROOT/cmake).read_text():
        raise ValueError('Another CMake owner would change')
    replacement={name:(ROOT/name).read_bytes() for name in (old,new,cmake)}
    args.output.mkdir(parents=True,exist_ok=False);changes=[]
    for name,raw in replacement.items():
        path=source/name;prior=sha(path) if path.exists() else None;path.write_bytes(raw);pins[name]=sha(path)
        changes.append(dict(path=name,before=prior,after=pins[name]))
    if any(sha(source/name)!=pin for name,pin in pins.items()):raise ValueError('Source drift during ownership refresh')
    canonical=dict(base['canonical_source_hashes']);canonical.update({name:pins[name] for name in replacement})
    report=dict(base,status='prepared',passed=False,validation_not_run=True,source_hashes=pins,
                canonical_source_hashes=canonical,changes=changes,complete_function_bodies_preserved=True,
                previous_prepared_sha256=sha(args.prepared),previous_validation_sha256=sha(args.validation),
                actual_failed_regression_sha256=sha(args.regression),split_proof_sha256=sha(args.split),
                tool_sha256=sha(Path(__file__)))
    (args.output/'prepared.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status='prepared',changed_files=len(changes),complete_function_bodies_preserved=True)),flush=True)


if __name__=='__main__':main()
