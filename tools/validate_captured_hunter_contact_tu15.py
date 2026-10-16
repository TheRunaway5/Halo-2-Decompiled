"""Authenticate the actual unresolved D1878 ->18B4C0 public-world caller."""
import json
import struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def validate(case, metadata, regions):
    assert metadata['entry'] == 0x18b4c0 and metadata['public_campaign_placement']
    assert metadata['ordinal'] == 1
    build = json.loads((case / 'build.json').read_text())
    completed = metadata['completed']
    assert build['returncode'] == (0 if completed else 1)
    assert build['scope']['actual_completed_object_material_caller' if completed else 'actual_unresolved_callee_observer']
    assert file_sha(Path(build['source'])) == build['source_sha256']
    assert file_sha(Path(build['command'][0])) == build['binary_sha256']
    assert file_sha(Path(build['command'][1])) == NEW_PIN
    for name, digest in build['linked_files_sha256'].items():
        assert file_sha(Path(name)) == digest
    for name in ('before.bin', 'native-input.bin', 'native-input-full.bin',
                 'native-fp-input.bin', 'native.json'):
        assert file_sha(case / name) == build['files_sha256'][name]
    if completed:
        for name in ('after.bin', 'native-exit.bin'):
            assert file_sha(case / name) == build['files_sha256'][name]
    proof_path = case.parents[1] / ('hunter-material-world-v1.json' if completed else 'hunter-unresolved-world-observer-v1.json')
    proof = json.loads(proof_path.read_text())
    overrides = list(build['override_sources'].values())
    candidate = proof['candidate_source' if completed else 'diagnostic_source']
    candidate_sha = proof['candidate_sha256' if completed else 'diagnostic_sha256']
    assert any(item['source'] == candidate and item['source_sha256'] == candidate_sha for item in overrides)
    assert file_sha(Path(proof['original_source'])) == proof['original_sha256']
    assert file_sha(Path(candidate)) == candidate_sha

    def read(address, size):
        for base, data in regions:
            if base <= address and address + size <= base + len(data):
                return data[address-base:address-base+size]
        raise AssertionError(hex(address))

    def word(address, size=4):
        return int.from_bytes(read(address, size), 'little')

    raw = (case / 'native-input.bin').read_bytes()
    full = (case / 'native-input-full.bin').read_bytes()
    fp = (case / 'native-fp-input.bin').read_bytes()
    assert len(raw) == 168 and len(full) == 180 and full == raw + fp
    depth, status, cw, mxcsr = struct.unpack('<IHHI', fp)
    assert depth == 0 and cw == 0x27f and mxcsr & 0x1f80 == 0x1f80
    gp = struct.unpack_from('<8I', raw)
    assert struct.unpack_from('<I', raw, 164)[0] == 0x60200000
    assert 0x60000000 <= gp[4] <= 0x601ffff0
    call = _Code(Xbe(NEW_DEFAULT), '.text').at(0xd1878)
    assert call.mnemonic == 'call' and call.operands[0].imm == metadata['entry']
    assert word(gp[4]) == call.address + call.size
    arguments = [word(gp[4] + 4), word(gp[4] + 8)]
    assert gp[0] == metadata['player_unit'] and gp[1] == 0x10000
    assert gp[2] == arguments[0] and arguments[1] == gp[7] == metadata['unit']
    read(arguments[0], 2)
    assert word(word(0x51df54) + 8) == metadata['tick'] == 259
    assert word(word(0x4f3218) + 8) == 1
    headers = word(word(0x4ecbd0) + 0x44)

    def body(datum):
        header = headers + (datum & 65535) * 12
        assert word(header, 2) == datum >> 16
        return word(header + 8)

    hunter, player, weapon = (body(metadata[name]) for name in ('unit', 'player_unit', 'weapon'))
    assert word(hunter) == 0xfcc41a7c and word(weapon) == 0xfad418a0
    assert word(hunter + 0x12c) == metadata['actor']
    assert word(weapon + 0x14) == metadata['unit']
    assert word(player + 0x13c) == metadata['player']
    actor = word(word(0x5020c8) + 0x44) + (metadata['actor'] & 65535) * 0x888
    assert word(actor, 2) == metadata['actor'] >> 16
    assert word(actor + 0x18) == metadata['unit'] and word(actor + 0x30) == 31
    path = Path(build['command'][2])
    assert file_sha(path) == '7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4'
    parent = json.loads((case.parent / 'native.json').read_text())
    assert parent['completed_ticks'] >= 199
    if not completed:assert parent['completed_ticks'] == 199
    if not completed:
        assert 'at d1878 -> 18b4c0 return d187d' in parent['failure']
    return dict(map=str(path), map_sha256=file_sha(path), tick=metadata['tick'],
                actual_nested_caller=call.address, actual_arguments=arguments,
                callee_stack_arguments=arguments[:1], caller_following_stack_word=arguments[1],
                material_output_address=arguments[0], material_output_bytes=2,
                actor=metadata['actor'], unit=metadata['unit'], target_unit=gp[0],
                caller_stack=gp[4], x87_depth=depth, x87_status=status,
                x87_control_word=cw, mxcsr=mxcsr, public_campaign_placement=True,
                native_completed=completed, unresolved_input_observed_without_return=not completed,
                whole_object_tick_verified=False, full_weapon_parity=False,
                validation_tool_sha256=file_sha(Path(__file__)))
