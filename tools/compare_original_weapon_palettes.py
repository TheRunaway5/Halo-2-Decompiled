#!/usr/bin/env python3
"""Compare native palette arithmetic with read-only original live observations.

Run through tools/run_test.py. Compilers, executables and working outputs stay
under HALO2_TEST_SCRATCH. Only small reproducible evidence is retained explicitly.
Original WORLD nodes are inputs; native animation/pose production is not tested.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

ROOT = Path(__file__).absolute().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from audit_rendering import Cache
from differential_source_model_palette import literal_node_map


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--parts', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    output = args.output.absolute(); output.mkdir(parents=True, exist_ok=False)
    reference = args.reference.absolute()
    pins = {}

    def pin(path):
        pins[str(path.absolute())] = sha(path)

    source = json.loads((reference / 'source.json').read_text())
    launch = json.loads((reference / 'launch.json').read_text())
    cleanup = json.loads((reference / 'cleanup.json').read_text())
    assert source['live_text_sha256'] == 'd9c6fd7e7475862b1f19c2da80d7f2f6dc41da53619991002e9546e5f9e629fe'
    assert not source['guest_code_patches'] and not source['guest_memory_patches'] and not source['guest_register_writes']
    assert all(cleanup[k] for k in ('capture_completed', 'all_processes_exited', 'original_session_unchanged', 'unnecessary_working_disk_removed'))
    assert sha(reference / 'capture-tool-source.py') == launch['tool_sha256']
    parts = json.loads(args.parts.read_text()); map_path = Path(parts['map'])
    assert sha(map_path) == parts['map_sha256']
    palettes = json.loads((reference / 'model-palettes.json').read_text())['palettes']
    poses = json.loads((reference / 'first-person-poses.json').read_text())['poses']
    assert len(palettes) == 8 and len(poses) == 4
    data = bytearray(struct.pack('<II', 0x50414c32, len(palettes)))
    cases = []
    cache = Cache(map_path)
    try:
        for ordinal, palette in enumerate(palettes):
            assert palette['entry'] == 0x13a2e0 and palette['return_pc'] == 0x4b973
            assert palette['fp'] == 1 and palette['effective_nodes_observed'] and palette['effective_nodes_equal_input']
            pose = next(p for p in poses if p['tick_before'] == p['tick_after'] == palette['tick'])
            model_pose = next(m for m in pose['models'] if m['model'] == palette['model'])
            nodes = (reference / f'palette-{ordinal}-input-nodes.bin').read_bytes()
            effective = reference / f'palette-{ordinal}-effective-nodes.bin'
            assert nodes == effective.read_bytes() == (reference / model_pose['nodes_file']).read_bytes()
            assert hashlib.sha256(nodes).hexdigest() == palette['input_nodes_sha256'] == model_pose['nodes_sha256']
            _, mode = cache.tag(palette['model'], 'mode')
            inverse = b''.join(cache.data[n+0x28:n+0x5c] for n in cache.block(mode+0x48, 96))
            node_count = palette['node_count']; assert len(nodes) == len(inverse) == node_count * 52
            regions = cache.u32(mode + 0x1c); assert regions == len(palette['selected_sections'])
            groups = cache.block(mode+0x34, 12)
            group_index = struct.unpack_from('<b', cache.data, mode+0x3c+palette['lod'])[0]
            assert 0 <= group_index < len(groups)
            compounds = cache.block(groups[group_index]+4, 16)
            compound_bytes = b''.join(cache.data[n:n+16] for n in compounds)
            data.extend(struct.pack('<7I', ordinal, palette['model'], cache.u16(mode+4), palette['lod'], node_count, regions, len(compounds)))
            data.extend(nodes); data.extend(inverse); data.extend(compound_bytes)
            sections = cache.block(mode+0x24, 92); selections = []
            for selection in palette['selected_sections']:
                if selection == 255:
                    fallback, mapping, resource = 0, [], None
                else:
                    at = sections[selection]
                    mapping, resource = literal_node_map(cache, at)
                    fallback = cache.data[at+0x24]
                    pin(ROOT / resource['file'])
                data.extend(struct.pack('<3I', selection, fallback, len(mapping))); data.extend(bytes(mapping))
                selections.append(dict(section=selection, no_map_count=fallback, node_map=mapping, resource=resource))
            original = reference / palette['palette_file']; assert sha(original) == palette['palette_sha256']
            for path in (original, effective, reference / f'palette-{ordinal}-input-nodes.bin', reference / model_pose['nodes_file']):
                pin(path)
            cases.append(dict(id=ordinal, model=palette['model'], tick=palette['tick'], node_count=node_count,
                              compound_count=len(compounds), selections=selections, original=palette['palette_file']))
    finally:
        cache.close()
    input_path = scratch / 'input.bin'; input_path.write_bytes(data)
    cpp = ROOT / 'tools/probe_original_weapon_palettes.cpp'
    production = [ROOT / 'src/source_model_palette.cpp', ROOT / 'src/transform.cpp']
    # Reuse the small retained header/source closure instead of creating another source tree.
    closure = ROOT / 'analysis/weapons-tu15-goal-20261013/needler-live-palette-native-v1'
    origins = json.loads((closure / 'build.json').read_text())['source_origins']
    for name, record in origins.items():
        assert sha(Path(name)) == sha(Path(record['snapshot'])) == record['sha256']
        pin(Path(record['snapshot']))
    initial = {str(p): sha(p) for p in [cpp, *production]}
    binary = scratch / 'palette-probe'
    command = ['c++', '-std=c++20', '-O2', '-ffp-contract=off', '-ffunction-sections', '-fdata-sections',
               '-I' + str(closure / 'source/include'), str(cpp), *map(str, production), '-Wl,--gc-sections', '-o', str(binary)]
    compile_result = subprocess.run(command, capture_output=True, text=True)
    (output / 'compile.log').write_text((compile_result.stdout + compile_result.stderr)[-65536:])
    assert compile_result.returncode == 0, compile_result.stderr[-2000:]
    native_output = scratch / 'native-output.bin'
    run = subprocess.run([str(binary), str(input_path), str(native_output)], capture_output=True, text=True)
    (output / 'run.log').write_text((run.stdout + run.stderr)[-65536:]); assert run.returncode == 0, run.stderr
    result = native_output.read_bytes(); magic, count = struct.unpack_from('<II', result)
    assert magic == 0x50414c32 and count == len(cases)
    offset = 8; compared_words = 0; rows = []
    for case in cases:
        ordinal, model, base, region_count, matrices = struct.unpack_from('<5I', result, offset); offset += 20
        assert (ordinal, model) == (case['id'], case['model'])
        ranges = [list(struct.unpack_from('<II', result, offset + i*8)) for i in range(region_count)]; offset += region_count*8
        raw = result[offset:offset+matrices*48]; offset += len(raw)
        original = (reference / case['original']).read_bytes()
        expected_base, expected_regions = struct.unpack_from('<HH', original)
        expected_ranges = [list(struct.unpack_from('<HH', original, 4+i*4)) for i in range(expected_regions)]
        assert (base, region_count, ranges) == (expected_base, expected_regions, expected_ranges)
        assert len(original) == 0x44 + matrices*48
        expected = original[0x44:]
        mismatches = [i for i,(a,b) in enumerate(zip(struct.unpack('<'+'I'*(len(raw)//4), raw), struct.unpack('<'+'I'*(len(expected)//4), expected))) if a != b]
        row = dict(**case, base_count=base, region_ranges=ranges, matrix_count=matrices,
                   compared_float_words=len(raw)//4, mismatch_count=len(mismatches), first_mismatches=mismatches[:12],
                   native_palette_sha256=hashlib.sha256(raw).hexdigest(), original_palette_sha256=hashlib.sha256(expected).hexdigest())
        rows.append(row); compared_words += len(raw)//4
    assert offset == len(result)
    for name, digest in initial.items():
        assert sha(Path(name)) == digest
    for name, record in origins.items():
        assert sha(Path(name)) == record['sha256']
    for name in ('source.json','launch.json','cleanup.json','first-person-poses.json','model-palettes.json','capture-tool-source.py'):
        pin(reference / name)
    pin(args.parts); pin(map_path)
    shutil.copyfile(cpp, output / 'probe-source.cpp')
    shutil.copyfile(Path(__file__), output / 'comparison-source.py')
    shutil.copyfile(input_path, output / 'input.bin'); shutil.copyfile(native_output, output / 'native-output.bin')
    for path in output.iterdir():
        if path.is_file(): pin(path)
    report = dict(passed=not any(r['mismatch_count'] for r in rows),
        timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        scope=__doc__, original_reference=str(reference), original_full_live_text_authenticated=True,
        original_WORLD_nodes_used_as_inputs=True, actual_original_returned_palettes_compared=True,
        native_entry='halo2::source_model_palette', cases=rows, compared_float_words=compared_words,
        all_defined_region_header_fields_compared=True, unused_header_slots_excluded=True,
        production_source_sha256=initial, compile_command=command, native_binary_sha256=sha(binary),
        linked_files_sha256=pins, runner_scratch=str(scratch),
        retained_artifacts=[dict(path=str(p), bytes=p.stat().st_size, reason='Small reproducible palette comparison evidence') for p in output.iterdir() if p.is_file()],
        native_pose_production_verified=False, native_framebuffer_compared=False, full_weapon_parity=False)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], cases=len(rows), compared_float_words=compared_words,
                         mismatches=sum(r['mismatch_count'] for r in rows), report=str(output / 'report.json'))), flush=True)
    assert report['passed'], rows


if __name__ == '__main__':
    main()
