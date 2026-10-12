#!/usr/bin/env python3
"""Export every authored multiplayer sound resource without selecting gameplay.

Literal snd!/ugh! fields and resource descriptors are independent of the native
loader. The owned-map file namespace is an explicit PC adapter, not execution
of original Xbox file-registry initialization. No sound request, random state,
listener, voice, admission, timing or playback result is supplied.
"""
import argparse
import json
from pathlib import Path
from inspect_multiplayer import Cache, MAPS, ROOT
from inventory_mp_weapon_effects import DLC, sha


def export(output):
    paths = [ROOT / 'assets/disc/maps' / (name + '.map') for name in MAPS]
    paths += [ROOT / 'assets/multiplayer-map-pack/demos/dlc' / bank / (name + '.map')
              for bank, names in DLC.items() for name in names]
    shared = ROOT / 'assets/disc/maps'
    sources = set()
    rows, maps, total = [], [], 0
    for path in paths:
        c = Cache(path)
        g = next(t['offset'] for t in c.tags.values() if t['group'] == 'ugh!' and
                 t['offset'] is not None)
        ranges = c.block(g + 0x20, 12)
        permutations = c.block(g + 0x28, 16)
        chunks = c.block(g + 0x40, 12)
        sound_tags, cases, byte_count, locations = 0, 0, 0, set()
        for datum, tag in sorted(c.tags.items()):
            if tag['group'] != 'snd!' or tag['offset'] is None:
                continue
            sound_tags += 1
            s = tag['offset']
            rate, encoding, codec = c.data[s + 3:s + 6]
            assert rate <= 2 and encoding <= 2 and c.data[s + 2] < 54
            if encoding == 2:
                codec = 4
            first_range = c.h(s + 8)
            assert first_range >= 0
            for pitch in range(c.data[s + 10]):
                r = ranges[first_range + pitch]
                first, count = c.h(r + 8), c.h(r + 10)
                assert first >= 0 and count > 0 and first + count <= len(permutations)
                for permutation in range(count):
                    p = permutations[first + permutation]
                    first_chunk, count_chunks = c.h(p + 12), c.h(p + 14)
                    assert first_chunk >= 0 and count_chunks > 0
                    assert first_chunk + count_chunks <= len(chunks)
                    rows.append(' '.join(map(str, ('S', path.relative_to(ROOT),
                        f'{datum:08x}', pitch, permutation, codec,
                        2 if encoding == 2 else encoding + 1,
                        (22050, 44100, 32000)[rate], c.data[s + 2], c.u(p + 8),
                        count_chunks))))
                    for at in chunks[first_chunk:first_chunk + count_chunks]:
                        pointer, size_flags = c.u(at), c.u(at + 4)
                        location, offset = pointer >> 30, pointer & 0x3fffffff
                        size = size_flags & 0x3fffffff
                        source = path if location == 0 else shared / (
                            '', 'mainmenu.map', 'shared.map', 'single_player_shared.map')[location]
                        assert size > 0 and offset + size <= source.stat().st_size
                        sources.add(source)
                        locations.add(location)
                        byte_count += size
                        rows.append(' '.join(map(str, ('C', source.relative_to(ROOT),
                            offset, size, f'{pointer:08x}', f'{size_flags:08x}'))))
                    cases += 1
        maps.append(dict(path=str(path.relative_to(ROOT)), sha256=sha(path),
                         resident_sound_tags=sound_tags, encoded_cases=cases,
                         resource_locations=sorted(locations), encoded_case_bytes=byte_count))
        total += cases
        c.data.close()
        c.stream.close()
        print(json.dumps(dict(map=path.stem, sounds=sound_tags, cases=cases)), flush=True)
    output.mkdir(parents=True, exist_ok=True)
    fixture = output / 'fixtures.tsv'
    fixture.write_text('\n'.join(rows) + '\n')
    report = dict(schema='owned-mp-all-sound-resources-v1', scope=__doc__,
                  tool_sha256=sha(Path(__file__)),
                  cache_reader_sha256=sha(ROOT / 'tools/inspect_multiplayer.py'),
                  fixture_sha256=sha(fixture), maps=maps, cases=total,
                  resource_files={str(p.relative_to(ROOT)): sha(p) for p in sorted(sources)},
                  original_sound_size_owner='21ABD0/21F4D0 lower30 bits')
    (output / 'report.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(maps=len(maps), cases=total,
                         fixture_sha256=sha(fixture), report_sha256=sha(output / 'report.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    export(parser.parse_args().output)
