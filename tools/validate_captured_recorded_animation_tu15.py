"""Authenticate original row55 storage callbacks or the public Hunter query."""
import json
import struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def validate(case, metadata, regions):
    entry = metadata['entry']
    assert entry in (0x2035b0, 0x2035f0, 0x2039b0)
    assert metadata['completed']
    build = json.loads((case / 'build.json').read_text())
    assert build['returncode'] == 0 and build['scope']['original_recorded_animation_startup']
    assert file_sha(Path(build['source'])) == build['source_sha256']
    assert file_sha(Path(build['command'][0])) == build['binary_sha256']
    assert file_sha(Path(build['command'][1])) == NEW_PIN
    for name, digest in build['linked_files_sha256'].items():
        assert file_sha(Path(name)) == digest
    for name in ('before.bin', 'after.bin', 'native-input.bin', 'native-input-full.bin',
                 'native-fp-input.bin', 'native-exit.bin', 'native.json', 'native-stores.bin'):
        assert file_sha(case / name) == build['files_sha256'][name]
    proof_path = Path(build.get('recorded_animation_world_proof',
                               case.parents[1] / 'hunter-recorded-animation-world-v1.json'))
    proof = json.loads(proof_path.read_text())
    assert file_sha(Path(proof['base_source'])) == proof['base_sha256']
    assert file_sha(Path(proof['candidate_source'])) == proof['candidate_sha256']
    assert any(item['source'] == proof['candidate_source'] and
               item['source_sha256'] == proof['candidate_sha256']
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
    assert word(word(0x51df2c)) == 0x45f1d8
    image = Xbe(NEW_DEFAULT)
    assert file_sha(NEW_DEFAULT) == NEW_PIN
    assert struct.unpack('<9I', image.read(0x44b68c, 36)) == (
        0x2035b0, 0x31d400, 0x2035f0, 0x203610, 0, 0, 0, 0, 0)
    pool = word(0x5021fc)
    caller, arguments = None, []
    if entry == 0x2035b0:
        assert metadata['actual_startup_callback'] and pool == 0
        assert word(gp[4]) == 0x60300000
    else:
        assert pool and read(pool, 20) == b'recorded animations\0'
        assert word(pool + 0x20) == 1 and word(pool + 0x24) == 0xa0
        if entry == 0x2035f0:
            assert metadata['actual_startup_callback'] and word(gp[4]) == 0x60300000
        else:
            assert metadata['public_campaign_placement'] and metadata['ordinal'] == 1
            call = _Code(image, '.text').at(0xe558d)
            assert call.mnemonic == 'call' and call.operands[0].imm == entry
            assert word(gp[4]) == call.address + call.size
            caller, arguments = call.address, [word(gp[4] + 4)]
            assert arguments == [metadata['unit']] and gp[7] == metadata['unit']
            assert word(word(0x51df54) + 8) == metadata['tick'] == 268
            assert word(word(0x4f3218) + 8) == 1
            actor = word(word(0x5020c8) + 0x44) + (metadata['actor'] & 65535)*0x888
            assert word(actor, 2) == metadata['actor'] >> 16
            assert word(actor + 0x18) == metadata['unit'] and word(actor + 0x30) == 31
            assert word(actor + 0x54) == metadata['character']
            assert word(pool + 0x29, 1) == 1
    path = Path(build['command'][2])
    assert file_sha(path) == '7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4'
    return dict(map=str(path), map_sha256=file_sha(path), entry=entry,
                actual_nested_caller=caller, callee_stack_arguments=arguments,
                caller_stack=gp[4], pool=pool, x87_depth=depth, x87_status=status,
                x87_control_word=cw, mxcsr=mxcsr, native_completed=True,
                original_recorded_animation_startup=True,
                validation_tool_sha256=file_sha(Path(__file__)), full_weapon_parity=False)
