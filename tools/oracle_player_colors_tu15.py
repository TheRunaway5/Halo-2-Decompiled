#!/usr/bin/env python3
"""Replay unchanged TU1.5 830C0/83050/1AA2B0 player color instructions.

Actual Lockout globals/team palettes remain read-only reference inputs. Only
appearance, team and neutral arguments are supplied; no game return is mocked.
Run under tools/run_test.py; retain only the small fixture and provenance.
"""
import argparse
import hashlib
import json
from pathlib import Path

from oracle_multiplayer_hud_positions import (
    Retail, ROOT, Xbe, UC_X86_REG_EDI, UC_X86_REG_ESI)
from inspect_multiplayer_hud import HudCache

TU15_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    cache = HudCache(ROOT / 'assets/disc/maps/lockout.map')
    trace = Retail()
    image = Xbe(args.xbe)
    assert hashlib.sha256(image.data).hexdigest() == TU15_PIN
    for section in image.sections:
        trace.g.mem_write(section.va, image.section_bytes(section))
    page = cache.magic & ~4095
    trace.g.mem_map(page, ((len(cache.data) + cache.magic - page + 4095) // 4096) * 4096)
    trace.g.mem_write(cache.magic, bytes(cache.data))
    for datum, tag in cache.tags.items():
        if tag['offset'] is not None:
            trace.w(trace.table + (datum & 65535) * 16 + 8, cache.magic + tag['offset'])
    matg = next(tag for tag in cache.tags.values() if tag['group'] == 'matg')
    trace.w(0x4ecc1c, cache.magic + matg['offset'])
    trace.w(0x4f0414, trace.table)
    trace.w(0x4ecc20, 1)  # A present scenario admits the actual matg palette.
    appearance, output = trace.alloc(bytes(16)), trace.alloc(bytes(48))
    profiles = [(8, 12, 9, 1), (3, 4, 5, 6), (255, 128, 18, 127)]
    cases = [(profile, team, neutral) for profile in profiles
             for team in range(-2, 10) for neutral in (False, True)]
    cases += [((index,) * 4, -1, False) for index in range(256)]
    rows = ['# Unchanged Xbox TU1.5 player color producer 830C0',
            '# XBE SHA256 ' + TU15_PIN]
    globals_address = cache.magic + matg['offset']
    for index in range(trace.r(globals_address + 0x160)):
        address = trace.r(globals_address + 0x164) + index * 12
        rows.append(f'palette {index} ' + ' '.join(
            f'{trace.r(address + channel * 4):08x}' for channel in range(3)))
    mulg = cache.tags[cache.u(matg['offset'] + 0x16c)]
    universal = cache.block(mulg['offset'], 0x20)[0]
    for index, address in enumerate(cache.block(universal + 0x10, 12)):
        rows.append(f'team {index} ' + ' '.join(
            f'{cache.u(address + channel * 4):08x}' for channel in range(3)))
    for colors, team, neutral in cases:
        profile = bytes(colors) + bytes(12)
        trace.g.mem_write(appearance, profile)
        trace.run(0x830c0, argv=(team & 0xffffffff, int(neutral)),
                  registers=((UC_X86_REG_EDI, appearance), (UC_X86_REG_ESI, output)))
        words = [trace.r(output + i * 4) for i in range(12)]
        rows.append(f'{bytes(colors).hex()} {team} {int(neutral)} ' +
                    ' '.join(f'{word:08x}' for word in words))
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text('\n'.join(rows) + '\n')
    report = dict(method=__doc__, xbe_sha256=TU15_PIN,
                  oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  map_sha256=hashlib.sha256(cache.data).hexdigest(),
                  cases=len(cases), instruction_patches=[], supplied_game_returns=[],
                  fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest())
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {len(cases)} unchanged TU1.5 player color calls')


if __name__ == '__main__':
    main()
