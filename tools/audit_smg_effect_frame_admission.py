#!/usr/bin/env python3
"""Audit unchanged original frame-gate owners and the actual heap failure stop."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from oracle_smg_effect_frame_admission_1_1 import ROOT, OUT, PRIOR, PROTECTION
from oracle_smg_effect_owner_1_1 import sha, save, encoded
from audit_smg_effect_system_time import buffers
from xbe import Xbe


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT / 'audit.json')
    args = parser.parse_args()
    measured = OUT / 'measured'
    report = json.loads((measured / 'report.json').read_text())
    for file, pin in PRIOR.items():
        assert sha(ROOT / file) == pin, file
    assert report['frozen_pins'] == PRIOR
    assert report['oracle_sha256'] == sha(ROOT / 'tools/oracle_smg_effect_frame_admission_1_1.py')
    repeated = ['report.json', 'native-clock-inputs.json', 'native-clock-stderr.log'] + [row['file'] for row in report['instructions']]
    for file in repeated:
        assert (measured / file).read_bytes() == (OUT / 'independent-repeat' / file).read_bytes(), file
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest() == 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
    executed, rows = set(), 0
    static = json.loads((OUT / 'static/report.json').read_text())
    for directory, listings in [(measured, report['instructions']), (OUT / 'static', static['routines'])]:
        for listing in listings:
            path = directory / listing['file']
            assert sha(path) == listing['sha256']
            lines = path.read_text().splitlines()
            assert len(lines) == listing['rows']
            for line in lines:
                address, raw, _ = line.split(' ', 2)
                literal = bytes.fromhex(raw)
                assert image.read(int(address, 16), len(literal)) == literal
                rows += 1
                if directory == measured and listing['file'] == 'instructions/all-executed.asm':
                    executed.add(address)
    for table in static['tables']:
        assert image.read(int(table['address'], 16), len(bytes.fromhex(table['raw_hex']))).hex() == table['raw_hex']
    assert not static['image_global_scope']['within_raw_file_bytes']
    assert static['image_global_scope']['reader_zero_filled4_hex'] == '00000000'
    writer_sites = {row['address'] for row in static['literal_global_references'] if row['operand_access'] & 2}
    assert writer_sites == {'00190624', '00190652', '00190714', '001907c8', '00190870', '001908c4', '00190b6c', '00190b8a'}
    assert not writer_sites & executed
    assert not any(int(row['address'], 16) <= 0x4f4864 < int(row['address'], 16) + row['size'] for row in report['stores'])
    assert [row['instruction'] for row in report['frame_records']] == ['0012cbc0', '001902e0']
    for record in report['frame_records']:
        assert record['state']['frame4f4860_12c0_hex'][8:16] == '00000000'
    final = report['final_frame_state']
    assert final['frame4f4860_12c0_hex'][8:16] == '00000000'
    assert final['game1200_hex'][0x1120 * 2:0x1120 * 2 + 2] == '00'
    for state in report['states']:
        assert state['current'] == report['states'][0]['current']
    protection = report['permission_provider']
    assert sha(PROTECTION / 'report.json') == protection['original_report_sha256']
    assert sha(PROTECTION / 'compiled-compatibility-access.cpp') == protection['compiled']['source_sha256']
    assert sha(PROTECTION / 'compiled-compatibility-access.so') == protection['compiled']['library_sha256']
    for file, row in protection['primary_sources'].items():
        assert sha(PROTECTION / 'primary-source' / file) == row['sha256']
    assert len(report['actual_memory_protection']) == 6
    for call in report['actual_memory_protection']:
        assert call['args'][2] == '00000004'
        assert call['raw_before_sha256'] == call['raw_after_sha256']
        assert call['before_cpu']['registers'] == call['after_cpu']['registers']
        assert int(call['after_cpu']['esp'], 16) - int(call['before_cpu']['esp'], 16) == 16
        assert call['applied']['actual_guest_access'] == 7 and call['applied']['windows_access'] == '00000040'
    zero = next(row for row in report['calls'] if row['entry'] == '002d7a1a' and row['caller'] == '00082828')
    assert zero['stack12_words'][:3] == ['82c67000', '00000000', '00000004']
    assert zero['return_esp_delta'] == 16
    assert not any(row['original_return'] == '00082828' for row in report['actual_memory_protection'])
    stopped = report['stages'][-1]
    invalid = stopped['invalid']
    assert stopped['completion'] == 'blocked_original_instruction'
    assert stopped['stopped_eip'] == invalid['instruction'] == '003382b6'
    assert invalid['exception_code'] == 21 and not invalid['original_result_supplied']
    assert invalid['recent_original_instructions'][-1]['address'] == '003382b4'
    assert image.read(0x3382b4, 3).hex() == 'cd2dcc'
    assert '003382b4' in executed and '003382b6' not in executed
    output = [row for row in report['reads'] if row['instruction'] == '00338297']
    message = b''.join(bytes.fromhex(row['raw_hex']) for row in output)
    assert message == b'*** Software Failure: Heap constructor failed in ROCKALL_FRONT_END ***\n\x00'
    stack_pointer = int(invalid['cpu']['esp'], 16)
    descriptor_pointer = int(invalid['cpu']['registers']['ecx'], 16)
    raw_stack = bytes.fromhex(invalid['stack40_hex'])
    length, maximum, pointer = struct.unpack_from('<HHI', raw_stack, descriptor_pointer - stack_pointer)
    assert (length, maximum) == (len(message) - 1, len(message))
    assert output[0]['address'] == f'{pointer:08x}'
    assert all(int(row['address'], 16) == pointer + index for index, row in enumerate(output))
    replays = []
    for stage in report['stages']:
        before = buffers(report['states'][stage['before_state']])
        after = {(name, address): raw for name, address, raw in buffers(report['states'][stage['after_state']])}
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
            assert value.hex() == after[name, address], name
            replays.append(dict(stage=stage['name'], buffer=name, bytes=len(value), overlapping_stores=count, full_bytes_equal=True))
    clock = json.loads((measured / 'native-clock-inputs.json').read_text())
    assert len(report['actual_tick_deliveries']) == len(report['actual_tick_returns']) == 92
    for delivery, returned in zip(report['actual_tick_deliveries'], report['actual_tick_returns']):
        assert delivery['actual_output4_hex'] == returned['eax4_hex'] == clock[delivery['native_observation_index']]['clock4_hex']
        assert delivery['before_cpu'] == delivery['after_cpu']
    calls = {row['entry'] for row in report['calls']}
    assert not calls & {'001904d0', '00190650', '00190690', '0012c850', '00147dd0', '00139640', '001789d0', '00176410', '001764b0', '0024a820', '00249210'}
    result = dict(schema='source-smg-effect-frame-admission-audit-v1', tool_sha256=sha(__file__), original_report_sha256=sha(measured / 'report.json'), byte_identical_files=len(repeated), prior_pins_intact=len(PRIOR), source_instruction_rows=rows, executed_original_addresses=len(executed), full_buffer_replays=replays, actual_protection_calls=6, original_zero_size_protect_returns_without_kernel=True, actual_source_tick_reads=92, original_diagnostic_message_hex=message.hex(), original_diagnostic_descriptor=dict(length=length, maximum_length=maximum, buffer_pointer=f'{pointer:08x}'), actual_original_int2d_cpu_exception=True, original_state_writers_listed_not_executed=sorted(writer_sites), state4f4864=0, game_readiness=0, admitted_multiplayer_aging_not_tested=True, unchanged_current_effect_and_particle_state=True)
    save(args.output, encoded(result))
    print(json.dumps(dict(output=str(args.output), sha256=sha(args.output), repeated_files=len(repeated), buffer_replays=len(replays), prior_pins=len(PRIOR), source_addresses=len(executed))))


if __name__ == '__main__':
    main()
