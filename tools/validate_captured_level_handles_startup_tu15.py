"""Authenticate the actual registered row13 constructor in the native world."""
import json
import struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def validate(case, metadata, regions):
    assert metadata['entry'] == 0x1a5410 and metadata['actual_startup_callback']
    build = json.loads((case / 'build.json').read_text())
    assert build['returncode'] == (0 if metadata['completed'] else 1)
    assert build['scope']['registered_level_handles_constructor']
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
    assert struct.unpack_from('<8I', raw) == (0, 0, 0, 0, 0x600f0000, 0, 0, 0)
    assert raw[32:160] == bytes(128) and struct.unpack_from('<II', raw, 160) == (2, 0x60200000)
    assert struct.unpack('<IHHI', fp) == (0, 0, 0x27f, 0x1f80)
    assert word(0x600f0000) == 0x60300000
    assert word(0x4fafbc) == word(0x4fafb8) == 0
    assert word(0x51df54) == word(0x5020c8) == word(0x4f54f4) == word(0x4ecbd0) == 0
    assert word(0x474cb8) and word(0x4f3218)
    assert word(word(0x4f3218) + 0x1120, 1) == 0
    image = Xbe(NEW_DEFAULT)
    assert file_sha(NEW_DEFAULT) == NEW_PIN
    row = 0x44aed0 + 13*36
    assert image.read(row, 16) == struct.pack('<4I', 0x1a5410, 0x1a54b0, 0x1a59d0, 0x1a5a10)
    assert word(row) == metadata['entry'] and word(row + 8) == 0x1a59d0
    path = Path(build['command'][2])
    assert file_sha(path) == '7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4'
    return dict(map=str(path), map_sha256=file_sha(path), actual_registered_startup_entry=True,
                registration_row=row, row_index=13, actual_retail_caller=False,
                callee_stack_arguments=[], source_clock_constructed=False,
                pool_bindings=[0x4fafbc, 0x4fafb8], allocator=word(0x474cb8),
                native_completed=metadata['completed'], full_weapon_parity=False,
                validation_tool_sha256=file_sha(Path(__file__)))
