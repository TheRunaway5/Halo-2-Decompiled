#!/usr/bin/env python3
"""Remove only pinned original world-W NoContraction decorations.

This performs binary inspection/patching only. No GLSL compilation, original
instruction execution or GPU replay occurs. Every executable instruction,
descriptor/interface, clip demotion and final depth quantizer is unchanged.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/frontend/title-camera-matrix-audit/original-world-depth'
OUT = ROOT / 'analysis/frontend/title-camera-matrix-audit/world-depth-fragment-arithmetic'
XEMU = ROOT / 'analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained'
VARIANTS = ('identity', 'relaxed-kahan-only', 'relaxed-reconstruction-only', 'relaxed-world-W')
MODULES = {
    'b1982086b75332da63d573c80c99261676c4edcff9934cff9987eb76d144d54a',
    '11ff7733daf5d75efb456a2be5a6c927043932ffe29f666efc9db98413ba242a',
    'f38e87afdc85a9b8c1824e514f9d12cb2d5b1960025da229c5acbc6b1021b811'}
KAHAN_IDS = {29, 33, 44, 47}
RECONSTRUCTION_IDS = {150, 189, 193, 197, 202, 204, 205, 213, 216,
                      225, 226, 232, 233, 234, 235, 247, 249, 251, 256, 259}
RESULT_OPS = {12, 57, 61, 65, 79, 127, 129, 131, 133, 136, 148}


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def parse(raw):
    if len(raw) % 4 or len(raw) < 20:
        raise ValueError('malformed original SPIR-V size')
    words = list(struct.unpack('<' + 'I' * (len(raw) // 4), raw))
    if words[0] != 0x07230203:
        raise ValueError('not SPIR-V')
    instructions, names, definitions, decorations = [], {}, {}, []
    at, function = 5, None
    while at < len(words):
        length, opcode = words[at] >> 16, words[at] & 0xffff
        if not length or at + length > len(words):
            raise ValueError('malformed original instruction')
        operands = words[at + 1:at + length]
        if opcode == 5:
            names[operands[0]] = struct.pack('<' + 'I' * (len(operands) - 1), *operands[1:]).split(b'\0')[0].decode()
        if opcode == 54:
            function = operands[1]
        item = dict(word=at, opcode=opcode, operands=operands, function=function)
        instructions.append(item)
        if opcode in RESULT_OPS:
            definitions[operands[1]] = item
        if opcode == 71 and operands[1:] == [42]:
            if length != 3:
                raise ValueError('unexpected NoContraction instruction size')
            decorations.append(item)
        if opcode == 56:
            function = None
        at += length
    return words, instructions, names, definitions, decorations


def build_variant(raw, variant):
    if sha(raw) not in MODULES:
        raise ValueError('fragment module is outside the three pinned original shaders')
    words, instructions, names, definitions, decorations = parse(raw)
    ids = {d['operands'][0] for d in decorations}
    if len(decorations) != 24 or ids != KAHAN_IDS | RECONSTRUCTION_IDS:
        raise ValueError('original W NoContraction target set differs')
    functions = {name: id_ for id_, name in names.items()}
    kahan, main = functions['kahan_det(vf2;vf2;'], functions['main']
    if any(definitions[id_]['function'] != kahan for id_ in KAHAN_IDS):
        raise ValueError('determinant decorations escaped original kahan helper')
    if any(definitions[id_]['function'] != main for id_ in RECONSTRUCTION_IDS):
        raise ValueError('reconstruction decorations escaped original main')
    start = definitions[150]['word']
    end = next(i['word'] for i in instructions if i['opcode'] == 61 and
               i['function'] == main and names.get(i['operands'][2]) == 'vtxD0')
    if any(not start <= definitions[id_]['word'] < end for id_ in RECONSTRUCTION_IDS):
        raise ValueError('NoContraction target escaped W-only corridor before first color input')
    selected = {'identity': set(), 'relaxed-kahan-only': KAHAN_IDS,
                'relaxed-reconstruction-only': RECONSTRUCTION_IDS,
                'relaxed-world-W': ids}.get(variant)
    if selected is None:
        raise ValueError('unknown arithmetic experiment')
    output, removed = words[:5], []
    for item in instructions:
        at = item['word']
        length = words[at] >> 16
        if item['opcode'] == 71 and item['operands'][1:] == [42] and item['operands'][0] in selected:
            id_ = item['operands'][0]
            definition = definitions[id_]
            removed.append(dict(target=id_, decoration_word=at,
                definition_word=definition['word'], opcode=definition['opcode'],
                operands=definition['operands'], function=names[definition['function']]))
        else:
            output.extend(words[at:at + length])
    result = struct.pack('<' + 'I' * len(output), *output)
    _, after, _, _, after_decorations = parse(result)
    # Exact sequence proof, not just a hash of selected instruction classes.
    executable_before = [words[i['word']:i['word'] + (words[i['word']] >> 16)]
                         for i in instructions if i['opcode'] != 71]
    executable_after = [output[i['word']:i['word'] + (output[i['word']] >> 16)]
                        for i in after if i['opcode'] != 71]
    if executable_before != executable_after:
        raise ValueError('patch changed something besides a decoration')
    expected_after = [words[i['word']:i['word'] + (words[i['word']] >> 16)]
                      for i in instructions if not (i['opcode'] == 71 and
                          i['operands'][1:] == [42] and i['operands'][0] in selected)]
    if output[5:] != [word for instruction in expected_after for word in instruction]:
        raise ValueError('nonselected bytes changed')
    demotions = sum(i['opcode'] == 5380 for i in instructions)
    if demotions != 2 or sum(i['opcode'] == 5380 for i in after) != demotions:
        raise ValueError('original clip demotion changed')
    return result, dict(variant=variant, original_sha256=sha(raw), variant_sha256=sha(result),
        NoContraction_before=len(decorations), NoContraction_after=len(after_decorations),
        removed=removed, W_main_word_range=[start, end],
        executable_and_all_nonselected_bytes_identical=True, original_demotions=demotions,
        original_nextfloat_depth_quantizer_preserved=True,
        texture_color_instructions_and_all_their_decorations_preserved=True)


def source_pin():
    path = 'hw/xbox/nv2a/pgraph/glsl/psh.c'
    raw = (XEMU / path).read_bytes()
    tree = json.loads((XEMU / 'tree.json').read_text())
    entry = next(e for e in tree['tree'] if e['path'] == path)
    git_blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
    if git_blob != entry['sha'] or len(raw) != entry['size']:
        raise ValueError('pinned original Xemu source Git blob differs')
    text = raw.decode()
    for literal in ('precise float cd = a.y*b.x;', 'precise float err = fma(-a.y, b.x, cd);',
                    'precise float res = fma(a.x, b.y, -cd) + err;',
                    'precise float bc0 = area(unscaled_xy, vtxPos1.xy, vtxPos2.xy);',
                    'precise float zvalue = vtxPos0.w + (bc1*(vtxPos1.w - vtxPos0.w) + bc2*(vtxPos2.w - vtxPos0.w));'):
        if literal not in text:
            raise ValueError('original precision-producing expression differs')
    return dict(path=str((XEMU / path).relative_to(ROOT)), source_sha256=sha(raw),
        git_blob_sha1=git_blob, commit=tree['sha'],
        native_depth_helper_sha256=sha((ROOT / 'shaders/particle_depth.glsl').read_bytes()),
        original_precise_producer_lines=[920, 921, 922, 1008, 1009, 1010, 1022],
        observed_scope='All24 original NoContraction decorations belong to W reconstruction/determinant, none to texture/color arithmetic.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    source = json.loads((BASE / 'original-world-depth.json').read_text())
    if source['status'] != 'passed' or source['failures']:
        raise ValueError('invalid original extraction')
    report = dict(status='passed', scope=__doc__, source_pin=source_pin(),
        capture_sha256=source['capture_sha256'], passes=[], GPU_replay_run=False,
        production_precision_changed=False)
    for row in source['passes']:
        info = row['stages']['4']['spirv']
        raw = (BASE / info['file']).read_bytes()
        if sha(raw) != info['sha256'] or len(raw) != info['byte_size']:
            raise ValueError('original module identity differs')
        variants = {}
        for variant in VARIANTS:
            modified, proof = build_variant(raw, variant)
            name = f'event-{row["event"]}-{variant}.spv'
            (args.output / name).write_bytes(modified)
            variants[variant] = dict(file=name, bytes=len(modified), **proof)
        report['passes'].append(dict(event=row['event'], role=row['role'],
            original_resource=row['stages']['4']['shader'], variants=variants))
    (args.output / 'fragment-arithmetic-variants.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Pinned W-only decoration variants generated offline; no compilation or replay.')


if __name__ == '__main__':
    main()
