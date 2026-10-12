#!/usr/bin/env python3
"""Execute original standard-text setup and draw-time reveal prefix. CPU only."""
import hashlib
import json
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_ESP

from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from xbe import Xbe

OUT = ROOT / 'analysis/frontend/settings-typewriter-original'
OUT.mkdir(parents=True, exist_ok=True)
cache = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
assert hashlib.sha256(cache.data).hexdigest() == '1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
_, wigl = cache.tag(next(k for k, t in cache.tags.items() if t['group'] == 'wigl'), 'wigl')
_, screen = cache.tag(next(k for k, t in cache.tags.items() if t['group'] == 'wgit' and cache.u16(t['address'] - cache.magic + 4) == 19), 'wgit')
_, matg = cache.tag(next(k for k, t in cache.tags.items() if t['group'] == 'matg'), 'matg')
loc = matg + 0x190
locale = []
for index in range(cache.u32(loc)):
    at = cache.u32(loc + 8) + index * 8
    offset = cache.u32(at + 4)
    locale.append((cache.u32(at), cache.string(cache.u32(loc + 12) + offset, cache.u32(loc + 4) - offset)))
_, unic = cache.tag(cache.u32(screen + 28), 'unic')
localized = dict(locale[cache.u16(unic + 16):cache.u16(unic + 16) + cache.u16(unic + 18)])
pane = cache.block(screen + 32, 76)[0]
definitions = cache.block(pane + 28, 44)
assert len(definitions) == 8
actual_shared = bytes(cache.data[wigl:wigl + 0x180])
assert actual_shared[0x48:0x50].hex() == '3200db009a99993e'

retail = Retail()
guest = retail.g
image = Xbe(ROOT / 'assets/disc/default.xbe')
shared = retail.alloc(actual_shared)
widget = retail.alloc(bytes(0x400))
buffer = widget + 0x74
definition = retail.alloc(bytes(44))
screen_parent = retail.alloc(bytes(0x100))
bounds = retail.alloc(struct.pack('<4h', 0, 0, 100, 200))
selected_string = ''
services = {}
visited = {}
visits = 0


def observe(g, pc, size, _):
    global visits
    visits += 1
    if pc not in visited:
        raw = bytes(g.mem_read(pc, size))
        assert raw == image.read(pc, size), hex(pc)
        visited[pc] = raw
    if pc in (0x22eeee, 0x230134, 0x148350, 0x13ee20):
        services[hex(pc)] = services.get(hex(pc), 0) + 1
    if pc == 0x22eeee:
        retail.ret(screen_parent)
    elif pc == 0x230134:
        # Supplied localization service returns the literal actual unic text.
        g.mem_write(g.reg_read(UC_X86_REG_ESI), (selected_string + '\0').encode('utf-16le'))
        retail.ret(0)
    elif pc == 0x148350:
        retail.ret(shared)
    elif pc == 0x13ee20:
        # Font readiness/prediction is outside the prefix under test.
        sp = g.reg_read(UC_X86_REG_ESP)
        retail.ret(1)
        g.reg_write(UC_X86_REG_ESP, sp + 8)


guest.hook_add(UC_HOOK_CODE, observe)


def setup(index):
    global selected_string
    at = definitions[index]
    selected_string = localized[cache.u32(at + 36)]
    guest.mem_write(widget, bytes(0x400))
    retail.w(widget, 0x458940)
    retail.run(0x22f532, registers=[(UC_X86_REG_ECX, buffer)])
    guest.mem_write(definition, bytes(cache.data[at:at + 44]))
    retail.run(0x253765, argv=[widget, index, definition])
    result = dict(index=index, authored_font=cache.u16(at + 10), flags=cache.u32(at),
        text=selected_string, length=retail.read(buffer + 0x3e, '<h')[0],
        runtime_font=retail.r(buffer + 4), buffer_flags=retail.read(buffer + 0x14, '<H')[0],
        scale_word=retail.r(buffer + 0x20), cursor=retail.read(buffer + 0x3c, '<h')[0],
        last_clock=retail.r(buffer + 0x40), definition_hex=bytes(cache.data[at:at + 44]).hex())
    return result


def draw(clock):
    before = retail.read(buffer + 0x3c, '<hhI')
    retail.w(0x54d5b8, clock)
    retail.run(0x22cd48, stop=0x22ce32,
        argv=[bounds, bounds, 0, 0x3f800000, bounds], registers=[(UC_X86_REG_EBX, buffer)])
    temporary = guest.reg_read(UC_X86_REG_EBP) - 0x830
    units = list(struct.unpack('<1024H', guest.mem_read(temporary, 2048)))
    units = units[:units.index(0)]
    after = retail.read(buffer + 0x3c, '<hhI')
    return dict(clock=clock, cursor_before=before[0], length=before[1], last_clock_before=before[2],
        cursor_after=after[0], last_clock_after=after[2], rendered_units=units,
        rendered_utf16_hex=struct.pack('<' + 'H' * len(units), *units).hex())


setups, timelines, controls = [], [], []
for index in range(8):
    initialized = setup(index)
    setups.append(initialized)
    if index == 0:
        assert initialized['runtime_font'] == 1 and initialized['cursor'] == -1
        continue
    assert initialized['flags'] == 9 and initialized['authored_font'] == 9
    assert initialized['runtime_font'] == 1 and initialized['buffer_flags'] == 4
    assert initialized['scale_word'] == 0x3e99999a and initialized['cursor'] == 0
    assert initialized['length'] == len(selected_string.encode('utf-16le')) // 2
    # Includes exact threshold, repeated draw, long frame and completion.
    clocks = [100, 100, 149, 150, 150, 1000, 1000, 1049, 1050]
    clocks += list(range(1100, 1100 + initialized['length'] * 50, 50))
    timeline = [draw(clock) for clock in clocks]
    assert timeline[0]['rendered_units'] == [0xdb]
    assert timeline[3]['cursor_after'] == 1 and timeline[3]['rendered_units'] == [0xdb]
    assert timeline[5]['cursor_after'] == 2  # No catch-up for850ms gap.
    assert timeline[-1]['cursor_after'] == -1
    timelines.append(dict(index=index, draws=timeline))
for label, clock, cursor, last in (
        ('zero-clock sentinel first', 0, 0, 0),
        ('zero-clock sentinel second', 50, 0, 0),
        ('unsigned rollover threshold', 0x22, 0, 0xfffffff0),
        ('unsigned rollover below threshold', 0x21, 0, 0xfffffff0),
        ('finished does not restart', 1000, -1, 100),
        ('final cursor disappears on next draw', 150, 1, 100)):
    setup(1)
    if label.startswith('final'):
        guest.mem_write(buffer + 0x3e, struct.pack('<h', 2))
    guest.mem_write(buffer + 0x3c, struct.pack('<h', cursor))
    retail.w(buffer + 0x40, last)
    controls.append(dict(label=label, **draw(clock)))
# Original set_text/update_length preserves clock and activates only an already
# active cursor; standard setup flag8 explicitly reactivates afterward.
for cursor in (2, -1):
    setup(1)
    guest.mem_write(buffer + 0x3c, struct.pack('<h', cursor))
    retail.w(buffer + 0x40, 1234)
    supplied = retail.alloc('Changed\0'.encode('utf-16le'))
    retail.run(0x22f545, argv=[supplied], registers=[(UC_X86_REG_ECX, buffer)])
    controls.append(dict(label='set_text active' if cursor >= 0 else 'set_text finished',
        cursor_before=cursor, cursor_after=retail.read(buffer + 0x3c, '<h')[0],
        length=retail.read(buffer + 0x3e, '<h')[0], last_clock_after=retail.r(buffer + 0x40)))

instructions = {}
for name, start, end in (('setup', 0x253765, 0x2538a6),
        ('reveal-prefix', 0x22cd48, 0x22ce32),
        ('length-update', 0x22f4db, 0x22f4fa)):
    raw = image.read(start, end - start)
    assembly = '\n'.join(f'{i.address:08X} {i.bytes.hex():24} {i.mnemonic} {i.op_str}'
        for i in Cs(CS_ARCH_X86, CS_MODE_32).disasm(raw, start)) + '\n'
    (OUT / f'{name}.asm').write_text(assembly)
    instructions[name] = hashlib.sha256(raw).hexdigest()
report = dict(passed=True, scope='Unchanged253765 setup/22CD0A/original256-unit buffer copy and length update; unchanged22CD48 draw prefix through22CE32. Actualscreen19/wigl/unic. Caller screen/localization/shared-global getter and font readiness are explicit services. No raster/GPU/framebuffer/font measurement or whole lifecycle claim.',
    executable_sha256=PIN, cache_sha256=hashlib.sha256(cache.data).hexdigest(),
    script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    shared_typing_bytes=actual_shared[0x48:0x50].hex(), delay_ms=50, cursor_glyph=0xdb,
    shared_scale_word=0x3e99999a, patched_instructions=[], services=services,
    executed_visits=visits, executed_unique_instructions=len(visited),
    executed_instruction_bank_sha256=hashlib.sha256(b''.join(struct.pack('<I', at) + raw for at, raw in sorted(visited.items()))).hexdigest(),
    instruction_authorities=instructions, setups=setups, timelines=timelines, controls=controls)
(OUT / 'typewriter-original.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({k: report[k] for k in ('passed', 'delay_ms', 'cursor_glyph', 'shared_scale_word', 'executed_visits', 'executed_unique_instructions')}, indent=2))
cache.close()
