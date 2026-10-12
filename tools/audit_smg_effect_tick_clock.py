#!/usr/bin/env python3
"""Independent exact-byte audit of the source tick-clock continuation."""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_effect_tick_clock_1_1 import ROOT, OUT, FROZEN, BACKING, system
from oracle_smg_effect_owner_1_1 import sha, save, encoded
from audit_smg_effect_system_time import buffers
from xbe import Xbe


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT / 'audit.json')
    args = parser.parse_args()
    directory = OUT / 'measured'
    report = json.loads((directory / 'report.json').read_text())
    for file, pin in FROZEN.items():
        assert sha(ROOT / file) == pin, file
    assert report['frozen_pins'] == FROZEN
    assert sha(ROOT / 'tools/oracle_smg_effect_tick_clock_1_1.py') == report['oracle_sha256']
    repeated = ['report.json', 'native-clock-inputs.json', 'native-clock-stderr.log'] + [row['file'] for row in report['instructions']]
    for file in repeated:
        assert (directory / file).read_bytes() == (OUT / 'independent-repeat' / file).read_bytes(), file
    prior = json.loads((system.OUT / 'measured-real/report.json').read_text())
    assert report['stages'][0] == prior['stages'][0]
    for old, new in zip(prior['calls'], report['calls']):
        assert all(new[key] == value for key, value in old.items()), old['id']
    assert report['actual_platform_clock_delivery'] == prior['actual_platform_clock_delivery']
    assert report['states'][0] == prior['states'][0]
    assert all(state['current'] == report['states'][0]['current'] for state in report['states'])
    clock = json.loads((directory / 'native-clock-inputs.json').read_text())
    assert clock[:2] == json.loads((system.OUT / 'measured-real/native-clock-inputs.json').read_text())
    assert len(clock) == 4 and [row['phase'] for row in clock] == ['initialize', 'query', 'update', 'update']
    assert sha(directory / 'native-clock-inputs.json') == report['native_clock_inputs_sha256']
    contract = report['provider_contract']
    assert json.loads((OUT / 'provider/contract.json').read_text()) == contract
    assert sha(OUT / 'provider/compiled-tick-clock.cpp') == contract['compiled_source_sha256']
    assert sha(OUT / 'provider/compiled-tick-clock.exe') == contract['compiled_binary_sha256']
    original = (system.OUT / 'provider/compiled-system-clock.cpp').read_text()
    replacement = '}else if(op==\'p\') {auto now=get_now();auto next=pit_next(now);Report("update",now,next);}else std::abort();'
    assert (OUT / 'provider/compiled-tick-clock.cpp').read_text().replace(replacement, '}else std::abort();') == original
    for file, pin in contract['prior_system_time_provider']['inputs'].items():
        assert sha(ROOT / file) == pin, file
    for file, pin in contract['prior_system_time_provider']['prior_primary_provenance']['primary_sources'].items():
        assert sha(system.NATIVE / 'primary-source' / file) == pin, file
    assert 'Contains the number of milliseconds elapsed since the system was started.' in system.SDK.read_text()
    assert 'XBSYSAPI EXPORTNUM(156) xbox::dword_xt VOLATILE xbox::KeTickCount = 0;' in (system.NATIVE / 'primary-source/EmuKrnlKe.cpp').read_text()
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest() == 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
    executed, instruction_rows = set(), 0
    for listing in report['instructions']:
        path = directory / listing['file']
        assert sha(path) == listing['sha256']
        lines = path.read_text().splitlines()
        assert len(lines) == listing['rows']
        for line in lines:
            address, raw, _ = line.split(' ', 2)
            literal = bytes.fromhex(raw)
            assert image.read(int(address, 16), len(literal)) == literal, address
            instruction_rows += 1
            if listing['file'] == 'instructions/all-executed.asm':
                executed.add(address)
    assert {'00337a40', '00337a45', '00337a47'} <= executed
    assert image.read(0x417cd4, 4).hex() == '9c000080'
    relocation, = report['data_import_relocations']
    assert relocation['original4_hex'] == '9c000080'
    assert relocation['relocated4_hex'] == BACKING.to_bytes(4, 'little').hex()
    assert relocation['instruction'] == '00337a40'
    assert len(report['actual_tick_deliveries']) == len(report['actual_tick_returns']) == 2
    tick_cases = []
    for delivery, returned in zip(report['actual_tick_deliveries'], report['actual_tick_returns']):
        index = delivery['native_observation_index']
        assert clock[index]['phase'] == 'update'
        assert delivery['actual_output4_hex'] == returned['eax4_hex'] == clock[index]['clock4_hex']
        assert delivery['output_pointer'] == f'{BACKING:08x}'
        assert delivery['before_cpu'] == delivery['after_cpu']
        assert [row['kind'] for row in clock[index]['observations']] == [2, 2]
        reads = [row for row in report['reads'] if row['instruction'] == '00337a45' and row['order'] > delivery['order']]
        assert reads[0]['raw_hex'] == delivery['actual_output4_hex'] and reads[0]['address'] == f'{BACKING:08x}'
        tick_cases.append(dict(return_address=delivery['original_return'], output4_hex=delivery['actual_output4_hex'], eax4_hex=returned['eax4_hex'], native_observation_index=index, original_load_and_return_executed=True))
    for row in clock:
        assert not row['system_time_changed'] and len(row['timer_table']) == 32
        assert all(entry['forward_self'] and entry['reverse_self'] and entry['time8_hex'] == '00000000ffffffff' for entry in row['timer_table'])
    replays = []
    for stage in report['stages']:
        before = buffers(report['states'][stage['before_state']])
        after = {(name, address): raw for name, address, raw in buffers(report['states'][stage['after_state']])}
        calls = [row for row in report['calls'] if row['phase'] == stage['name']]
        low = min(row['order'] for row in calls)
        high = max(row.get('return_order', low) for row in calls) if stage['completion'] == 'complete_original_return' else max(row['order'] for row in report['stores']) + 1
        for name, address, raw in before:
            value = bytearray.fromhex(raw)
            expected = after[name, address]
            assert len(expected) == len(raw)
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
            assert value.hex() == expected, (stage['name'], name)
            replays.append(dict(stage=stage['name'], buffer=name, bytes=len(value), overlapping_stores=count, changed=raw != expected, full_bytes_equal=True))
    for stage in report['stages']:
        assert not stage['services']
    stopped = report['stages'][-1]
    assert stopped['completion'] == 'blocked_original_instruction' and stopped['stopped_eip'] == '800000b6'
    assert stopped['invalid']['kernel_ordinal'] == 182 and stopped['invalid']['kernel_target_unexecuted']
    assert stopped['invalid']['args'] == ['82d46000', '00001000', '00000004'] and stopped['invalid']['original_return'] == '00082227'
    calls = {row['entry']: row for row in report['calls']}
    assert all('return_cpu' in calls[entry] for entry in ['00139880', '0007b540', '00190bf0'])
    assert not set(calls) & {'0012c850', '00147dd0', '00139640', '001789d0', '00176410', '001764b0', '0024a820', '00249210'}
    result = dict(schema='source-smg-effect-tick-clock-audit-v1', tool_sha256=sha(__file__), original_report_sha256=sha(directory / 'report.json'), byte_identical_files=len(repeated), prior_pins_intact=len(FROZEN), original_instruction_rows=instruction_rows, executed_original_addresses=len(executed), full_buffer_replays=replays, unchanged_current_effect_particle_player_pose_clock_rng_pvs_state=True, exact_prior_clock_prefix=True, tick_cases=tick_cases, actual_options_and_menu_request_owners_return=True, next_original_platform_stop=stopped['invalid'], unexecuted_outer_elapsed_and_particle_aging=True, no_game_or_particle_state_inputs=report['new_game_or_particle_state_inputs'] == report['instruction_patches'] == [], native_private_records_separate=True)
    save(args.output, encoded(result))
    print(json.dumps(dict(output=str(args.output), sha256=sha(args.output), repeated_files=len(repeated), buffer_replays=len(replays), prior_pins=len(FROZEN), original_addresses=len(executed))))


if __name__ == '__main__':
    main()
