"""Authenticate the actual13A7BA registered row16 resource-cache new-map callback."""
import json
import struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def validate(case, metadata, regions):
    assert metadata['entry'] == 0x126730 and metadata['actual_registered_new_map_callback']
    assert metadata['ordinal'] == 1
    build = json.loads((case / 'build.json').read_text())
    assert build['returncode'] == (0 if metadata['completed'] else 1)
    assert build['scope']['actual_registered_new_map_callback'] and build['scope']['cold_original_map_start']
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
    gp = struct.unpack_from('<8I', raw)
    sp = gp[4]
    assert gp[0] == 0x126730 and gp[6] == 16*36
    assert 0x60000000 <= sp < 0x601fffc0 and struct.unpack_from('<I', raw, 164)[0] == 0x60200000
    assert struct.unpack('<IHHI', fp) == (0, 0, 0x27f, 0x1f80)
    assert [word(sp + 4*i) for i in range(4)] == [0x13a7bc, 0x60080000, 0x191dfc, 0x60080000]
    game = word(0x4f3218)
    assert word(game + 8) == 1 and word(game + 0x1120, 1) == 0 and word(game + 2, 2) == 0xffff
    assert word(0x52be48) == 0 and word(word(0x51df54) + 8) == 0
    for slot, capacity in ((0x4fafbc, 20), (0x4fafb8, 50)):
        pool = word(slot)
        assert pool and word(pool + 0x20) == capacity and word(pool + 0x24) == 8
        assert word(pool + 0x29, 1) == 1 and word(pool + 0x3c) <= capacity
    image = Xbe(NEW_DEFAULT)
    assert file_sha(NEW_DEFAULT) == NEW_PIN
    row = 0x44aed0 + 16*36
    assert image.read(row, 16) == struct.pack('<4I', 0x126640, 0x126700, 0x126730, 0x31d400)
    physical = word(0x4f2950)
    assert physical == word(0x4f2964) and 0x80061000 <= physical
    assert physical + 0x3fe000 <= 0x83145000
    assert any(base <= physical and physical + 0x3fe000 <= base + len(data) for base, data in regions)
    call = _Code(image, '.text').at(0x13a7ba)
    assert call.bytes.hex() == 'ffd0' and word(row + 8) == metadata['entry']
    fixture = json.loads(Path(build['fixture_proof']).read_text())
    source = Path(fixture['base_source']).read_text()
    assert file_sha(Path(fixture['base_source'])) == fixture['base_sha256']
    for patch in fixture['patches']:
        assert source.count(patch['before']) == 1
        source = source.replace(patch['before'], patch['after'])
    assert source == Path(build['source']).read_text()
    assert fixture['candidate_sha256'] == build['source_sha256']
    path = Path(build['command'][2])
    assert file_sha(path) == '7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4'
    return dict(map=str(path), map_sha256=file_sha(path), actual_registered_new_map_callback=True,
                registration_row=row, row_index=16, actual_nested_caller=call.address,
                actual_retail_caller=False, callee_stack_arguments=[], cold_original_map_start=True,
                source_clock_tick=0, game_ready=False, selected_bsp=-1, before_player_actor_birth=True,
                pool_bindings=[0x4fafbc, 0x4fafb8], resource_cache_physical_base=physical,
                native_completed=metadata['completed'],
                full_weapon_parity=False, validation_tool_sha256=file_sha(Path(__file__)))
