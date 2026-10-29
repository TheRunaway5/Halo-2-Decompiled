#!/usr/bin/env python3
"""Replay genuine needle tag/graphics preparation in run_test-owned scratch.

This opt-in diagnostic reads the installed source and accepted COW reference.
Only apply_selected's map/texture dependencies are copied. No source, selected
cache, registry or runtime is written. Keep only its bounded JSON receipt.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import sys
import time


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def identity(path):
    return dict(bytes=path.stat().st_size, sha256=sha(path))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--resources', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--reference', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--graphics-helper', type=Path,
                        help='Declared replacement helper; all data/dependencies stay in resources')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    output = args.output.resolve()
    if output.exists() or not output.is_relative_to(scratch):
        raise ValueError('Choose a new output inside HALO2_TEST_SCRATCH')
    output.mkdir()
    source, resources = args.source.resolve(strict=True), args.resources.resolve(strict=True)
    sys.path.insert(0,str(resources/'tools'))
    from mcc_content import MccCache
    if args.graphics_helper is None:
        from mcc_graphics_corrections import apply_selected
    else:
        spec=importlib.util.spec_from_file_location('needle_graphics_control',args.graphics_helper)
        module=importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        apply_selected=module.apply_selected
    from prepare_mcc_gameplay import save_changed_cache
    from vista_tag_conversion import TagGraph, apply_combat, apply_netgame
    from mcc_instant_charge import apply_instant_charge_plan, plan_instant_charge
    from mcc_bitmap_correction_bundle_v3 import MAP_NAME
    from mcc_bitmap_correction_bundle_v4 import BUNDLE_V4, MAP_NAME as MAP_V4
    from mcc_bitmap_correction_bundle_v5 import BUNDLE_V5, MAP_NAME as MAP_V5
    from mcc_bitmap_correction_bundle_v6 import BUNDLE_V6, OWNERS_V6
    from mcc_bitmap_correction_bundle_v7 import BUNDLE_V7, read_v7, profiles_for_map_v7
    from mcc_decal_bitmap_corrections import BUNDLE as DECALS, read_bundle as read_decals
    names = {'zanzibar.map',MAP_NAME,'needle.map','shared.map'}
    for bundle,name in ((BUNDLE_V4,MAP_V4),(BUNDLE_V5,MAP_V5)):
        if (resources/bundle).is_file():
            names.add(name)
    if (resources/BUNDLE_V6).is_file():
        names.update(owner['map'] for owner in OWNERS_V6)
    if (resources/BUNDLE_V7).is_file():
        names.update(owner['map'] for owner in read_v7(resources)[1]['owners'])
    if (resources/DECALS).is_file():
        names.update(owner['map'] for owner in read_decals(resources)[1]['owners'])
    source_maps=source/'h2_maps_win64_dx11'
    maps=output/'halo2/h2_maps_win64_dx11';maps.mkdir(parents=True)
    before={name:identity(source_maps/name) for name in sorted(names|{'textures.dat'})}
    for name in before:
        shutil.copy2(source_maps/name,maps/name)
    needle=maps/'needle.map'
    report=dict(platform=sys.platform, source=str(source), output=str(output),
                copied_files=len(before), copied_bytes=sum(row['bytes'] for row in before.values()),
                source_inputs=before, expected_reference=identity(args.reference),
                stages=dict(copied=identity(needle)), retained_large_artifacts=[])
    policy=json.loads((resources/'tools/data/mcc-native-hud-bootstrap.json').read_text())
    expected=next(row for row in policy['prepared_inputs'] if row['map']=='needle')
    report['HUD_expected']=expected
    paths=['tools/mcc_graphics_corrections.py','tools/prepare_mcc_gameplay.py',
           'tools/mcc_native_map_owners.py','tools/data/mcc-native-hud-bootstrap.json']
    report['helper_sha256']={name:sha(resources/name) for name in paths}
    if args.graphics_helper is not None:
        report['helper_sha256']['tools/mcc_graphics_corrections.py']=sha(args.graphics_helper)
        report['declared_graphics_helper']=str(args.graphics_helper.resolve(strict=True))
    cache,shared=MccCache(needle),MccCache(maps/'shared.map')
    try:
        charge=plan_instant_charge(cache)
        charged=apply_instant_charge_plan(cache,charge)
        scenario,at=cache.tag(cache.u32(cache.index+12),'scnr')
        assert cache.u16(at+16)==1
        graph=TagGraph(cache,shared)
        apply_netgame(graph,scenario['address'])
        apply_combat(graph,scenario['address'],graph.resolve(cache.u32(cache.index+16),'matg'))
        report['tag_projection']=dict(instant_charge=charged, stores=graph.stores)
        if charged['changed_store_blocks'] or any(row['map']=='needle.map' and row['before']!=row['after'] for row in graph.stores):
            save_changed_cache(cache)
    finally:
        cache.close();shared.close()
    report['stages']['tag_converted']=identity(needle)
    revisions={'needle.map':dict(source_sha256=before['needle.map']['sha256'],output_sha256=sha(needle))}
    started=time.monotonic()
    graphics=apply_selected(resources,output,tag_revisions=revisions)
    report['graphics_seconds']=time.monotonic()-started
    report['stages']['graphics_corrected']=identity(needle)
    report['graphics_needle_changes']=[row for row in graphics['changes'] if row['path'].endswith('/needle.map')]
    report['graphics_resource_layout']=graphics.get('resource_layout')
    report['corrected_texture_bank']=identity(maps/'textures.dat')
    target=identity(needle)
    report['matches_HUD_expected']=target==dict(bytes=expected['bytes'],sha256=expected['sha256'])
    actual,reference=needle.read_bytes(),args.reference.read_bytes()
    offsets=[index for index,(left,right) in enumerate(zip(actual,reference)) if left!=right]
    regions=[]
    for at in offsets:
        if regions and regions[-1][1]==at:
            regions[-1][1]=at+1
        else:
            regions.append([at,at+1])
    report['binary_diff']=dict(actual_bytes=len(actual),reference_bytes=len(reference),
        differing_bytes=len(offsets),regions=regions[:128],regions_count=len(regions),
        examples=[dict(offset=lo,actual=actual[lo:hi].hex(),reference=reference[lo:hi].hex())
                  for lo,hi in regions[:32] if hi-lo<=64])
    descriptor=[]
    cache=MccCache(needle)
    reference_cache=MccCache(args.reference)
    try:
        for profile in profiles_for_map_v7(resources,'needle.map'):
            tag=next(t for t in cache.tags.values() if t['group']=='bitm' and t['name']==profile['tag'])
            at=cache.block(tag['offset']+68,168)[0]
            other=next(t for t in reference_cache.tags.values() if t['group']=='bitm' and t['name']==profile['tag'])
            other_at=reference_cache.block(other['offset']+68,168)[0]
            descriptor.append(dict(tag=profile['tag'],offset=at,reference_offset=other_at,
                actual_pointer=cache.u32(at+56),reference_pointer=reference_cache.u32(other_at+56),
                actual_extent=cache.u32(at+80),reference_extent=reference_cache.u32(other_at+80),
                actual_hex=bytes(cache.data[at:at+168]).hex(),reference_hex=bytes(reference_cache.data[other_at:other_at+168]).hex()))
    finally:
        cache.close();reference_cache.close()
    report['needle_sky_descriptors']=descriptor
    partitions=[]
    for row in descriptor:
        def partition(path,pointer,size):
            with path.open('rb') as stream:
                stream.seek(pointer)
                return stream.read(size)
        actual=partition(maps/'textures.dat',row['actual_pointer'],row['actual_extent'])
        reference=partition(args.reference.parent/'textures.dat',row['reference_pointer'],row['reference_extent'])
        partitions.append(dict(tag=row['tag'],identical=actual==reference,
            actual_sha256=hashlib.sha256(actual).hexdigest(),reference_sha256=hashlib.sha256(reference).hexdigest(),
            bytes=len(actual)))
    report['pointer_target_comparison']=partitions
    report['original_source_unchanged']=all(identity(source_maps/name)==record for name,record in before.items())
    report['reference_unchanged']=identity(args.reference)==report['expected_reference']
    report['passed']=report['matches_HUD_expected'] and report['original_source_unchanged'] and report['reference_unchanged'] and all(
        row['identical'] for row in partitions)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],report=str(args.report),stages=report['stages'],
                         binary_diff=report['binary_diff'],descriptors=descriptor,
                         original_source_unchanged=report['original_source_unchanged'])),flush=True)
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
