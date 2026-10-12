#!/usr/bin/env python3
"""Audit source consumers, unchanged stops, and independent ownership traces."""
import hashlib
import json
from oracle_smg_audio_ownership_1_1 import ROOT, OUT, FROZEN, PIN, CACHE_PIN, digest
from oracle_smg_firing_1_1 import Xbe

ORACLE_PIN = 'dc006540ccf04c6a66b6793ba3faf36de107ac3f3e2544a38be16a23a2afc36f'
INSPECTOR_PIN = 'edf8d5a044f5ee0ae2410e8873cc493b26e9ef31e71b50c938e5b4a7649d9a38'
SDL_PIN = '2079364cf9548e37eed2937e01a22a8c190ffd3bf283c50f669310a122654ae9'
SHARED_PIN = '084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af'


def word(raw, offset, size=4):
    return int.from_bytes(raw[offset:offset+size], 'little')


def main():
    assert digest(ROOT/'tools/oracle_smg_audio_ownership_1_1.py') == ORACLE_PIN
    assert digest(ROOT/'tools/inspect_smg_audio_host_contract_1_1.py') == INSPECTOR_PIN
    assert digest(ROOT/'tools/probe_sdl_audio_capabilities.py') == SDL_PIN
    for path, pin in FROZEN.items():
        assert digest(ROOT/path) == pin, path
    suite = json.loads((OUT/'frozen/suite.json').read_text())
    for row in suite['artifacts']:
        assert digest(ROOT/row['path']) == row['sha256'], row['path']
    source = Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(source.data).hexdigest() == PIN
    assert digest(ROOT/'assets/disc/maps/lockout.map') == CACHE_PIN
    shared = ROOT/'assets/disc/maps/shared.map'
    assert digest(shared) == SHARED_PIN
    measured_path = OUT/'measured/ownership.json'
    repeat_path = OUT/'reexecution/ownership.json'
    assert measured_path.read_bytes() == repeat_path.read_bytes()
    report = json.loads(measured_path.read_text())
    assert report['oracle_sha256'] == ORACLE_PIN
    assert report['source_sha256'] == PIN and report['cache_sha256'] == CACHE_PIN
    assert report['frozen_prior_pins'] == FROZEN
    assert report['direct_kernel_call_sites_supplied']
    assert not report['new_source_available_or_device_result_supplied']
    assert not report['new_source_owner_pool_fields_supplied']
    assert not report['complete_normal_audio_lifecycle_proven']
    assert not report['native_gameplay_update_applied']
    callbacks = finalizers = reads = read_bytes = 0
    for case in report['cases']:
        before, after = case['before'], case['after']
        creation = case['creation']
        assert creation['actual_rejected_fp_request']['return_eax'] == 'ffffffff'
        assert creation['created_original_source_datum'] == 'ef730000'
        assert creation['creation_kernel_call_site_supplied']
        assert creation['ordinary_available_admission_or_playback_not_claimed']
        source_before = bytes.fromhex(before['source_bc_hex'])
        assert word(source_before, 0xA8) == 0xFFFFFFFF and word(source_before, 0xAC, 2) == 0xFFFF
        assert word(source_before, 0x14) == 0x44B14C
        assert word(source.read(0x44B14C, 32), 0x10) == 0x18E2F0
        assert source_before[4] & 2
        for state in [before, *(row['after'] for row in case['phases']), after]:
            assert state['audio_initialized'] == 0 and state['driver_pointer'] == '00000000'
            assert state['source_owner_pool_pointer'] == '00000000'
            assert state['source_owner_allocator_receiver'] == '00000000'
            assert state['original_source_callback_table_hex'] == source.read(0x44B14C, 32).hex()
            assert state['shared_seed'] == before['shared_seed']
            assert state['local_seed'] == before['local_seed']
        assert [row['instruction'] for row in case['inherited_constructor_boundaries']] == ['00336983', '00337a45', '002d8606']
        assert all(not row['platform_result_supplied'] for row in case['inherited_constructor_boundaries'])
        pin_before = bytes.fromhex(before['selected_chunk']['cache_entry_10_hex'])
        pin_after = bytes.fromhex(after['selected_chunk']['cache_entry_10_hex'])
        assert pin_before[4] == 1 and pin_after[4] == 0
        header_before = bytes.fromhex(before['source_pool']['header_4c_hex'])
        header_after = bytes.fromhex(after['source_pool']['header_4c_hex'])
        assert [word(header_before, i) for i in (0x34, 0x38, 0x3C)] == [1, 1, 1]
        assert bytes.fromhex(before['source_pool']['bitmap_hex'])[0] == 1
        assert not case['source_constructor_owner_voice_or_available_result_supplied']
        assert not case['complete_source_removal_claimed']
        assert not case['full_normal_scheduler_or_platform_playback_claimed']
        if case['callback_stop']:
            callbacks += 1
            stop = case['callback_stop']
            assert stop['instruction'] == '0018e2f5' and stop['instruction_bytes'] == source.read(0x18E2F5, 3).hex() == '8a4829'
            assert stop['owner_pool_pointer'] == '00000000' and not stop['result_or_owner_field_supplied']
            assert [row['entry'] for row in case['calls']] == ['00128a70', '001280b0', '0018e2f0']
            assert case['calls'][-1]['stack'] == ['00010002', 'ef730000', '00000003']
            assert header_before == header_after
            assert before['source_pool']['bitmap_hex'] == after['source_pool']['bitmap_hex']
            assert not case['phases'][0]['completed'] and not case['direct_finalizer_omits_owner_callback']
            assert word(bytes.fromhex(after['source_bc_hex']), 0, 2) == 0xEF73
            assert len(case['stores']) > 0
            assert [row['instruction'] for row in case['stores'] if not row['address'].startswith('021')] == ['0012812d', '00128130']
        else:
            finalizers += 1
            assert case['direct_finalizer_omits_owner_callback']
            assert [row['entry'] for row in case['calls']] == ['001280b0', '00128ae0', '0016d4c0']
            assert [word(header_after, i) for i in (0x34, 0x38, 0x3C)] == [0, 0, 0]
            assert bytes.fromhex(after['source_pool']['bitmap_hex']) == bytes(len(bytes.fromhex(after['source_pool']['bitmap_hex'])))
            assert word(bytes.fromhex(after['source_bc_hex']), 0, 2) == 0
            assert all(row.get('completed', True) for row in case['phases'])
            if not case['initial_resource_loaded']:
                assert case['phases'][-1]['entry'] == '00122360'
                assert before['globals']['004e9ebc'] == '004e7938'
                assert after['globals']['004e9ebc'] == '00000000'
                completion = [row for row in case['stores'] if row['instruction'] == '001223ac']
                assert len(completion) == 1 and completion[0]['size'] == 1
                assert completion[0]['before_hex'] == '00' and completion[0]['after_hex'] == '01'
        assert not bytes.fromhex(after['source_bc_hex'])[4] & 2
        for row in case['provider_requests']:
            assert row['real_host_service'] and not row['source_constructor_result_fabricated']
            if row['provider'] == 'ReadFile':
                reads += 1
                assert row['completion_flag_not_written_by_provider']
                assert row['file_offset'] % 512 == 0 and row['bytes_read'] % 512 == 0
                with shared.open('rb') as stream:
                    stream.seek(row['file_offset'])
                    raw = stream.read(row['bytes_read'])
                assert hashlib.sha256(raw).hexdigest() == row['raw_sha256']
                read_bytes += len(raw)
        if pin_after[2]:
            chunk = bytes.fromhex(after['selected_chunk']['runtime_0c_hex'])
            buffer = after['selected_chunk']['buffer']
            with shared.open('rb') as stream:
                stream.seek(word(chunk, 0) & 0x3FFFFFFF)
                raw = stream.read(buffer['aligned_bytes'])
            assert hashlib.sha256(raw).hexdigest() == buffer['aligned_sha256']
            assert hashlib.sha256(raw[:buffer['authored_bytes']]).hexdigest() == buffer['authored_sha256']
    assert callbacks == finalizers == 2 and reads == 3
    rows = listings = 0
    observed_stop_seen = False
    for report_path in (measured_path, repeat_path):
        original = json.loads(report_path.read_text())
        for listing in [*original['instruction_listings'], original['executed_original_listing']]:
            path = report_path.parent/'instructions'/listing['file']
            assert digest(path) == listing['sha256']
            for line in path.read_text().splitlines():
                address, encoded, *_ = line.split()
                raw = bytes.fromhex(encoded)
                assert source.read(int(address, 16), len(raw)) == raw
                if listing is original['executed_original_listing'] and address == '0018e2f5':
                    observed_stop_seen = True
                if report_path == measured_path:
                    rows += 1
            if report_path == measured_path:
                listings += 1
    assert not observed_stop_seen
    inspection_path = OUT/'source-inspection.json'
    inspection = json.loads(inspection_path.read_text())
    assert inspection['inspector_sha256'] == INSPECTOR_PIN and inspection['source_sha256'] == PIN
    assert not inspection['new_original_device_execution_or_settings_result_supplied']
    assert not inspection['retail_Xbox_saved_audio_or_AV_pack_state_known']
    assert inspection['SDL_preference_or_output_format_is_not_original_DSCAPS']
    assert inspection['original_import_slots'] == [
        {'slot': '00417c50', 'original_raw_hex': '18000080', 'ordinal': 24,
         'primary_export_symbol': 'ExQueryNonVolatileSetting', 'runtime_service_result_supplied': False},
        {'slot': '00417ce4', 'original_raw_hex': '64010080', 'ordinal': 356,
         'primary_export_symbol': 'HalBootSMCVideoMode', 'runtime_service_result_supplied': False}]
    frozen = json.loads((ROOT/'analysis/multiplayer/map-pack/smg-audio-pool-prefix/source-fixture.json').read_text())
    stop = inspection['unchanged_frozen_settings_stop']
    assert stop == frozen['source_device_prefix']['first_unprovided_dependency']
    assert stop['kernel_ordinal'] == 24 and stop['unprovided_nonvolatile_query']['ValueIndex'] == '00000009'
    assert not stop['unprovided_nonvolatile_query']['source_nonvolatile_settings_value_or_return_supplied']
    for listing in inspection['instruction_listings']:
        path = inspection_path.parent/'instructions'/listing['file']
        assert digest(path) == listing['sha256'] and listing['source_inspection_only']
        for line in path.read_text().splitlines():
            address, encoded, *_ = line.split()
            raw = bytes.fromhex(encoded)
            assert source.read(int(address, 16), len(raw)) == raw
            rows += 1
        listings += 1
    for row in inspection['primary_compatibility_references']:
        assert digest(ROOT/row['path']) == row['sha256']
    default_probes = 0
    for name in ('sdl-measured.json', 'sdl-reexecution.json'):
        probe = json.loads((OUT/name).read_text())
        assert probe['tool_sha256'] == SDL_PIN and not probe['provider_forced']
        assert not probe['original_settings_caps_constructor_or_playback_result_supplied']
        assert not probe['Xbox_settings_or_APU_resource_counts_obtained']
        assert not probe['physical_DAC_format_latency_or_consumption_proven']
        assert not probe['audible_playback_tested']
        assert probe['actual_init_return'] == 0 and probe['actual_driver'] == 'pipewire'
        assert probe['actual_default_info']['actual_return'] == 0
        assert probe['actual_default_info']['provider_preferred_spec'] == {'freq': 48000, 'format': 0x8010, 'channels': 2}
        assert [row['allowed_changes'] for row in probe['paused_open_cases']] == [0, 15]
        for row in probe['paused_open_cases']:
            assert row['actual_device_id'] != 0 and row['actual_status'] == 2 and row['actual_queued_bytes'] == 0
            assert row['actual_closed'] and not row['queued_or_unpaused']
            assert row['obtained_application_spec']['format'] == 0x8120
            default_probes += 1
    fixture = {'source_sha256': PIN, 'cache_sha256': CACHE_PIN, 'shared_sha256': SHARED_PIN,
        'suite_sha256': digest(OUT/'frozen/suite.json'), 'oracle_sha256': ORACLE_PIN,
        'source_settings_and_capability_inspection': inspection,
        'independently_reexecuted_original_ownership_cases': report['cases'],
        'instruction_listings': report['instruction_listings'], 'executed_original_listing': report['executed_original_listing'],
        'ordinary_scheduler_calls_device_settings_voice_or_playback_not_proven': True,
        'native_gameplay_update_applied': False}
    path = OUT/'source-fixture.json'
    path.write_text(json.dumps(fixture, indent=2)+'\n')
    audit = {'source_fixture_sha256': digest(path), 'suite_sha256': digest(OUT/'frozen/suite.json'),
        'independent_original_ownership_cases': 4, 'full_owner_callback_stops': callbacks,
        'direct_unpin_and_finalizer_cases': finalizers, 'aligned_file_read_callbacks_verified': reads,
        'aligned_file_bytes_verified': read_bytes, 'original_instruction_listings_verified': listings,
        'original_instruction_rows_verified': rows,
        'unique_executed_ownership_source_instructions': report['executed_original_listing']['instructions'],
        'original_settings_import_ordinal': 24, 'original_boot_AV_pack_import_ordinal': 356,
        'real_paused_SDL_device_open_queries': default_probes, 'earlier_frozen_pins_verified': FROZEN,
        'Xbox_saved_setting_or_device_capability_result_supplied': False,
        'full_original_audio_initialization_or_playback_proven': False, 'native_gameplay_update_applied': False}
    (OUT/'audit.json').write_text(json.dumps(audit, indent=2)+'\n')
    print(json.dumps({key: value for key, value in audit.items() if key != 'earlier_frozen_pins_verified'}, indent=2))


if __name__ == '__main__':
    main()
