#!/usr/bin/env python3
"""Audit every sound resource in an explicit, manifest-verified DLC map bank.

This reads owned cache metadata and original bytes. Map selection/activation,
sound events, RNG/selection, listeners, source voices and playback are separate.
The explicit PC file namespace is not executed Xbox file-registry startup.
"""
import argparse
import json
from pathlib import Path
from inspect_multiplayer import Cache, ROOT
from inventory_mp_weapon_effects import sha


def label(path):
    path = path.resolve()
    return str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)


def export(bank, manifest_path, shared, output):
    bank, manifest_path, shared = bank.resolve(), manifest_path.resolve(), shared.resolve()
    manifest = json.loads(manifest_path.read_text())
    storage = Path(manifest['storage_root']).resolve()
    entries = {item['path']: item for item in manifest['files']}
    paths = sorted(bank.glob('*.map'))
    assert paths
    rows, maps, sources = [], [], set()
    for path in paths:
        relative = str(path.relative_to(storage))
        assert relative in entries and entries[relative]['sha256'] == sha(path)
        assert entries[relative]['size'] == path.stat().st_size
        c = Cache(path)
        g = next(t['offset'] for t in c.tags.values() if t['group'] == 'ugh!' and t['offset'] is not None)
        ranges, permutations, chunks = c.block(g + 32, 12), c.block(g + 40, 16), c.block(g + 64, 12)
        cases, sounds, byte_count, locations = 0, 0, 0, set()
        for datum, tag in sorted(c.tags.items()):
            if tag['group'] != 'snd!' or tag['offset'] is None:
                continue
            sounds += 1
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
                    assert first_chunk >= 0 and count_chunks > 0 and first_chunk + count_chunks <= len(chunks)
                    rows.append('S ' + json.dumps(label(path), ensure_ascii=False) + ' ' + ' '.join(map(str, (
                        f'{datum:08x}', pitch, permutation, codec, 2 if encoding == 2 else encoding + 1,
                        (22050, 44100, 32000)[rate], c.data[s + 2], c.u(p + 8), count_chunks))))
                    for at in chunks[first_chunk:first_chunk + count_chunks]:
                        pointer, size_flags = c.u(at), c.u(at + 4)
                        location, offset = pointer >> 30, pointer & 0x3fffffff
                        size = size_flags & 0x3fffffff
                        source = path if location == 0 else shared / (
                            '', 'mainmenu.map', 'shared.map', 'single_player_shared.map')[location]
                        assert size > 0 and offset + size <= source.stat().st_size
                        rows.append('C ' + json.dumps(label(source), ensure_ascii=False) + ' ' +
                                    f'{offset} {size} {pointer:08x} {size_flags:08x}')
                        sources.add(source)
                        locations.add(location)
                        byte_count += size
                    cases += 1
        maps.append(dict(path=label(path), sha256=sha(path), imported_entry=relative,
                         header_name=c.text(408), resident_sound_tags=sounds,
                         encoded_cases=cases, encoded_case_bytes=byte_count,
                         resource_locations=sorted(locations)))
        c.data.close()
        c.stream.close()
        print(json.dumps(dict(map=path.stem, sounds=sounds, cases=cases)), flush=True)
    output.mkdir(parents=True, exist_ok=True)
    fixture = output / 'fixtures.tsv'
    fixture.write_text('\n'.join(rows) + '\n')
    report = dict(schema='owned-explicit-dlc-sound-resources-v1', scope=__doc__,
                  tool_sha256=sha(Path(__file__)), cache_reader_sha256=sha(ROOT / 'tools/inspect_multiplayer.py'),
                  import_manifest=dict(path=label(manifest_path), sha256=sha(manifest_path),
                                       source_archive=manifest['source'], source_sha256=manifest['source_sha256']),
                  fixture_sha256=sha(fixture), maps=maps, cases=sum(item['encoded_cases'] for item in maps),
                  resource_files={label(path): sha(path) for path in sorted(sources)})
    (output / 'report.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(maps=len(maps), cases=report['cases'], fixture_sha256=sha(fixture),
                         report_sha256=sha(output / 'report.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--content-bank', type=Path, required=True)
    parser.add_argument('--import-manifest', type=Path, required=True)
    parser.add_argument('--shared-maps', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    export(args.content_bank, args.import_manifest, args.shared_maps, args.output)
