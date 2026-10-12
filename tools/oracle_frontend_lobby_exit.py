#!/usr/bin/env python3
"""Execute original widget exit timing for Setup23 -> Map206.

Literal globals31/32 and unchanged22E957/22E65E/22E89C/22E3CD/148119
produce all samples. Only148350 supplies relocated unchanged wigl data.
This is a widget timing fixture, not a complete screen/channel GPU replay.
"""
from pathlib import Path
import hashlib
import json
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *

CACHE_PIN = '1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
cache = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
assert hashlib.sha256(cache.data).hexdigest() == CACHE_PIN
r = Retail()
g = r.g
_, p = cache.tag(next(t['datum'] for t in cache.tags.values() if t['group'] == 'wigl'), 'wigl')
globals_offset = p
globals_data = bytearray(cache.data[p:p + 452])
animations = cache.block(p + 0x120, 44)
relocated = bytearray()
assets = []
for index, q in enumerate(animations):
    raw = bytearray(cache.data[q:q + 44])
    for field in (8, 20, 36):
        keys = cache.block(q + field, 20)
        if keys:
            struct.pack_into('<I', raw, field + 4,
                r.alloc(b''.join(cache.data[k:k + 20] for k in keys)))
    relocated.extend(raw)
    if index in (30, 31):
        assets.append(dict(index=index + 1, milliseconds=cache.u32(q + 16),
            exit_keys_hex=[cache.data[k:k + 20].hex() for k in cache.block(q + 20, 20)]))
struct.pack_into('<I', globals_data, 0x124, r.alloc(relocated))
wigl = r.alloc(globals_data)
executed = {}
watch = (0x22e957, 0x22e65e, 0x22e89c, 0x22e3cd, 0x148119)

def hook(_machine, address, _size, _context):
    if address in watch:
        executed[address] = executed.get(address, 0) + 1
    if address == 0x148350:
        r.ret(wigl)
g.hook_add(UC_HOOK_CODE, hook)

def node(index, kind):
    pointer = r.alloc(bytes(0x100))
    r.w(0x54d5b8, 10000)
    r.run(0x22e27b, argv=[0], registers=[(UC_X86_REG_ESI, pointer), (UC_X86_REG_EAX, kind)])
    r.w(pointer, 0x458788)
    r.w(pointer + 0x70, 23)
    if kind:
        parent = node(31, 0)
        r.w(pointer + 0x10, parent)
    g.mem_write(pointer + 0x68, struct.pack('<h', index - 1))
    g.mem_write(0x54dbe0, bytes(35 * 4))
    if kind == 2:
        # Original row v5 is22E930: it applies the supplied definition to
        # the row and descendants. This fixture has no unrelated row data.
        table = r.alloc(bytes(g.mem_read(0x459f10, 0x60)))
        r.w(table + 0x14, 0x22e930)
        r.w(pointer, table)
    r.run(0x22e957, argv=[3], registers=[(UC_X86_REG_ECX, pointer)])
    assert r.r(pointer + 0x44) == 0  # exit ignores authored intro delay
    return pointer

def sample(pointer, index, kind, time, timeline):
    r.w(0x54d5b8, 10000 + time)
    r.run(0x22e3cd, argv=[10000 + time], registers=[(UC_X86_REG_ESI, pointer)])
    return dict(timeline=timeline, index=index, kind=kind, elapsed_ms=time,
        frame=r.read(pointer + 0x3e, '<h')[0],
        end_ms=r.r(pointer + 0x50) - 10000,
        queued=pointer in r.read(0x54dbe0, '<35I'),
        words=[f'{word:08x}' for word in r.read(pointer + 0x58, '<4I')])

rows = []
for index, kind in ((31, 0), (31, 6), (31, 8), (32, 2)):
    # Fresh samples expose the1ms interpolation minimum and late disposal.
    for time in (0, 1, 16, 17, 124, 125, 126, 249, 250, 251, 266, 499, 500, 501):
        pointer = node(index, kind)
        rows.append(sample(pointer, index, kind, time, f'fresh-{index}-{kind}-{time}'))
    for name, times in (('exact', [0, 125, 249, 250, 251, 499, 500]),
                        ('late', [0, 17, 124, 249, 266, 499, 500, 515, 516]),
                        ('60hz', [i * 1000 // 60 for i in range(33)])):
        pointer = node(index, kind)
        for time in times:
            rows.append(sample(pointer, index, kind, time, f'{name}-{index}-{kind}'))

for row in rows:
    if row['kind'] == 0 and row['timeline'] == 'exact-31-0':
        assert row['queued'] == (row['elapsed_ms'] >= 500)
    if row['kind'] == 0 and row['timeline'] == 'late-31-0':
        assert row['queued'] == (row['elapsed_ms'] >= 516)
    if row['kind']:
        assert not row['queued']
out = ROOT / 'analysis/frontend/lobby/exit-timing-original.json'
out.write_text(json.dumps(dict(scope=__doc__, xbe_sha256=PIN,
    mainmenu_sha256=CACHE_PIN, assets=assets,
    executed={f'{k:08x}': v for k, v in sorted(executed.items())},
    supplied=['148350: relocated literal wigl', 'base widget/row headers and parent links',
              'UI clock: explicit integer timelines'], cases=rows), indent=2) + '\n')
fixture = ROOT / 'tests/fixtures/frontend_lobby_exit_original.tsv'
fixture.write_text('# Original22E957/22E65E/22E89C/22E3CD/148119; XBE ' + PIN + '\n' +
    '# timeline index kind elapsed_ms frame end_ms queued x y z alpha (float words)\n' +
    ''.join(f"{x['timeline']} {x['index']} {x['kind']} {x['elapsed_ms']} {x['frame']} "
            f"{x['end_ms']} {int(x['queued'])} {' '.join(x['words'])}\n" for x in rows))
print(f'{len(rows)} original exit samples retained; JSON SHA256 {hashlib.sha256(out.read_bytes()).hexdigest()}')

# The authored Setup list uses skin0, with item-background and end-cap bitmap
#children. Its +6 value7 is row capacity, not the +4 skin index. Execute the
#original getter against the relocated literal pane, then the original focus
#predicates used by the existing flag2 end-cap children.
setup_tag, setup_offset = cache.tag(next(t['datum'] for t in cache.tags.values()
    if t['group'] == 'wgit' and t['name'].endswith('squad_settings_dialog')), 'wgit')
setup_pane = cache.block(setup_offset + 32, 76)[0]
setup_list = cache.block(setup_pane + 12, 24)[0]
list_data = cache.data[setup_list:setup_list + 24]
pane_data = bytearray(cache.data[setup_pane:setup_pane + 76])
struct.pack_into('<I', pane_data, 16, r.alloc(list_data))
literal_pane = r.alloc(pane_data)
skin_index = cache.u16(setup_list + 4)
skin_ref = cache.block(globals_offset + 0x138, 8)[skin_index]
skin_tag, skin_offset = cache.tag(cache.u32(skin_ref + 4), 'skin')
stock_setup = dict(list_raw=list_data.hex(), skin_index=skin_index,
    visible_rows=cache.u16(setup_list + 6),
    skin=dict(datum=f"{skin_tag['datum']:08x}", name=skin_tag['name']),
    text_children=len(cache.block(skin_offset + 28, 44)),
    bitmap_children=[dict(flags=cache.u32(q), datum=f'{cache.u32(q + 28):08x}',
        name=cache.tags[cache.u32(q + 28)]['name'], raw=cache.data[q:q + 56].hex())
        for q in cache.block(skin_offset + 36, 56)])
assert stock_setup['skin_index'] == 0 and stock_setup['visible_rows'] == 7
assert stock_setup['skin']['datum'] == 'e4d902b9'
assert len(stock_setup['bitmap_children']) == 2
assert [x['datum'] for x in stock_setup['bitmap_children']] == ['e4db02bb', 'e4dc02bc']
focus = 0
def marker_hook(_machine, address, _size, _context):
    if address == 0x148d91:
        r.ret(focus)
    if address == 0x22f899:
        r.ret(literal_pane)
g.hook_add(UC_HOOK_CODE, marker_hook)
def marker_node(kind, table, parent=0, screen=0):
    pointer = r.alloc(bytes(0x100))
    r.run(0x22e27b, argv=[0], registers=[(UC_X86_REG_ESI, pointer), (UC_X86_REG_EAX, kind)])
    r.w(pointer, table)
    if parent: r.w(pointer + 0x10, parent)
    if screen:
        r.w(pointer + 0x70, screen)
        r.w(pointer + 0x74, 3)
        r.w(pointer + 0x78, 4)
    return pointer
old = marker_node(0, 0x45c518, screen=23)
new = marker_node(0, 0x45c650, screen=206)
list_pointer = marker_node(1, 0x45c5f8, old)
r.w(old + 0x14, list_pointer)
resolved_skin = r.run(0x24c0b3, registers=[(UC_X86_REG_EAX, list_pointer)])
assert resolved_skin == skin_index
stock_setup['original_24C0B3_skin_index'] = resolved_skin
items = [marker_node(2, 0x459f10, list_pointer) for _ in range(4)]
r.w(list_pointer + 0x14, items[0])
for a, b in zip(items, items[1:]): r.w(a + 0x18, b)
markers = []
for outgoing, focus in ((False, items[0]), (True, new)):
    focused = r.run(0x24bae6, registers=[(UC_X86_REG_EAX, list_pointer)])
    for index, item in enumerate(items):
        selected = bool(r.run(0x24c3f8, argv=[item], registers=[(UC_X86_REG_ECX, list_pointer)]) & 255)
        before = bool(r.run(0x24c40b, argv=[item], registers=[(UC_X86_REG_EDI, list_pointer)]) & 255)
        markers.append(dict(outgoing=outgoing, row=index, selected=selected,
            before=before, frame=0 if selected else 1 if before else 2,
            focused_row=items.index(focused) if focused in items else None))
assert [x['frame'] for x in markers] == [0, 2, 2, 2, 1, 1, 1, 1]
marker_report = out.with_name('exit-markers-original.json')
marker_report.write_text(json.dumps(dict(xbe_sha256=PIN, mainmenu_sha256=CACHE_PIN,
    executed=['24C0B3', '24BBF3', '24BAE6', '22ED7A', '24C3F8', '24C40B'],
    supplied=['22F899: relocated literal Setup pane',
              '148D91: explicit current focused widget', 'base list/row headers and links'],
    scope='Original Setup skin0 getter and list focus predicates for its existing flag2 end-cap.',
    stock_setup=stock_setup, cases=markers), indent=2) + '\n')
marker_fixture = fixture.with_name('frontend_lobby_exit_markers_original.tsv')
marker_fixture.write_text('# outgoing row selected before bitmap_frame; original24BAE6/24C3F8/24C40B\n' +
    ''.join(f"{int(x['outgoing'])} {x['row']} {int(x['selected'])} {int(x['before'])} {x['frame']}\n"
            for x in markers))
print(f'{len(markers)} original outgoing-list marker predicate cases retained')
