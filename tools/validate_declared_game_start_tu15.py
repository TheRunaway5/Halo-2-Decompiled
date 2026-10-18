"""Authenticate an explicit native191DF0 startup bridge before object birth.

The options pointer aliases the already delivered game+8 record. This is a
declared API input, not a captured retail caller or a completed game startup.
"""
import json
import struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def validate(case, metadata, regions):
    assert metadata['entry'] == 0x191df0 and metadata['declared_game_start_bridge']
    alias = metadata['options_alias_current_game_options']
    assert (alias or metadata.get('options_separate_copy')) and not metadata['public_campaign_placement']
    build = json.loads((case / 'build.json').read_text())
    assert build['scope']['declared_game_start_bridge'] and build['scope']['before_player_actor_birth']
    assert build['scope']['source_ready_not_supplied'] and not build['scope']['actual_retail_caller']
    assert build['returncode'] == (0 if metadata['completed'] else 1)
    assert file_sha(Path(build['source'])) == build['source_sha256']
    assert file_sha(Path(build['command'][0])) == build['binary_sha256']
    assert file_sha(Path(build['command'][1])) == NEW_PIN
    for name, digest in build['linked_files_sha256'].items():
        assert file_sha(Path(name)) == digest
    proof = json.loads(Path(build['game_start_world_proof']).read_text())
    source = Path(proof['base_source']).read_text()
    assert file_sha(Path(proof['base_source'])) == proof['base_sha256']
    for patch in proof['patches']:
        assert source.count(patch['before']) == 1
        source = source.replace(patch['before'], patch['after'])
    assert source == Path(proof['candidate_source']).read_text()
    assert file_sha(Path(proof['candidate_source'])) == proof['candidate_sha256']
    assert any(item['source'] == proof['candidate_source'] and
               item['source_sha256'] == proof['candidate_sha256']
               for item in build['override_sources'].values())
    names = ['before.bin', 'native-input.bin', 'native-input-full.bin',
             'native-fp-input.bin', 'native-stores.bin', 'native.json']
    if metadata['completed']:
        names += ['after.bin', 'native-exit.bin']
    for name in names:
        assert file_sha(case / name) == build['files_sha256'][name]

    def word(address, size=4):
        for base, data in regions:
            if base <= address and address + size <= base + len(data):
                return int.from_bytes(data[address-base:address-base+size], 'little')
        raise AssertionError(hex(address))

    raw = (case / 'native-input.bin').read_bytes()
    fp = (case / 'native-fp-input.bin').read_bytes()
    assert len(raw) == 168 and (case / 'native-input-full.bin').read_bytes() == raw + fp
    game = word(0x4f3218)
    options = game + 8 if alias else 0x60080000
    assert struct.unpack_from('<8I', raw) == (options, 0, 0, 0, 0x600f0000, 0, 0, 0)
    if not alias:
        assert word(options, 0x1118) == word(game + 8, 0x1118)
    if build['scope'].get('cold_original_map_start'):
        assert not alias and word(0x52be48) == 0
    assert raw[32:160] == bytes(128)
    assert struct.unpack_from('<II', raw, 160) == (2, 0x60200000)
    assert struct.unpack('<IHHI', fp) == (0, 0, 0x27f, 0x1f80)
    assert word(0x600f0000) == 0x60300000
    assert word(game + 8) == 1 and word(game + 0xc, 2) == 1 and word(game + 0xe, 2) == 30
    assert word(game + 0x18) == 0x12345678 and word(game + 0x132, 2) == 1
    assert word(game + 0x1120, 1) == 0 and word(game + 2, 2) == 0xffff
    assert word(game + 0x128, 2) == 0
    clock = word(0x51df54)
    assert word(clock + 8) == 0 and word(clock + 2, 2) == 30
    for slot, stride, capacity in ((0x5020c8, 0x888, 256), (0x4f54f4, 0x21c, 16),
                                   (0x4ecbd0, 12, 2048)):
        pool = word(slot)
        assert word(pool + 0x24) == stride and word(pool + 0x20) == capacity
        assert word(pool + 0x3c) == 0
    image = Xbe(NEW_DEFAULT)
    assert file_sha(NEW_DEFAULT) == NEW_PIN
    code = _Code(image, '.text')
    assert code.at(0x191703).bytes.hex() == '8bc6'
    call = code.at(0x191705)
    assert call.mnemonic == 'call' and call.operands[0].imm == metadata['entry']
    assert code.at(0x13a8ba).bytes.hex() == '8d5008'
    assert code.at(0x13a8be).bytes.hex() == 'b946040000'
    path = Path(build['command'][2])
    assert file_sha(path) == '7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4'
    return dict(map=str(path), map_sha256=file_sha(path), declared_game_start_bridge=True,
                options_alias_current_game_options=alias, options=options,
                declared_register_input='EAX=exact delivered game options; other GP/XMM zero',
                original_retail_callsite_ABI_reference=call.address, actual_retail_caller=False,
                callee_stack_arguments=[], game=game, game_ready=False, selected_bsp=-1,
                requested_bsp=0, source_tick=0, before_player_actor_birth=True,
                native_completed=metadata['completed'], full_weapon_parity=False,
                validation_tool_sha256=file_sha(Path(__file__)))
