#!/usr/bin/env python3
"""Publish the authored H2EK Dust II cache for the native engine.

Run through tools/run_test.py. This producer appends native-format lightmap
resources and a separately owned original-program material bank. It does not
alias a stock map or import Windows shader bytecode as native programs.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct

import numpy as np
from mcc_content import MccCache
from mcc_native_map_owner_publication import publish_staged_owner_index
from mcc_native_map_owners import bank_identity
from dust2_main_hud import install_main_hud


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, sort_keys=True, indent=2) + '\n')


def compact_visibility(cache, bsp):
    """Merge list subparts without rewriting any source triangle indices."""
    data = cache.data
    sections = cache.block(bsp['offset']+156,176) + cache.block(bsp['offset']+312,200)
    rows = []
    for section in sections:
        count = cache.u16(section)
        if not count:
            continue
        pointer, size, header = (cache.u32(section+x) for x in (40,44,48))
        assert pointer >> 30 == 0
        resources = cache.block(section+56,16)
        base = pointer + header + 8
        parts = resources[0]
        subs = next(r for r in resources if cache.u16(r+4)==8)
        indices = next(r for r in resources if cache.u16(r+4)==32)
        start, length = base+cache.u32(indices+12), cache.u32(indices+8)
        # H2EK's original compiled list is authoritative. Never stitch it as
        # a strip: that introduces cross-triangle faces and changes winding.
        assert length//2 == cache.u16(section+2)*3, 'Recompile the original ASS; prior strip repack is invalid'
        digest = hashlib.sha256(data[start:start+length]).hexdigest()
        part_count = cache.u32(parts+8)//72
        part_base, sub_base = base+cache.u32(parts+12), base+cache.u32(subs+12)
        new = []
        for part in range(part_count):
            at = part_base+part*72
            first, extent = cache.u16(at+6), cache.u16(at+8)
            assert extent % 3 == 0 and first+extent <= length//2
            struct.pack_into('<HH',data,at+10,len(new),int(bool(extent)))
            if extent:
                new.append((first,extent,65535,part))
        assert len(new)*8 <= cache.u32(subs+8)
        before = cache.u32(pointer+16)
        for i, row in enumerate(new):
            struct.pack_into('<4H',data,sub_base+i*8,*row)
        struct.pack_into('<I',data,subs+8,len(new)*8)
        struct.pack_into('<I',data,pointer+16,len(new))
        struct.pack_into('<H',data,section+36,len(new))
        struct.pack_into('<I',data,pointer+56,0) # native conservative culling
        assert digest == hashlib.sha256(data[start:start+length]).hexdigest()
        rows.append(dict(before=before,after=len(new),index_sha256=digest,indices_unchanged=True))
    return dict(sections=len(rows),before_subparts=sum(x['before'] for x in rows),
        after_subparts=sum(x['after'] for x in rows),rows=rows)


def vertex_stream_digest(cache):
    bsp=next(t for t in cache.tags.values() if t['group']=='sbsp')
    digest=hashlib.sha256()
    for at in cache.block(bsp['offset']+312,200):
        if not cache.u16(at):
            continue
        pointer,header=cache.u32(at+40),cache.u32(at+48)
        for r in cache.block(at+56,16):
            if cache.u16(r+4)==56 and cache.u16(r+6) in (0,1,2):
                offset=pointer+header+8+cache.u32(r+12)
                digest.update(cache.data[offset:offset+cache.u32(r+8)])
    return digest.hexdigest()


def encode_lighting(source, buckets, output, reference_cache=None, main_hud=None, imported=None):
    cache = MccCache(source)
    assert main_hud is not None and imported is not None, 'Export the main-game HUD with export_dust2_hud first'
    hud = install_main_hud(cache, main_hud, imported)
    vertex_hash=vertex_stream_digest(cache)
    if reference_cache:
        reference=MccCache(reference_cache)
        assert vertex_hash==vertex_stream_digest(reference), 'Recompiled vertices no longer match the baked lighting order'
        del reference
    data = cache.data
    scenario = cache.tags[cache.u32(cache.index + 12)]
    # The authored backdrop uses desert geometry and a clear sky color. The
    # stock Use Clear Color flag also admits the original render-sky lighting
    # query for dynamic objects (Cache::object_sky_lighting_available).
    skies = cache.block(scenario['offset']+8,8)
    assert len(skies)==1
    sky = cache.tags[cache.u32(skies[0]+4)]
    assert sky['group']=='sky ' and sky['name']=='scenarios\\multi\\dust2_mombasa\\desert'
    sky_flags_before=cache.u32(sky['offset']+16)
    struct.pack_into('<I',data,sky['offset']+16,sky_flags_before|32)
    fog=cache.block(sky['offset']+0x48,24)
    assert len(fog)==1
    data[sky['offset']+0xa0:sky['offset']+0xac]=data[fog[0]:fog[0]+12]
    bsp_rows = cache.block(scenario['offset'] + 0x210, 0x44)
    assert len(bsp_rows) == 1
    bsp = cache.tags[cache.u32(bsp_rows[0] + 0x14)]
    visibility = compact_visibility(cache,bsp)
    source_bsp_sha = hashlib.sha256(data[bsp['offset']:bsp['offset']+bsp['size']]).hexdigest()
    instances = cache.block(bsp['offset'] + 320, 88)
    definitions = cache.block(bsp['offset'] + 312, 200)
    assert len(instances) == 485 and len(definitions) == 485
    assert cache.u32(scenario['offset'] + 0x398) == 1, 'Authored map identity is required'
    group_magic = cache.u32(cache.index) - cache.index - 32
    old_table = cache.pointer(cache.u32(cache.index + 8), group_magic, len(cache.tags)*16)
    old_count = cache.u32(cache.index + 24)
    old_names, names_size, old_offsets = (cache.u32(x) for x in (36, 40, 44))
    names = bytes(data[old_names:old_names+names_size])
    offsets = bytearray(data[old_offsets:old_offsets+old_count*4])
    table = bytearray(data[old_table:old_table+old_count*16])
    new_handle = (0xE001 << 16) | old_count
    scenario_name = b'scenarios\\multi\\dust2_mombasa\\dust2_mombasa'
    struct.pack_into('<I', offsets, (scenario['datum'] & 65535)*4, len(names))
    names += scenario_name+b'\0'
    data[208:464] = scenario_name.ljust(256,b'\0')

    def align(value, size=16):
        return (value + size - 1) // size * size

    # Metadata precedes resources, keeping the resident address span bounded.
    start = align(len(data))
    meta = bytearray(136 + 104)
    base_address = start + cache.magic
    group = 136
    struct.pack_into('<II', meta, 128, 1, base_address + group)
    struct.pack_into('<I', meta, group+4, cache.u32(bsp['offset']+8))
    struct.pack_into('<I', meta, group+28, 0xffffffff)

    def block(field, count, stride):
        at = align(len(meta), 4)
        meta.extend(bytes(at-len(meta) + count*stride))
        struct.pack_into('<II', meta, field, count, base_address+at if count else 0)
        return at

    for i, at in enumerate(instances):
        if i < 411:
            struct.pack_into('<H',data,at+86,1)
    render = [i for i, at in enumerate(instances) if cache.u16(at+86) == 1]
    assert render == list(range(411))
    bucket_at = block(group+64, len(render), 56)
    info = block(group+72, len(instances), 4)
    for i in range(len(instances)):
        struct.pack_into('<h', meta, info+i*4, -1)
    refs = block(group+80, len(instances), 12)
    resources, paths, total = [], [], 0
    for i, instance in enumerate(instances):
        struct.pack_into('<hh', meta, refs+i*12, 0, i if i < 411 else -1)
        if i >= 411:
            continue
        section = cache.u16(instance+52)
        vertex_count = cache.u16(definitions[section])
        path = buckets / f'{i:03d}.bin'
        assert path.stat().st_size == vertex_count*24
        total += vertex_count
        at = bucket_at + i*56
        struct.pack_into('<H', meta, at, 3)
        res = block(at+28, 3, 16)
        # Type, primary locator, secondary locator, size, resource offset.
        for n, (kind, channel, size, offset) in enumerate(((0,32,64,0),
                (2,0,vertex_count*12,64), (2,1,vertex_count*4,64+vertex_count*12))):
            struct.pack_into('<BBHHHII', meta, res+n*16, kind,0,0,0,channel,size,offset)
        sections = block(refs+i*12+4, 1, 2)
        struct.pack_into('<H', meta, sections, 0)
        resources.append((at, vertex_count))
        paths.append(path)
    assert total == 4754431
    data.extend(bytes(start-len(data)))
    meta_end = start + len(meta)
    cursor = align(meta_end)
    payloads = []
    clipped = 0
    for (at, count), path in zip(resources, paths):
        samples = np.fromfile(path, dtype='<f4').reshape(count, 6)
        assert np.isfinite(samples).all()
        clipped += int(np.any(samples[:,:3] > 1, axis=1).sum())
        rgba = np.empty((count,4), dtype=np.uint8)
        rgba[:,:3] = np.rint(np.clip(samples[:,:3], 0, 1)*255).astype(np.uint8)
        rgba[:,3] = 255
        declaration = bytearray(64)
        declaration[0:2] = bytes((47,12))
        declaration[32:34] = bytes((46,4))
        raw = bytes(8) + declaration + samples[:,3:6].astype('<f4').tobytes() + rgba.tobytes()
        struct.pack_into('<III', meta, at+12, cursor,len(raw),0)
        payloads.append((cursor,raw))
        cursor = align(cursor + len(raw))
    data.extend(meta)
    for offset, raw in payloads:
        data.extend(bytes(offset-len(data))); data.extend(raw)
    data.extend(bytes(cursor-len(data)))
    name_offset = len(names)
    names += b'scenarios\\multi\\dust2_mombasa\\native_lightmap\0'
    names_at = len(data); data.extend(names)
    offsets_at = len(data); data.extend(offsets+struct.pack('<I',name_offset))
    table_at = align(len(data)); data.extend(bytes(table_at-len(data)))
    table.extend(struct.pack('<4I', int.from_bytes(b'ltmp','big'),new_handle,base_address,len(meta)))
    data.extend(table)
    for at, value in ((8,len(data)), (32,old_count+1), (36,names_at), (40,len(names)), (44,offsets_at),
            (cache.index+8,table_at+group_magic), (cache.index+24,old_count+1), (bsp_rows[0]+0x1c,new_handle)):
        struct.pack_into('<I',data,at,value)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)
    verified = MccCache(output)
    assert verified.tags[new_handle]['size'] == len(meta)
    final_bsp = verified.tags[bsp['datum']]
    final_bsp_sha = hashlib.sha256(verified.data[final_bsp['offset']:final_bsp['offset']+final_bsp['size']]).hexdigest()
    return dict(vertices=total, buckets=411, instance_refs=485, clipped_hdr_vertices=clipped, main_game_hud=hud,
        render_sky=dict(flags_before=sky_flags_before,flags_after=sky_flags_before|32,
                        clear_color_source='authored desert atmospheric fog color'),
        incident_direction='authored receiver hemisphere plus source sun',
        irradiance='Cycles diffuse direct and indirect; 48 samples; six diffuse bounces',
        metadata_bytes=len(meta), raw_resource_bytes=sum(len(raw) for _,raw in payloads),
        source_sha256=sha(source), output_sha256=sha(output), visibility=visibility,
        compiled_bsp_sha256=final_bsp_sha, triangle_index_bytes_unchanged=True,
        position_normal_uv_stream_sha256=vertex_hash, bake_order_verified=bool(reference_cache))


def prepare_compiled_lighting(source,output,main_hud,imported,*,compact=True):
    cache=MccCache(source)
    hud=install_main_hud(cache,main_hud,imported)
    scenario=cache.tags[cache.u32(cache.index+12)]
    refs=cache.block(scenario['offset']+0x210,68)
    assert len(refs)==1
    bsp=cache.tags[cache.u32(refs[0]+20)]
    lightmap=cache.tags[cache.u32(refs[0]+28)]
    assert lightmap['group']=='ltmp' and lightmap['address']
    groups=cache.block(lightmap['offset']+128,104)
    assert len(groups)==1 and cache.u32(groups[0]+4)==cache.u32(bsp['offset']+8)
    visibility=compact_visibility(cache,bsp) if compact else dict(
        compiler_subparts_preserved=True,compiler_accelerators_preserved=True,
        compiler_collision_render_references_preserved=True)
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_bytes(cache.data)
    return dict(native_classic_lightmaps=True,lighting_tag=lightmap['name'],main_game_hud=hud,
        visibility=visibility,source_sha256=sha(source),output_sha256=sha(output),
        compiled_instances=len(cache.block(bsp['offset']+320,88)),triangle_index_bytes_unchanged=True)


def publish(source, buckets, imported, output, report, main_hud, reference_cache=None,compiled_lighting=False,output_name='dust2_mombasa'):
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    output = output.resolve()
    assert output.is_relative_to(scratch) and not output.exists(), 'Use a fresh owned scratch destination'
    maps = output/'halo2/h2_maps_win64_dx11'
    maps.mkdir(parents=True)
    shutil.copytree(imported/'halo2/h2_fonts',output/'halo2/h2_fonts')
    stock = imported/'halo2/h2_maps_win64_dx11/shared.map'
    shutil.copyfile(stock, maps/'shared.map')
    assert output_name and all(c in 'abcdefghijklmnopqrstuvwxyz0123456789_' for c in output_name)
    relative = f'halo2/Mods/{output_name}/{output_name}.map'
    target = output/relative
    lighting = prepare_compiled_lighting(source,target,main_hud,imported) if compiled_lighting else encode_lighting(source, buckets, target, reference_cache, main_hud, imported)
    # H2EK self-contained v13 caches contain their own segmented texture bank.
    # A hardlink exposes the same immutable bytes under the native bank name.
    for name in ('textures.dat','sounds_en.dat','sounds_neutral.dat','sounds_remastered.dat'):
        os.link(target, maps/name)
    bankdir = 'native-shaders/maps/dust2_mombasa'
    bank = output/bankdir
    (bank/'payloads').mkdir(parents=True)
    records, donors = {}, []
    first = None
    donor_names=('03a_oldmombasa','turf','zanzibar','03b_newmombasa','lockout','01b_spacestation','00a_introduction') if compiled_lighting else ('03b_newmombasa','lockout','01b_spacestation','00a_introduction')
    for name in donor_names:
        directory = imported/'native-shaders'/name
        raw = (directory/'library.json').read_bytes()
        lib = json.loads(raw)
        if first is None:
            first = copy.deepcopy(lib)
        donors.append(dict(map=name, library_sha256=hashlib.sha256(raw).hexdigest()))
        for row in lib['shader_tags']:
            key = (row['group'],row['name'])
            if key not in records:
                records[key] = copy.deepcopy(row)
                shutil.copyfile(directory/'payloads'/row['payload'], bank/'payloads'/row['payload'])
    # Same native rock/sand program; the map's custom shader changes its bitmap.
    rock = copy.deepcopy(records[('shad','scenarios\\shaders\\solo\\earthcity\\ec_ocean_floor')])
    rock['name'] = 'scenarios\\multi\\dust2_mombasa\\shaders\\desert_sedimentary'
    replaced = 0
    for ref in rock['references']:
        if ref[1][0] == 'bitm' and ref[1][1].endswith('ec_ocean_floor'):
            ref[1][1] = 'scenarios\\bitmaps\\detail_maps\\nature\\rock\\sedimentary_rock_a'
            replaced += 1
    assert replaced == 3 # authored parameter and both native implementation banks
    records[('shad',rock['name'])] = rock
    sand=copy.deepcopy(records[('shad','scenarios\\shaders\\solo\\earthcity\\ec_ocean_floor')])
    sand['name']='scenarios\\multi\\dust2_mombasa\\shaders\\desert_sand'
    replaced=0
    for ref in sand['references']:
        if ref[1][0]=='bitm' and ref[1][1].endswith('ec_ocean_floor'):
            ref[1][1]='scenarios\\bitmaps\\detail_maps\\nature\\ground\\sand'
            replaced+=1
    assert replaced==3
    records[('shad',sand['name'])]=sand
    cache_names={t['name'].replace('/','\\') for t in MccCache(target).tags.values() if t['group']=='shad'}
    road_name=r'scenarios\multi\dust2_mombasa\shaders\polished_road'
    if road_name in cache_names:
        road=copy.deepcopy(records[('shad',r'scenarios\shaders\solo\earthcity\base_paver')])
        road['name']=road_name
        replaced=0
        for ref in road['references']:
            if ref[1]==['bitm',r'scenarios\bitmaps\solo\earthcity\base_road1']:
                ref[1][1]=r'scenarios\multi\dust2_mombasa\bitmaps\polished_road'
                replaced+=1
        assert replaced==3, 'Preserve authored parameter and both original implementation bitmap references'
        records[('shad',road_name)]=road
    rasterizer = first['rasterizer']
    shutil.copyfile(imported/f'native-shaders/{donor_names[0]}/payloads'/rasterizer['payload'], bank/'payloads'/rasterizer['payload'])
    first.update(map='dust2_mombasa',shader_tags=list(records.values()),custom_authoring=dict(donors=donors,
        rock='ec_ocean_floor native program with sedimentary_rock_a base-map reference',
        hud='Main-game HUD definitions and bitmap partitions; existing MultiplayerHud renderer'))
    save(bank/'library.json',first)
    for destination in (bank/'function-tables.bin',output/'native-shaders/function-tables.bin'):
        shutil.copyfile(imported/'native-shaders/function-tables.bin', destination)
    save(output/'native-shaders/catalog.json',dict(schema='H2_PRESERVED_NATIVE_SHADER_CATALOG_V1', maps=[]))
    inventory = {str(p.relative_to(output/'halo2')):dict(bytes=p.stat().st_size,sha256=sha(p)) for p in (maps/'shared.map',target)}
    provenance = dict(schema='H2_MCC_TU15_PREPARATION_V1',external_installation_requirement='Halo MCC only',
        source_files=inventory,native_shader_library=dict(maps={}),custom_authoring=lighting)
    stock_id = inventory['h2_maps_win64_dx11/shared.map']
    conversion = dict(profile=provenance['schema'],maps=[dict(map='shared.map',role='resources',
        source_sha256=stock_id['sha256'],output_sha256=stock_id['sha256'])])
    save(output/'tag-conversion.json',conversion)
    save(output/'provenance.json',provenance)
    bank_id = bank_identity(bankdir,(bank/'library.json').read_bytes())
    receipt = dict(map=relative,input_bytes=target.stat().st_size,input_sha256=sha(target),source_sha256=sha(target),
        cache_role='scenario',shared_directory='halo2/h2_maps_win64_dx11',native_mode='owned-bank',
        bank_owner_map=relative,producer='prepare_dust2_native.py',profile='H2_DUST2_NATIVE_AUTHORING_V1',
        **{k:bank_id[k] for k in ('bank_directory','bank_namespace','bank_library_sha256')})
    raw_receipt = (json.dumps(receipt,sort_keys=True)+'\n').encode()
    (output/'custom-owner-receipt.json').write_bytes(raw_receipt)
    provenance=publish_staged_owner_index(output,provenance,custom_receipts=[raw_receipt])
    save(output/'provenance.json',provenance)
    save(report,dict(schema='H2_DUST2_NATIVE_PREPARATION_V1',lighting=lighting,donors=donors,
        material_records=len(records),output=str(output),runtime_verified=False))


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for flag in ('source','imported','output','report','main-hud'):
        parser.add_argument('--'+flag,required=True,type=Path)
    parser.add_argument('--buckets',type=Path)
    parser.add_argument('--compiled-lighting',action='store_true')
    parser.add_argument('--reference-cache',type=Path)
    parser.add_argument('--output-name',default='dust2_mombasa')
    args=parser.parse_args()
    publish(**vars(args))
