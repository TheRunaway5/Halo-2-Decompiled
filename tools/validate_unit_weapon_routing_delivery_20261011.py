#!/usr/bin/env python3
"""Pin the coherent build and bounded weapon-routing evidence, with live limits."""
import datetime
import hashlib
import json
import re
import subprocess
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT/'build-weapon-routing-20261011'
SNAP = ROOT/'.local/weapon-routing-source-20261011'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def artifact(path):
    return dict(path=str(path), sha256=sha(path))

def main():
    manifest = SNAP/'source-manifest-routing.json'
    captured = json.loads(manifest.read_text())
    mismatches = [p for p, pin in captured['files'].items() if sha(SNAP/p) != pin]
    assert not mismatches, mismatches
    generated = json.loads((SNAP/'tests/fixtures/unit_weapon_routing_manifest.json').read_text())
    assert generated['generated_cpp_sha256'] == sha(SNAP/'src/gameplay/weapons/unit_weapon_routing.cpp')
    assert len(generated['complete_native_entries']) == 57
    assert len(generated['lowered_instructions']) == 5093
    assert len(generated['external_owners']) == 101
    assert not generated['production_opcode_decoder']
    assert not generated['production_original_code_execution']
    assert not generated['captured_runtime_results_used']
    assert generated == json.loads((BUILD/'unit_weapon_routing_regenerated.manifest.json').read_text())
    assert generated['generated_cpp_sha256'] == sha(BUILD/'unit_weapon_routing_regenerated.cpp')
    original = json.loads((SNAP/'tests/fixtures/unit_weapon_routing_original.provenance.json').read_text())
    cases = original['cases']
    assert len(cases) == 165
    stops = Counter(c['stopped_owner'] for c in cases if c['stopped_owner'] is not None)
    assert sum(stops.values()) == 149
    assert sum(c['stores'] for c in cases) == 2659
    assert len(original['tested_native_instruction_addresses']) == 831
    assert not original['captured_lower_outputs_used']
    assert all(not c['external_calls'] for c in cases)
    lookup = json.loads((SNAP/'tests/fixtures/unit_action_lookup_original.provenance.json').read_text())
    assert (lookup['queries'], lookup['mapped_queries'], lookup['valid_queries']) == (1584, 36, 258)
    fixtures = []
    for name, provenance in (('unit_weapon_routing', original), ('unit_action_lookup', lookup)):
        fixture = SNAP/'tests/fixtures'/f'{name}_original.tsv'
        repeat = BUILD/f'{name}_repeat.tsv'
        assert sha(fixture) == provenance['fixture_sha256'] == sha(repeat)
        fixtures.append(dict(fixture=artifact(fixture), repeat=artifact(repeat)))
    versions = json.loads((BUILD/'version-correspondence/report.json').read_text())
    assert versions['paired'] == versions['requested'] == 62
    assert versions['tool_sha256'] == sha(SNAP/'tools/research_unit_weapon_routing_u5.py')
    assert all(len(r['full_body_candidates']) == 1 for r in versions['owners'])
    for p, pin in versions['source_pins'].items():
        assert sha(Path(p)) == pin
    assert json.loads((BUILD/'lookup-red-green.json').read_text())['red_exit'] == 1
    tests = []
    for name, log, needle in (
        ('six source and native regressions', 'weapon-routing-focused-tests.log', '100% tests passed out of 6'),
        ('two pickup and inventory regressions', 'weapon-routing-pickup-regressions.log', '100% tests passed out of 2'),
        ('all live Cache query results', 'weapon-routing-lookup-detailed.log', '1584 original authored action queries, 36 mapped class queries, 1326 genuine unavailable actions'),
        ('previous live query regression control', 'weapon-routing-lookup-red.log', 'Live authored action differs from original graph query at row 936'),
        ('ASAN UBSAN leak and failure ABI checks', 'weapon-routing-sanitizer.log', '149 absent-owner rejections, 149 partial-owner ABI failure controls'),
        ('bounded current-header compatibility check', 'weapon-routing-current-headers-tests.log', '149 absent-owner rejections, 149 partial-owner ABI failure controls'),
        ('actual SDL dual pickup firing reload throw and offhand drop', 'weapon-routing-dual-runtime.log', 'PASS actual SDL dual pickup, two independently firing/reloading weapon datums'),
        ('actual SDL four-view grenade sequence', 'weapon-routing-grenade-runtime.log', 'PASS authored grenade world geometry submitted independently to every local view'),
        ('shipping runtime TU1.5 with Vulkan validation', 'weapon-routing-shipping-runtime.log', 'Multiplayer frames=10')):
        path = ROOT/'build'/log
        text = path.read_text()
        assert needle in text, (name, text[-2000:])
        assert not re.search(r'Validation (?:Error|Warning)|VUID-|ERROR: AddressSanitizer|runtime error:', text), name
        tests.append(dict(name=name, passed=True, log=artifact(path)))
    assert 'split_views=4' in (ROOT/'build/weapon-routing-shipping-runtime.log').read_text()
    assert 'views=4' in (ROOT/'build/weapon-routing-grenade-runtime.log').read_text()
    launchers, shared = [], []
    for name in ('Halo2-Launcher', 'Halo2-Title-Launcher'):
        for folder, results, own in ((BUILD, launchers, True), (ROOT, shared, False)):
            path = folder/name
            info = subprocess.check_output([str(path), '--launcher-info'], text=True)
            selected = re.search(r'^Runtime: (.+)$', info, re.MULTILINE)
            assert selected, info
            runtime = Path(selected.group(1))
            assert runtime.is_file()
            if own:
                assert runtime == BUILD/'halo2_native', info
            out = BUILD/(name+('-info.txt' if own else '-shared-info.txt'))
            out.write_text(info)
            results.append(dict(launcher=artifact(path), info=artifact(out),
                                selected_runtime=artifact(runtime), validated_by_this_report=own))
    report = dict(
        recorded_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        complete_original_gameplay_or_rendered_parity=False,
        live_complete_admission_drop_routing=False, live_original_transition_manager=False,
        live_original_third_person_layer_composition=False, live_original_Havok=False,
        source_snapshot=str(SNAP), source_manifest=artifact(manifest), captured_files=len(captured['files']),
        manifest_mismatches=mismatches, validator=artifact(Path(__file__)),
        generated_manifest=artifact(SNAP/'tests/fixtures/unit_weapon_routing_manifest.json'),
        regenerated_source=artifact(BUILD/'unit_weapon_routing_regenerated.cpp'),
        original_fixtures=fixtures, complete_compiled_bodies=57, new_compiled_bodies=31,
        complete_compiled_instructions=5093, new_compiled_instructions=2949,
        original_call_cases=165, returned_original_cases=16, required_owner_stops=149,
        stopped_owner_counts=dict(stops), ordered_stores=2659, reached_unique_instructions=831,
        successful_lower_results_supplied=0, differential_lower_calls_replayed=0,
        absent_owner_rejections=149, partial_owner_failure_ABI_controls=149,
        original_lookup_queries=1584, mapped_class_queries=36, available_queries=258,
        genuine_unavailable_queries=1326, version_correspondence=artifact(BUILD/'version-correspondence/report.json'),
        lookup_regression_control=artifact(BUILD/'lookup-red-green.json'), tests=tests,
        runtime=artifact(BUILD/'halo2_native'),
        binaries=[artifact(BUILD/p) for p in ('unit_weapon_routing_tests', 'unit_weapon_routing_sanitized',
                  'unit_action_lookup_tests', 'unit_action_lookup_previous', 'native_weapon_runtime_tests',
                  'native_weapon_gameplay_tests', 'multiplayer_grenade_runtime_tests',
                  'unit_weapon_routing_current_headers')],
        build_configuration=artifact(BUILD/'CMakeCache.txt'), build_log=artifact(ROOT/'build/weapon-routing-build.log'),
        final_build_log=artifact(ROOT/'build/weapon-routing-final-build.log'),
        current_header_observation=[artifact(ROOT/p) for p in ('src/physics/runtime/physics_contact_runtime_internal.hpp',
                                    'include/halo2/physics_contact_runtime.hpp')],
        whole_concurrent_checkout_freshness_claimed=False,
        preserved_social_sdk=artifact(BUILD/'libdiscord_partner_sdk.so'),
        social_sdk_notices=artifact(BUILD/'Discord-Social-SDK-Notices.txt'), launchers=launchers,
        shared_launchers_observed_without_replacement=shared,
        remaining=['Live source construction/allocator/registration and actual body/manager/extension/current-saved locals',
                   'Full original transitions, third-person primary/secondary firing and reload layer composition',
                   'Complete capability/admission/inventory/drop routing on those live banks',
                   'Live source BSP/body/controller/Havok world integration and Xbox CPU estimate proof',
                   'Grenade angular/contact/particle and non-SMG charge/heat/lunge behavior',
                   'Matched original rendered/action sequences across weapons/maps/characters/split-screen/LAN'])
    output = BUILD/'weapon-routing-validation.json'
    output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(tests=len(tests), cases=165, captured_files=len(captured['files']),
                          manifest_mismatches=0, validation_sha256=sha(output),
                          complete_original_gameplay_or_rendered_parity=False)))

if __name__ == '__main__':
    main()
