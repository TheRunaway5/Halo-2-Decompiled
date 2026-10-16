"""Authenticate the actual public204553 ->266D30 notification caller."""
import json
import struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def validate(case, metadata, regions):
    assert metadata['entry'] in (0x266d30,0x2658b0) and metadata['public_campaign_placement']
    link = metadata['entry'] == 0x2658b0
    assert metadata['ordinal'] == 1
    completed = metadata['completed']
    build = json.loads((case / 'build.json').read_text())
    assert build['returncode'] == (0 if completed else 1)
    if link:
        assert build['scope']['actual_nested_ai_observer']
    else:
        assert build['scope']['actual_notification_caller' if completed else 'actual_unresolved_notification_caller']
    assert file_sha(Path(build['source'])) == build['source_sha256']
    assert file_sha(Path(build['command'][0])) == build['binary_sha256']
    assert file_sha(Path(build['command'][1])) == NEW_PIN
    for name, digest in build['linked_files_sha256'].items():
        assert file_sha(Path(name)) == digest
    names = ['before.bin', 'native-input.bin', 'native-input-full.bin',
             'native-fp-input.bin', 'native.json']
    if completed:
        names += ['after.bin', 'native-exit.bin']
    for name in names:
        assert file_sha(case / name) == build['files_sha256'][name]
    if link:
        proof = json.loads((case.parents[1] / 'hunter-ai-observer-world-v1.json').read_text())
        assert file_sha(Path(proof['base_source'])) == proof['base_sha256']
        assert file_sha(Path(proof['diagnostic_source'])) == proof['diagnostic_sha256']
        assert any(item['source'] == proof['diagnostic_source'] and
                   item['source_sha256'] == proof['diagnostic_sha256']
                   for item in build['override_sources'].values())
    elif completed:
        proof = json.loads((case.parents[1] / 'hunter-notification-world-v1.json').read_text())
        assert file_sha(Path(proof['base_source'])) == proof['base_sha256']
        assert file_sha(Path(proof['candidate_source'])) == proof['candidate_sha256']
        assert any(item['source'] == proof['candidate_source'] and
                   item['source_sha256'] == proof['candidate_sha256']
                   for item in build['override_sources'].values())
    else:
        proof = json.loads((case.parents[1] / 'hunter-notification-world-observer-v1.json').read_text())
        assert file_sha(Path(proof['base_source'])) == proof['base_sha256']
        assert file_sha(Path(proof['diagnostic_source'])) == proof['diagnostic_sha256']
        assert any(item['source'] == proof['diagnostic_source'] and
                   item['source_sha256'] == proof['diagnostic_sha256']
                   for item in build['override_sources'].values())

    def read(address, size):
        for base, data in regions:
            if base <= address and address + size <= base + len(data):
                return data[address-base:address-base+size]
        raise AssertionError(hex(address))

    def word(address, size=4):
        return int.from_bytes(read(address, size), 'little')

    raw = (case / 'native-input.bin').read_bytes()
    fp = (case / 'native-fp-input.bin').read_bytes()
    assert len(raw) == 168 and (case / 'native-input-full.bin').read_bytes() == raw + fp
    depth, status, cw, mxcsr = struct.unpack('<IHHI', fp)
    assert depth == 0 and cw == 0x27f and mxcsr & 0x1f80 == 0x1f80
    gp = struct.unpack_from('<8I', raw)
    assert struct.unpack_from('<I', raw, 164)[0] == 0x60200000
    assert 0x60000000 <= gp[4] <= 0x601fffe0
    call = _Code(Xbe(NEW_DEFAULT), '.text').at(0x204589 if link else 0x204553)
    assert call.mnemonic == 'call' and call.operands[0].imm == metadata['entry']
    assert word(gp[4]) == call.address + call.size
    arguments = [word(gp[4] + 4 + index*4) for index in range(1 if link else 6)]
    if link:
        assert arguments[0] == gp[7] and gp[0] == gp[5] == metadata['actor']
    else:
        assert arguments[:3] == [metadata['actor'], metadata['player_unit'], 4]
        assert arguments[3] == gp[7] and arguments[4:] == [0xffffffff, 0xffffffff]
        assert gp[5] == metadata['actor'] and gp[3] == metadata['player_unit']
    assert word(word(0x51df54) + 8) == metadata['tick'] == 268
    assert word(word(0x4f3218) + 8) == 1
    actor = word(word(0x5020c8) + 0x44) + (metadata['actor'] & 65535)*0x888
    assert word(actor, 2) == metadata['actor'] >> 16
    assert word(actor + 0x18) == metadata['unit'] and word(actor + 0x30) == 31
    assert word(actor + 0x54) == metadata['character']
    headers = word(word(0x4ecbd0) + 0x44)
    header = headers + (metadata['unit'] & 65535)*12
    assert word(header, 2) == metadata['unit'] >> 16
    assert word(word(header + 8)) == 0xfcc41a7c
    # Notifications may refer to a previously held object identity; require
    # the captured caller relationship, not an invented live-target admission.
    path = Path(build['command'][2])
    assert file_sha(path) == '7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4'
    parent = json.loads((case.parent / 'native.json').read_text())
    assert parent['completed_ticks'] >= 208
    if not completed:
        assert parent['completed_ticks'] == 208
        assert ('at 204589 -> 2658b0 return 20458e' if link else 'at 204553 -> 266d30 return 204558') in parent['failure']
    return dict(map=str(path), map_sha256=file_sha(path), tick=metadata['tick'],
                actual_nested_caller=call.address, callee_stack_arguments=arguments,
                actor=metadata['actor'], unit=metadata['unit'], notified_object=arguments[0] if link else arguments[1],
                caller_stack=gp[4], x87_depth=depth, x87_status=status,
                x87_control_word=cw, mxcsr=mxcsr, public_campaign_placement=True,
                native_completed=completed, validation_tool_sha256=file_sha(Path(__file__)),
                whole_object_tick_verified=False, full_weapon_parity=False)
