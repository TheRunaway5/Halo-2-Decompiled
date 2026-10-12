#!/usr/bin/env python3
"""Execute unchanged TU1.1 engine-clock text caller and Unicode formatter.

The signed-short engine clock is an explicit fixture. Original clamping,
minute/second formatting and Unicode library calls execute without replacement.
This does not execute the HUD string-cache producer or graphics drawing.
"""
import hashlib
import json
from pathlib import Path
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX
from oracle_mp_spawn import ROOT, Cache, ENGINE_GLOBALS, VERSIONS
from oracle_mp_spawn_slayer_accounting import SlayerOracle

OUT = ROOT / 'analysis/multiplayer/integration/spawn-source'
BUFFER = 0x4133000

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    oracle = SlayerOracle()
    oracle.fixture(cache)
    cases = []
    for value in (-32768, -1, 0, 1, 9, 10, 30, 59, 60, 61, 119, 120, 480, 900, 1800, 32767):
        oracle.sh(ENGINE_GLOBALS + 0xe0, value)
        oracle.g.mem_write(BUFFER, bytes(64))
        oracle.accounting_events = []
        oracle.call(0x1600e0, registers=((UC_X86_REG_EAX, 0xe423),
            (UC_X86_REG_ECX, 32), (UC_X86_REG_EDX, BUFFER)))
        raw = bytes(oracle.g.mem_read(BUFFER, 64))
        words = struct.unpack('<32H', raw)
        text = ''.join(chr(v) for v in words[:words.index(0)])
        assert not oracle.accounting_events
        cases.append(dict(clock_seconds=value, text=text))
    dependencies = {f'tools/{name}': digest(ROOT / 'tools' / name) for name in (
        'oracle_mp_clock_text_1_1.py', 'oracle_mp_spawn_slayer_accounting.py',
        'oracle_mp_spawn.py', 'oracle_mp_spawn_ownership.py', 'oracle_mp_spawn_lifecycle.py')}
    executable = ROOT / VERSIONS['tu11']['path']
    from oracle_mp_spawn import Xbe
    xbe = Xbe(executable)
    listing = []
    for address, size in ((0x1600e0, 0x69), (0x163280, 0xa5)):
        listing += [f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'
            for i in Cs(CS_ARCH_X86, CS_MODE_32).disasm(xbe.read(address, size), address)]
    path = OUT / 'clock-text-instructions.txt'
    path.write_text('\n'.join(listing) + '\n')
    report = dict(scope=__doc__, cases=cases, dependencies=dependencies,
        executable=str(executable.relative_to(ROOT)), executable_sha256=digest(executable),
        cache='assets/disc/maps/lockout.map', cache_sha256=digest(ROOT / 'assets/disc/maps/lockout.map'),
        supplied_leaves=[], source_listing=str(path.relative_to(ROOT)), source_listing_sha256=digest(path))
    path = OUT / 'clock-text-report.json'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(cases)} unchanged original clock-text cases; {path}')

if __name__ == '__main__':
    main()
