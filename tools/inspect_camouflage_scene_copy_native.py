#!/usr/bin/env python3
"""Static native-water comparison against frozen whole original scene-copy output.

This captures the inspected native source and compares literal program words
and the documented tap expression. It does not execute the native CPU helper,
compile GLSL, run a GPU, or claim water framebuffer behavior.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/native-water-comparison.json'
ORIGINAL = ROOT / 'analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json'
ORIGINAL_PIN = 'e96e09b2c9a0e41e179e98d587c310b353583e59a74919eacdf315ecff6309f8'


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def number(word):
    return struct.unpack('<f', struct.pack('<I', word))[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    args = ap.parse_args()
    assert sha(ORIGINAL.read_bytes()) == ORIGINAL_PIN
    original = json.loads(ORIGINAL.read_text())
    files = {name: (ROOT / name).read_text() for name in ('include/halo2/water.hpp', 'src/water.cpp', 'shaders/water_copy.frag', 'src/renderer.cpp')}
    native = files['src/water.cpp']
    program = native[native.index('PixelProgram water_copy_program()'):native.index('std::array<ShaderVector,4> water_copy_taps')]
    taps = native[native.index('std::array<ShaderVector,4> water_copy_taps'):native.index('WaterFrame water_frame')]
    values = [0] * 60
    for index, value in re.findall(r'w\[(\d+)\]\s*=\s*(0x[0-9a-fA-F]+)', program):
        values[int(index)] = int(value, 16)
    raw_program = next(event['program_payload_hex'] for event in original['result']['events'] if event['entry'] == 0x3fa3d0)
    actual = list(struct.unpack('<60I', bytes.fromhex(raw_program)))
    assert values == actual
    assert '((i&1)? .25f:-.25f)/width' in taps and '((i&2)? .25f:-.25f)/height' in taps
    source = original['resources'][0]
    destination = original['resources'][-1]
    actual_taps = [event for event in original['result']['events'] if event['entry'] == 0x34100 and event['corner_bits'] == [0, 0] and 1 <= event['attribute'] <= 4]
    actual_taps.sort(key=lambda event: event['attribute'])
    projection = []
    for i, event in enumerate(actual_taps):
        observed = event['output_after_bits'][:2]
        actual_uv = [bits(number(observed[0]) / source['width']), bits(number(observed[1]) / source['height'])]
        expression_uv = [bits((.25 if i & 1 else -.25) / source['width']), bits((.25 if i & 2 else -.25) / source['height'])]
        projection.append(dict(attribute=i + 1, actual_source_pixel_xy_bits=observed,
                               actual_normalized_projection_bits=actual_uv,
                               native_expression_with_documented_source_dimensions_bits=expression_uv,
                               same=False))
        assert actual_uv != expression_uv
    renderer = files['src/renderer.cpp']
    start = renderer.index('if(water_copy_pass)for(')
    uniforms = renderer[start:renderer.index('measure(frame_timing.view_constants_ms)', start)]
    start = renderer.index('if(water_copy_pass && !v.water.empty())')
    execution = renderer[start:renderer.index('vkCmdBindVertexBuffers', start)]
    result = dict(schema='static-native-water-scene-copy-comparison-v1', boundary=__doc__,
                  original_report_sha256=ORIGINAL_PIN,
                  literal_program_words=values, original_program_equal=True,
                  native_files={name: dict(sha256=sha((ROOT / name).read_bytes()), text=text) for name, text in files.items() if name != 'src/renderer.cpp'},
                  native_renderer=dict(sha256=sha((ROOT / 'src/renderer.cpp').read_bytes()), uniform_source=uniforms, execution_source=execution),
                  native_program_function=program, native_tap_function=taps,
                  captured_source_dimensions=[source['width'], source['height']],
                  captured_destination_dimensions=[destination['width'], destination['height']],
                  normalized_tap_projections=projection,
                  limits=['Literal source comparison and declared nearest binary32 arithmetic projection only; no compiled native helper or GPU execution.',
                          'The observed original pixel coordinates belong to ready target19 with full captured camera rectangle; other rectangles/destinations need separate execution.',
                          'The water shader fixes destination512x256. The inspected renderer derives taps from host source extent; neither is a complete generic352E0/34A90 rectangle contract.'])
    args.output.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(program_words=60, equal=True, differing_taps=len(projection), output_sha256=sha(args.output.read_bytes())), indent=2))


if __name__ == '__main__':
    main()
