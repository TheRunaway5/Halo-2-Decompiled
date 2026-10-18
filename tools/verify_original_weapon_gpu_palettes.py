#!/usr/bin/env python3
"""Join consumed original GPU palette words to live CPU and native palettes.

Byte equality identifies the observed palette input, not a complete GPU frame
boundary, native pose producer, shader arithmetic result or framebuffer parity.
Run through tools/run_test.py; only this small comparison report is retained.
"""
import argparse
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
from original_weapon_gpu_evidence import load_json


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--geometry', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--inspection', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    pins = {}

    def pin(path):
        pins[str(Path(path).absolute())] = sha(path)

    geometry = load_json(args.geometry, pins); native = load_json(args.native, pins)
    assert geometry['passed'] and native['passed']
    assert geometry['geometry_owner_verified_from_ordered_indices_and_all_consumed_vertex_bytes']
    for report in (geometry, native):
        for name, digest in report['linked_files_sha256'].items():
            assert sha(name) == digest, name
            pins[name] = digest
    inspection = args.inspection.absolute()
    gpu = load_json(inspection / 'report.json', pins)
    assert gpu['passed'] and geometry['original_inspection_sha256'] == sha(inspection / 'report.json')
    native_bytes = (args.native.parent / 'native-output.bin').read_bytes()
    assert sha(args.native.parent / 'native-output.bin') == native['linked_files_sha256'][str((args.native.parent / 'native-output.bin').absolute())]
    magic, count = struct.unpack_from('<II', native_bytes); assert magic == 0x50414c32 and count == len(native['cases'])
    offset, palettes = 8, {}
    for case in native['cases']:
        ordinal, model, base, regions, matrices = struct.unpack_from('<5I', native_bytes, offset); offset += 20
        ranges = [struct.unpack_from('<II', native_bytes, offset+i*8) for i in range(regions)]; offset += regions*8
        raw = native_bytes[offset:offset+matrices*48]; offset += len(raw)
        assert (ordinal, model, base, list(map(list, ranges)), matrices) == (case['id'], case['model'], case['base_count'], case['region_ranges'], case['matrix_count'])
        assert hashlib.sha256(raw).hexdigest() == case['native_palette_sha256'] == case['original_palette_sha256']
        palettes[ordinal] = (case, raw)
    assert offset == len(native_bytes)
    rows = []
    for owner in geometry['geometry_owners']:
        event = load_json(inspection / f"event-{owner['event']}.json", pins)
        assert event['action']['baseVertex'] == 0
        identity = event['index_only_candidates']; assert len(identity) == 1
        assert (identity[0]['model'], identity[0]['section'], identity[0]['part']) == (owner['model'], owner['section'], owner['part'])
        vertex = event['stages']['0']
        program = inspection / vertex['source']['file']; pin(program)
        assert sha(program) == vertex['source']['sha256']
        shader = program.read_text()
        constants = next(value for block in vertex['blocks'] for value in block['values'] if value['name'] == 'c')['members']
        reflected = next(variable for block in vertex['blocks'] for variable in block['block']['variables'] if variable['name']=='c')
        assert len(constants) == reflected['type']['elements']
        assert [c['name'] for c in constants] == [f'c[{i}]' for i in range(len(constants))]
        indirect = 'c[A0+50]' in shader
        if indirect:
            assert all(f'c[A0+{i}]' in shader for i in (50,51,52))
            assert 'return int(floor(src + 0.001));' in shader
            attribute = next(a for a in event['vertex_inputs'] if a['name']=='v1' and a['used'])
            fmt = attribute['format']
            assert not attribute['genericEnabled'] and (fmt['type'],fmt['compByteWidth'],fmt['compType']) == (0,1,2)
            assert fmt['compCount'] in (1,2,3)
            address_producers = re.findall(r'MUL\((R\d+),([xyzw]+), v1(?:\.([xyzw]+))?, c\[7\]\.w\);', shader)
            assert len(address_producers) == 1
            address_register, mask, swizzle = address_producers[0]
            swizzle = swizzle or 'xyzw'
            if len(swizzle) == 1: swizzle *= 4
            selected_components = re.findall(r'ARL\((?:A0|_temp_addr), '+address_register+r'\.([xyzw])\);', shader)
            assert selected_components and set(selected_components).issubset(set(mask))
            consumed_components = {'xyzw'.index(swizzle['xyzw'.index(component)]) for component in selected_components}
            assert consumed_components == set(range(fmt['compCount']))
            bank = next(v for v in event['vertex_data'] if v['slot'] == attribute['vertexBuffer'])
            data_path = inspection / bank['data']['file']; pin(data_path)
            data = data_path.read_bytes(); assert sha(data_path) == bank['data']['sha256']
            index_path = inspection / event['indices']['file']; pin(index_path)
            raw_indices = index_path.read_bytes(); assert sha(index_path) == event['indices']['sha256']
            stride = event['index_buffer']['byteStride']; assert stride in (2,4)
            indices = struct.unpack('<'+('H' if stride==2 else 'I')*(len(raw_indices)//stride), raw_indices)
            bones = sorted({data[i*bank['binding']['byteStride']+attribute['byteOffset']+j]
                            for i in indices for j in consumed_components})
            scale = constants[7]['f32'][3]
            addresses = {bone:math.floor(f32(f32(f32(bone/255)*scale)+f32(.001))) for bone in bones}
            assert all(address == bone*3 for bone,address in addresses.items())
        else:
            assert all(f'c[{i}]' in shader for i in (50,51,52))
            assert 'c[A0+' not in shader
            bones, addresses, scale = [0], {0:0}, None
        gpu_words = []
        for bone in bones:
            for row in range(3):
                assert 50+addresses[bone]+row < len(constants)
                gpu_words.extend(int(word,16) for word in constants[50+addresses[bone]+row]['u32'])
        comparisons = []
        for ordinal, (case, raw) in palettes.items():
            if case['model'] != owner['model']:
                continue
            regions = [i for i,r in enumerate(case['selections']) if r['section'] == owner['section']]
            assert len(regions) == 1
            first, size = case['region_ranges'][regions[0]]
            assert all(bone < size for bone in bones)
            expected = []
            for bone in bones:
                expected.extend(struct.unpack_from('<12I', raw, (first+bone)*48))
            assert len(expected) == len(gpu_words)
            mismatch = sum(a!=b for a,b in zip(expected,gpu_words))
            comparisons.append(dict(case=ordinal, tick=case['tick'], mismatched_float_words=mismatch))
        matched = [c for c in comparisons if c['mismatched_float_words']==0]
        assert len(matched) == 1, (owner['event'], comparisons)
        rows.append(dict(event=owner['event'], model=owner['model'], section=owner['section'], part=owner['part'],
            consumed_local_nodes=bones, indirect_palette=indirect, address_scale=scale,
            compared_float_words=len(gpu_words), matching_original_CPU_and_native_palette=matched[0],
            other_observed_pose_comparisons=comparisons, shader_sha256=vertex['source']['sha256']))
    assert len(rows) == 11 and {r['matching_original_CPU_and_native_palette']['tick'] for r in rows} == {434,436}
    source_folder = args.output.with_name(args.output.stem + '-sources')
    source_folder.mkdir(exist_ok=True)
    for source_path in (Path(__file__), Path(__file__).with_name('original_weapon_gpu_evidence.py')):
        destination = source_folder / source_path.name
        if destination.exists(): assert sha(destination) == sha(source_path)
        else: shutil.copyfile(source_path, destination)
        pin(destination)
    report = dict(passed=True, timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        scope=__doc__, verified_draws=len(rows), compared_float_words=sum(r['compared_float_words'] for r in rows),
        all_consumed_palette_words_bit_equal=True, uniquely_matching_observed_pose_for_each_draw=True,
        original_CPU_and_native_palette_comparison_sha256=sha(args.native), original_geometry_owner_comparison_sha256=sha(args.geometry),
        draws=rows, linked_files_sha256=pins, tool_sha256=sha(Path(__file__)),
        original_GPU_full_frame_boundary_verified=False, native_pose_production_verified=False,
        native_shader_arithmetic_verified=False, native_framebuffer_parity=False,
        physical_xbox_verified=False, full_weapon_parity=False, runner_scratch=os.environ['HALO2_TEST_SCRATCH'])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, draws=len(rows), compared_float_words=report['compared_float_words'], report=str(args.output.absolute()))), flush=True)


if __name__ == '__main__':
    main()
