"""Authenticate the actual public211C1B descriptor callback on ranged Hunter input."""
import json
import struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def validate(case, metadata, regions):
    assert metadata['entry'] in (0x2afa50, 0x27fa20) and metadata['public_campaign_placement']
    assert metadata['ordinal'] == 1
    build = json.loads((case / 'build.json').read_text())
    assert build['returncode'] == (0 if metadata['completed'] else 1)
    assert build['scope']['actual_nested_ai_observer']
    assert build['scope']['original_recorded_animation_startup']
    assert file_sha(Path(build['source'])) == build['source_sha256']
    assert file_sha(Path(build['command'][0])) == build['binary_sha256']
    assert file_sha(Path(build['command'][1])) == NEW_PIN
    for name, digest in build['linked_files_sha256'].items():
        assert file_sha(Path(name)) == digest
    names = ['before.bin', 'native-input.bin', 'native-input-full.bin',
             'native-fp-input.bin', 'native.json', 'native-stores.bin']
    if metadata['completed']:
        names += ['after.bin', 'native-exit.bin']
    for name in names:
        assert file_sha(case / name) == build['files_sha256'][name]
    world = json.loads(Path(build['recorded_animation_world_proof']).read_text())
    for kind in ('base', 'candidate'):
        assert file_sha(Path(world[kind + '_source'])) == world[kind + '_sha256']
    assert any(item['source'] == world['candidate_source'] and
               item['source_sha256'] == world['candidate_sha256']
               for item in build['override_sources'].values())
    if build.get('ai_static_observer_proof'):
        observer = json.loads(Path(build['ai_static_observer_proof']).read_text())
        observed_source = Path(observer['base_source']).read_text()
        assert file_sha(Path(observer['base_source'])) == observer['base_sha256']
        for patch in observer['patches']:
            assert observed_source.count(patch['before']) == 1
            observed_source = observed_source.replace(patch['before'], patch['after'])
        assert observed_source == Path(observer['candidate_source']).read_text()
        assert file_sha(Path(observer['candidate_source'])) == observer['candidate_sha256']
        assert any(item['source'] == observer['candidate_source'] and
                   item['source_sha256'] == observer['candidate_sha256']
                   for item in build['override_sources'].values())
    fixture = json.loads(Path(build['fixture_proof']).read_text())
    source = Path(fixture['base_source']).read_text()
    assert file_sha(Path(fixture['base_source'])) == fixture['base_sha256']
    for patch in fixture['patches']:
        assert source.count(patch['before']) == 1
        source = source.replace(patch['before'], patch['after'])
    assert source == Path(build['source']).read_text()
    assert fixture['candidate_source'] == build['source']
    assert fixture['candidate_sha256'] == build['source_sha256']

    def word(address, size=4):
        for base, data in regions:
            if base <= address and address + size <= base + len(data):
                return int.from_bytes(data[address-base:address-base+size], 'little')
        raise AssertionError(hex(address))

    raw = (case / 'native-input.bin').read_bytes()
    fp = (case / 'native-fp-input.bin').read_bytes()
    assert len(raw) == 168 and (case / 'native-input-full.bin').read_bytes() == raw + fp
    depth, status, cw, mxcsr = struct.unpack('<IHHI', fp)
    assert depth == 0 and cw == 0x27f and mxcsr & 0x1f80 == 0x1f80
    gp = struct.unpack_from('<8I', raw)
    assert struct.unpack_from('<I', raw, 164)[0] == 0x60200000
    sp = gp[4]
    assert 0x60000000 <= sp < 0x601fffc0
    image = Xbe(NEW_DEFAULT)
    assert file_sha(NEW_DEFAULT) == NEW_PIN
    call = _Code(image, '.text').at(0x211c1b if metadata['entry'] == 0x2afa50 else 0x2afa89)
    descriptor = word(0x480c00 + 0x150*4)
    assert image.read(0x480c00 + 0x150*4, 4) == struct.pack('<I', descriptor)
    assert image.read(descriptor, 18).hex() == '0500000050fa2a0000000000020009001300'
    assert word(sp) == call.address + call.size
    assert word(descriptor + 4) == 0x2afa50
    if metadata['entry'] == 0x2afa50:
        assert call.bytes.hex() == 'ff5204' and gp[2] == descriptor
        arguments = [word(sp + offset) for offset in (4, 8, 12)]
        assert arguments == [0x150, gp[7], gp[1]] and gp[1] in (0, 1)
    else:
        assert call.mnemonic == 'call' and call.operands[0].imm == metadata['entry']
        assert [word(sp + offset) for offset in (12, 16, 20)] == [0x211c1e, 0x150, gp[7]]
        assert word(sp + 24) in (0, 1) and gp[6] == gp[7]
        arguments = []
    pool = word(0x505e5c)
    assert word(pool + 0x24) == 0x418 and (gp[7] & 65535) < word(pool + 0x20)
    thread = word(pool + 0x44) + (gp[7] & 65535)*0x418
    assert word(thread, 2) == gp[7] >> 16
    if metadata['entry'] == 0x2afa50:
        assert gp[6] == thread
    assert word(0x505e64) == gp[7]
    assert word(word(0x51df54) + 8) == metadata['tick']
    assert word(word(0x4f3218) + 8) == 1
    actor = word(word(0x5020c8) + 0x44) + (metadata['actor'] & 65535)*0x888
    assert word(actor, 2) == metadata['actor'] >> 16
    assert word(actor + 0x18) == metadata['unit'] and word(actor + 0x54) == metadata['character']
    assert word(actor + 0x30) == 31
    objects = word(0x4ecbd0)
    assert word(objects + 0x24) == 12
    def body(datum):
        header = word(objects + 0x44) + (datum & 65535)*12
        assert word(header, 2) == datum >> 16
        return word(header + 8)
    hunter, weapon = body(metadata['unit']), body(metadata['weapon'])
    assert word(hunter) == 0xfcc41a7c and word(hunter + 0x12c) == metadata['actor']
    assert word(weapon) == 0xfad418a0 and word(weapon + 0x14) == metadata['unit']
    players = word(0x4f54f4)
    player = word(players + 0x44) + (metadata['player'] & 65535)*0x21c
    assert word(players + 0x24) == 0x21c and word(player, 2) == metadata['player'] >> 16
    assert word(player + 0x2c) == metadata['player_unit']
    assert word(body(metadata['player_unit']) + 0x13c) == metadata['player']
    path = Path(build['command'][2])
    assert file_sha(path) == '7ea355ef79f13be83fe78546c0fd3b1009ef71c0dd2257b24996420d856f78f4'
    parent = json.loads((case.parent / 'native.json').read_text())
    assert parent['declared_player_forward_offset'] == 15 and parent['declared_player_vertical_offset'] == 1
    assert parent['declared_player_octant'] == 0 and parent['completed_ticks'] >= metadata['tick'] - 60
    if not metadata['completed']:
        assert parent['completed_ticks'] == metadata['tick'] - 60
        expected = ('at 211c1b -> 2afa50 return 211c1e' if metadata['entry'] == 0x2afa50
                    else 'at 2afa89 -> 27fa20 return 2afa8e')
        assert expected in parent['failure']
    return dict(map=str(path), map_sha256=file_sha(path), tick=metadata['tick'],
                actual_nested_caller=call.address, descriptor=descriptor, builtin=0x150,
                callee_stack_arguments=arguments, thread=gp[7], thread_record=thread,
                caller_stack=sp, x87_depth=depth, x87_status=status,
                x87_control_word=cw, mxcsr=mxcsr, public_campaign_placement=True,
                native_completed=metadata['completed'], full_weapon_parity=False,
                validation_tool_sha256=file_sha(Path(__file__)))
