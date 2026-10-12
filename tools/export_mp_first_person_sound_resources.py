#!/usr/bin/env python3
"""Export literal sound resources referenced by the owned SMG FP graphs.

Authored references and encoded resources only; no activity, permutation,
RNG, listener, scheduler admission or voice success is supplied to gameplay.
The file namespace is the explicit native owned-map adapter, not a claim of
executed original Xbox file-registry initialization.
"""
import argparse
import json
from pathlib import Path
from inspect_multiplayer import Cache, MAPS, ROOT
from inventory_mp_weapon_effects import DLC, sha


def fnv(data):
    result = 0xcbf29ce484222325
    for value in data:
        result = ((result ^ value) * 0x100000001b3) & 0xffffffffffffffff
    return result


def export(output):
    paths = [ROOT / 'assets/disc/maps' / (name + '.map') for name in MAPS]
    paths += [ROOT / 'assets/multiplayer-map-pack/demos/dlc' / bank / (name + '.map')
              for bank, names in DLC.items() for name in names]
    shared = ROOT / 'assets/disc/maps'
    rows, reports = [], []
    resources = {}
    for path in paths:
        c = Cache(path)
        weapon = next(t for t in c.tags.values() if t['group'] == 'weap' and
                      t['name'] == r'objects\weapons\rifle\smg\smg')
        first_person = c.block(weapon['offset'] + 0x2a8, 0x10)
        sound_ids = set()
        references = []
        for fp in first_person:
            graph = c.u(fp + 12)
            tag = c.tags[graph]
            assert tag['group'] == 'jmad' and tag['offset'] is not None
            for index, ref in enumerate(c.block(tag['offset'] + 0x14, 12)):
                sound = c.u(ref + 4)
                references.append(dict(graph=f'{graph:08x}', reference=index,
                                       sound=f'{sound:08x}', flags=c.h(ref + 8) & 0xffff))
                if sound != 0xffffffff:
                    sound_ids.add(sound)
        gestalt = next(t for t in c.tags.values() if t['group'] == 'ugh!' and
                       t['offset'] is not None)['offset']
        ranges = c.block(gestalt + 0x20, 12)
        permutations = c.block(gestalt + 0x28, 16)
        chunks = c.block(gestalt + 0x40, 12)
        map_cases, locations = 0, set()
        for sound in sorted(sound_ids):
            t = c.tags[sound]
            assert t['group'] == 'snd!' and t['offset'] is not None
            s = t['offset']
            rate, encoding, codec = c.data[s + 3:s + 6]
            assert rate <= 2 and encoding <= 2
            if encoding == 2:
                codec = 4
            for pitch in range(c.data[s + 10]):
                r = ranges[c.h(s + 8) + pitch]
                first, count = c.h(r + 8), c.h(r + 10)
                assert first >= 0 and count > 0
                for permutation in range(count):
                    p = permutations[first + permutation]
                    first_chunk, count_chunks = c.h(p + 12), c.h(p + 14)
                    assert first_chunk >= 0 and count_chunks > 0
                    encoded = bytearray()
                    for chunk in chunks[first_chunk:first_chunk + count_chunks]:
                        pointer, size_flags = c.u(chunk), c.u(chunk + 4)
                        location, offset = pointer >> 30, pointer & 0x3fffffff
                        size = size_flags & 0x3fffffff
                        assert 0 < size <= 128 * 1024 * 1024
                        locations.add(location)
                        source = path if location == 0 else shared / (
                            '', 'mainmenu.map', 'shared.map', 'single_player_shared.map')[location]
                        key = (source, offset, size)
                        if key not in resources:
                            with source.open('rb') as f:
                                f.seek(offset)
                                resources[key] = f.read(size)
                            assert len(resources[key]) == size
                        encoded.extend(resources[key])
                    rows.append(' '.join(map(str, ('S', path.relative_to(ROOT),
                        f'{sound:08x}', pitch, permutation, codec,
                        2 if encoding == 2 else encoding + 1,
                        (22050, 44100, 32000)[rate], c.data[s + 2], c.u(p + 8), len(encoded),
                        f'{fnv(encoded):016x}'))))
                    map_cases += 1
        reports.append(dict(path=str(path.relative_to(ROOT)), sha256=sha(path),
                            graph_references=references, sound_tags=len(sound_ids),
                            encoded_cases=map_cases, resource_locations=sorted(locations)))
        c.data.close()
        c.stream.close()
        print(json.dumps(dict(map=path.stem, sound_tags=len(sound_ids),
                              encoded_cases=map_cases)), flush=True)
    output.mkdir(parents=True, exist_ok=True)
    fixture = output / 'fixtures.tsv'
    fixture.write_text('\n'.join(rows) + '\n')
    report = dict(schema='owned-mp-first-person-sound-resources-v1',
                  tool_sha256=sha(Path(__file__)), cache_reader_sha256=sha(ROOT / 'tools/inspect_multiplayer.py'),
                  fixture_sha256=sha(fixture), maps=reports, cases=len(rows),
                  resource_files={str(p.relative_to(ROOT)): sha(p) for p in
                                  sorted({x[0] for x in resources})},
                  scope=__doc__, original_sound_size_owner='21ABD0 lower30 bits')
    (output / 'report.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(maps=len(reports), cases=len(rows), report_sha256=sha(output / 'report.json'))))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', required=True, type=Path)
    export(p.parse_args().output)
