#!/usr/bin/env python3
"""Replay unchanged retail armor inputs, profile copy and emblem palette query.

Object and player pools are host allocations. Actual Lockout matg palette
words are transplanted without alteration. The profile copy stops before the
unrelated color producer; the input and palette functions run to their RET.
This is original CPU execution, not an original GPU framebuffer comparison.
"""
from pathlib import Path
import hashlib
import json
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from oracle_multiplayer_hud_positions import Retail, PIN
from audit_rendering import Cache, file_sha
from xbe import Xbe
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDI, UC_X86_REG_ESI

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/armor-emblem'
OUT.mkdir(parents=True, exist_ok=True)
c = Cache(ROOT / 'assets/disc/maps/lockout.map')
r = Retail()
layer_targets = []
for layer in (1, 2, 3, 8, 15):
    words = struct.unpack('<9I', r.g.mem_read(0x43fa1c + layer*0x24, 0x24))
    layer_targets.append(dict(layer=layer, target=words[0],
                              raw_words=[f'{word:08x}' for word in words]))
assert layer_targets[0]['target'] == 1
assert layer_targets[1]['target'] == 0xffffffff
(OUT / 'original-layer-targets.json').write_text(json.dumps(layer_targets, indent=2) + '\n')
objects = r.alloc(bytes(0x50))
table = r.alloc(bytes(12))
obj = r.alloc(bytes(0x340))
r.w(objects + 0x44, table)
r.w(table + 8, obj)
r.w(0x4e0300, objects)
scenario = r.alloc(bytes(0x30))
value, enabled = r.alloc(bytes(4)), r.alloc(bytes(4))
names = [0x0d0005ff, 0x170005fa, 0x170005fb, 0x180005fc,
         0x1d0005fd, 0x1d0005fe, 0, 0x0c0005cb]
cases = [(f, f % 32, 0) for f in range(256)]
cases += [(26, b, 0) for b in range(256)]
cases += [(26, 9, flags) for flags in range(256)]
rows = []
for kind in [-1, 0, 1, 2]:
    r.w(0x4e0350, 0 if kind == -1 else scenario)
    r.g.mem_write(scenario + 0x10, struct.pack('<H', max(kind, 0)))
    for foreground, background, flags in cases:
        r.g.mem_write(obj + 0x24, bytes([foreground, background, flags, 0xa5]))
        for name in names:
            r.run(0xbcc20, argv=[0, name, value, enabled])
            supported = r.g.reg_read(UC_X86_REG_EAX) & 255
            rows.append(f'I {kind} {foreground} {background} {flags} {name:08x} '
                        f'{r.r(value):08x} {supported} {r.r(enabled) & 255}')

players = r.alloc(bytes(0x50))
player = r.alloc(bytes(0x21c))
r.w(players + 0x44, player)
r.w(0x4e8c24, players)
for foreground, background, flags in cases:
    emblem = bytes([foreground, background, flags, 0xa5])
    r.g.mem_write(player + 0x89, emblem)
    r.run(0x14bfc0, stop=0x14c01c,
          registers=[(UC_X86_REG_EAX, 0), (UC_X86_REG_EBX, 0)])
    copied = bytes(r.g.mem_read(obj + 0x24, 4))
    assert copied == emblem
    rows.append(f'C {emblem.hex()} {copied.hex()}')

_, matg = c.tag(c.u32(c.u32(16) + 16), 'matg')
palette = b''.join(c.data[p:p+12] for p in c.block(matg + 0x160, 12))
globals_copy = r.alloc(bytes(0x168))
r.w(globals_copy + 0x160, len(palette)//12)
r.w(globals_copy + 0x164, r.alloc(palette))
r.w(0x4e034c, globals_copy)
appearance, colors = r.alloc(bytes(16)), r.alloc(bytes(48))
r.w(0x4e0350, scenario)
for index in range(256):
    r.g.mem_write(appearance, bytes([3, 4, index, 255-index]) + bytes(12))
    r.run(0x7f790, argv=[0xffffffff, 0],
          registers=[(UC_X86_REG_EDI, appearance), (UC_X86_REG_ESI, colors)])
    words = struct.unpack('<6I', r.g.mem_read(colors + 24, 24))
    rows.append(f'P {index} {255-index} ' + ' '.join(f'{w:08x}' for w in words))
fixture = ROOT / 'tests/fixtures/player_armor_emblem_original.tsv'
fixture.write_text('# Original BCC20 -> BC970;14BFC0 prefix;7F790 ->1A06A0\n'
                   '# XBE sha256 ' + PIN + '\n' + '\n'.join(rows) + '\n')
report = dict(method=__doc__, xbe_sha256=PIN, map_sha256=file_sha(c.path),
              input_cases=4*len(cases)*len(names), copy_cases=len(cases), palette_cases=256,
              palette_entries=len(palette)//12, fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest())
# The map-pack TU1.1 producer has the same object fields and scenario predicate.
update = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
update_pin = 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
assert hashlib.sha256(update.data).hexdigest() == update_pin
u = Retail()
for section in update.sections:
    u.g.mem_write(section.va, update.section_bytes(section))
u.g.mem_write(0x1000000, bytes(r.g.mem_read(0x1000000, r.next-0x1000000)))
u.w(0x4e78d0, objects)
updated_cases = 0
for line in rows:
    if not line.startswith('I '):
        continue
    _, kind, foreground, background, flags, name, expected, supported, active = line.split()
    kind = int(kind)
    u.w(0x4e7920, 0 if kind == -1 else scenario)
    u.g.mem_write(scenario + 0x10, struct.pack('<H', max(kind, 0)))
    u.g.mem_write(obj + 0x24, bytes([int(foreground), int(background), int(flags), 0xa5]))
    result = u.run(0xbdb10, argv=[0, int(name,16), value, enabled])
    assert (u.r(value), result&255, u.r(enabled)&255) == (int(expected,16), int(supported), int(active)), line
    updated_cases += 1
report.update(update_version='map-pack TU1.1', update_xbe_sha256=update_pin, update_input_cases=updated_cases,
              update_method='unchanged BDB10 ->BD860; exact comparison with every retail input case')
# Validate the exact owned u5 executable selected by the installed launcher.
owned_path = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
owned = Xbe(owned_path)
owned_pin = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
assert hashlib.sha256(owned.data).hexdigest() == owned_pin
for section in owned.sections:
    u.g.mem_write(section.va, owned.section_bytes(section))
u.w(0x4ecbd0, objects)
owned_cases = 0
for line in rows:
    if not line.startswith('I '):
        continue
    _, kind, foreground, background, flags, name, expected, supported, active = line.split()
    kind = int(kind)
    u.w(0x4ecc20, 0 if kind == -1 else scenario)
    u.g.mem_write(scenario + 0x10, struct.pack('<H', max(kind, 0)))
    u.g.mem_write(obj + 0x24, bytes([int(foreground), int(background), int(flags), 0xa5]))
    result = u.run(0xbec50, argv=[0, int(name,16), value, enabled])
    assert (u.r(value), result&255, u.r(enabled)&255) == (int(expected,16), int(supported), int(active)), line
    owned_cases += 1
report.update(owned_xbe_sha256=owned_pin, owned_input_cases=owned_cases,
              owned_method='unchanged BEC50 ->BE9A0; exact comparison with every retail input case')
(OUT / 'original.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
