#!/usr/bin/env python3
"""Freeze original BD090/BD610 deferred-node admission with retail assets.

BD090's unchanged selector interval begins at BD2F1 after the independent
default-orientation copy. Actual captured object headers prove that an owned
orientation bank exists. BD610 executes unchanged until it either enters
BDB60 or passes its admission branch. This freezes the caller's mask, rather
than treating an object-update bank as the rendered node bank. No live render
collector, geometry cache residency or rendered interpolation is inferred.
"""
import hashlib
import json
import struct
from pathlib import Path

from inspect_multiplayer import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN, STACK
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX,
    UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_ESI)


def indices(raw, count):
    words = struct.unpack('<8I', raw)
    return [i for i in range(count) if words[i//32] & (1 << (i%32))]


class Probe(Retail):
    def __init__(self):
        super().__init__()
        self.mode = ''
        self.result = None

    def hook(self, uc, pc, size, user):
        if self.mode in ('initial', 'render') and pc == 0xbdb60:
            mask = uc.reg_read(UC_X86_REG_ECX)
            count = uc.reg_read(UC_X86_REG_EAX)
            raw = bytes(uc.mem_read(mask, 32)) if mask else bytes([255])*32
            self.result = dict(sample=True, node_count=count,
                mask_present=bool(mask), mask_hex=raw.hex(),
                active_nodes=indices(raw, count))
            uc.emu_stop()
        elif self.mode == 'render' and pc == 0xbd7ca:
            self.result = dict(sample=False)
            uc.emu_stop()
        elif self.mode == 'truncate' and pc == 0xbdb82:
            self.result = dict(callback_node_count=uc.reg_read(UC_X86_REG_ESI))
            uc.emu_stop()
        super().hook(uc, pc, size, user)


def initial_probe(model_raw, node_count, owned=True):
    r = Probe()
    model = r.alloc(model_raw)
    dummy = r.alloc(bytes(256))
    r.prepare()
    r.w(STACK+0x12, int(owned))
    for at, value in ((0x60, model), (0x24, node_count), (0x18, dummy),
                      (0x14, dummy), (0xfc, 0x10000)):
        r.w(STACK+at, value)
    r.g.reg_write(UC_X86_REG_EBX, dummy)
    r.g.reg_write(UC_X86_REG_EBP, dummy)
    r.mode = 'initial'
    r.g.emu_start(0xbd2f1, 0x3000000, count=200000)
    assert r.result
    result = r.result
    mask = r.alloc(bytes.fromhex(result['mask_hex'])) if result['mask_present'] else 0
    r.mode, r.result = 'truncate', None
    r.run(0xbdb60, stop=0xbdb82, argv=[dummy, dummy, dummy], registers=[
        (UC_X86_REG_ECX, mask), (UC_X86_REG_EAX, node_count),
        (UC_X86_REG_EDX, 0x10000)])
    result['callback_node_count'] = r.g.reg_read(UC_X86_REG_ESI)
    return result


def render_probe(c, obj, collector):
    r = Probe()
    header = bytes.fromhex(obj['object_header_hex'])
    block = r.alloc(header + bytes(0x3000-len(header)))
    handle = int(obj['handle'], 16)
    datum = int(obj['definition'], 16)
    unit_raw = bytes(c.data[c.tags[datum]['offset']:c.tags[datum]['offset']+0x3c])
    model_datum = struct.unpack_from('<I', unit_raw, 0x38)[0]
    model_raw = bytes(c.data[c.tags[model_datum]['offset']:c.tags[model_datum]['offset']+0xe4])
    mode_datum = struct.unpack_from('<I', model_raw, 4)[0]
    mode_raw = bytes(c.data[c.tags[mode_datum]['offset']:c.tags[mode_datum]['offset']+0x68])
    for tag, raw in ((datum, unit_raw), (model_datum, model_raw), (mode_datum, mode_raw)):
        r.w(r.table+(tag&65535)*16+8, r.alloc(raw))
    objects = r.alloc(bytes(((handle&65535)+1)*12))
    r.w(objects+(handle&65535)*12+8, block)
    globals_ = r.alloc(bytes(0x48))
    r.w(globals_+0x44, objects)
    r.w(0x4e0300, globals_)
    r.mode = 'render'
    r.prepare(argv=[handle, r.alloc(bytes(collector)), r.alloc(bytes(obj['node_bytes']))])
    r.g.emu_start(0xbd610, 0x3000000, count=200000)
    assert r.result
    return r.result


def section_records(c, root):
    result = []
    for ordinal, s in enumerate(c.block(root+0x24, 0x5c)):
        location, size, header = c.u(s+0x38), c.u(s+0x3c), c.u(s+0x44)
        path = c.path if location >> 30 == 0 else c.path.parent / (
            '', 'mainmenu.map', 'shared.map', 'single_player_shared.map')[location >> 30]
        with path.open('rb') as stream:
            stream.seek(location & 0x3fffffff)
            raw = stream.read(size)
        assert len(raw) == size
        base = size-header-4
        node_maps = [p for p in c.block(s+0x48, 16) if c.h(p+4) == 100]
        assert len(node_maps) <= 1
        nodes = []
        if node_maps:
            p = node_maps[0]
            nodes = list(raw[base+c.u(p+12):base+c.u(p+12)+c.u(p+8)])
            assert len(nodes) == struct.unpack_from('<H', raw, 0x6c)[0]
        result.append(dict(section=ordinal, classification=c.h(s),
            node_map=nodes, raw_sha256=hashlib.sha256(raw).hexdigest()))
    return result


def main():
    path = ROOT/'analysis/frontend/profile-regression/original-model-menu-hidden-grip-control.json'
    snapshot = json.loads(path.read_text())
    c = Cache(ROOT/'assets/disc/maps/mainmenu.map')
    definitions, objects = {}, []
    for obj in snapshot['objects']:
        unit = int(obj['definition'], 16)
        if unit not in definitions:
            model = c.u(c.tags[unit]['offset']+0x38)
            p = c.tags[model]['offset']
            raw = bytes(c.data[p:p+0xe4])
            mode = c.u(p+4)
            mode_root = c.tags[mode]['offset']
            count = c.u(mode_root+0x48)
            deferred = raw[0xa4:0xc4]
            trigger = raw[0xc4:0xe4]
            sections = section_records(c, mode_root)
            trigger_sections = indices(trigger, len(sections))
            for s in sections:
                s['triggers_deferred_sampling'] = s['section'] in trigger_sections
            initial = initial_probe(raw, count)
            assert initial['active_nodes'] == [i for i in range(count) if i not in indices(deferred, count)]
            no_owned_bank = initial_probe(raw, count, False)
            assert not no_owned_bank['mask_present'] and no_owned_bank['callback_node_count'] == count
            definitions[unit] = dict(unit=f'{unit:08x}', model=f'{model:08x}',
                render_model=f'{mode:08x}', node_count=count,
                deferred_mask_hex=deferred.hex(), deferred_nodes=indices(deferred, count),
                trigger_section_mask_hex=trigger.hex(), trigger_sections=trigger_sections,
                initial=initial, no_owned_orientation_bank=no_owned_bank,
                sections=sections, root_header_hex=raw.hex())
        info = definitions[unit]
        header = bytes.fromhex(obj['object_header_hex'])
        region_count = c.u(c.tags[int(info['render_model'],16)]['offset']+0x1c)
        cases = []
        for section in [255]+list(range(len(info['sections']))):
            collector = [section]+[255]*(region_count-1)
            result = render_probe(c, obj, collector)
            assert result['sample'] == (section in info['trigger_sections'])
            if result['sample']:
                assert result['mask_hex'] == info['deferred_mask_hex']
                assert result['node_count'] == obj['node_count']
            cases.append(dict(collector=collector, original=result))
        objects.append(dict(object=obj['object'], unit=obj['definition'],
            flags_word=f'{struct.unpack_from("<I",header,4)[0]:08x}',
            deferred_flag_30=bool(struct.unpack_from('<I',header,4)[0] & (1<<30)),
            orientation_byte_count=struct.unpack_from('<h',header,0x110)[0],
            orientation_offset=struct.unpack_from('<h',header,0x112)[0], cases=cases))
    captured_banks = []
    for filename in ('original-model-actual-fourway-ui-node-bank.json',
                     'original-model-menu-hidden-grip-control.json'):
        bank_path = path.parent/filename
        bank = json.loads(bank_path.read_text())
        rows = []
        for obj in bank['objects']:
            raw = bytes.fromhex(obj['object_header_hex'])
            flags = struct.unpack_from('<I',raw,4)[0]
            rows.append(dict(object=obj['object'], unit=obj['definition'],
                flags_word=f'{flags:08x}', deferred_flag_30=bool(flags & (1<<30)),
                source_bank_stage=('object-update bank with deferred nodes' if flags & (1<<30)
                    else 'object bank after deferred sampling; separate render interpolation remains unproved')))
        captured_banks.append(dict(filename=filename,
            sha256=hashlib.sha256(bank_path.read_bytes()).hexdigest(), objects=rows))
    out = dict(xbe_sha256=PIN, cache_sha256=hashlib.sha256(c.data).hexdigest(),
        snapshot_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        definitions=list(definitions.values()), objects=objects, captured_banks=captured_banks,
        scope=__doc__, boundary='Collectors here are explicit discriminating inputs, not a captured live menu admission bank. Asset node maps retain palette membership, not nonzero GPU weights. Initial source object banks and final rendered/interpolated banks are distinct.')
    output = ROOT/'analysis/frontend/profile-regression/frontend-model-node-admission-original.json'
    output.write_text(json.dumps(out, indent=2)+'\n')
    print(json.dumps(dict(definitions=len(definitions), objects=len(objects),
        collector_cases=sum(len(o['cases']) for o in objects),
        initial_masks=[dict(unit=d['unit'], active=d['initial']['active_nodes'],
            callback_node_count=d['initial']['callback_node_count']) for d in definitions.values()],
        fixture_sha256=hashlib.sha256(output.read_bytes()).hexdigest()), indent=2))


if __name__ == '__main__':
    main()
