#!/usr/bin/env python3
"""Audit independent original driver/title/DSP lookup stops without success."""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_effect_dsp_owner_1_1 import ROOT, OUT, PRIOR, RTL, SDK, RTL_CODE
from oracle_smg_effect_owner_1_1 import sha, save, encoded
from audit_smg_effect_frame_configuration import apply_stores
from xbe import Xbe


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT / 'audit.json')
    args = parser.parse_args()
    report = json.loads((OUT / 'measured/report.json').read_text())
    static = json.loads((OUT / 'static/report.json').read_text())
    for name, expected in PRIOR.items():
        assert sha(ROOT / name) == expected, name
    assert report['frozen_pins'] == PRIOR
    assert report['oracle_sha256'] == sha(ROOT / 'tools/oracle_smg_effect_dsp_owner_1_1.py')
    assert report['instruction_patches'] == report['new_game_or_particle_state_inputs'] == []
    files = ['report.json', 'native-clock-inputs.json', 'native-clock-stderr.log'] + [row['file'] for row in report['instructions']]
    for name in files:
        assert (OUT / 'measured' / name).read_bytes() == (OUT / 'independent-repeat' / name).read_bytes(), name
    static_files = ['report.json'] + [row['file'] for row in static['routines']]
    for name in static_files:
        assert (OUT / 'static' / name).read_bytes() == (OUT / 'independent-static' / name).read_bytes(), name
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest() == report['source_xbe_sha256'] == static['source_xbe_sha256']
    executed, original_rows = set(), 0
    for directory, listings in [(OUT / 'measured', report['instructions']), (OUT / 'static', static['routines'])]:
        for listing in listings:
            path = directory / listing['file']
            assert sha(path) == listing['sha256']
            lines = path.read_text().splitlines()
            assert len(lines) == listing['rows']
            for line in lines:
                address, raw, _ = line.split(' ', 2)
                literal = bytes.fromhex(raw)
                assert image.read(int(address, 16), len(literal)) == literal
                original_rows += 1
                if directory.name == 'measured' and listing['file'] == 'instructions/all-executed.asm':
                    executed.add(address)
    metadata = report['verified_compatibility_rtl']
    assert sha(RTL) == metadata['primary_rtl_sha256']
    assert sha(SDK) == metadata['primary_sdk_sha256']
    assert sha(OUT / 'provider/compiled-rtl.cpp') == metadata['generated_source_sha256']
    assert sha(OUT / 'provider/compiled-rtl.bin') == metadata['compiled_text_sha256']
    assert sha(metadata['actual_host_strlen_library']) == metadata['actual_host_strlen_library_sha256']
    binary = bytes.fromhex(metadata['compiled_text_hex'])
    for row in report['outside_source_text']:
        address = int(row['address'], 16)
        if RTL_CODE <= address < RTL_CODE + len(binary):
            assert bytes.fromhex(row['guest_bytes_hex']) == binary[address - RTL_CODE:address - RTL_CODE + row['size']]
    for file, expected in static['primary_source_files'].items():
        assert sha(ROOT / file) == expected, file
    assert report['prior_item_local_birth_and_fixed_phase_fields_identical']
    assert len(report['stages']) == 5
    assert [row['completion'] for row in report['stages']] == ['complete_original_return'] * 2 + ['blocked_original_instruction'] * 3
    driver, startup, loader = report['dsp_records']
    assert driver['entry'] == '002205c0' and driver['authored_argument'] == '00451220'
    assert driver['before']['source_arena517ff0_c_hex'] == '00' * 12
    assert driver['after']['source_arena517ff0_c_hex'][:16] == '00000000d82a0000'
    assert report['stages'][2]['invalid']['instruction'] == '00220611'
    assert report['stages'][2]['invalid']['address'] == '00000000' and report['stages'][2]['invalid']['access'] == 20
    assert startup['entry'] == '00012000'
    failure = report['stages'][3]['invalid']
    assert failure['kernel_ordinal'] == 301 and failure['stack'][0] == 'c0000001'
    assert failure['original_return_site'] == '002d81de'
    assert '0001201a' not in executed and '00012015' not in executed
    assert loader['entry'] == '002dbda1' and loader['authored_argument'] == '0045aea0'
    assert image.read(0x192d39, 5).hex() == '68a0ae4500'
    failure = report['stages'][4]['invalid']
    assert failure['kernel_ordinal'] == 327 and failure['original_return'] == '002dbd6e'
    assert failure['section_header_pointer'] == '00010a38' and failure['actual_section_reference_count'] == 0
    section = static['sections_in_original_header_order'][31]
    assert section['name'] == 'DSPImage' and section['header38_hex'] == failure['section_header38_hex']
    assert failure['already_mapped_actual_section_sha256'] == failure['raw_file_section_sha256'] == section['raw_section_sha256']
    assert failure['head_reference4_hex'] == failure['tail_reference4_hex'] == '00000000'
    assert [row['name'] for row in report['rtl_calls']].count('RtlInitAnsiString') == 33
    assert [row['name'] for row in report['rtl_calls']].count('RtlEqualString') == 32
    comparisons = [row for row in report['rtl_calls'] if row['name'] == 'RtlEqualString']
    assert [row['return_al'] for row in comparisons] == ['00'] * 31 + ['01']
    assert len(report['actual_host_strlen_calls']) == 33
    assert report['actual_host_strlen_calls'][0]['actual_input_nul_bytes_hex'] == '445350496d61676500'
    assert report['actual_host_strlen_calls'][-1]['actual_input_nul_bytes_hex'] == '445350496d61676500'
    assert all(row['actual_host_libc_result'] <= 0xffff for row in report['actual_host_strlen_calls'])
    for call in report['rtl_calls']:
        if call['name'] == 'RtlInitAnsiString':
            delta = int(call['after_cpu']['esp'], 16) - int(call['before_cpu']['esp'], 16)
            assert delta == 12
            assert call['output_descriptor8_hex'][-8:] == int(call['arguments'][1], 16).to_bytes(4, 'little').hex()
        else:
            assert int(call['after_cpu']['esp'], 16) - int(call['before_cpu']['esp'], 16) == 16
            assert call['output_descriptor8_hex'] == call['input_descriptor8_hex']
    replays = []
    for record in report['dsp_records']:
        stores = report['stores'][record['first_store']:record['final_store']]
        for name, address in [('source_arena517ff0_c_hex', 0x517ff0), ('owner_globals52646c_10_hex', 0x52646c), ('owner_globals518478_10_hex', 0x518478), ('directsound_static_critical_section38d158_1c_hex', 0x38d158), ('original_header2000_hex', 0x10000)]:
            actual, count = apply_stores(record['before'][name], address, stores)
            assert actual == record['after'][name], (record['entry'], name)
            replays.append(dict(entry=record['entry'], buffer=name, bytes=len(actual) // 2, overlapping_stores=count, full_bytes_equal=True))
        assert record['before']['current'] == record['after']['current']
        assert record['before']['frame']['frame4f4860_12c0_hex'] == record['after']['frame']['frame4f4860_12c0_hex']
        assert record['before']['frame']['game1200_hex'] == record['after']['frame']['game1200_hex']
    assert all(not row['original_result_supplied'] for row in [report['stages'][2]['invalid'], failure])
    assert report['final_dsp_state']['driver_pointer'] == report['final_dsp_state']['dsp_pointer'] == '00000000'
    assert report['final_dsp_state']['frame']['frame4f4860_12c0_hex'][8:16] == '00000000'
    assert report['final_dsp_state']['frame']['game1200_hex'][0x1120 * 2:0x1120 * 2 + 2] == '00'
    unavailable = {'00192d30', '00381ead', '00383dd7', '00147dd0', '00139640', '001789d0', '00176410', '001764b0', '0024a820', '00249210'}
    assert not unavailable & {row['entry'] for row in report['calls']}
    result = dict(schema='source-smg-effect-dsp-owner-audit-v1', oracle_sha256=report['oracle_sha256'], report_sha256=sha(OUT / 'measured/report.json'), static_sha256=sha(OUT / 'static/report.json'), prior_pins_checked=len(PRIOR), deterministic_dynamic_files_compared=len(files), deterministic_static_files_compared=len(static_files), audited_original_instruction_rows=original_rows, executed_original_addresses=len(executed), compiled_primary_rtl_executed_addresses=len(report['compiled_rtl_executed_addresses']), actual_rtl_calls=len(report['rtl_calls']), actual_host_strlen_calls=len(report['actual_host_strlen_calls']), buffer_replays=replays, source_fields_and_prior_effect_particles_unchanged=True, missing_title_error_mapper_301=True, exact_DSPImage_header_and_327_request=True, no_section_or_DirectSound_or_DSP_or_gameplay_success_supplied=True, limits='No complete constructor, successful section reference/VM ownership, DirectSound device/download, natural outer-frame gameplay admission, effect aging, rendered flash or Xbox hardware claim.')
    save(args.output, encoded(result))
    print(dict(output=str(args.output), sha256=sha(args.output), original_rows=original_rows, files=len(files) + len(static_files)))


if __name__ == '__main__':
    main()
