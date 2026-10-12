#!/usr/bin/env python3
"""Verify original preparation gates, owned phase transitions and native correction."""
from collections import Counter
import json
from pathlib import Path

from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import Xbe, _Code, literal, complete_tokens
from oracle_owned_round_phase import PINS
from oracle_round_preparation import ENTRIES
from sanitize_round_lifecycle import no_active_build
from verify_round_reset import expanded
from verify_round_reset_scenario import sha

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'analysis/multiplayer/game-variants/round-preparation'
OWNED = ROOT/'analysis/multiplayer/game-variants/owned-round-phase'


def read(path):
    data = json.loads(path.read_text())
    for digest, raw in data['raw_blobs'].items():
        import hashlib
        assert hashlib.sha256(bytes.fromhex(raw)).hexdigest() == digest
    return data, expanded(data, data)


def verify_listing(path, image, code):
    rows = path.read_text().splitlines()
    for row in rows:
        address, raw, assembly = row.split(' ', 2)
        address, raw = int(address, 16), bytes.fromhex(raw)
        instruction = code.at(address)
        assert instruction.bytes == image.read(address, len(raw)) == raw
        assert (instruction.mnemonic+' '+instruction.op_str).strip() == assembly.strip()
    return len(rows)


def main():
    no_active_build(ROOT/'build')
    paths = set()
    source_pairs = []
    data, preparation = read(OUT/'report.json')
    assert (OUT/'report.json').read_bytes() == (OUT/'reproduced/report.json').read_bytes()
    assert (OUT/'report.json').read_bytes() == (ROOT/'tests/fixtures/round_preparation_original.json').read_bytes()
    assert data['oracle_sha256'] == sha(ROOT/'tools/oracle_round_preparation.py')
    assert preparation['instruction_patches'] == preparation['supplied_returns'] == []
    for path, pin in data['source_dependencies'].items():
        assert sha(ROOT/path) == pin, path
        paths.add(path)
    wrappers, phases, stopped_phases, listing_rows = 0, 0, 0, 0
    for name, version in preparation['versions'].items():
        spec = SPECS[name]
        assert version['source_sha256'] == sha(ROOT/spec['path']) == spec['pin']
        image, code = Xbe(spec['path']), _Code(Xbe(spec['path']), '.text')
        paths.update((spec['path'], spec['tsv']))
        starts = {int(row.split('\t')[0], 16)for row in (ROOT/spec['tsv']).read_text().splitlines()if row[:1].isdigit()}
        source_pairs.append((name, image, literal(code, ENTRIES[name][0], starts)[1]))
        assert len(version['wrappers']) == 96 and len(version['phases']) == 12
        for record in version['wrappers']:
            assert record['before'] == record['after']
            stop = record['engine_present'] and record['entity'] != 0xffffffff
            assert (record['stop'] is not None) == stop
            assert record['completion'] == ('stopped_before_dirty_entity_owner' if stop else 'complete_original_return')
            if stop:
                assert record['stop']['instruction'] == f'{ENTRIES[name][1]:08x}'
                assert record['stop']['arguments'] == [record['entity'], record['flags']]
            wrappers += 1
        for record in version['phases']:
            before = bytes.fromhex(record['before']['engine_hex'])
            after = bytes.fromhex(record['after']['engine_hex'])
            assert len(before) == len(after) == 0xc18
            expected = bytearray(before)
            index = (record['index']+1)&65535
            expected[0x6e:0x70] = index.to_bytes(2, 'little')
            if record['entity'] == 0xffffffff:
                assert record['stop'] is None and record['completion'] == 'complete_original_return'
                expected[0x6c:0x6e] = (3).to_bytes(2, 'little')
                expected[0xc00:0xc04] = record['rate'].to_bytes(4, 'little', signed=True)
            else:
                assert record['stop']['instruction'] == f'{ENTRIES[name][1]:08x}'
                assert record['stop']['arguments'] == [record['entity'], 8]
                assert record['completion'] == 'stopped_before_dirty_entity_owner'
                stopped_phases += 1
            assert bytes(expected) == after
            assert record['before']['clock_hex'] == record['after']['clock_hex']
            phases += 1
        for listing in data['instruction_listings']:
            if not listing['file'].startswith(name+'-'):
                continue
            path = OUT/'instructions'/listing['file']
            assert sha(path) == listing['sha256']
            assert path.read_bytes() == (OUT/'reproduced/instructions'/listing['file']).read_bytes()
            assert verify_listing(path, image, code) == listing['rows']
            listing_rows += listing['rows']
            paths.update((str(path.relative_to(ROOT)), str((OUT/'reproduced/instructions'/listing['file']).relative_to(ROOT))))
    later = next(row for row in source_pairs if row[0] == 'u5')
    for name, image, instructions in source_pairs:
        if name != 'u5':
            assert complete_tokens(instructions, image) == complete_tokens(later[2], later[1])
    owned_data, owned = read(OWNED/'report.json')
    assert (OWNED/'report.json').read_bytes() == (OWNED/'reproduced/report.json').read_bytes()
    assert owned_data['oracle_sha256'] == sha(ROOT/'tools/oracle_owned_round_phase.py')
    assert owned_data['prior_pins'] == PINS
    for path, pin in (owned_data['source_dependencies']|PINS).items():
        assert sha(ROOT/path) == pin, path
        paths.add(path)
    assert owned['source_sha256'] == SPECS['tu11']['pin']
    assert owned['cache_sha256'] == sha(ROOT/'assets/disc/maps/lockout.map')
    assert owned['generator_sha256'] == sha(ROOT/'analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/generator.exe')
    _, base = read(ROOT/'analysis/multiplayer/game-variants/round-reset-scenario/report.json')
    for key in ('preparation', 'sound_initializer', 'independent_device_constructor',
                'tag_postprocess', 'tag_index_delivery', 'new_preparation', 'variant', 'source_player'):
        assert owned[key] == base[key], key
    assert owned['whole_guest_text_unchanged']
    assert owned['instruction_patches'] == owned['new_supplied_gameplay_returns'] == []
    assert owned['activation_context']['simulation_initialized_byte'] == 0
    assert len(owned['events']) == 370 and len(owned['cycles']) == 2
    request, first, rejected, activation = owned['events'][:4]
    assert request['entry'] == '001614f0' and request['before']['phase'] == 0 and request['after']['status'] == 1
    assert first['entry'] == '0015dbc0' and first['after']['phase'] == 1
    assert rejected['entry'] == '0015f3a0' and int(rejected['returned_eax'], 16)&255 == 0
    assert activation['entry'] == '0014e9e0' and activation['completion'] == 'complete_original_return'
    before = activation['before']['players']['records'][0]
    after = activation['after']['players']['records'][0]
    assert int(before['flags'], 16)&1 == 0 and int(after['flags'], 16)&1 == 1
    assert any(call['entry'] == '000688f0'for call in activation['calls'])
    for event in owned['events']:
        assert event['guarded'] and event['completion'] == 'complete_original_return' and event['stop'] is None
        assert event['before']['game_over'] == event['after']['game_over'] == 0
        for service in event['services']:
            assert service['rng_write_instruction'] == '0016f5e0'
            assert int(service['after'], 16) == (int(service['before'], 16)*1664525+1013904223)&0xffffffff
    assert len(owned['storage_release_records']) == 23
    assert all('after' in row and row['writes']for row in owned['storage_release_records'])
    for index, cycle in enumerate(owned['cycles']):
        assert cycle['index'] == index and len(cycle['events']) == 183
        sequence = [owned['events'][n]for n in cycle['events']]
        assert sequence[0]['entry'] == '0015f3a0' and int(sequence[0]['returned_eax'], 16)&255 == 1
        assert cycle['spawn']['completion'] == 'complete_original_return'
        assert int(cycle['spawn']['returned_eax'], 16)&255 == 1
        assert cycle['direct_spawn_is_independent_of_admission']
        for n, event in enumerate(sequence[2:]):
            assert event['entry'] == '0015e070'
            assert event['after']['phase'] == (2 if n < 150 else 3 if n < 180 else 1)
            assert event['after']['index'] == index+int(n >= 150)
            assert event['after']['countdown'] == (150-n if n < 150 else 180-n)
            if n:
                assert event['before']['tick'] == sequence[2+n-1]['after']['tick']+1
        last = sequence[-1]
        assert last['storage_releases'] == (2 if index == 0 else 21)
        calls = [call['entry']for call in last['calls']]
        assert calls.count('0015b460') == calls.count('00159180') == 1
        assert calls.count('000bd1f0') == last['storage_releases']
        assert len(last['services']) == 14
        player = last['after']['players']['records'][0]
        raw = bytes.fromhex(player['raw_hex'])
        assert player['current'] == player['previous'] == 'ffffffff'
        assert raw[0x174:0x178] == b'\xff'*4 and int.from_bytes(raw[0x218:0x21a], 'little') == index+1
        assert int.from_bytes(raw[0x1ac:0x1ae], 'little', signed=True) == 1
        assert int(player['flags'], 16)&9 == 9
        world = last['after']['world']
        assert world['object_count'] == world['object_high_water'] == 19
        assert world['component_count'] == world['physics_counter'] == 15
        assert Counter(row['definition']for row in cycle['constructed_world_after']['objects']) == Counter(
            row['definition']for row in base['cycles'][index]['constructed_world_after']['objects'])
        pool = bytes.fromhex(cycle['audio_after']['looping_header_hex'])
        assert int.from_bytes(pool[0x38:0x3c], 'little') == int.from_bytes(pool[0x3c:0x40], 'little') == 6
    image = Xbe(SPECS['tu11']['path']);code = _Code(image, '.text')
    listing = owned_data['executed_instruction_listing'];path = OWNED/'instructions'/listing['file']
    assert sha(path) == listing['sha256']
    assert path.read_bytes() == (OWNED/'reproduced/instructions'/listing['file']).read_bytes()
    assert verify_listing(path, image, code) == listing['rows']
    paths.update((str(path.relative_to(ROOT)), str((OWNED/'reproduced/instructions'/listing['file']).relative_to(ROOT))))
    sanitizers = json.loads((OUT/'sanitizers/report.json').read_text())
    for path, pin in sanitizers['instrumented_sources'].items():
        assert sha(ROOT/path) == pin, path
        paths.add(path)
    for result in sanitizers['results']:
        assert result['returncode'] == 0 and result['stderr'] == ''
        assert sha(Path(result['command'][0])) == result['binary_sha256']
    extra = json.loads((OUT/'sanitizer-preparation.json').read_text())
    assert extra['returncode'] == 0 and extra['stderr'] == ''
    assert '1746 whole original phase/post-world records and 36 preparation records pass' in extra['stdout']
    assert sha(Path(extra['command'][0])) == extra['binary_sha256']
    ctest = (OUT/'ctest.log').read_text()
    assert '100% tests passed out of 2' in ctest
    for target in ('original_round_lifecycle', 'native_game_end'):
        assert any(target in row and 'Passed' in row for row in ctest.splitlines())
    assert extra['stdout'].strip() in (OUT/'ctest-details.log').read_text()
    paths.update(('CMakeLists.txt', 'include/halo2/source_round_lifecycle.hpp',
        'tools/oracle_round_preparation.py', 'tools/oracle_owned_round_phase.py', 'tools/verify_round_preparation.py',
        'tools/sanitize_round_lifecycle.py', 'tests/fixtures/round_preparation_original.json',
        'tests/fixtures/round_lifecycle_original.json', 'tests/fixtures/round_lifecycle_cap_original.json',
        'docs/MP_ROUND_PREPARATION.md', 'docs/MP_ROUND_LIFECYCLE_NATIVE.md', 'docs/GAME_MODE_PROGRESS.md',
        'assets/disc/maps/lockout.map', 'assets/disc/maps/mainmenu.map',
        'analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/generator.exe'))
    for directory in (OUT, OWNED):
        paths.update(str(p.relative_to(ROOT))for p in directory.rglob('*')if p.is_file()
                     and p.name not in ('verification.json', 'verify.log'))
    # Inspect ordinary binaries only after confirming no shared build is live.
    no_active_build(ROOT/'build')
    for name in ('round_lifecycle_tests', 'game_end_native_tests'):
        binary = ROOT/'build'/name
        relevant = {p for p in sanitizers['instrumented_sources']
                    if not p.startswith('tests/') and p != 'src/multiplayer.cpp'}
        relevant.add('tests/'+name+'.cpp')
        if name == 'game_end_native_tests':
            relevant.add('src/multiplayer.cpp')
        assert all((ROOT/p).stat().st_mtime_ns <= binary.stat().st_mtime_ns for p in relevant)
        paths.add(str(binary.relative_to(ROOT)))
    metrics = dict(original_wrapper_records=wrappers, original_preparation_records=phases,
                   stopped_preparation_records=stopped_phases, original_lifecycle_records=1746,
                   owned_guarded_api_calls=370, complete_original_inter_round_transitions=2,
                   source_activation_scans=1, positive_respawn_admission_queries=2,
                   original_object_deletions=23, rebuilt_objects_per_transition=19,
                   ordinary_player_update_flows=0, complete_native_matches_proven=0,
                   literal_preparation_instruction_rows=listing_rows,
                   observed_owned_instruction_rows=listing['rows'])
    result = dict(schema=1, metrics=metrics, artifact_pins={str(p):sha(ROOT/p)for p in sorted(paths)})
    output = OUT/'verification.json'
    output.write_text(json.dumps(result, sort_keys=True, separators=(',', ':'))+'\n')
    print(json.dumps(dict(verification=str(output), sha256=sha(output), pins=len(paths), metrics=metrics)))


if __name__ == '__main__':
    main()
