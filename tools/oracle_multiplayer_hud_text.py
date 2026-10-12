#!/usr/bin/env python3
"""Execute original Xbox numeric HUD text measurement and placement.

Supplies real conduit-13 header/glyph metadata in a ready glyph cache. Only
cache lookup and pixel readiness are hooked; glyph geometry, kerning, string
iteration, measurement, anchoring, centering, clipping and draw callbacks run
original instructions. Starts after text value/font/shader setup, stops before
the graphics draw, and calls original text drawing with a measurement callback.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from inspect_multiplayer_hud import HudCache, ROOT
from oracle_multiplayer_hud_positions import Retail, PIN, Xbe, FUNCTIONS_CSV, read_rows, listing
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP,
                              UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=ROOT / 'analysis/multiplayer/hud-text-instruction-oracle.json')
    parser.add_argument('--tsv', type=Path,
                        default=ROOT / 'analysis/multiplayer/hud-text-instruction-oracle.tsv')
    parser.add_argument('--instructions', type=Path,
                        default=ROOT / 'analysis/multiplayer/hud-text-retail-instructions.txt')
    args = parser.parse_args()
    cache = HudCache(ROOT / 'assets/disc/maps/lockout.map')
    retail = Retail()
    g = retail.g
    font_path = ROOT / 'assets/disc/maps/fonts/conduit-13'
    font = font_path.read_bytes()
    assert struct.unpack_from('<I', font, 0x200)[0] == 0xf0000001
    # Real file header, followed by a supplied ready-state flag. Font table
    # index 6 is conduit-13; this isolated environment loads it into cache 0.
    g.mem_write(0x4e2920, font[0x200:0x3c0])
    g.mem_write(0x4e2920 + 0x1c8, b'\1')
    retail.w(0x4e28f4 + 6 * 4, 0)
    g.mem_write(0x4e73a0, bytes(0x70))
    retail.w(0x4e73a0, 6)
    chars = '012'
    pool = retail.alloc(bytes(0x48))
    records = retail.alloc(bytes(0x38 * len(chars)))
    retail.w(pool + 0x44, records)
    retail.w(0x54d574, pool)
    metrics = []
    for index, char in enumerate(chars):
        file_offset = (struct.unpack_from('<I', font, 0x400 + ord(char) * 4)[0] + 0x4040) * 16
        retail.w(records + index * 0x38 + 0x10, 4)
        g.mem_write(records + index * 0x38 + 0x1c, font[file_offset:file_offset + 16])
        advance, _, width, height, bearing_x, bearing_y, _ = struct.unpack_from('<HHHHhhI', font, file_offset)
        metrics.append(dict(character=char, file_offset=f'{file_offset:x}', advance=advance,
                            width=width, height=height, bearing_x=bearing_x, bearing_y=bearing_y))
    glyph_calls = []

    def hook(uc, address, size, _):
        if address == 0x140b20:  # ready cache lookup of real glyph records
            assert uc.reg_read(UC_X86_REG_EAX) == 0
            codepoint = uc.reg_read(UC_X86_REG_ECX)
            retail.ret(chars.index(chr(codepoint)) if chr(codepoint) in chars else 0xffffffff)
        elif address == 0x140870:  # supplied pixels-ready cache; no pixel decoding
            assert uc.reg_read(UC_X86_REG_EAX) < len(chars)
            sp = uc.reg_read(UC_X86_REG_ESP)
            retail.ret(1)
            uc.reg_write(UC_X86_REG_ESP, sp + 8)
        elif address == 0x13e8e0:  # record callback arguments; still execute original
            sp = uc.reg_read(UC_X86_REG_ESP)
            values = retail.read(sp + 4, '<5I7f2I')
            glyph_calls.append(dict(character=chr(values[2]), x=values[5], y=values[6],
                                    width=values[9], height=values[10], scale=values[11]))

    g.hook_add(UC_HOOK_CODE, hook)
    bounds = retail.alloc(struct.pack('<4h', 0, 0, 1000, 1000))
    ink, line = retail.alloc(bytes(8)), retail.alloc(bytes(8))
    measurements = []
    for text in ('120', '0'):
        text_pointer = retail.alloc(struct.pack('<' + 'I' * (len(text) + 1), *map(ord, text), 0))
        glyph_calls.clear()
        retail.run(0x13ea60, argv=[bounds, text_pointer, 0x3f800000],
                   registers=[(UC_X86_REG_ESI, line), (UC_X86_REG_EDI, ink)])
        measurements.append(dict(text=text, ink=retail.read(ink, '<4h'),
                                 line=retail.read(line, '<4h'), glyphs=list(glyph_calls)))
    assert [m['ink'] for m in measurements] == [[0, 1, 22, 27], [0, 1, 22, 9]]

    tags = {t['name']: t for t in cache.tags.values() if t['group'] == 'nhdt'}
    widgets = []
    for tag_name, widget_name, text in (('ui\\hud\\smg', 'bullets_right', '120'),
                                         ('ui\\hud\\masterchief', 'frag_hidden', '0'),
                                         ('ui\\hud\\masterchief', 'plasma_hidden', '0')):
        row = next(row for row in cache.hud(tags[tag_name])['text'] if row['name'] == widget_name)
        assert not row['effects'] and row['fonts'] == [0, 0, 0]
        at = int(row['offset'], 16)
        widget = retail.alloc(cache.data[at:at + 0x54])
        widgets.append((row, widget, text))
    frame = retail.alloc(bytes(0x800)) + 0x400
    cases = []
    for flags in (0, 1, 8, 9):
        video = retail.video(flags)
        for players in range(1, 5):
            _, cells = retail.cells(players)
            for cell in cells:
                g.mem_write(0x4b9dd0, struct.pack('<4h', *cell['outer']))
                g.mem_write(0x4b9dd8, struct.pack('<4h', *cell['safe']))
                for row, widget, text in widgets:
                    slot = retail.run(0x13a690, argv=[row['anchor']])
                    retail.w(frame + 0x10, widget)
                    retail.w(frame + 0x14, slot)
                    g.mem_write(frame - 0x224, (text + '\0').encode('utf-16le'))
                    # Authored point/axes, original anchor+offset+effect, then
                    # original UTF16 measurement and justification rectangle.
                    retail.run(0x22aae3, stop=0x22ac98,
                               registers=[(UC_X86_REG_EBP, frame), (UC_X86_REG_EBX, widget),
                                          (UC_X86_REG_ESI, 0)])
                    rectangle = retail.read(frame - 0x10, '<4h')
                    anchor = retail.read(frame - 8, '<2f')
                    measured = retail.read(frame - 0x24, '<4h')
                    text_pointer = retail.alloc(struct.pack('<' + 'I' * (len(text) + 1), *map(ord, text), 0))
                    glyph_calls.clear()
                    retail.run(0x13f0e0, argv=[0x13e8e0, frame - 0x10, 0, 0, 0, 0x3f800000, text_pointer])
                    assert len(glyph_calls) == len(text)
                    # The original text draw uses coordinates local to the
                    # outer viewport. Translate only to the report's global
                    # 640x480 convention; no host glyph-placement calculation.
                    global_glyphs = [dict(call, x=call['x'] + cell['outer'][1],
                                         y=call['y'] + cell['outer'][0]) for call in glyph_calls]
                    final_bounds = [min(call['x'] for call in global_glyphs),
                                    min(call['y'] for call in global_glyphs),
                                    max(call['x'] + call['width'] for call in global_glyphs),
                                    max(call['y'] + call['height'] for call in global_glyphs)]
                    cases.append(dict(video_flags=flags, widescreen=video['widescreen'],
                                      progressive=video['progressive_5'], players=players, **cell,
                                      widget_name=row['name'], widget_file_offset=row['offset'], text=text,
                                      layout_slot=slot, anchor_local=anchor, measured_ink=measured,
                                      text_rectangle_local=rectangle, glyphs_local=list(glyph_calls),
                                      glyphs_global=global_glyphs, bounds=final_bounds))
    assert len(cases) == 120
    report = dict(xbe_sha256=PIN, font=str(font_path), font_sha256=hashlib.sha256(font).hexdigest(),
                  font_metrics=metrics, measurements=measurements, cases=cases,
                  entries=['0022aae3..0022ac98', '0013e9c0', '0013ea60', '0013e8e0',
                           '0013f0e0', '0013ef40', '0013f700', '0013f470', '0013f5a0',
                           '001224c0', '00122570', '001407d0', '00140900'],
                  scope='Real font headers and glyph metadata; original measurement and text positioning with supplied numeric strings120/0, already-selected conduit-13, and ready glyph cache. Font-cache lookup and pixel-readiness hooks supply environment only. Gameplay predicates, numeric/string-ID formatting, font choice, shader setup, glyph pixel decoding, graphics calls, final device scaling and framebuffer parity excluded.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    rows = ['widescreen\tplayers\tview\twidget_name\ttext\tleft\ttop\tright\tbottom']
    for case in cases:
        if not case['progressive']:
            rows.append('\t'.join(map(str, [int(case['widescreen']), case['players'], case['view'],
                                            case['widget_name'], case['text'], *case['bounds']])))
    args.tsv.parent.mkdir(parents=True, exist_ok=True)
    args.tsv.write_text('\n'.join(rows) + '\n')
    inventory = read_rows(FUNCTIONS_CSV)
    xbe = Xbe(ROOT / 'assets/disc/default.xbe')
    instructions = [f'Untouched retail default.xbe SHA256 {PIN}']
    for address in (0x22aa16, 0x13e9c0, 0x13ea60, 0x13e8e0, 0x13f0e0,
                    0x13ef40, 0x13f700, 0x13f470, 0x13f5a0, 0x1224c0,
                    0x122570, 0x1407d0, 0x140900):
        row = inventory[address]
        instructions.extend(['', f"{row['va']} size {row['size']} {row['status']}"])
        instructions.extend(listing(xbe.read(address, int(row['size'])), address, inventory))
    args.instructions.parent.mkdir(parents=True, exist_ok=True)
    args.instructions.write_text('\n'.join(instructions) + '\n')
    print(f'{len(cases)} original-instruction numeric HUD text cases; JSON {args.output}; TSV {args.tsv}')


if __name__ == '__main__':
    main()
