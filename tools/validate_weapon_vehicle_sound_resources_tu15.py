#!/usr/bin/env python3
"""Compare native loading/PC decoding with literal weapon/vehicle sound bytes.

Offline Xbox metadata is the reference. This follows the declared dependency
closure used by the presence audit, without admitting gameplay or fabricating
sound selection. Run through tools/run_test.py; large fixtures remain private.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

from audit_weapon_vehicle_sound_presence_tu15 import GROUPS, PIN, SCHEMAS, references, schema
from inspect_multiplayer import Cache, ROOT
from original_xbox_pose_case import BASE, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output.mkdir(parents=True, exist_ok=True)
    scope = BASE / 'authored-scope.json'
    report = dict(passed=False, scope=__doc__, reference_scope_sha256=sha(scope),
                  schema_commit=PIN, schema_sha256=SCHEMAS, maps=[], source_files={},
                  tool_sha256=sha(Path(__file__)), binary_sha256=sha(args.binary),
                  runtime_event_selection_verified=False, xbox_pcm_equality_verified=False,
                  runner_scratch=str(scratch), large_artifacts_retained=[])
    with tempfile.TemporaryDirectory(prefix='weapon-sound-resources-', dir=scratch) as owned:
        work = Path(owned); fixture = work / 'fixtures.tsv'
        try:
            sounds = set(); roots = set(); total = 0
            with fixture.open('w') as output:
                for item in json.loads(scope.read_text())['maps']:
                    path = Path(item['path'])
                    if sha(path) != item['sha256']:
                        raise ValueError('Reference map changed: ' + str(path))
                    c = Cache(path)
                    try:
                        visited = set(); owned_roots = 0
                        def visit(datum):
                            if datum in visited: return
                            visited.add(datum)
                            tag = c.tags[datum]
                            if tag['group'] not in GROUPS: return
                            if tag['offset'] is None:
                                raise ValueError('Reference closure has a nonresident owner')
                            for child in references(c, schema(tag['group']), tag['offset']):
                                visit(child)
                        for datum, tag in c.tags.items():
                            if tag['group'] in ('weap', 'vehi'):
                                roots.add((tag['group'], tag['name'])); owned_roots += 1; visit(datum)
                        gestalt = [tag for tag in c.tags.values() if tag['group'] == 'ugh!' and tag['offset'] is not None]
                        if len(gestalt) != 1: raise ValueError('Ambiguous original sound gestalt')
                        g = gestalt[0]['offset']
                        ranges, permutations, chunks = c.block(g+32, 12), c.block(g+40, 16), c.block(g+64, 12)
                        count = 0; sound_tags = 0
                        for datum in sorted(visited):
                            tag = c.tags[datum]
                            if tag['group'] != 'snd!': continue
                            sound_tags += 1; sounds.add(tag['name']); s = tag['offset']
                            if s is None: raise ValueError('Nonresident original sound definition')
                            rate, encoding, codec = c.data[s+3:s+6]
                            assert rate <= 2 and encoding <= 2 and c.data[s+2] < 54
                            if encoding == 2: codec = 4
                            first_range = c.h(s+8); assert first_range >= 0
                            for pitch in range(c.data[s+10]):
                                r = ranges[first_range+pitch]; first, size = c.h(r+8), c.h(r+10)
                                assert first >= 0 and size > 0 and first+size <= len(permutations)
                                for permutation in range(size):
                                    p = permutations[first+permutation]
                                    first_chunk, chunk_count = c.h(p+12), c.h(p+14)
                                    assert first_chunk >= 0 and chunk_count > 0 and first_chunk+chunk_count <= len(chunks)
                                    output.write('S ' + json.dumps(str(path)) + ' ' + ' '.join(map(str, (
                                        f'{datum:08x}', pitch, permutation, codec,
                                        2 if encoding == 2 else encoding+1, (22050,44100,32000)[rate],
                                        c.data[s+2], c.u(p+8), chunk_count))) + '\n')
                                    for at in chunks[first_chunk:first_chunk+chunk_count]:
                                        pointer, size_flags = c.u(at), c.u(at+4)
                                        location, offset, length = pointer >> 30, pointer & 0x3fffffff, size_flags & 0x3fffffff
                                        resource = path if location == 0 else ROOT/'assets/disc/maps'/(
                                            '', 'mainmenu.map', 'shared.map', 'single_player_shared.map')[location]
                                        assert length > 0 and offset+length <= resource.stat().st_size
                                        if str(resource) not in report['source_files']:
                                            report['source_files'][str(resource)] = sha(resource)
                                        output.write('C ' + json.dumps(str(resource)) + f' {offset} {length} {pointer:08x} {size_flags:08x}\n')
                                    count += 1
                        report['maps'].append(dict(path=str(path), sha256=item['sha256'], roots=owned_roots,
                                                   sound_definitions=sound_tags, encoded_cases=count))
                        total += count
                        print(json.dumps(dict(map=path.name, sounds=sound_tags, cases=count)), flush=True)
                    finally:
                        c.data.close(); c.stream.close()
            report.update(weapon_paths=sum(group == 'weap' for group, _ in roots),
                          vehicle_paths=sum(group == 'vehi' for group, _ in roots),
                          sound_paths=len(sounds), encoded_cases=total,
                          fixture_sha256=sha(fixture), fixture_bytes=fixture.stat().st_size)
            command = [str(args.binary.resolve()), str(fixture), str(ROOT)]
            report['command'] = command
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=900)
            report.update(exit_code=result.returncode, native_output=result.stdout[-16384:])
            unchanged = sha(args.binary) == report['binary_sha256'] and all(
                sha(Path(path)) == pin for path, pin in report['source_files'].items())
            report.update(inputs_unchanged=unchanged, passed=result.returncode == 0 and unchanged and
                          len(report['maps']) == 38 and report['weapon_paths'] == 63 and report['vehicle_paths'] == 37)
        except Exception as error:
            report['error'] = str(error)
    report['fixture_scratch_removed'] = not work.exists()
    (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({key: report[key] for key in ('passed','fixture_scratch_removed')} |
                     {'native_output':report.get('native_output'), 'error':report.get('error')}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__': raise SystemExit(main())
