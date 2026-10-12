#!/usr/bin/env python3
"""Independently audit source-owned heap capacity and natural DSP startup stop."""
import argparse
import hashlib
import json
import sys
from pathlib import Path
from oracle_smg_effect_frame_configuration_1_1 import ROOT, OUT, PRIOR
from oracle_smg_effect_frame_admission_1_1 import PROTECTION
from oracle_smg_effect_owner_1_1 import sha, save, encoded
from audit_smg_effect_system_time import buffers
from xbe import Xbe


def apply_stores(raw, address, stores):
    value = bytearray.fromhex(raw)
    count = 0
    for row in stores:
        pointer = int(row['address'], 16)
        literal = bytes.fromhex(row['written_hex'])
        left, right = max(pointer, address), min(pointer + len(literal), address + len(value))
        if left < right:
            value[left - address:right - address] = literal[left - pointer:right - pointer]
            count += 1
    return value.hex(), count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT / 'audit-v2.json')
    args = parser.parse_args()
    measured = OUT / 'measured'
    report = json.loads((measured / 'report.json').read_text())
    for name, expected in PRIOR.items():
        assert sha(ROOT / name) == expected, name
    assert report['frozen_pins'] == PRIOR
    assert report['oracle_sha256'] == sha(ROOT / 'tools/oracle_smg_effect_frame_configuration_1_1.py')
    assert report['new_game_or_particle_state_inputs'] == report['instruction_patches'] == []
    repeated = ['report.json', 'native-clock-inputs.json', 'native-clock-stderr.log'] + [row['file'] for row in report['instructions']]
    for name in repeated:
        assert (measured / name).read_bytes() == (OUT / 'independent-repeat' / name).read_bytes(), name
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest() == report['source_xbe_sha256']
    executed, rows = set(), 0
    for listing in report['instructions']:
        path = measured / listing['file']
        assert sha(path) == listing['sha256']
        lines = path.read_text().splitlines()
        assert len(lines) == listing['rows']
        for line in lines:
            address, raw, _ = line.split(' ', 2)
            literal = bytes.fromhex(raw)
            assert image.read(int(address, 16), len(literal)) == literal
            rows += 1
            if listing['file'] == 'instructions/all-executed.asm':
                executed.add(address)
    static = json.loads((OUT / 'static/report.json').read_text())
    static_rows = 0
    for listing in static['routines']:
        path = OUT / 'static' / listing['file']
        assert sha(path) == listing['sha256']
        assert path.read_bytes() == (OUT / 'independent-static' / listing['file']).read_bytes()
        for line in path.read_text().splitlines():
            address, raw, _ = line.split(' ', 2)
            literal = bytes.fromhex(raw)
            assert image.read(int(address, 16), len(literal)) == literal
            static_rows += 1
    assert (OUT / 'static/report.json').read_bytes() == (OUT / 'independent-static/report.json').read_bytes()
    table = static['memory_class_callback_table']
    assert image.read(0x45ad78, 0x40).hex() == table['raw_hex']
    dsp = static['dsp_section']
    assert hashlib.sha256(image.read(int(dsp['original_virtual_address'], 16), dsp['raw_size'])).hexdigest() == dsp['raw_sha256']
    assert image.read(0x45aea0, 9) == b'DSPImage\x00'
    assert image.read(0x192dbe, 6).hex() == static['dsp_record_writer']['original_bytes_hex']
    assert '00192dbe' not in executed
    permission = report['permission_provider']
    assert sha(PROTECTION / 'report.json') == permission['original_report_sha256']
    for name, row in permission['primary_sources'].items():
        assert sha(PROTECTION / 'primary-source' / name) == row['sha256'], name
    assert sha(PROTECTION / 'compiled-compatibility-access.cpp') == permission['compiled']['source_sha256']
    assert sha(PROTECTION / 'compiled-compatibility-access.so') == permission['compiled']['library_sha256']
    configuration, = report['configuration_records']
    assert configuration['complete_original_return']
    assert configuration['before']['heap_capacity4d6d24_8_hex'] == '0000000000000000'
    assert configuration['after']['heap_capacity4d6d24_8_hex'] == '0000100000000100'
    stores = report['stores'][configuration['first_store']:configuration['final_store']]
    for instruction, address, raw in [('00066f21', '004d6d24', '00001000'), ('00066f2b', '004d6d28', '00000100')]:
        matches = [row for row in stores if row['instruction'] == instruction]
        assert len(matches) == 1 and matches[0]['address'] == address and matches[0]['written_hex'] == raw
        assert instruction in executed
    configuration_replays = []
    for name, address in [('handle47e548_8_hex', 0x47e548), ('literal4d0000_7000_hex', 0x4d0000), ('heap_capacity4d6d24_8_hex', 0x4d6d24)]:
        value, count = apply_stores(configuration['before'][name], address, stores)
        assert value == configuration['after'][name], name
        configuration_replays.append(dict(buffer=name, bytes=len(value) // 2, overlapping_stores=count, full_bytes_equal=True))
    assert configuration['before']['current'] == configuration['after']['current']
    assert all(state['current'] == report['states'][0]['current'] for state in report['states'])
    assert [row['completion'] for row in report['stages']] == ['complete_original_return', 'complete_original_return', 'blocked_original_instruction']
    assert all(not row['services'] for row in report['stages'])
    ctor, = [row for row in report['calls'] if row['entry'] == '00330de0']
    assert ctor['caller'] == '00082939' and ctor['return_cpu']['registers']['eax'] == '08000860'
    assert '00082939' in executed and '001909bd' in executed
    assert len(report['actual_memory_protection']) == 10
    assert report['actual_memory_protection'][6]['args'] == ['82b67000', '00100000', '00000004']
    for row in report['actual_memory_protection']:
        assert row['raw_before_sha256'] == row['raw_after_sha256']
        assert row['before_cpu']['registers'] == row['after_cpu']['registers']
        assert int(row['after_cpu']['esp'], 16) - int(row['before_cpu']['esp'], 16) == 16
        assert row['applied']['actual_guest_access'] == 7 and row['applied']['windows_access'] == '00000040'
    stop = report['stages'][-1]
    assert stop['stopped_eip'] == stop['invalid']['instruction'] == '00055777'
    assert stop['invalid']['address'] == '00000000' and stop['invalid']['size'] == 4 and stop['invalid']['access'] == 19
    assert not stop['invalid']['original_result_supplied']
    pointer_reads = [row for row in report['reads'] if row['instruction'] == '00055770']
    assert len(pointer_reads) == 1 and pointer_reads[0]['address'] == '00518480' and pointer_reads[0]['raw_hex'] == '00000000'
    assert image.read(0x55770, 9).hex() == '8b1580845100568b32'
    assert [row['instruction'] for row in report['frame_records']] == ['0012cbc0', '001902e0']
    assert report['final_frame_state']['frame4f4860_12c0_hex'][8:16] == '00000000'
    assert report['final_frame_state']['game1200_hex'][0x1120 * 2:0x1120 * 2 + 2] == '00'
    calls = {row['entry'] for row in report['calls']}
    assert not calls & {'001904d0', '00190650', '00190690', '00192d30', '00147dd0', '00139640', '001789d0', '00176410', '001764b0', '0024a820', '00249210'}
    replays = []
    for stage in report['stages']:
        before = buffers(report['states'][stage['before_state']])
        after = {(name, address): raw for name, address, raw in buffers(report['states'][stage['after_state']])}
        stage_calls = [row for row in report['calls'] if row['phase'] == stage['name']]
        low = min(row['order'] for row in stage_calls)
        high = max(row.get('return_order', low) for row in stage_calls) if stage['completion'] == 'complete_original_return' else max(row['order'] for row in report['stores']) + 1
        selected = [row for row in report['stores'] if low < row['order'] < high]
        for name, address, raw in before:
            value, count = apply_stores(raw, address, selected)
            assert value == after[name, address], name
            replays.append(dict(stage=stage['name'], buffer=name, bytes=len(value) // 2, overlapping_stores=count, full_bytes_equal=True))
    clock = json.loads((measured / 'native-clock-inputs.json').read_text())
    old_clock = json.loads((ROOT / 'analysis/multiplayer/integration/inventory-source/smg-effect-system-time/measured-real/native-clock-inputs.json').read_text())
    assert clock[:len(old_clock)] == old_clock
    assert len(report['actual_tick_deliveries']) == len(report['actual_tick_returns']) == 92
    for delivery, returned in zip(report['actual_tick_deliveries'], report['actual_tick_returns']):
        assert delivery['actual_output4_hex'] == returned['eax4_hex'] == clock[delivery['native_observation_index']]['clock4_hex']
        assert delivery['before_cpu'] == delivery['after_cpu']
    result = dict(schema='source-smg-effect-frame-configuration-audit-v1', tool_sha256=sha(__file__), original_report_sha256=sha(measured / 'report.json'),
                  prior_pins_intact=len(PRIOR), byte_identical_files=len(repeated), source_instruction_rows=rows, executed_original_addresses=len(executed), static_original_rows=static_rows, independent_static_byte_identical_files=13,
                  configuration_full_buffer_replays=configuration_replays, full_buffer_replays=replays,
                  original_capacity_stores_executed=True, original_heap_constructor_returns=True, actual_protection_operations=10,
                  actual_source_tick_reads=92, new_guest_gameplay_inputs=0, exact_unavailable_pointer='00518480', exact_stop='00055777',
                  unchanged_current_effect_and_particle_state=True, frame_state=0, game_readiness=0, admitted_multiplayer_aging_not_tested=True)
    save(args.output, encoded(result))
    print(json.dumps(dict(output=str(args.output), sha256=sha(args.output), repeated_files=len(repeated), full_buffer_replays=len(replays), configuration_replays=len(configuration_replays), prior_pins=len(PRIOR), source_addresses=len(executed))))


if __name__ == '__main__':
    main()
