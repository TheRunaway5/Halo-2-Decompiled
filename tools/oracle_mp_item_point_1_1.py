#!/usr/bin/env python3
"""Independent unchanged FBFA0 execution on literal frozen current item states.

This imports actual original allocated-object snapshots and cache bytes. It does
not produce live node poses/contact state or implement a replacement service.
All external operand reads must belong to imported original inputs.
"""
import argparse
import collections
import csv
import json
from pathlib import Path
import struct

from export_mp_pickup_outer_fixtures_1_1 import ROOT, BASE, sha, require, source_pins, json_bytes
from oracle_smg_firing_1_1 import Xbe, Cache, _Code, _trace, PIN, CACHE_PIN, POOL, HEADERS, TAGS, STACK, STOP
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_XMM0, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG, UC_X86_REG_MXCSR)

OUT = BASE / 'pickup-item-point'
ENTRIES = (0xfbfa0, 0xbacc0, 0xb9b90, 0xb9c20, 0x1dc8f0, 0x1dc940, 0x143580, 0x1441d0, 0x14ba50)
STAGES = (0xfbfc4, 0xfbff8, 0xfc005, 0xfc014, 0xfc018, 0xfc022, 0xfc12e, 0xfc167, 0xfc173, 0xfc19c, 0xfc1b8)
REGS = dict(eax=UC_X86_REG_EAX, ebx=UC_X86_REG_EBX, ecx=UC_X86_REG_ECX, edx=UC_X86_REG_EDX,
    esi=UC_X86_REG_ESI, edi=UC_X86_REG_EDI, ebp=UC_X86_REG_EBP)


class ItemPointReplay:
    def __init__(self, image, cache):
        self.image = image; self.cache = cache; self.g = Uc(UC_ARCH_X86, UC_MODE_32)
        self.g.mem_map(0x10000, 0x800000)
        for section in image.sections:
            self.g.mem_write(section.va, image.section_bytes(section))
        for address, size in ((POOL, 0x100000), (0x2000000, 0x200000), (STOP, 0x1000), (0x4000000, 0x10000)):
            self.g.mem_map(address, size)
        page = cache.magic & ~0xfff
        self.g.mem_map(page, ((len(cache.data) + cache.magic - page + 0xfff) // 0x1000) * 0x1000)
        self.g.mem_write(cache.magic, bytes(cache.data))
        self.tag_records = {}
        for datum, tag in cache.tags.items():
            if tag['offset'] is not None:
                at = TAGS + (datum & 0xffff) * 16 + 8
                self.write_u(at, tag['offset'] + cache.magic); self.tag_records[at] = datum
        # Exactly the existing source fixture's resident BSP allocation mapping.
        row = cache.block(cache.scenario + 0x210, 0x44)[0]
        self.bsp_offset, self.bsp_size, self.bsp_base = (cache.u(row + n) for n in (0, 4, 8))
        self.g.mem_write(self.bsp_base, bytes(cache.data[self.bsp_offset:self.bsp_offset + self.bsp_size]))
        self.bsp_root = cache.u(self.bsp_offset + 4)
        self.collision = self.u(self.bsp_root + 0x18)
        self.bindings = {0x4e78d0: POOL, 0x4eb114: TAGS, 0x4e7910: self.collision,
            0x4e7918: self.bsp_root, POOL + 0x44: HEADERS}
        for address, value in self.bindings.items(): self.write_u(address, value)
        self.active = False; self.code = _Code(image, '.text'); self.executed = set()
        self.g.hook_add(UC_HOOK_CODE, self.hook)
        self.g.hook_add(UC_HOOK_MEM_READ, self.mem_read)
        self.g.hook_add(UC_HOOK_MEM_WRITE, self.mem_write)

    def write_u(self, address, value): self.g.mem_write(address, struct.pack('<I', value & 0xffffffff))
    def u(self, address, size=4): return int.from_bytes(self.g.mem_read(address, size), 'little')
    def raw(self, address, size): return bytes(self.g.mem_read(address, size)).hex()
    def words(self, address, count=3): return [f'{self.u(address + i * 4):08x}' for i in range(count)]
    def registers(self): return {name: f'{self.g.reg_read(reg):08x}' for name, reg in REGS.items()}
    def xmm(self): return [f'{self.g.reg_read(UC_X86_REG_XMM0 + i) & 0xffffffff:08x}' for i in range(8)]

    def cache_record(self, address, size):
        if self.bsp_base <= address and address + size <= self.bsp_base + self.bsp_size:
            at = self.bsp_offset + address - self.bsp_base; kind = 'relocated_sbsp'
        else:
            at = address - self.cache.magic; kind = 'cache_metadata'
        require(0 <= at and at + size <= len(self.cache.data), 'Cache read outside original bytes')
        require(bytes(self.g.mem_read(address, size)) == bytes(self.cache.data[at:at + size]), 'Cache operand changed')
        return dict(address=f'{address:08x}', file_offset=f'{at:08x}', size=size, kind=kind, raw_hex=self.raw(address, size))

    def origin(self, address, size):
        if self.body <= address and address + size <= self.body + self.body_size: return 'literal_current_object'
        if self.header <= address and address + size <= self.header + 12: return 'literal_current_header'
        if any(at <= address and address + size <= at + 4 for at in self.bindings): return 'explicit_resident_binding'
        if address in self.tag_records and size == 4: return 'actual_cache_tag_residency'
        if 0x2000000 <= address and address + size <= 0x2200000: return 'original_stack_or_output'
        if self.output <= address and address + size <= self.output + 12: return 'original_output'
        if self.bsp_base <= address and address + size <= self.bsp_base + self.bsp_size: return 'relocated_sbsp'
        if self.cache.magic <= address and address + size <= self.cache.magic + len(self.cache.data): return 'cache_metadata'
        if 0x10000 <= address and address + size <= 0x810000: return 'original_xbe'
        raise AssertionError(f'Undefined external operand {address:08x}/{size}')

    def mem_read(self, g, access, address, size, value, user):
        if not self.active: return
        origin = self.origin(address, size)
        if origin in ('original_stack_or_output', 'original_output'): return
        instruction = g.reg_read(UC_X86_REG_EIP); key = (instruction, address, size)
        if key in self.read_keys: return
        self.read_keys.add(key)
        record = dict(instruction=f'{instruction:08x}', address=f'{address:08x}', size=size,
            origin=origin, raw_hex=self.raw(address, size))
        if origin in ('cache_metadata', 'relocated_sbsp'): record.update(self.cache_record(address, size))
        if origin == 'original_xbe': require(bytes(g.mem_read(address, size)) == self.image.read(address, size), 'Original executable operand changed')
        if origin == 'literal_current_object': record['body_offset'] = f'{address - self.body:03x}'
        self.reads.append(record)

    def mem_write(self, g, access, address, size, value, user):
        if not self.active: return
        require(self.origin(address, size) in ('original_stack_or_output', 'original_output'), 'Item-point producer mutated imported state')

    def finish(self, address):
        for pending in self.pending[:]:
            if pending['return'] != address: continue
            row = pending['row']; row['return_eax'] = f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
            if pending.get('output') is not None: row['output_hex'] = self.raw(pending['output'], pending['size'])
            self.pending.remove(pending)

    def hook(self, g, address, size, user):
        if not self.active: return
        self.finish(address); self.executed.add(address)
        sp = g.reg_read(UC_X86_REG_ESP)
        if address in ENTRIES:
            row = dict(entry=f'{address:08x}', caller=f'{self.u(sp):08x}', registers=self.registers(),
                stack=[f'{self.u(sp + 4 + n * 4):08x}' for n in range(6)])
            self.calls.append(row)
            pending = {'return': self.u(sp), 'row': row}
            if address in (0xbacc0, 0xfbfa0): pending.update(output=g.reg_read(UC_X86_REG_ESI if address == 0xbacc0 else UC_X86_REG_EAX), size=12)
            if address == 0xb9c20: pending.update(output=g.reg_read(UC_X86_REG_EBX), size=0x70)
            if address == 0x1dc940:
                pending.update(output=self.u(sp + 20), size=0x70)
                row.update(region_pointer=f'{self.u(sp + 4):08x}', node_map_pointer=f'{self.u(sp + 8):08x}',
                    node_matrix_pointer=f'{self.u(sp + 12):08x}', mirror=self.u(sp + 16, 1), maximum=self.u(sp + 24))
            if address == 0x143580:
                row['quaternion_bits'] = self.words(g.reg_read(UC_X86_REG_ECX), 4)
                pending.update(output=g.reg_read(UC_X86_REG_EAX), size=0x24)
            if address == 0x1441d0:
                row.update(node_matrix_hex=self.raw(g.reg_read(UC_X86_REG_ECX), 0x34),
                    local_marker_matrix_hex=self.raw(g.reg_read(UC_X86_REG_EDX), 0x34))
                pending.update(output=self.u(sp + 4), size=0x34)
            if address == 0x14ba50:
                collision = self.u(sp + 4); signed = g.reg_read(UC_X86_REG_EDX) & 0xffff
                row.update(collision=f'{collision:08x}', signed_plane=f'{signed:04x}',
                    plane_record=self.cache_record(self.u(collision + 12) + (signed & 0x7fff) * 16, 16))
                pending.update(output=g.reg_read(UC_X86_REG_EAX), size=16)
            self.pending.append(pending)
        if address in STAGES:
            row = dict(instruction=f'{address:08x}', registers=self.registers(), xmm_bits=self.xmm())
            if address in (0xfc022, 0xfc12e, 0xfc167, 0xfc173):
                row.update(marker_point_bits=self.words(sp + 0x80), plane_buffer_bits=self.words(sp + 0x10, 4))
            self.stages.append(row)

    def execute(self, original):
        state = original['object_before']; raw = bytes.fromhex(state['raw_hex'])
        self.body = int(state['address'], 16); self.body_size = len(raw)
        self.header = HEADERS + (int(state['datum'], 16) & 0xffff) * 12
        self.output = int(original['registers']['eax'], 16)
        self.g.mem_write(self.body, raw); self.g.mem_write(self.header, bytes.fromhex(state['header_hex']))
        self.write_u(STACK, STOP)
        for i, word in enumerate(original['stack']): self.write_u(STACK + 4 + i * 4, int(word, 16))
        for name, reg in REGS.items(): self.g.reg_write(reg, int(original['registers'][name], 16))
        self.g.reg_write(UC_X86_REG_ESP, STACK)
        for reg, value in ((UC_X86_REG_FPCW, 0x027f), (UC_X86_REG_MXCSR, 0x1f80),
                (UC_X86_REG_FPSW, 0), (UC_X86_REG_FPTAG, 0xffff)): self.g.reg_write(reg, value)
        self.calls = []; self.stages = []; self.reads = []; self.read_keys = set(); self.pending = []
        self.active = True; self.g.emu_start(0xfbfa0, STOP, count=100000); self.finish(STOP); self.active = False
        require(self.g.reg_read(UC_X86_REG_EIP) == STOP and not self.pending, 'Incomplete original item-point execution')
        point = self.words(self.output); eax = self.g.reg_read(UC_X86_REG_EAX); al = eax & 255
        require(point == original['point']['bits'] and eax == int(original['return_eax'], 16), 'Original item point replay differs')
        require(self.raw(self.body, len(raw)) == raw.hex(), 'Original item body changed')
        return dict(point_bits=point, return_al=al, return_eax=f'{eax:08x}', calls=self.calls, stages=self.stages, external_reads=self.reads,
            imported_header_hex=state['header_hex'], imported_body_hex=raw.hex(), imported_node_matrix_hex=state['node_matrix_hex'],
            explicit_bindings={f'{at:08x}': f'{value:08x}' for at, value in self.bindings.items()},
            configured_fpcw='027f', configured_mxcsr='00001f80')


def preserve(path, data):
    if path.exists(): require(path.read_bytes() == data, f'Refusing changed item-point evidence: {path}')
    else: path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--output-dir', type=Path, default=OUT)
    args = parser.parse_args(); out = args.output_dir.resolve(); require(ROOT / 'analysis' in out.parents, 'Ignored evidence only')
    reports = {suite: json.loads((BASE / suite / 'report.json').read_text()) for suite in ('pickup-visibility', 'pickup-retreat')}
    pins = source_pins(reports)
    for path, pin in {
            'tools/oracle_mp_pickup_consumer_measured_1_1.py': '0fd18d4f19876552180d2cf6149c9961c5ff1d7a557bfb09b9e9fafa15918da7',
            'analysis/multiplayer/integration/inventory-source/pickup-consumer-measured/manifest.json': 'a26f5cfe698881f815a06404e46583d9ee3906d0adffdbf2b36c3b3602e2ce7d',
        }.items(): require(sha(ROOT / path) == pin, 'Prior freeze changed'); pins[path] = pin
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    require(sha(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03') == PIN, 'Original executable changed')
    require(sha(ROOT / 'assets/disc/maps/lockout.map') == CACHE_PIN, 'Actual map changed')
    measured = json.loads((BASE / 'pickup-consumer-measured/report.json').read_text())
    consumer_map = {(c['suite'], c['source_call_index']): c['global_id'] for c in measured['cases']}
    rows = []; replay = ItemPointReplay(image, cache); marker_checks = 0
    for suite, report in reports.items():
        current_consumer = None
        for index, original in enumerate(report['calls']):
            if original['entry'] == '000c57a0': current_consumer = consumer_map[(suite, index)]
            if original['entry'] != '000fbfa0': continue
            nested = []
            for following in report['calls'][index + 1:]:
                if following['entry'] not in ('000bacc0', '000b9c20', '001dc8f0', '001dc940', '0014ba50'): break
                nested.append(following)
            result = replay.execute(original)
            for old in nested:
                new = next(c for c in result['calls'] if c['entry'] == old['entry'])
                if 'output_hex' in old:
                    if old['entry'] in ('000b9c20', '001dc940'):
                        require(new['output_hex'][:4] == old['output_hex'][:4] and new['output_hex'][8:] == old['output_hex'][8:], 'Defined original marker bytes differ')
                    else: require(new['output_hex'] == old['output_hex'], 'Original plane bytes differ')
                    marker_checks += 1
                if old['entry'] == '001dc8f0': require(new['return_eax'] == old['return_eax'], 'Original marker selection differs')
            rows.append(dict(id=len(rows), suite=suite, source_call_index=index, caller=original['caller'], phase=original['phase'],
                consumer_global_id=current_consumer if original['caller'] == '000c5845' else None,
                datum=original['object_before']['datum'], definition=original['object_before']['definition'],
                original_output_bits=original['point']['bits'], original_return_al=int(original['return_eax'], 16) & 255,
                original_nested_calls=nested, **result))
    require(len(rows) == 208 and sum(r['return_al'] for r in rows) == 3, 'Bounded item-point coverage changed')
    starts = {int(r['address'], 16) for r in csv.DictReader((ROOT / 'analysis/map-pack-update/functions.tsv').open(), delimiter='\t')}
    instructions = []
    for entry in ENTRIES:
        routine = _trace(replay.code, entry, starts | set(ENTRIES)); code = [replay.code.at(a) for a in sorted(routine.code)]
        path = out / 'instructions' / f'{entry:08x}.asm'
        data = ('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}' for i in code) + '\n').encode()
        preserve(path, data)
        instructions.append(dict(entry=f'{entry:08x}', name=f'{entry:08x}.asm', sha256=sha(path), rows=len(code),
            executed=sum(i.address in replay.executed for i in code)))
    report = dict(tool_sha256=sha(Path(__file__)), source_pins=pins, cases=rows, instructions=instructions,
        original_marker_plane_output_checks=marker_checks,
        coverage=dict(cases=len(rows), root_fallback=205, marker_projection=3, consumer_correlated=207),
        state_boundary='Literal original current object/header/node/region bytes and real cache/BSP imported as inputs. No live pose, Havok/contact or world-registration producer executes here. Existing pose reciprocal boundary retained; item-point arithmetic executes original instructions without services.',
        fpu_boundary='Same explicitly configured source fixture FPCW027F/MXCSR1F80; no recovered Xbox initialization claim.')
    preserve(out / 'report.json', json_bytes(report)); print(json.dumps(dict(coverage=report['coverage'], report_sha256=sha(out / 'report.json'))))


if __name__ == '__main__': main()
