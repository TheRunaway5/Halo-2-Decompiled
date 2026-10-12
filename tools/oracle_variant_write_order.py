#!/usr/bin/env python3
"""Compare ordered/duplicate variant writes using unchanged retail/TU instructions.

The original tag builders1489B5/149F88 iterate settings rows in authored order.
This oracle executes the original initializer, setters, getters and copy
validator for every overlapping descriptor pair in each of seven families.
Globals are NULL (no localized title); no setter/getter/validation result is
supplied, and no native code is imported. Native preference codecs and menu
callbacks are outside this oracle's scope.
"""
import hashlib
import json
import struct
from collections import Counter
from pathlib import Path
from oracle_game_variants import Source, ROOT, VERSIONS, RESULTS, VARIANT, MENU_PIN
from audit_rendering import Cache
from unicorn.x86_const import UC_X86_REG_EAX

OUT = ROOT / 'analysis/multiplayer/game-variants'
FIXTURE = ROOT / 'tests/fixtures/variant_write_order_original.tsv'
VALIDATORS = {'retail': 0x19d620, 'tu11': 0x1a0e90}


def descriptors(source):
    result = []
    for parameter in range(112):
        address = source.u(source.spec['table'] + parameter * 4)
        if address:
            source.call(address, (RESULTS, RESULTS + 4, RESULTS + 8))
            result.append([source.u(RESULTS + offset) for offset in (0, 4, 8)])
        else:
            result.append([-1, 0, 0])
    return result


def mask(descriptor):
    kind, offset, operand = descriptor
    if kind == 1:
        bit = operand & 31
        return {offset + bit // 8: 1 << (bit % 8)}
    return {offset + n: 255 for n in range({3: 1, 4: 2, 5: 4}.get(kind, 0))}


def main():
    menu = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
    assert hashlib.sha256(menu.data).hexdigest() == MENU_PIN
    original = Source(VERSIONS['retail']); table = descriptors(original)
    pairs = [(a, b) for a, x in enumerate(table) for b, y in enumerate(table)
             if a < b and any(bits & mask(y).get(offset, 0)
                              for offset, bits in mask(x).items())]
    assert len(pairs) == 150
    cases = []
    for engine in range(7):
        for a, b in pairs:
            for writes in ([(a, 1), (b, 2)], [(b, 2), (a, 1)],
                           [(a, 261), (b, -1), (a, 2)]):
                cases.append((engine, writes, sorted({a, b})))
    _, goof = menu.tag(next(t['datum'] for t in menu.tags.values()
                           if t['group'] == 'goof'), 'goof')
    rows = menu.block(goof + 40, 20)
    assert len(rows) == 39
    for row in rows:
        writes = [(menu.u32(p), struct.unpack_from('<i',menu.data,p + 4)[0])
                  for p in menu.block(row + 8, 8)]
        cases.append((menu.u32(row + 4), writes, sorted({p for p, _ in writes})))
    cases += [(0, [(2, 10), (1, 2), (2, 3)], [1, 2, 3, 9]),
              (0, [(2, 32768), (1, 2)], [1, 2]),
              (0, [(1, 2), (2, 32768)], [1, 2]),
              (0, [(89, 261), (88, -1), (92, 0), (30, 1)], [89, 88, 92, 30]),
              (0, [(2, 7), (1, 4), (2, 2), (89, 261), (88, -1)], [2, 89, 88]),
              (0, [(112, 1), (0xffffffff, -2147483648)], [112, 0xffffffff])]
    reports = {}
    for name, spec in VERSIONS.items():
        source = Source(spec)
        assert descriptors(source) == table
        results = []
        for index, (engine, writes, queries) in enumerate(cases):
            source.init(engine)
            for parameter, value in writes:
                source.set(parameter, value)
            raw = source.raw()
            values = [(parameter, source.get(parameter)) for parameter in queries]
            valid = source.call(VALIDATORS[name], registers=[(UC_X86_REG_EAX, VARIANT)]) & 255
            assert source.raw() == raw
            results.append([engine, writes, raw, valid, values])
            if index and index % 1000 == 0:
                print(f'{name}: {index} ordered inputs compared', flush=True)
        reports[name] = results
    assert reports['retail'] == reports['tu11']
    lines = ['# Unchanged retail/TU1.1 ordered setter/getter/validator outputs; no native code imported.']
    for engine, writes, raw, valid, queries in reports['retail']:
        fields = ['O', engine, len(writes)] + [x for pair in writes for x in pair]
        fields += [raw, valid, len(queries)] + [x for pair in queries for x in pair]
        lines.append(' '.join(map(str, fields)))
    FIXTURE.write_text('\n'.join(lines) + '\n')
    result = dict(scope=__doc__, source_versions=VERSIONS, validators=VALIDATORS,
                  descriptor_pairs=pairs, rows=len(cases), authored_rows=len(rows),
                  result_counts=dict(Counter(r[3] for r in reports['retail'])),
                  fixture_sha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (OUT / 'ordered-settings-original.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(rows=len(cases), overlapping_pairs=len(pairs), authored_rows=len(rows))), flush=True)


if __name__ == '__main__':
    main()
