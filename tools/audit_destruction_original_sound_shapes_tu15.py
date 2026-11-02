#!/usr/bin/env python3
"""Inspect original destruction sound shapes before native bank preparation.

Read-only original-map metadata, not runtime input or a playback result. This
checks the current prepared bank loader's literal bounds and identifies default
operands/extra owners which a native importer must retain explicitly.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from inspect_multiplayer import Cache


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory', type=Path, required=True)
    parser.add_argument('--reference-scope', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    inventory = json.loads(args.inventory.read_text())
    scope = json.loads(args.reference_scope.read_text())
    if not inventory.get('passed') or inventory.get('vehicle_paths') != 37:
        raise ValueError('Require the complete original vehicle destruction inventory')
    names = {name for owner in inventory['owners'] for name in owner['sounds']}
    maps = {row['name']: row['reference_sha256'] for row in inventory['maps']}
    inputs = {str(path.resolve()): sha(path) for path in (args.inventory, args.reference_scope,
        Path(__file__), Path(__file__).with_name('inspect_multiplayer.py'))}
    banks = {name: {} for name in sorted(names)}
    for row in scope['maps']:
        path = Path(row['path'])
        if path.name not in maps:
            continue
        digest = sha(path)
        if digest != maps[path.name] or digest != row['sha256']:
            raise ValueError('Original metadata input changed: ' + str(path))
        inputs[str(path)] = digest
        cache = Cache(path)
        try:
            roots = [tag for tag in cache.tags.values() if tag['group'] == 'snd!' and
                tag['name'] in names and tag['offset'] is not None]
            if not roots:
                continue
            gestalts = [tag for tag in cache.tags.values() if tag['group'] == 'ugh!' and tag['offset'] is not None]
            if len(gestalts) != 1:
                raise ValueError('Ambiguous original sound gestalt')
            g = gestalts[0]['offset']
            ranges, permutations = cache.block(g + 32, 12), cache.block(g + 40, 16)
            playbacks, scales = cache.block(g, 56), cache.block(g + 8, 20)
            intervals, flags = cache.block(g + 24, 10), cache.block(g + 56, 1)
            for tag in roots:
                at = tag['offset']; literal = bytes(cache.data[at:at + 20])
                unsigned = lambda offset: int.from_bytes(literal[offset:offset + 2], 'little')
                pitch_count = literal[10]
                first_range = unsigned(8)
                if first_range + pitch_count > len(ranges):
                    raise ValueError('Original sound pitch interval escapes its owner')
                pitches = []
                for r in ranges[first_range:first_range + pitch_count]:
                    first, count = cache.h(r + 8), cache.h(r + 10)
                    if first < 0 or count < 0 or first + count > len(permutations):
                        raise ValueError('Original sound permutation interval escapes its owner')
                    interval, bit = cache.h(r + 2), int.from_bytes(cache.data[r + 6:r + 8], 'little')
                    begin, size = bit >> 3, (count + 7) // 8
                    pitches.append(dict(permutations=count,
                        explicit_interval=0 <= interval < len(intervals),
                        interval_hex=bytes(cache.data[intervals[interval]:intervals[interval] + 10]).hex()
                            if 0 <= interval < len(intervals) else None,
                        used_bit_offset=bit,
                        used_bytes_hex=bytes(cache.data[flags[begin]:flags[begin] + size]).hex()
                            if size and begin + size <= len(flags) else None))
                playback, scale = unsigned(6), literal[11]
                shape = dict(flags=unsigned(0), sound_class=literal[2], codec=list(literal[3:6]),
                    maximum_play_time=int.from_bytes(literal[16:20], 'little'), pitch_count=pitch_count,
                    playback_is_explicit=playback != 0xffff and playback < len(playbacks),
                    scale_is_explicit=scale != 0xff and scale < len(scales),
                    playback_hex=bytes(cache.data[playbacks[playback]:playbacks[playback] + 56]).hex()
                        if playback != 0xffff and playback < len(playbacks) else None,
                    scale_hex=bytes(cache.data[scales[scale]:scales[scale] + 20]).hex()
                        if scale != 0xff and scale < len(scales) else None,
                    promotion=literal[12], custom_playback=literal[13], extra_info=unsigned(14), pitches=pitches)
                shape['v2_literal_bounds_satisfied'] = (pitch_count == 1 and literal[12:14] == b'\xff\xff' and
                    unsigned(14) == 0xffff and literal[2] < 54 and literal[3] < 3 and literal[4] < 3 and
                    literal[5] <= 4 and all(1 <= p['permutations'] <= 32 and p['used_bytes_hex'] is not None for p in pitches))
                signature = hashlib.sha256(json.dumps(shape, sort_keys=True).encode()).hexdigest()
                entry = banks[tag['name']].setdefault(signature, dict(shape=shape, maps=[]))
                entry['maps'].append(path.name)
        finally:
            cache.data.close(); cache.stream.close()
    missing = [name for name, variants in banks.items() if not variants]
    if any(sha(path) != digest for path, digest in inputs.items()):
        raise ValueError('Original sound shape input changed during inspection')
    report = dict(passed=not missing, scope=__doc__, inputs=inputs, banks=banks, missing=missing,
        inputs_unchanged=True,
        sound_banks=len(banks), metadata_shapes=sum(len(v) for v in banks.values()),
        v2_literal_bounds_failures=[name for name, variants in banks.items()
            if any(not v['shape']['v2_literal_bounds_satisfied'] for v in variants.values())],
        default_operand_research_required=[name for name, variants in banks.items()
            if any(not v['shape']['playback_is_explicit'] or not v['shape']['scale_is_explicit'] or
                any(not p['explicit_interval'] for p in v['shape']['pitches']) for v in variants.values())],
        complete_import_contract_or_payload_verified=False, runtime_binding_verified=False,
        original_waveform_or_device_verified=False, full_goal_complete=False, retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    text = json.dumps(report, indent=2) + '\n'
    if len(text.encode()) > 1024 * 1024:
        raise ValueError('Original sound shape report exceeds its bounded budget')
    args.output.write_text(text)
    print(json.dumps({key: report[key] for key in ('passed', 'sound_banks', 'metadata_shapes',
        'v2_literal_bounds_failures', 'default_operand_research_required', 'missing')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
