#!/usr/bin/env python3
"""Audit measured platform time, original bytes, state and independent replay."""
import argparse
import hashlib
import json
from pathlib import Path

from oracle_smg_effect_system_time_1_1 import ROOT, OUT, NATIVE, SDK, FROZEN, body
from oracle_smg_effect_owner_1_1 import sha, save, encoded
from xbe import Xbe


def buffers(state):
    current = state['current']
    result = []

    def add(name, pointer, raw):
        if raw is not None:
            result.append((name, int(pointer, 16), raw))

    for pool in current['pools']:
        label = pool['global_address']
        add(label + '.header', pool['pointer'], pool.get('header4c_hex'))
        add(label + '.bitmap', pool.get('bitmap_pointer', '0'), pool.get('bitmap_hex'))
    for pool in state['retained_ever_allocated_slots']:
        for slot in pool['slots']:
            add(pool['global_address'] + '.slot' + str(slot['index']), slot['pointer'], slot['raw_hex'])
    for label, field in (('clock', 'raw24_hex'), ('player', 'raw21c_hex'), ('shared_seed', 'raw8_hex')):
        add(label, current[label]['pointer'], current[label][field])
    for owner in current['local_owner_storage']:
        add(owner['name'], owner['pointer'], owner['raw_hex'])
    for record in state['source_frame_globals']:
        add('global' + record['address'], record['address'], record['raw_hex'])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT / 'audit.json')
    args = parser.parse_args()
    measured = OUT / 'measured-real'
    report = json.loads((measured / 'report.json').read_text())
    assert report['frozen_pins'] == FROZEN
    for file, pin in FROZEN.items():
        assert sha(ROOT / file) == pin, file
    assert sha(ROOT / 'tools/oracle_smg_effect_system_time_1_1.py') == report['oracle_sha256']
    assert report['new_game_or_particle_state_inputs'] == report['instruction_patches'] == []
    repeated = ['report.json', 'native-clock-inputs.json', 'native-clock-stderr.log'] + [row['file'] for row in report['instructions']]
    for file in repeated:
        assert (measured / file).read_bytes() == (OUT / 'independent-repeat' / file).read_bytes(), file

    contract = report['provider_contract']
    assert json.loads((OUT / 'provider/provider-contract-v2.json').read_text()) == contract
    for file, pin in contract['inputs'].items():
        assert sha(ROOT / file) == pin, file
    primary = contract['prior_primary_provenance']
    for file, pin in primary['primary_sources'].items():
        assert sha(NATIVE / 'primary-source' / file) == pin, file
    old = (NATIVE / 'compiled-timer.cpp').read_text()
    compiled = (OUT / 'provider/compiled-system-clock.cpp').read_text()
    assert compiled.startswith(old[:old.index('static void Hex')])
    ke = (NATIVE / 'primary-source/EmuKrnlKe.cpp').read_text()
    query = body(ke, r'xbox::KeQuerySystemTime\s*\([^)]*\)')
    assert query in compiled
    assert hashlib.sha256(query.encode()).hexdigest() == contract['original_query_body_sha256']
    assert sha(OUT / 'provider/compiled-system-clock.cpp') == contract['compiled_source_sha256']
    assert sha(OUT / 'provider/compiled-system-clock.exe') == contract['compiled_binary_sha256']
    assert 'CurrentTime The system time in 100-nanosecond intervals since January 1, 1601, in GMT.' in SDK.read_text()
    assert 'XBSYSAPI EXPORTNUM(128) xbox::void_xt NTAPI xbox::KeQuerySystemTime' in ke

    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    source_pin = 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
    assert hashlib.sha256(image.data).hexdigest() == source_pin
    audited_rows = 0
    executed = set()
    for listing in report['instructions']:
        file = measured / listing['file']
        assert sha(file) == listing['sha256']
        lines = file.read_text().splitlines()
        assert len(lines) == listing['rows']
        for line in lines:
            address, raw, _ = line.split(' ', 2)
            literal = bytes.fromhex(raw)
            assert image.read(int(address, 16), len(literal)) == literal, address
            audited_rows += 1
            if listing['file'] == 'instructions/all-executed.asm':
                executed.add(address)
    assert '00337a45' not in executed
    assert image.read(0x417bc8, 4) == bytes.fromhex('80000080')
    assert image.read(0x417cd4, 4) == bytes.fromhex('9c000080')

    replays = []
    for stage in report['stages']:
        before = buffers(report['states'][stage['before_state']])
        after = buffers(report['states'][stage['after_state']])
        assert before == after
        calls = [row for row in report['calls'] if row['phase'] == stage['name']]
        low = min(row['order'] for row in calls)
        high = max(row.get('return_order', low) for row in calls) if stage['completion'] == 'complete_original_return' else max(row['order'] for row in report['stores']) + 1
        for name, address, raw in before:
            value = bytearray.fromhex(raw)
            count = 0
            for store in report['stores']:
                if not low < store['order'] < high:
                    continue
                pointer = int(store['address'], 16)
                literal = bytes.fromhex(store['written_hex'])
                left, right = max(pointer, address), min(pointer + len(literal), address + len(value))
                if left < right:
                    value[left - address:right - address] = literal[left - pointer:right - pointer]
                    count += 1
            assert value.hex() == raw, (stage['name'], name)
            replays.append(dict(stage=stage['name'], buffer=name, bytes=len(value), overlapping_stores=count, full_bytes_equal=True))

    clock = json.loads((measured / 'native-clock-inputs.json').read_text())
    assert sha(measured / 'native-clock-inputs.json') == report['native_clock_inputs_sha256']
    assert len(clock) == 2 and [row['phase'] for row in clock] == ['initialize', 'query']
    initial, observed = clock
    assert [row['kind'] for row in initial['observations']] == [1, 2, 2, 3]
    assert [row['kind'] for row in observed['observations']] == [2, 2]
    initial_time = int.from_bytes(bytes.fromhex(initial['system12_hex'][:16]), 'little')
    assert initial_time == initial['observations'][-1]['value']
    for row in clock:
        assert not row['system_time_changed']
        assert len(row['timer_table']) == 32
        assert all(entry['forward_self'] and entry['reverse_self'] and entry['time8_hex'] == '00000000ffffffff' for entry in row['timer_table'])
    assert len(report['actual_platform_clock_delivery']) == 1
    delivery = report['actual_platform_clock_delivery'][0]
    output = observed['actual_query']['output8_hex']
    assert delivery['actual_output8_hex'] == output == observed['system12_hex'][:16]
    assert delivery['native_observation_index'] == 1 and delivery['kernel_ordinal'] == 128
    assert delivery['before_cpu']['registers'] == delivery['after_cpu']['registers']
    assert int(delivery['after_cpu']['esp'], 16) - int(delivery['before_cpu']['esp'], 16) == 8
    consumed = [row for row in report['reads'] if row['instruction'] in ('0032804c', '0032804f')]
    assert len(consumed) == 2
    assert consumed[0]['address'] == delivery['output_pointer']
    assert int(consumed[1]['address'], 16) == int(delivery['output_pointer'], 16) + 4
    assert ''.join(row['raw_hex'] for row in consumed) == output
    # Original add/adc constant and divisor are literal instruction operands.
    assert image.read(0x328054, 5) == bytes.fromhex('050080c12a')
    assert image.read(0x32805e, 6) == bytes.fromhex('81d1214e62fe')
    assert image.read(0x328059, 5) == bytes.fromhex('6880969800')
    epoch = (-((0xfe624e21 << 32) | 0x2ac18000)) & ((1 << 64) - 1)
    divisor = 0x989680
    assert epoch == 116444736000000000 and divisor == 10000000
    time_call = next(row for row in report['calls'] if row['entry'] == '0032803e')
    actual_time = int(time_call['return_cpu']['registers']['eax'], 16)
    assert (int.from_bytes(bytes.fromhex(output), 'little') - epoch) // divisor == actual_time
    thunk = next(row for row in report['calls'] if row['entry'] == '00336983')
    assert thunk['return_esp_delta'] == 8 and thunk['stack12_words'][0] == delivery['output_pointer']
    rand = next(row for row in report['calls'] if row['entry'] == '00328505')
    assert rand['return_cpu']['registers']['eax'] == '00000029'
    assert all(not row['services'] for row in report['stages'])
    stop = report['stages'][-1]
    assert stop['completion'] == 'blocked_original_instruction' and stop['stopped_eip'] == '00337a45'
    assert stop['invalid']['kernel_ordinal'] == 156 and stop['invalid']['instruction_unexecuted']
    assert stop['invalid']['target'] == '8000009c' and stop['invalid']['aliased4_hex'] == 'fdffbf2f'
    visited = {row['entry'] for row in report['calls']}
    assert not visited & {'0012c850', '00139640', '00147dd0', '001789d0', '00176410', '001764b0', '0024a820', '00249210'}
    result = dict(schema='source-smg-effect-system-time-audit-v1', tool_sha256=sha(__file__), original_report_sha256=sha(measured / 'report.json'), byte_identical_files=len(repeated), prior_pins_intact=len(FROZEN), primary_source_pins_intact=len(primary['primary_sources']), audited_instruction_rows=audited_rows, executed_original_addresses=len(executed), full_buffer_replays=replays, platform_output8_consumed_unchanged=True, original_time_null_eax=f'{actual_time:08x}', original_crt_rand_eax=rand['return_cpu']['registers']['eax'], void_eax_preserved=True, actual_source_clock_queries=1, guest_tick_counter_transport=False, first_unavailable_data_ordinal=156, unexecuted_particle_elapsed_and_aging_owners=True, native_private_records_separate_not_expected_byte_identity=True)
    save(args.output, encoded(result))
    print(json.dumps(dict(output=str(args.output), sha256=sha(args.output), repeated_files=len(repeated), buffer_replays=len(replays), original_addresses=len(executed), prior_pins=len(FROZEN))))


if __name__ == '__main__':
    main()
