#!/usr/bin/env python3
"""Execute retail/TU1.1 variant validation, normalization and literal titles.

No validation, normalization, initializer or localization return is supplied.
The no-locale cases use NULL globals. Localized cases run the original string
table allocator/loader, list lookup, UTF8 converter and bounded UTF16 copies
against literal mainmenu bytes. Only raw cache reads and memory protection are
platform seams; the original allocator and loaded flag writes execute.
"""
import hashlib
import json
import struct
from collections import Counter
from pathlib import Path
from oracle_game_variants import Source, ROOT, VERSIONS, VARIANT, MENU_PIN, VALUES
from audit_rendering import Cache
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

OUT = ROOT / 'analysis/multiplayer/game-variants'
FIXTURE = ROOT / 'tests/fixtures/game_variant_normalization_original.tsv'
LOCAL = {
    'retail': dict(validator=0x19d620, table=0x4e3b44, language=0x47ff38,
        loader=0x1a01e0, pool_index=0x4e6420, pool_min=0x4e642c,
        pool_top=0x4e6440, read=0x122d60, protect=0x2d15da),
    'tu11': dict(validator=0x1a0e90, table=0x4eb114, language=0x487440,
        loader=0x1a3b40, pool_index=0x4ed9f0, pool_min=0x4ed9fc,
        pool_top=0x4eda10, read=0x1244b0, protect=0x2d7a1a),
}


class Localized(Source):
    def __init__(self, name, cache):
        super().__init__(VERSIONS[name])
        self.local = LOCAL[name]; self.cache = cache; self.seams = []
        self.g.mem_map(0x4000000, 0x400000)
        self.g.mem_map(0x84000000, 0x400000)
        self.next = 0x4000000
        c = cache
        _, p = c.tag(next(t['datum'] for t in c.tags.values() if t['group'] == 'matg'), 'matg')
        globals = self.alloc(c.data[p:p + 0x1ac])
        table = self.alloc(bytes(0x10000 * 16))
        self.w(self.spec['globals'], globals); self.w(self.local['table'], table)
        self.w(self.local['language'], 0)  # Explicit English environment.
        mulg = c.u32(p + 0x16c); _, m = c.tag(mulg, 'mulg')
        header = bytearray(c.data[m:m + 8]); universal = c.block(m, 0x20)
        assert len(universal) == 1
        element = self.alloc(c.data[universal[0]:universal[0] + 0x20])
        struct.pack_into('<I', header, 4, element)
        self.w(table + (mulg & 0xffff) * 16 + 8, self.alloc(header))
        strings = c.u32(universal[0] + 0x1c); _, u = c.tag(strings, 'unic')
        self.w(table + (strings & 0xffff) * 16 + 8, self.alloc(c.data[u:u + 0x34]))
        self.w(self.local['pool_index'], 0)
        self.w(self.local['pool_min'], 0x4000000)
        self.w(self.local['pool_top'], 0x4400000)
        self.g.hook_add(UC_HOOK_CODE, self.platform)
        # The literal on-disk locale header starts at188: its pointer words
        # are empty, count/size are190/194 and file offsets are198/19C.
        self.call(self.local['loader'], registers=[(UC_X86_REG_EDI, globals + 0x188)])
        assert self.g.mem_read(globals + 0x1a0, 1) == b'\x01'
        assert len([s for s in self.seams if s['kind'] == 'read']) == 2
        self.loaded_header = list(struct.unpack('<7I', self.g.mem_read(globals + 0x188, 28)))

    def alloc(self, data):
        address = self.next; self.next = (address + len(data) + 15) & ~15
        assert self.next < 0x4100000 + 0x10000
        self.g.mem_write(address, bytes(data)); return address

    def w(self, at, value): self.g.mem_write(at, struct.pack('<I', value & 0xffffffff))

    def returned(self, value=0, pop=0):
        sp = self.g.reg_read(UC_X86_REG_ESP)
        self.g.reg_write(UC_X86_REG_EAX, value)
        self.g.reg_write(UC_X86_REG_EIP, self.u(sp))
        self.g.reg_write(UC_X86_REG_ESP, sp + 4 + pop)

    def platform(self, guest, address, size, _):
        sp = guest.reg_read(UC_X86_REG_ESP)
        if address == self.local['read']:
            index, offset = self.u(sp + 4), self.u(sp + 8)
            destination, count = guest.reg_read(UC_X86_REG_EDX), guest.reg_read(UC_X86_REG_ESI)
            assert index == 0xffffffff and destination >= 0x84000000
            assert 0 <= offset < len(self.cache.data) and offset + count <= len(self.cache.data)
            guest.mem_write(destination, self.cache.data[offset:offset + count])
            self.seams.append(dict(kind='read', offset=offset, count=count, destination=destination,
                sha256=hashlib.sha256(self.cache.data[offset:offset + count]).hexdigest()))
            self.returned(1, 8)
        elif address == self.local['protect']:
            args = [self.u(sp + 4 + i * 4) for i in range(3)]
            assert args[0] >= 0x84000000 and args[2] == 4
            self.seams.append(dict(kind='protect', args=args)); self.returned(1, 12)


def inputs(source, variants):
    seen = {}
    def add(label): seen.setdefault(source.raw(), label)
    for engine in range(7):
        source.init(engine); add(f'default-{engine}')
        for parameter in range(112):
            for value in VALUES:
                source.init(engine); source.set(parameter, value)
                add(f'setter-{engine}-{parameter}-{value}')
        source.init(engine); default = bytes.fromhex(source.raw())
        for offset in range(0x130):
            mutated = bytearray(default); mutated[offset] ^= 0x80
            source.g.mem_write(VARIANT, bytes(mutated)); add(f'byte-{engine}-{offset}')
    for index, variant in enumerate(variants):
        source.init(variant['engine'])
        for parameter, value in variant['overrides'].items(): source.set(parameter, value)
        add(f'authored-{index}')
    return [(label, raw) for raw, label in seen.items()]


def normalize(source, validator, cases, locale):
    source.init(0); fallback = source.raw()
    results = []
    for index, (label, raw) in enumerate(cases):
        source.g.mem_write(VARIANT, bytes.fromhex(raw))
        valid = source.call(validator, registers=[(UC_X86_REG_EAX, VARIANT)]) & 255
        assert source.raw() == raw, ('validator mutated caller', label)
        accepted = source.call(source.spec['normalize'], registers=[(UC_X86_REG_EBX, VARIANT)]) & 255
        after = source.raw()
        assert valid == accepted and after == (raw if accepted else fallback), label
        results.append([locale, label, raw, after, accepted])
        if index and index % 2000 == 0: print(f'{source.spec["path"]}: compared {index} inputs', flush=True)
    return results


def main():
    from inspect_multiplayer import Cache as VariantCache
    cache = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
    assert hashlib.sha256(cache.data).hexdigest() == MENU_PIN
    variants = VariantCache(ROOT / 'assets/disc/maps/mainmenu.map').inspect_variants()['variants']
    assert len(variants) == 39
    reports = {}; source_cases = None; localized_cases = None; localized_defaults = None
    OUT.mkdir(parents=True, exist_ok=True)
    for name, spec in VERSIONS.items():
        s = Source(spec)
        if source_cases is None: source_cases = inputs(s, variants)
        rows = normalize(s, LOCAL[name]['validator'], source_cases, 0)
        local = Localized(name, cache)
        defaults = []
        for engine in range(7):
            local.init(engine)
            defaults.append([engine, local.raw()])
        if localized_defaults is None: localized_defaults = defaults
        else: assert defaults == localized_defaults, 'localized defaults differ across executable versions'
        if localized_cases is None:
            localized_cases = []
            for engine, raw in defaults:
                localized_cases.append((f'localized-default-{engine}', raw))
                for parameter, value in [(2, -1), (10, 32768), (91, 1), (89, 5)]:
                    local.g.mem_write(VARIANT, bytes.fromhex(raw)); local.set(parameter, value)
                    localized_cases.append((f'localized-edit-{engine}-{parameter}-{value}', local.raw()))
        rows += normalize(local, LOCAL[name]['validator'], localized_cases, 1)
        reports[name] = rows
        decoder = Cs(CS_ARCH_X86, CS_MODE_32)
        listing = []
        for address in sorted(s.executed | local.executed):
            ins = next(decoder.disasm(s.image.read(address, 15), address))
            listing.append(f'{address:08x} {ins.bytes.hex():30} {ins.mnemonic} {ins.op_str}')
        (OUT / f'normalization-{name}-instructions.txt').write_text('\n'.join(listing) + '\n')
        (OUT / f'normalization-{name}-locale.json').write_text(json.dumps(dict(
            loaded_header=local.loaded_header, platform_seams=local.seams), indent=2) + '\n')
    assert reports['retail'] == reports['tu11'], 'normalization contracts differ'
    rows = reports['retail']
    lines = ['# Unchanged retail/TU1.1 validity/normalization; literal loaded English titles and no-locale paths.']
    for engine, raw in localized_defaults: lines.append(f'D {engine} {raw}')
    for locale, label, before, after, valid in rows:
        lines.append(f'N {locale} {label} {before} {after} {valid}')
    FIXTURE.write_text('\n'.join(lines) + '\n')
    result = dict(scope=__doc__, source_versions=VERSIONS, extra_source_addresses=LOCAL,
        mainmenu_sha256=MENU_PIN, rows=len(rows), localized_defaults=localized_defaults,
        result_counts=dict(Counter(f'locale{r[0]}-valid{r[4]}' for r in rows)),
        fixture_sha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (OUT / 'normalization-original.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result['result_counts']), flush=True)


if __name__ == '__main__': main()
