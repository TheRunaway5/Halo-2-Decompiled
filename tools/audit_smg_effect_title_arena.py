#!/usr/bin/env python3
"""Audit primary/native301 and exact original title TLS receiver stop."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from oracle_smg_effect_title_arena_1_1 import ROOT, OUT, OLD_OUT, PRIOR
from oracle_smg_effect_owner_1_1 import sha, save, encoded
from audit_smg_effect_frame_configuration import apply_stores
from xbe import Xbe


def pe_export(path, name):
    """Read the actual named export from its pinned PE32 file, without loading."""
    raw = path.read_bytes()
    assert raw[:2] == b'MZ'
    pe, = struct.unpack_from('<I', raw, 0x3c)
    assert raw[pe:pe + 4] == b'PE\0\0'
    machine, count = struct.unpack_from('<HH', raw, pe + 4)
    assert machine == 0x14c
    optional_size, = struct.unpack_from('<H', raw, pe + 20)
    optional = pe + 24
    assert struct.unpack_from('<H', raw, optional)[0] == 0x10b
    sections = []
    for index in range(count):
        offset = optional + optional_size + index * 40
        vsize, rva, rsize, file_offset = struct.unpack_from('<4I', raw, offset + 8)
        sections.append((rva, max(vsize, rsize), file_offset, rsize))

    def offset(rva):
        for base, size, file_offset, rsize in sections:
            if base <= rva < base + size:
                assert rva - base < rsize
                return file_offset + rva - base
        assert rva < struct.unpack_from('<I', raw, optional + 60)[0]
        return rva

    export_rva, export_size = struct.unpack_from('<II', raw, optional + 96)
    export = offset(export_rva)
    functions, names, function_table, name_table, ordinal_table = struct.unpack_from('<5I', raw, export + 20)
    for index in range(names):
        pointer, = struct.unpack_from('<I', raw, offset(name_table) + index * 4)
        start = offset(pointer)
        current = raw[start:raw.index(b'\0', start)].decode('ascii')
        if current == name:
            ordinal, = struct.unpack_from('<H', raw, offset(ordinal_table) + index * 2)
            assert ordinal < functions
            rva, = struct.unpack_from('<I', raw, offset(function_table) + ordinal * 4)
            assert not export_rva <= rva < export_rva + export_size
            start = offset(rva)
            return dict(name=name, machine=machine, export_rva=f'{rva:08x}', file_offset=start, file16_hex=raw[start:start + 16].hex(), pe_file_sha256=sha(path))
    raise AssertionError(name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT / 'audit-v2.json')
    args = parser.parse_args()
    report = json.loads((OUT / 'measured/report.json').read_text())
    old = json.loads((OLD_OUT / 'measured/report.json').read_text())
    old_manifest = json.loads((OLD_OUT / 'manifest.json').read_text())
    for path, expected in PRIOR.items():
        assert sha(ROOT / path) == expected, path
    for path, expected in old_manifest['files'].items():
        assert sha(ROOT / path) == expected, path
    assert report['frozen_pins'] == PRIOR
    assert report['oracle_sha256'] == sha(ROOT / 'tools/oracle_smg_effect_title_arena_1_1.py')
    assert report['instruction_patches'] == report['new_game_or_particle_state_inputs'] == []
    files = ['report.json', 'native-clock-inputs.json', 'native-clock-stderr.log'] + [row['file'] for row in report['instructions']]
    for name in files:
        assert (OUT / 'measured' / name).read_bytes() == (OUT / 'independent-repeat' / name).read_bytes(), name
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest() == report['source_xbe_sha256']
    rows, executed = 0, set()
    for listing in report['instructions']:
        path = OUT / 'measured' / listing['file']
        assert sha(path) == listing['sha256']
        lines = path.read_text().splitlines()
        assert len(lines) == listing['rows']
        for line in lines:
            address, literal, _ = line.split(' ', 2)
            raw = bytes.fromhex(literal)
            assert image.read(int(address, 16), len(raw)) == raw
            rows += 1
            if listing['file'] == 'instructions/all-executed.asm':
                executed.add(address)
    assert report['stages'][:3] == old['stages'][:3]
    assert report['configuration_records'] == old['configuration_records']
    assert report['dsp_records'][0] == old['dsp_records'][0]
    mapper, = report['actual_error_mapper_calls']
    assert report['actual_error_mapper_observations'] == [dict(actual_status32=0xc0000001, actual_dos_error32=31)]
    assert mapper['kernel_ordinal'] == 301 and mapper['original_return'] == '002d81de'
    assert mapper['actual_status_word'] == 'c0000001' and mapper['actual_native_return_word'] == '0000001f'
    assert mapper['after_cpu']['registers']['eax'] == '0000001f'
    for name, value in mapper['before_cpu']['registers'].items():
        if name != 'eax':
            assert mapper['after_cpu']['registers'][name] == value
    assert int(mapper['after_cpu']['esp'], 16) - int(mapper['before_cpu']['esp'], 16) == 8
    contract = report['verified_error_mapper_contract']
    assert sha(ROOT / 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/Cxbx-Reloaded-EmuKrnlRtl.cpp') == contract['primary_rtl_sha256']
    assert sha(ROOT / 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h') == contract['primary_sdk_sha256']
    assert sha(OUT / 'provider/compiled-error-mapper.cpp') == contract['generated_source_sha256']
    assert sha(OUT / 'provider/compiled-error-mapper.exe') == contract['compiled_executable_sha256']
    export = pe_export(Path(contract['native_ntdll_file']), 'RtlNtStatusToDosError')
    assert export['pe_file_sha256'] == contract['native_ntdll_sha256']
    assert export['file16_hex'] == report['actual_error_mapper_module']['actual_native_export16_hex']
    startup = report['stages'][3]
    assert startup['stopped_eip'] == startup['invalid']['instruction'] == '002d81cb'
    assert startup['invalid']['address'] == '00000287' and startup['invalid']['access'] == 20 and startup['invalid']['size'] == 4
    assert not startup['invalid']['original_result_supplied']
    source_reads = [row for row in report['reads'] if '002d81a6' <= row['instruction'] <= '002d81cb']
    assert [(row['instruction'], row['address'], row['size'], row['raw_hex']) for row in source_reads] == [
        ('002d81a6', '00ac0024', 1, '00'), ('002d81b2', '00ac0028', 4, '0000ad00'),
        ('002d81b8', '00ac0004', 4, 'b0fcab00'), ('002d81bf', '00487620', 4, '00000000'),
        ('002d81c4', '00abfcb0', 4, '7f020000'), ('002d81c7', '021fcdac', 4, '1f000000')]
    assert startup['invalid']['cpu']['registers']['eax'] == '0000027f'
    assert startup['invalid']['cpu']['registers']['ecx'] == '0000001f'
    assert '00012015' not in executed and '0001201a' not in executed
    assert not {'00192d30', '00381ead', '00383dd7', '0033785c', '00147dd0', '00139640', '001789d0'} & {row['entry'] for row in report['calls']}
    assert len(report['rtl_calls']) == 65 and len(report['actual_host_strlen_calls']) == 33
    def rtl_inputs(rows):
        return [dict(name=row['name'], arguments=row['arguments'], input_descriptor8_hex=row['input_descriptor8_hex'], output_descriptor8_hex=row['output_descriptor8_hex'], second_descriptor8_hex=row.get('second_descriptor8_hex'), return_al=row['return_al']) for row in rows]
    assert rtl_inputs(report['rtl_calls']) == rtl_inputs(old['rtl_calls'])
    assert report['stages'][-1]['invalid'] == old['stages'][-1]['invalid']
    buffers = []
    for record in report['dsp_records']:
        stores = report['stores'][record['first_store']:record['final_store']]
        for name, address in [('source_arena517ff0_c_hex', 0x517ff0), ('owner_globals52646c_10_hex', 0x52646c), ('owner_globals518478_10_hex', 0x518478), ('directsound_static_critical_section38d158_1c_hex', 0x38d158), ('original_header2000_hex', 0x10000)]:
            actual, count = apply_stores(record['before'][name], address, stores)
            assert actual == record['after'][name]
            buffers.append(dict(entry=record['entry'], buffer=name, bytes=len(actual) // 2, overlapping_stores=count, full_bytes_equal=True))
        assert record['before']['current'] == record['after']['current']
        assert record['before']['frame']['game1200_hex'] == record['after']['frame']['game1200_hex']
        assert record['before']['frame']['frame4f4860_12c0_hex'] == record['after']['frame']['frame4f4860_12c0_hex']
    assert report['prior_item_local_birth_and_fixed_phase_fields_identical']
    assert report['final_dsp_state']['source_arena517ff0_c_hex'][:8] == report['final_dsp_state']['driver_pointer'] == report['final_dsp_state']['dsp_pointer'] == '00000000'
    assert report['final_dsp_state']['frame']['frame4f4860_12c0_hex'][8:16] == '00000000'
    assert report['final_dsp_state']['frame']['game1200_hex'][0x1120 * 2:0x1120 * 2 + 2] == '00'
    tls_directory, = struct.unpack_from('<I', image.data, 0x12c)
    assert tls_directory == 0x418070
    tls_raw = image.read(tls_directory, 24)
    assert struct.unpack('<6I', tls_raw) == (0, 0, 0x487620, 0, 0x90, 0)
    owner_listing = ROOT / 'analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/002d6f2e.asm'
    for line in owner_listing.read_text().splitlines():
        address, raw, _ = line.split(' ', 2)
        literal = bytes.fromhex(raw)
        assert image.read(int(address, 16), len(literal)) == literal
    assert image.entry == 0x2d6f2e
    assert '002d6f66' not in executed and '002d6f80' not in executed
    tls_owner = dict(original_entry='002d6f2e', original_directory_pointer='00418070', directory24_hex=tls_raw.hex(), tls_index_address='00487620', zero_fill_word='00000090', original_size_store='002d6f66', original_size_store_bytes_hex=image.read(0x2d6f66, 5).hex(), original_index_store='002d6f80', original_index_store_bytes_hex=image.read(0x2d6f80, 2).hex(), complete_prior_owner_listing=str(owner_listing.relative_to(ROOT)), listing_sha256=sha(owner_listing), scope='Literal original TLS/entry bytes, listed only in this suite. Source layout/index initialization and actual title-thread TLS allocation are not inferred as executed outputs.')
    result = dict(schema='source-smg-effect-title-arena-audit-v2', report_sha256=sha(OUT / 'measured/report.json'), oracle_sha256=report['oracle_sha256'], prior_pins_checked=len(PRIOR), full_previous_manifest_files_checked=len(old_manifest['files']), deterministic_files_compared=len(files), original_instruction_rows=rows, executed_original_addresses=len(executed), native_mapper_export_file_correspondence=export, actual_error_mapping=report['actual_error_mapper_observations'], exact_original_TLS_receiver_reads=source_reads, listed_original_TLS_owner=tls_owner, full_buffer_replays=buffers, prior_driver_configuration_and_effect_fields_preserved=True, prior_DSPImage_loader_inputs_outputs_and_final327_stop_equal=True, no_title_TLS_or_arena_or_audio_or_DSP_or_gameplay_success_written=True, limits='Actual primary/native301 continuation stops at incompatible/uninitialized ordinary XAPI TLS slot receiver, before12015 VirtualAlloc. No whole title startup, arena allocation, device/DSP, game admission, elapsed/aging or rendered flash.')
    save(args.output, encoded(result))
    print(dict(output=str(args.output), sha256=sha(args.output), files=len(files), original_rows=rows))


if __name__ == '__main__':
    main()
