"""Authenticate the actual public11750D ->218020 unit callback."""
import json
import struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def validate(case, metadata, regions):
    assert metadata['entry'] == 0x218020 and metadata['public_campaign_placement']
    assert metadata['ordinal'] == 1
    completed = metadata['completed']
    build = json.loads((case / 'build.json').read_text())
    assert build['returncode'] == (0 if completed else 1)
    assert build['scope']['actual_nested_ai_observer'] and build['scope']['original_recorded_animation_startup']
    assert file_sha(Path(build['source'])) == build['source_sha256']
    assert file_sha(Path(build['command'][0])) == build['binary_sha256']
    assert file_sha(Path(build['command'][1])) == NEW_PIN
    for name, digest in build['linked_files_sha256'].items():
        assert file_sha(Path(name)) == digest
    names = ['before.bin', 'native-input.bin', 'native-input-full.bin', 'native-fp-input.bin', 'native.json']
    if completed:
        names += ['after.bin', 'native-exit.bin', 'native-stores.bin']
    for name in names:
        assert file_sha(case / name) == build['files_sha256'][name]
    proof = json.loads(Path(build['recorded_animation_world_proof']).read_text())
    for field in ('base', 'candidate'):
        assert file_sha(Path(proof[field + '_source'])) == proof[field + '_sha256']
    assert any(item['source'] == proof['candidate_source'] and item['source_sha256'] == proof['candidate_sha256']
               for item in build['override_sources'].values())
    fixture = json.loads((case.parents[1] / 'capture-public-hunter-sequence-v2.json').read_text())
    assert build['source'] == fixture['candidate_source'] and build['source_sha256'] == fixture['candidate_sha256']

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
    sp = gp[4]
    assert 0x60000000 <= sp <= 0x601fffc0
    image = Xbe(NEW_DEFAULT)
    assert file_sha(NEW_DEFAULT) == NEW_PIN
    call = _Code(image, '.text').at(0x11750d)
    assert call.mnemonic == 'call' and call.operands[0].imm == metadata['entry']
    assert word(sp) == call.address + call.size
    header = word(word(0x4ecbd0) + 0x44) + (metadata['unit'] & 65535)*12
    assert word(header, 2) == metadata['unit'] >> 16
    body = word(header + 8)
    assert word(body) == 0xfcc41a7c and word(body + 0x12c) == metadata['actor']
    assert gp[0] == gp[5] == gp[6] == metadata['unit'] and gp[2] == body
    assert word(sp + 8) == metadata['unit'] and word(sp + 16) == body
    actor = word(word(0x5020c8) + 0x44) + (metadata['actor'] & 65535)*0x888
    assert word(actor, 2) == metadata['actor'] >> 16
    assert word(actor + 0x18) == metadata['unit'] and word(actor + 0x54) == metadata['character']
    assert word(actor + 0x30) == 31
    assert word(word(0x51df54) + 8) == metadata['tick'] and 60 <= metadata['tick'] < 540
    assert word(word(0x4f3218) + 8) == 1
    reference = gp[1]
    reference_pool = word(0x505e70)
    reference_salt = None
    if reference != 0xffffffff:
        assert reference_pool and word(reference_pool + 0x24) == 0x54
        assert (reference & 65535) < word(reference_pool + 0x20)
        reference_salt = word(word(reference_pool + 0x44) + (reference & 65535)*0x54, 2)
    path = Path(build['command'][2])
    assert file_sha(path) == '7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4'
    parent = json.loads((case.parent / 'native.json').read_text())
    assert parent['completed_ticks'] >= metadata['tick'] - 60
    if not completed:
        assert parent['completed_ticks'] == metadata['tick'] - 60
        assert 'at 11750d -> 218020 return 117512' in parent['failure']
    return dict(map=str(path), map_sha256=file_sha(path), tick=metadata['tick'],
                actual_nested_caller=call.address, callee_stack_arguments=[], unit=metadata['unit'],
                input_reference=reference, reference_pool=reference_pool, observed_reference_salt=reference_salt,
                caller_stack=sp, x87_depth=depth, x87_status=status, x87_control_word=cw, mxcsr=mxcsr,
                public_campaign_placement=True, native_completed=completed,
                validation_tool_sha256=file_sha(Path(__file__)), full_weapon_parity=False)
