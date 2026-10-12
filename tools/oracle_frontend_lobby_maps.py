#!/usr/bin/env python3
"""Freeze original screen206 admission, focused preview and row-name bindings.

Original2C9928,2C9B7C,2C9AC4,2C9A73,19C4E0,19C5F0,1A0750 and
1A06F0 execute unchanged. Explicit allocation/iteration/widget/text-buffer and
signed-controller best-key seams are recorded. No reference/runtime mutation.
"""
import hashlib
import json
import struct
from pathlib import Path
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN, STOP
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *

OUT = ROOT / 'analysis/frontend/lobby'


class Maps(Retail):
    def __init__(self):
        super().__init__()
        c = self.cache = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
        _, matg = c.tag(next(d for d, t in c.tags.items() if t['group'] == 'matg'), 'matg')
        g = self.alloc(c.data[matg:matg + 0x180]); self.w(0x4e034c, g); self.w(0x4e0350, 1)
        header = c.block(matg + 0x178, 24)[0]
        copied = bytearray(c.data[header:header + 24])
        self.catalog = []
        self.definitions = {}
        for rel, stride in [(8, 0xb50), (16, 0xc64)]:
            entries = c.block(header + rel, stride)
            base = self.alloc(b''.join(c.data[p:p + stride] for p in entries))
            struct.pack_into('<I', copied, rel + 4, base)
            for i, p in enumerate(entries):
                if rel != 16 or c.u32(p) == 0xffffffff: continue
                self.catalog.append(dict(map_id=c.u32(p), bitmap=f'{c.u32(p+8):08x}',
                    name=self.file_utf16(p + 12, 32), description=self.file_utf16(p + 0x24c, 128),
                    scenario_path=c.string(p + 0xb4c, 256), sort_order=c.u32(p + 0xc4c),
                    flags=c.data[p + 0xc50]))
                self.definitions[c.u32(p)] = base + i * stride
        self.w(g + 0x17c, self.alloc(copied))
        paths = c.block(matg + 0x170, 8)[0]; raw = bytearray(c.data[paths:paths + 8])
        entries = c.block(paths, 0x108)
        struct.pack_into('<I', raw, 4, self.alloc(b''.join(c.data[p:p + 0x108] for p in entries)))
        self.w(g + 0x174, self.alloc(raw))
        self.valid_keys = [c.u32(p + 4) for p in c.block(header + 8, 0xb50) if c.u32(p + 4) != 0xffffffff]
        self.order = sorted(self.catalog, key=lambda m: m['sort_order'])
        self.source_items = self.alloc(b''.join(struct.pack('<HBBI', i, m['flags'] & 1, 0, m['map_id'])
            for i, m in enumerate(self.order)))
        self.source_pool = self.alloc(bytes(0x50)); self.w(self.source_pool + 0x38, len(self.order))
        self.output_items = self.alloc(bytes(16 * 8)); self.output_pool = self.alloc(bytes(0x50))
        self.w(self.output_pool + 0x44, self.output_items)
        self.list = self.alloc(bytes(0x800)); self.screen = self.alloc(bytes(0x800))
        self.widget = self.alloc(bytes(0x100)); self.bitmap_widget = self.alloc(bytes(0x100))
        self.row_widget = self.alloc(bytes(0x100))
        vt = self.alloc(bytes(0x40)); self.w(vt + 0x3c, STOP + 0x100); self.w(self.widget, vt)
        self.buffer = self.alloc(bytes(8)); bvt = self.alloc(bytes(8))
        self.w(bvt + 4, STOP + 0x104); self.w(self.buffer, bvt)
        screen_file = next(t['address'] - c.magic for t in c.tags.values()
            if t['group'] == 'wgit' and c.u16(t['address'] - c.magic + 4) == 206)
        self.screen_asset = dict(flags=c.u32(screen_file), panes=[])
        for pane in c.block(screen_file + 32, 76):
            b = c.block(pane + 36, 56); t = c.block(pane + 28, 44)
            self.screen_asset['panes'].append(dict(bitmaps=len(b), texts=len(t),
                preview_definition=c.data[b[1]:b[1] + 56].hex(),
                description_definition=c.data[t[0]:t[0] + 44].hex()))
        pane = c.block(screen_file + 32, 76)[0]
        bitmap_def = c.block(pane + 36, 56)[1]
        self.w(self.bitmap_widget + 0x70, self.alloc(c.data[bitmap_def:bitmap_def + 56]))
        self.pages = {}
        for m in self.catalog:
            datum = int(m['bitmap'], 16); _, p = c.tag(datum, 'bitm')
            raw = bytearray(c.data[p:p + 0x4c]); pages = c.block(p + 0x44, 0x74)
            data = self.alloc(b''.join(c.data[q:q + 0x74] for q in pages))
            struct.pack_into('<I', raw, 0x48, data)
            self.w(self.table + (datum & 0xffff) * 16 + 8, self.alloc(raw))
            self.pages[datum] = data
        self.context = {}; self.g.hook_add(UC_HOOK_CODE, self.services)
        self.w(0x47ff38, 0)  # Original cached language: English slot0.

    def file_utf16(self, p, capacity):
        return bytes(self.cache.data[p:p + capacity * 2]).decode('utf-16le').split('\0')[0]

    def utf16(self, p, capacity=128):
        return bytes(self.g.mem_read(p, capacity * 2)).decode('utf-16le').split('\0')[0]

    def arg(self, n): return self.r(self.g.reg_read(UC_X86_REG_ESP) + 4 + n * 4)
    def returned(self, value=0, pop=0):
        sp = self.g.reg_read(UC_X86_REG_ESP)
        self.g.reg_write(UC_X86_REG_EAX, value & 0xffffffff)
        self.g.reg_write(UC_X86_REG_EIP, self.r(sp)); self.g.reg_write(UC_X86_REG_ESP, sp + 4 + pop)

    def services(self, uc, a, size, _):
        reg = uc.reg_read
        if a == 0x24bb00: self.returned(reg(UC_X86_REG_EAX), 4)
        elif a == 0x20670: self.returned(0, 16)
        elif a == 0x19c6a0: self.returned(self.source_pool)
        elif a == 0x1a480d:
            self.context['allocated'] = 0; self.returned(self.output_pool, 8)
        elif a in (0x16b790, 0x23625d, 0x22f4cd): self.returned()
        elif a == 0x190565: self.returned(self.context['best'])
        elif a == 0x2b2327:
            i = self.context.get('iterator', 0); self.context['iterator'] = i + 1
            if i < len(self.order): self.w(reg(UC_X86_REG_EDI), self.source_items + i * 8)
            self.returned(i < len(self.order))
        elif a == 0x16b840:
            i = self.context['allocated']; self.context['allocated'] = i + 1; self.returned(i)
        elif a == 0x22edb8:
            args = [self.arg(i) for i in range(3)]
            self.context.setdefault('finders', []).append(args)
            assert args in ([8, 1, 0], [6, 2, 0], [6, 0, 0]), args
            self.returned(self.bitmap_widget if args[0] == 8 else self.widget, 12)
        elif a == 0x24c447: self.returned(self.context['focused'])
        elif a == 0x16bae0:
            self.returned(0 if self.context.get('no_datum') else self.output_items + self.context['focused'] * 8)
        elif a == 0x11cae0: self.returned(self.context.get('language', 0))
        elif a == 0x22e391: self.returned()
        elif a == STOP + 0x100: self.returned(self.buffer)
        elif a == STOP + 0x104:
            self.context['text'] = self.utf16(self.arg(0)); self.returned(0, 4)
        elif a == 0x6b380:
            i = self.context.get('selection_iterator', 0); self.context['selection_iterator'] = i + 1
            iterator = reg(UC_X86_REG_ESI)
            if i < self.context['allocated']:
                self.w(iterator, self.output_items + i * 8); self.w(iterator + 8, i)
            self.returned(i < self.context['allocated'])
        elif a == 0x24c515:
            self.context['selected'] = self.arg(0); self.returned(0, 4)

    def admission(self, best):
        self.context = dict(best=best)
        self.run(0x2c9928, argv=[1], registers=[(UC_X86_REG_ESI, self.list)])
        n = self.context['allocated']
        return [self.r(self.output_items + i * 8 + 4) for i in range(n)]


def main():
    m = Maps(); threshold = m.run(0x19c4e0, argv=[1])
    threshold = struct.unpack('<i', struct.pack('<I', threshold))[0]
    assert threshold == max(m.valid_keys)
    sort_pairs = []
    a = m.alloc(bytes(4)); b = m.alloc(bytes(4))
    for left in m.catalog:
        for right in m.catalog:
            m.w(a, left['map_id']); m.w(b, right['map_id'])
            compared = m.run(0x19c9a0, argv=[a, b])
            compared = struct.unpack('<i', struct.pack('<I', compared))[0]
            assert compared == (1 if left['sort_order'] > right['sort_order'] else
                -1 if left['sort_order'] < right['sort_order'] else 0)
            sort_pairs.append(dict(left=left['map_id'], right=right['map_id'], result=compared))
    profile = m.alloc(bytes(0x1e0)); best = m.alloc(bytes(4)); index = m.alloc(bytes(4))
    m.run(0x1a0750, registers=[(UC_X86_REG_EDX, profile)])
    assert bytes(m.g.mem_read(profile + 0xec, 16)) == bytes(16)
    m.run(0x1a06f0, argv=[best, index], registers=[(UC_X86_REG_ECX, profile)])
    default_best = struct.unpack('<i', struct.pack('<I', m.r(best)))[0]
    progress = []
    for keys in ([0, 0, 0, 0], [-1, -1, -1, -1], [101, 0, 0, 0], [805, 0, 0, 0],
                 [805, 999, 301, 805], [801, 705, 805, -1], [999, 999, 999, 999]):
        m.g.mem_write(profile + 0xec, struct.pack('<4i', *keys))
        m.run(0x1a06f0, argv=[best, index], registers=[(UC_X86_REG_ECX, profile)])
        progress.append(dict(keys=keys, best=struct.unpack('<i', struct.pack('<I', m.r(best)))[0], index=m.r(index)))
    admitted = []
    for value in (-1, 0, 101, threshold - 1, threshold, threshold + 1, 0x7fffffff):
        ids = m.admission(value); assert len(ids) == (12 if value >= threshold else 11)
        admitted.append(dict(best=value, maps=ids))
    m.admission(threshold)
    m.w(m.screen + 0x684, m.output_pool)
    previews = []; names = []; selected = []
    for i, item in enumerate(m.order):
        m.context = dict(focused=i, language=0, allocated=12)
        m.w(m.bitmap_widget + 0x8c, 0)
        m.run(0x2c9b7c, registers=[(UC_X86_REG_ECX, m.screen)])
        assert m.r(m.bitmap_widget + 0x8c) == m.pages[int(item['bitmap'], 16)]
        assert m.context['text'] == item['description']
        previews.append(dict(map_id=item['map_id'], bitmap=item['bitmap'], text=m.context['text'],
                             finders=m.context['finders'], rect=m.read(m.bitmap_widget + 0x20, '<4h')))
        m.w(m.row_widget + 0x70, i)
        m.context = dict(focused=i, allocated=12); m.w(m.list + 0x70, m.output_pool)
        m.run(0x2c9ac4, argv=[m.row_widget, i], registers=[(UC_X86_REG_ECX, m.list)])
        assert m.context['text'] == item['name']
        names.append(dict(map_id=item['map_id'], text=m.context['text'], finders=m.context['finders']))
        m.context = dict(allocated=12); m.w(0x51098c, item['map_id'])
        m.run(0x2c9a73, registers=[(UC_X86_REG_EAX, m.list)])
        assert m.context['selected'] == i
        selected.append(dict(map_id=item['map_id'], selected=m.context['selected']))
    nulls = []
    for what in ('no_datum', 'no_map'):
        m.context = dict(focused=0, allocated=12, no_datum=what == 'no_datum')
        if what == 'no_map': m.w(m.output_items + 4, 99999)
        m.w(m.bitmap_widget + 0x8c, 0x1234)
        m.run(0x2c9b7c, registers=[(UC_X86_REG_ECX, m.screen)])
        assert 'text' not in m.context and m.r(m.bitmap_widget + 0x8c) == 0x1234
        nulls.append(dict(kind=what, retained=True))
    m.admission(-1); m.context = dict(allocated=11); m.w(0x51098c, 25)
    foundation = next(x for x in m.catalog if x['flags'] & 1)
    m.w(0x51098c, foundation['map_id']); m.run(0x2c9a73, registers=[(UC_X86_REG_EAX, m.list)])
    assert 'selected' not in m.context
    result = dict(xbe_sha256=PIN, map_sha256=hashlib.sha256(m.cache.data).hexdigest(),
        boundary='Original instructions with allocation/iteration/widget/text buffer and explicit best signed-controller key seams; no transport/rendering proof.',
        screen_asset=m.screen_asset, catalog=m.catalog, valid_campaign_keys=m.valid_keys,
        threshold=threshold, default_best=default_best, progress=progress, admitted=admitted, sort_pairs=sort_pairs,
        previews=previews, names=names, selected=selected, nulls=nulls, hidden_last_map_keeps_initial=True)
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / 'maps-original.json').write_text(json.dumps(result, indent=2) + '\n')
    lines = ['# Original map admission/focus bindings; English slot0; pinned XBE and literal mainmenu assets.']
    lines.append(f'threshold\t{threshold}\t{default_best}')
    for key in m.valid_keys: lines.append(f'campaign_key\t{key}')
    for item in m.catalog:
        lines.append('\t'.join(map(str, ['map', item['map_id'], item['bitmap'], item['sort_order'],
             item['flags'], item['name'], item['description'], item['scenario_path']])))
    for row in progress: lines.append('\t'.join(map(str, ['progress', *row['keys'], row['best'], row['index']])))
    for row in sort_pairs: lines.append(f"sort\t{row['left']}\t{row['right']}\t{row['result']}")
    for row in admitted: lines.append('\t'.join(map(str, ['admitted', row['best'], *row['maps']])))
    for row in previews: lines.append('\t'.join(map(str, ['preview', row['map_id'], row['bitmap'], row['text'], *row['rect']])))
    for row in selected: lines.append(f"selected\t{row['map_id']}\t{row['selected']}")
    (OUT / 'maps-original.tsv').write_text('\n'.join(lines) + '\n')
    print(json.dumps(dict(threshold=threshold, default_best=default_best,
        admission=len(admitted), progress=len(progress), sorts=len(sort_pairs), previews=len(previews), names=len(names), selected=len(selected))))


if __name__ == '__main__': main()
