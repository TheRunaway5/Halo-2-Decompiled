"""Export literal original37A60 caller outputs, without a native oracle."""
from pathlib import Path
import hashlib
import json
import struct

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'analysis/multiplayer-shading/weather-plate-program-admission'
OUT = ROOT / 'analysis/multiplayer-shading/weather-plate-caller-profile'
OUT.mkdir(parents=True, exist_ok=True)
proof_path = SOURCE / 'derived-caller-proof.json'
proof = json.loads(proof_path.read_text())
assert proof['source_XBE_sha256'] == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
assert proof['original36B50_prefix_to37A60']['complete']
assert not proof['source_instructions_modified'] and not proof['SDK_or_external_callees_replaced']
lines = ['# Complete immutable original37A60 over derived actual entry and explicit controls.',
    '#21 input words: compositor13, depth start/end, selection near/far, projection near/far, FPCW, MXCSR.',
    '#65 output words: success, source profile(0=no draw;1=above-near W;2=near unsupported), depth,Z,W,program60.',
    '#False returns use retained output sentinels to represent absent optional output; original retained PS/no quad are independently checked.']
cases = []
for case in proof['cases']:
    assert case['complete'] and not case['external_boundaries']
    entry = next(event for event in case['source_events'] if event['PC'] == '0003A5C0')
    returned = next(event for event in case['source_events'] if event['PC'] == '00037DB6')
    arguments = [int(v, 16) for v in entry['source_arguments']]
    assert arguments[3:8] == [3, 0, 0, 1, 1]
    record = list(struct.unpack('<13I', bytes.fromhex(entry['source_compositor52_hex'])))
    banks = [int(v, 16) for v in entry['near_far_projection_words']]
    inputs = [*record, *arguments[1:3], *banks, int(entry['source_FPCW'], 16), int(entry['source_MXCSR'], 16)]
    success = int(returned['registers']['EAX'], 16) & 255
    positions = [event['position_XYZW_words'] for event in case['source_events'] if event['PC'] == '003FB2D0']
    if success:
        assert len(positions) == 4 and all(position[2:] == positions[0][2:] for position in positions)
        depth = int(returned['depth_word'], 16)
        profile = 1 if case['active_emitted_method_values_before_first_position']['030C'] == '00000001' else 2
        outputs = [1, profile, depth, *[int(v, 16) for v in positions[0][2:]], *[int(v, 16) for v in returned['program_words']]]
    else:
        assert not positions
        profile = 0
        before = list(struct.unpack('<60I', (SOURCE / 'derived37A60-entry-program_before.bin').read_bytes()))
        assert [int(v, 16) for v in case['final_program_words']] == before
        outputs = [0, 0, *([0xdeadbeef] * 3), *range(0xc0de0000, 0xc0de003c)]
    assert len(inputs) == 21 and len(outputs) == 65
    lines.extend(['# ' + case['label'], ' '.join(f'{v:08X}' for v in inputs) + ' | ' + ' '.join(f'{v:08X}' for v in outputs)])
    cases.append({'label': case['label'], 'actual_derived_entry': case['actual_entry_unchanged'],
        'source_controls': case['source_input_controls'], 'source_profile': profile,
        'complete_original_caller': True, 'full_original_output_words': 65,
        'false_optional_output_sentinels_not_original_stack_values': not bool(success)})
fixture = ROOT / 'tests/fixtures/retail_source_weather_plate_caller.tsv'
fixture.write_text('\n'.join(lines) + '\n')
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
report = {'scope': __doc__, 'tool_sha256': sha(Path(__file__)), 'original_caller_proof_sha256': sha(proof_path),
    'fixture': str(fixture.relative_to(ROOT)), 'fixture_sha256': sha(fixture), 'cases': cases,
    'source_instructions_replaced': [], 'native_outputs_used_as_oracle': False, 'GPU_or_guest_execution': False}
(OUT / 'fixture-provenance.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({'cases': len(cases), 'fixture_sha256': sha(fixture)}, indent=2))
