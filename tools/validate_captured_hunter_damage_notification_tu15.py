"""Authenticate public damage and following AI callback inputs.

The older paired damage module reports canonical instruction CB84D. Its
selected TU1.5 return address CC9D2 identifies the unchanged actual CALL.
"""
import json
import struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def validate(case, metadata, regions):
    assert metadata['entry'] in (0x1d1880, 0x26f260) and metadata['public_campaign_placement']
    following = metadata['entry'] == 0x26f260
    assert metadata['ordinal'] == 1
    completed = metadata['completed']
    build = json.loads((case / 'build.json').read_text())
    assert build['returncode'] == (0 if completed else 1)
    assert build['scope']['actual_nested_ai_observer']
    assert build['scope']['original_recorded_animation_startup']
    assert file_sha(Path(build['source'])) == build['source_sha256']
    assert file_sha(Path(build['command'][0])) == build['binary_sha256']
    assert file_sha(Path(build['command'][1])) == NEW_PIN
    for name, digest in build['linked_files_sha256'].items():
        assert file_sha(Path(name)) == digest
    names = ['before.bin', 'native-input.bin', 'native-input-full.bin',
             'native-fp-input.bin', 'native.json']
    if completed:
        names += ['after.bin', 'native-exit.bin', 'native-stores.bin']
    for name in names:
        assert file_sha(case / name) == build['files_sha256'][name]
    proof = json.loads(Path(build['recorded_animation_world_proof']).read_text())
    assert file_sha(Path(proof['base_source'])) == proof['base_sha256']
    assert file_sha(Path(proof['candidate_source'])) == proof['candidate_sha256']
    assert any(item['source'] == proof['candidate_source'] and
               item['source_sha256'] == proof['candidate_sha256']
               for item in build['override_sources'].values())
    if following and completed:
        static = json.loads(Path(build['ai_static_observer_proof']).read_text())
        assert file_sha(Path(static['base_source'])) == static['base_sha256']
        assert file_sha(Path(static['candidate_source'])) == static['candidate_sha256']
        assert any(item['source'] == static['candidate_source'] and
                   item['source_sha256'] == static['candidate_sha256']
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
    image = Xbe(NEW_DEFAULT)
    assert file_sha(NEW_DEFAULT) == NEW_PIN
    call = _Code(image, '.text').at(0x26e5e4 if following else 0xcc9cd)
    assert call.mnemonic == 'call' and call.operands[0].imm == metadata['entry']
    assert word(gp[4]) == call.address + call.size
    arguments = [word(gp[4] + 4 + 4*index) for index in range(2 if following else 4)]
    if following:
        assert arguments == [metadata['actor'], gp[2]] and gp[7] == metadata['actor']
        target_pool = word(0x50eef0)
        target = word(target_pool + 0x44) + (arguments[1] & 65535)*word(target_pool + 0x24)
        assert word(target, 2) == arguments[1] >> 16
        fixture = json.loads((case.parents[1] / 'capture-public-hunter-sequence-v2.json').read_text())
        assert build['source'] == fixture['candidate_source']
        assert build['source_sha256'] == fixture['candidate_sha256']
        assert file_sha(Path(fixture['base_source'])) == fixture['base_sha256']
    else:
        assert arguments == [metadata['player_unit'], metadata['unit'], gp[1], gp[0]]
        assert gp[3] == metadata['player_unit'] and gp[7] == metadata['unit']
    assert word(word(0x51df54) + 8) == metadata['tick'] == (279 if following else 278)
    assert word(word(0x4f3218) + 8) == 1
    pool = word(0x5021fc)
    assert pool and read(pool, 20) == b'recorded animations\0' and word(pool + 0x29, 1) == 1
    headers = word(word(0x4ecbd0) + 0x44)
    bodies = []
    for datum in (metadata['unit'], metadata['player_unit']):
        header = headers + (datum & 65535)*12
        assert word(header, 2) == datum >> 16
        bodies.append(word(header + 8))
    assert word(bodies[0]) == 0xfcc41a7c
    assert word(bodies[0] + 0x12c) == metadata['actor']
    assert word(bodies[1] + 0x13c) == (0xffffffff if following else metadata['player'])
    actor = word(word(0x5020c8) + 0x44) + (metadata['actor'] & 65535)*0x888
    assert word(actor, 2) == metadata['actor'] >> 16
    assert word(actor + 0x18) == metadata['unit'] and word(actor + 0x30) == 31
    path = Path(build['command'][2])
    assert file_sha(path) == '7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4'
    parent = json.loads((case.parent / 'native.json').read_text())
    assert parent['completed_ticks'] >= 218
    if not completed:
        assert parent['completed_ticks'] == (219 if following else 218)
        assert ('at 26e5e4 -> 26f260 return 26e5e9' if following else
                'at cb84d -> 1d1880 return cc9d2') in parent['failure']
    return dict(map=str(path), map_sha256=file_sha(path), tick=metadata['tick'],
                actual_nested_caller=call.address, paired_canonical_caller=None if following else 0xcb84d,
                callee_stack_arguments=arguments, actor=metadata['actor'], unit=metadata['unit'],
                caller_stack=gp[4], x87_depth=depth, x87_status=status,
                x87_control_word=cw, mxcsr=mxcsr, public_campaign_placement=True,
                native_completed=completed, full_weapon_parity=False,
                validation_tool_sha256=file_sha(Path(__file__)))
