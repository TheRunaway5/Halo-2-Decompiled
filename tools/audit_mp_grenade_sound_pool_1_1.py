#!/usr/bin/env python3
"""Audit genuine grenade-context sound pool ownership and absent-driver stop."""
import argparse
import json
import struct
from pathlib import Path
import oracle_mp_grenade_sound_pool_1_1 as oracle
from oracle_smg_firing_1_1 import Xbe, PIN, CACHE_PIN

ROOT = oracle.ROOT
OUT = oracle.OUT
motion = oracle.motion


def word(data, offset):
    return struct.unpack_from('<I', data, offset)[0]


def replay_range(initial, address, rows):
    output = bytearray(initial)
    finish = address + len(output)
    for row in rows:
        start = int(row['address'], 16)
        data = bytes.fromhex(row['written_hex'])
        assert len(data) == row['size']
        lo, hi = max(address, start), min(finish, start + len(data))
        if hi > lo:
            output[lo-address:hi-address] = data[lo-start:hi-start]
    return bytes(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output = args.output.resolve()
    assert not args.output.exists()
    first, second = OUT/'measured', OUT/'repeat'
    assert (first/'report.json').read_bytes() == (second/'report.json').read_bytes()
    report = json.loads((first/'report.json').read_text())
    assert report['oracle_sha256'] == motion.digest(ROOT/'tools/oracle_mp_grenade_sound_pool_1_1.py')
    assert not report['new_supplied_gameplay_fields']
    assert not report['new_supplied_source_returns'] and not report['instruction_patches']
    previous = ROOT/'analysis/multiplayer/integration/grenade-owned-release/checkpoint-manifest.json'
    assert report['previous_continuation_checkpoint_sha256'] == motion.digest(previous)
    freeze = json.loads(previous.read_text())
    prior_pins = {**freeze['source_files'], **freeze['verified_prior_pins']}
    for name, digest in prior_pins.items():
        assert motion.digest(ROOT/name) == digest, name
    prefix = report['prior_admitted_begin_correlation']
    old = json.loads((ROOT/prefix['report']).read_text())
    assert motion.digest(ROOT/prefix['report']) == prefix['report_sha256']
    for key in ('calls', 'states', 'reads', 'stores', 'points'):
        lo, hi = prefix['ranges'][key]
        assert lo == 0 and report[key][lo:hi] == old[key], key

    before = report['before']
    assert before['owner_pool_pointer'] == before['owner_state_pointer'] == before['driver_pointer'] == '00000000'
    assert before['allocator_pointer'] == '82d48288' and before['arena_pointer'] == '82d47000'
    assert before['arena_used'] == '001ae548' and before['arena_checksum'] == '08010d56'
    stages = report['stages']
    assert len(stages) == 2
    constructor, reset = stages
    assert constructor['entry'] == '0018a200' and constructor['completion'] == 'complete_original_return'
    assert constructor['invalid'] is None and constructor['services'] == []
    assert reset['entry'] == '0018a2a0' and reset['completion'] == 'blocked_original_instruction'
    assert reset['services'] == []
    stop = reset['invalid']
    assert stop['instruction'] == '0022310c' and stop['address'] == '00002abc'
    assert stop['access'] == 20 and stop['size'] == 4
    assert stop['registers']['esi'] == '00000000'
    assert stop['registers']['eax'] == '82efb83c' and stop['registers']['edx'] == '82efb614'
    assert '00223106' in stop['recent']
    a, b = constructor['sound_context'], reset['sound_context']
    for value in (a, b):
        assert value['owner_pool_pointer'] == '82ef5548' and value['owner_state_pointer'] == '82efb614'
        assert value['allocator_pointer'] == before['allocator_pointer']
        assert value['arena_pointer'] == before['arena_pointer']
        assert value['arena_used'] == '001b4858' and value['arena_checksum'] == 'b8ff067c'
        assert value['driver_pointer'] == '00000000'
    pool_a, pool_b = (bytes.fromhex(x['owner_pool60cc_hex']) for x in (a, b))
    state_a, state_b = (bytes.fromhex(x['owner_state244_hex']) for x in (a, b))
    assert len(pool_a) == len(pool_b) == 0x60cc
    assert len(state_a) == len(state_b) == 0x244
    assert pool_a[:0x20].rstrip(b'\0') == b'object looping sounds'
    assert word(pool_a, 0x20) == 1024 and word(pool_a, 0x24) == 0x18
    assert pool_a[0x28] == 0 and pool_a[0x29] == 0 and pool_a[0x2a] == 4
    assert pool_b[0x29] == 1 and pool_b[0x2a] == 4
    assert word(pool_a, 0x30) == 0x82d48288
    assert word(pool_a, 0x44) == 0x82ef5594 and word(pool_a, 0x48) == 0x82efb594
    for value in (pool_a, pool_b):
        assert all(word(value, i) == 0 for i in (0x34, 0x38, 0x3c))
        assert value[0x4c:0x604c] == b'\0'*(0x18*1024)
        assert value[0x604c:] == b'\0'*128
    assert state_a == b'\0'*0x244
    expected = (b'\xff'*0x20 + b'\0'*0x108 + b'\xff'*0x100 +
                bytes.fromhex('0000803f')*4 + struct.pack('<I', 0x1f40) + b'\0'*8)
    assert len(expected) == 0x244 and state_b == expected

    lo, hi = report['ranges']['stores']
    stores = report['stores'][lo:hi]
    reset_stores = [row for row in stores if row['phase'] == 'source_looping_sound_map_reset']
    assert replay_range(pool_a, 0x82ef5548, reset_stores) == pool_b
    assert replay_range(state_a, 0x82efb614, reset_stores) == state_b
    def one_store(instruction, address, written):
        values = [row for row in stores if row['instruction'] == instruction and row['address'] == address]
        assert len(values) == 1 and values[0]['written_hex'] == written
        return values[0]
    publish_pool = one_store('0018a243', '004f485c', '4855ef82')
    publish_state = one_store('0018a272', '004f4858', '14b6ef82')
    valid_store = one_store('0018a2b5', '82ef5571', '01')
    assert valid_store['before_hex'] == '00'
    assert publish_pool['order'] < publish_state['order'] < valid_store['order']
    assert not any(row['instruction'] == '0022310c' for row in stores)
    calls = report['calls'][report['ranges']['calls'][0]:]
    assert [row['entry'] for row in calls] == ['0018a200','0016d070','0016d0d0','0018a2a0','0016d220','002230e0']
    assert calls[0]['return_esp_delta'] == 4 and calls[1]['return_esp_delta'] == 16
    assert calls[2]['return_esp_delta'] == 28 and calls[4]['return_esp_delta'] == 4
    assert report['unreturned_original_calls'] == [calls[3]['id'], calls[5]['id']]
    assert all('return_cpu' not in calls[i] for i in (3, 5))
    final = report['final_state']
    for key in ('grenade','objects','clock','seed','player','command5c_hex','carried_weapon_own_states','effect_context'):
        assert final[key] == old['states'][-1][key], key
    # The physics snapshot also observes the shared arena cursor. Its change is
    # exactly the new original allocations, rather than a physics/body change.
    expected_physics = dict(old['states'][-1]['physics'])
    assert expected_physics['arena_cursor'] == int(before['arena_used'], 16)
    expected_physics['arena_cursor'] += 0x60cc + 0x244
    assert expected_physics['arena_cursor'] == int(b['arena_used'], 16)
    assert final['physics'] == expected_physics
    assert final['grenade']['phase'] == 1 and final['grenade']['timer'] == 1
    assert final['grenade']['counts'] == [2, 0] and final['grenade']['held_object'] == 'ffffffff'
    stop_point = [row for row in report['points'] if row['instruction'] == '0022310c']
    assert len(stop_point) == 1 and stop_point[0]['cpu']['xmm128'][3][-8:] == '3f800000'

    image_path = ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert motion.digest(image_path) == PIN
    assert motion.digest(ROOT/'assets/disc/maps/lockout.map') == CACHE_PIN
    image = Xbe(image_path)
    files, checked = {}, 0
    for directory in (first, second):
        assert (directory/'stderr.log').read_bytes() == b''
        for listing in report['instructions']:
            path = directory/listing['file']
            assert motion.digest(path) == listing['sha256']
            assert path.read_bytes() == (second/listing['file']).read_bytes()
            for line in path.read_text().splitlines():
                address, raw, _ = line.split(' ', 2)
                data = bytes.fromhex(raw)
                assert image.read(int(address, 16), len(data)) == data
                checked += 1
        for path in directory.rglob('*'):
            if path.is_file():
                files[str(path.relative_to(ROOT))] = motion.digest(path)
    for name in ('tools/oracle_mp_grenade_sound_pool_1_1.py', 'tools/audit_mp_grenade_sound_pool_1_1.py', 'docs/MP_GRENADE_SOUND_POOL_PREFIX_RESEARCH.md'):
        files[name] = motion.digest(ROOT/name)
    files[str(previous.relative_to(ROOT))] = motion.digest(previous)
    value = dict(schema=1, source_files=files, verified_prior_pins=prior_pins,
        independently_identical_pair=['measured','repeat'], original_xbe_sha256=PIN, original_cache_sha256=CACHE_PIN,
        prior_admitted_begin_correlation=prefix, source_instruction_rows=checked,
        full_observed_calls=len(report['calls']), full_observed_states=len(report['states']),
        ordered_original_stores=len(report['stores']), new_ordered_original_stores=len(stores),
        pool_publication_store=publish_pool, state_publication_store=publish_state, validity_owner_store=valid_store,
        complete_constructor_call=calls[0], complete_pool_reset_call=calls[4], genuine_stop=stop,
        full_constructor_context=a, full_reset_context=b, unreturned_original_calls=report['unreturned_original_calls'],
        native_gameplay_update_applied=False,
        scope='Original18A200 returns using the actual game allocator. Original18A2A0 writes/reset-validates the whole pool and state but stops at absent driver22310C. No new success, flag, pool, callback, clock, seed or provider is supplied; B8A30 is not resumed and grenade launch remains unproved.')
    motion.frozen_write(args.output, motion.encoded(value))
    manifest = dict(value)
    manifest['source_files'] = {**files, str(args.output.relative_to(ROOT)): motion.digest(args.output)}
    path = OUT/'manifest.json'
    assert not path.exists()
    motion.frozen_write(path, motion.encoded(manifest))
    print(json.dumps(dict(audit_sha256=motion.digest(args.output), manifest_sha256=motion.digest(path),
        source_instruction_rows=checked, source_files=len(manifest['source_files']), prior_pins=len(prior_pins))), flush=True)


if __name__ == '__main__':
    main()
