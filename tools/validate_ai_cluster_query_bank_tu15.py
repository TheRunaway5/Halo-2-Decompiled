"""Authenticate the original BSP background-sound query over a real aim input."""
import json
import re
import struct
from pathlib import Path

from audit_rendering import Cache, file_sha
from oracle_charged_effect_lifetime_bank_tu15 import regions
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from validate_charge_effect_bank_tu15 import SCOPE, SCOPE_PIN


def validate(case, metadata, rows):
    assert metadata['entry'] == 0x11e660
    build = json.loads((case / 'build.json').read_text())
    origin = Path(build['origin_case'])
    assert file_sha(origin / 'build.json') == build['origin_build_sha256']
    parent = json.loads((origin / 'build.json').read_text())
    assert parent['returncode'] == 1
    failure = (origin / 'failure.txt').read_text()
    assert 'at 26dd14:' in failure and '-> 11e660 return 26dd19' in failure
    gp = [int(s, 16) for s in re.search(r'\[gp ([0-9a-f ]+)\]', failure).group(1).split()]
    assert gp[0] == metadata['location']
    assert file_sha(origin / 'failure-bank.bin') == parent['files_sha256']['failure-bank.bin']
    assert file_sha(origin / 'failure.txt') == parent['files_sha256']['failure.txt']
    expected = list(regions(origin / 'failure-bank.bin'))
    assert len(expected) == len(rows)
    for (p, actual), (q, prior) in zip(rows, expected):
        assert p == q and len(actual) == len(prior)
        admitted = bytearray(prior)
        if p <= 0x600f0000 and 0x600f0004 <= p + len(prior):
            struct.pack_into('<I', admitted, 0x600f0000 - p, 0x60300000)
        assert actual == admitted
    for name in ('before.bin', 'native-input.bin', 'native.json'):
        assert file_sha(case / name) == build['files_sha256'][name]
    assert file_sha(SCOPE) == SCOPE_PIN and file_sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    instruction = _Code(image, '.text').at(0x26dd14)
    assert instruction.mnemonic == 'call' and instruction.op_str == '0x11e660'

    def read(p, n=4):
        for q, data in rows:
            if q <= p and p + n <= q + len(data):
                return data[p-q:p-q+n]
        raise AssertionError(hex(p))

    def u(p, n=4):
        return int.from_bytes(read(p, n), 'little')

    caller = (case / 'native-input.bin').read_bytes()
    assert len(caller) == 168
    assert struct.unpack_from('<8I', caller) == (metadata['location'], 0, 0, 0, 0x600f0000, 0, 0, 0)
    assert caller[32:160] == bytes(128)
    assert struct.unpack_from('<II', caller, 160) == (2, 0x60200000)
    assert read(metadata['location'], 8).hex() == metadata['location_bytes']
    cluster = u(metadata['location'] + 4, 2)
    bsp = u(0x4ecc18)
    palette = u(u(bsp + 0xa0) + cluster * 0xb0 + 0x72, 2)
    assert palette != 65535 and palette < u(bsp + 0xd4)
    datum = u(u(bsp + 0xd8) + palette * 100 + 0x24)
    path = Path(parent['command'][2])
    scope = json.loads(SCOPE.read_text())
    pin = next(m['sha256'] for m in scope['maps'] if Path(m['path']) == path)
    assert file_sha(path) == pin
    cache = Cache(path)
    try:
        tag = cache.tags[datum]
        assert tag['group'] == 'lsnd'
        root = u(u(0x4f0414) + (datum & 65535) * 16 + 8)
        assert root == tag['address']
        assert read(root, 4) == bytes(cache.data[root-cache.magic:root-cache.magic+4])
        return dict(map=str(path), map_sha256=pin, actual_failed_aim_caller=0x26dd14,
                    declared_component_caller=True, complete_weapon_update_verified=False,
                    location=metadata['location'], cluster=cluster, background_sound_palette=palette,
                    background_sound=tag['name'], background_sound_datum=datum,
                    background_sound_flags=u(root), validation_tool_sha256=file_sha(Path(__file__)))
    finally:
        cache.close()
