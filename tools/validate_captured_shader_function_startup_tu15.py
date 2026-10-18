"""Authenticate the native registered row29 shader-function constructor."""
import json
import struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def validate(case, metadata, regions):
    entry = metadata['entry']
    assert entry == 0x10fa30 and metadata['actual_startup_callback']
    build = json.loads((case / 'build.json').read_text())
    assert build['returncode'] == (0 if metadata['completed'] else 1)
    assert build['scope']['actual_registered_startup_entry'] and build['scope']['cold_original_map_start']
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
        names += ['after.bin', 'native-exit.bin', 'native-trace.bin']
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
    assert list(gp[:4]) == [0]*4 and list(gp[5:]) == [0]*3
    assert 0x60000000 <= sp < 0x601fffc0 and struct.unpack_from('<I', raw, 164)[0] == 0x60200000
    depth,status,control,mxcsr = struct.unpack('<IHHI', fp)
    assert depth == 0 and control == 0x27f and mxcsr & ~0x3f == 0x1f80
    assert word(sp) == 0x60300000
    assert word(0x51daf4) == 0 and word(0x52be48) == 0
    assert word(word(0x51df54) + 8) == 0
    game = word(0x4f3218)
    assert word(game + 0x1120,1) == 0
    resource,used = word(0x4f2950),word(0x4f2954)
    assert 0x80061000 <= resource and resource + used + 0x304 <= 0x83145000
    image = Xbe(NEW_DEFAULT)
    assert file_sha(NEW_DEFAULT) == NEW_PIN
    row = 0x44aed0 + 29*36
    assert image.read(row,16) == struct.pack('<4I',0x10fa30,0x10fa80,0x10fa90,0x10fb60)
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
    return dict(map=str(path),map_sha256=file_sha(path),actual_registered_startup_entry=True,
                registration_row=row,row_index=29,actual_retail_caller=False,callee_stack_arguments=[],
                source_clock_tick=0,game_ready=False,source_physics_world_constructed=False,
                resource_base=resource,resource_used_before=used,native_completed=metadata['completed'],
                full_weapon_parity=False,validation_tool_sha256=file_sha(Path(__file__)))
