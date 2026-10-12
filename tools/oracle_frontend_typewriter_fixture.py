#!/usr/bin/env python3
"""Freeze a compact native regression from unchanged original typewriter code."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct

from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EIP
from oracle_multiplayer_hud_positions import ROOT, PIN

baseline_path = ROOT / 'analysis/frontend/settings-typewriter-original/typewriter-original.json'
baseline_hash = hashlib.sha256(baseline_path.read_bytes()).hexdigest()
spec = importlib.util.spec_from_file_location('original_typewriter', ROOT / 'tools/oracle_frontend_typewriter.py')
original = importlib.util.module_from_spec(spec)
spec.loader.exec_module(original)
assert hashlib.sha256(baseline_path.read_bytes()).hexdigest() == baseline_hash
baseline = json.loads(baseline_path.read_text())
fixture = ROOT / 'tests/fixtures/frontend_typewriter_original.tsv'
report_path = ROOT / 'analysis/frontend/settings-typewriter-original/typewriter-fixture.json'
rows, cases = [], []


def hex_units(value):
    if isinstance(value, str):
        value = value.encode('utf-16le', errors='surrogatepass')
    return value.hex() or '-'


def row_draw(label, text, interval, glyph, nonzero, result):
    rows.append(' '.join(map(str, ['D', label, interval, f'{glyph:04x}', result['clock'], int(nonzero),
        result['cursor_before'], result['length'], result['last_clock_before'], hex_units(text),
        result['cursor_after'], result['last_clock_after'], result['rendered_utf16_hex'] or '-'])))
    cases.append(dict(label=label, operation='draw', text_utf16_hex=hex_units(text),
        interval_ms=interval, cursor_glyph=glyph, nonzero_bounds=nonzero, **result))


for timeline in baseline['timelines']:
    text = baseline['setups'][timeline['index']]['text']
    for i, result in enumerate(timeline['draws']):
        row_draw(f'screen19-{timeline["index"]}-{i}', text, 50, 0xdb, True, result)

retail, guest = original.retail, original.guest
stop_at = None
def stop_hook(machine, address, size, context):
    if address == stop_at:
        machine.emu_stop()
guest.hook_add(UC_HOOK_CODE, stop_hook)
# A larger explicit backing allocation lets us test the 1024-unit draw scratch
# separately from the actual standard buffer's 256-unit set_text capacity.
buffer = retail.alloc(bytes(0x1100))
retail.run(0x22f532, registers=[(UC_X86_REG_ECX, buffer)])
constructor = retail.read(buffer + 0x3c, '<hhI')
assert constructor == [-1, 0, 0]
rows.append('C -1 0 0')


def seed(text, cursor, length=None, last=100):
    raw = text.encode('utf-16le', errors='surrogatepass')
    guest.mem_write(buffer + 0x44, raw + b'\0\0')
    if length is None:
        length = len(raw) // 2
        units = list(struct.unpack('<' + 'H' * length, raw))
        if 0 in units:
            length = units.index(0)
    guest.mem_write(buffer + 0x3c, struct.pack('<hhI', cursor, length, last))


def draw(clock, rect=(0, 0, 100, 200), interval=50, glyph=0xdb):
    before = retail.read(buffer + 0x3c, '<hhI')
    guest.mem_write(original.shared + 0x48, struct.pack('<hH', interval, glyph))
    guest.mem_write(original.bounds, struct.pack('<4h', *rect))
    retail.w(0x54d5b8, clock)
    source = list(struct.unpack('<1024H', guest.mem_read(buffer + 0x44, 2048)))
    nonempty = source[0] != 0
    nonzero = rect[3] != rect[1] and rect[2] != rect[0]
    stop = 0x22ce32 if nonempty and nonzero else (0x22d095 if not nonempty else 0x22d098)
    retail.run(0x22cd48, stop=stop,
        argv=[original.bounds, original.bounds, 0, 0x3f800000, original.bounds],
        registers=[(UC_X86_REG_EBX, buffer)])
    units = []
    if nonempty and nonzero:
        temporary = guest.reg_read(UC_X86_REG_EBP) - 0x830
        units = list(struct.unpack('<1024H', guest.mem_read(temporary, 2048)))
        units = units[:units.index(0)]
    after = retail.read(buffer + 0x3c, '<hhI')
    return dict(clock=clock, cursor_before=before[0], length=before[1], last_clock_before=before[2],
        cursor_after=after[0], last_clock_after=after[2], rendered_units=units,
        rendered_utf16_hex=struct.pack('<' + 'H' * len(units), *units).hex())


controls = [
    ('zero-clock-first', 'AB', 0, 2, 0, 0, 50, 0xdb, (0, 0, 100, 200)),
    ('zero-clock-second', 'AB', 0, 2, 0, 50, 50, 0xdb, (0, 0, 100, 200)),
    ('unsigned-rollover-at', 'AB', 0, 2, 0xfffffff0, 0x22, 50, 0xdb, (0, 0, 100, 200)),
    ('unsigned-rollover-below', 'AB', 0, 2, 0xfffffff0, 0x21, 50, 0xdb, (0, 0, 100, 200)),
    ('finished', 'AB', -1, 2, 100, 1000, 50, 0xdb, (0, 0, 100, 200)),
    ('final-before-advance', 'AB', 1, 2, 100, 150, 50, 0xdb, (0, 0, 100, 200)),
    ('zero-width', 'AB', 0, 2, 100, 150, 50, 0xdb, (0, 5, 100, 5)),
    ('zero-height', 'AB', 0, 2, 100, 150, 50, 0xdb, (5, 0, 5, 100)),
    ('reversed-bounds', 'AB', 0, 2, 100, 150, 50, 0xdb, (100, 200, 0, 0)),
    ('empty', '', 0, 0, 100, 150, 50, 0xdb, (0, 0, 100, 200)),
    ('embedded-NUL', 'A\0BC', 1, 1, 100, 150, 50, 0xdb, (0, 0, 100, 200)),
    ('NUL-before-cursor', 'A\0BC', 3, 1, 100, 150, 50, 0xdb, (0, 0, 100, 200)),
    ('custom-cursor', 'AB', 0, 2, 100, 150, 50, ord('_'), (0, 0, 100, 200)),
    ('zero-delay', 'AB', 0, 2, 100, 100, 0, 0xdb, (0, 0, 100, 200)),
    ('negative-delay-below', 'AB', 0, 2, 100, 150, -1, 0xdb, (0, 0, 100, 200)),
    ('negative-delay-at', 'AB', 0, 2, 100, 99, -1, 0xdb, (0, 0, 100, 200)),
    ('copy1023-finished', 'A' * 1050, -1, 1050, 100, 150, 50, 0xdb, (0, 0, 100, 200)),
    ('cursor-clamp1022', 'A' * 1050, 32767, 1023, 100, 150, 50, 0xdb, (0, 0, 100, 200)),
]
for label, text, cursor, length, last, clock, interval, glyph, rect in controls:
    seed(text, cursor, length, last)
    row_draw(label, text, interval, glyph, rect[3] != rect[1] and rect[2] != rect[0],
        draw(clock, rect, interval, glyph))
text = 'A\ud83d\ude00B'
seed(text, 0, last=0)
for i, clock in enumerate((100, 150, 200, 250, 300, 350)):
    row_draw(f'UTF16-surrogate-{i}', text, 50, 0xdb, True, draw(clock))
for cursor in (-1, 2):
    for text in ('Changed', 'A\0BC', 'A\ud83d\ude00B'):
        seed('Old', cursor, last=1234)
        supplied = retail.alloc(text.encode('utf-16le', errors='surrogatepass') + b'\0\0')
        before = retail.read(buffer + 0x3c, '<hhI')
        retail.run(0x22f545, argv=[supplied], registers=[(UC_X86_REG_ECX, buffer)])
        after = retail.read(buffer + 0x3c, '<hhI')
        label = f'set-text-{cursor}-{len(cases)}'
        rows.append(' '.join(map(str, ['U', label, *before, hex_units(text), *after])))
        cases.append(dict(label=label, operation='set_text', before=before, after=after,
            supplied_utf16_hex=hex_units(text)))
for cursor in (-1, 2):
    seed('AB', cursor, last=1234)
    before = retail.read(buffer + 0x3c, '<hhI')
    stop_at = 0x253882
    retail.prepare(registers=[(UC_X86_REG_EAX, buffer)])
    guest.emu_start(0x25387d, 0, count=200000)
    assert guest.reg_read(UC_X86_REG_EIP) == stop_at
    stop_at = None
    after = retail.read(buffer + 0x3c, '<hhI')
    rows.append(' '.join(map(str, ['B', f'begin-{cursor}', *before, *after])))
    cases.append(dict(label=f'begin-{cursor}', operation='begin', before=before, after=after))
fixture.write_text('\n'.join(rows) + '\n')
report = dict(passed=True, executable_sha256=PIN, baseline_sha256=baseline_hash,
    script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    fixture=str(fixture.relative_to(ROOT)), fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
    rows=len(rows), draw_rows=sum(row.startswith('D ') for row in rows), cases=cases,
    instruction_visits=original.visits, unique_original_instructions=len(original.visited),
    source_buffer_policy='Actual standard text setup/copy uses256-unit buffer; synthetic long draw inputs explicitly test only original1024-unit draw scratch, not constructor capacity.',
    scope='Unchanged original setup/buffer/reveal instructions; prefix before advance,UTF16 units,NUL,zero bounds,reversed bounds,overflow clock,signed delay,clamp/copy limits and lifecycle. No native execution or GPU/framebuffer proof.')
report_path.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({key: report[key] for key in ('passed', 'rows', 'draw_rows', 'fixture_sha256')}, indent=2))
