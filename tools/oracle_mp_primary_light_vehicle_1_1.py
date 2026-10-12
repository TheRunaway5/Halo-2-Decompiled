#!/usr/bin/env python3
"""Repeat the bounded original Lockout primary-light-vehicle option control.

The whole original row-spawn producer executes. Its object allocator is the
existing, explicitly unavailable InventoryTrace service; no object success is
manufactured. The authenticated 1.5 image supplies independent static evidence,
not a claimed 1.5 execution. Existing source suites remain unchanged.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_mp_inventory_1_1 import InventoryTrace, ROOT, GAME, SCRATCH, SEED
from oracle_smg_firing_1_1 import PIN, CACHE_PIN, Xbe, Cache
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

U5 = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/'
          'Halo 2 DLC/content/u5English/default.xbe')
U5_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':')).encode()


class OptionTrace(InventoryTrace):
    def __init__(self, value):
        super().__init__()
        self.inventory_services()
        self.entries = SCRATCH + 0x1000
        # Declared isolated row-bank input, not an executed game initializer.
        for row in range(100):
            self.w(self.entries + row * 8, -1)
            self.sh(self.entries + row * 8 + 4, 0)
            self.sh(self.entries + row * 8 + 6, 0)
        self.g.mem_write(GAME + 0x209, bytes([value]))
        self.option_reads, self.stores, self.requests = [], [], []
        self.pending_stores = []
        self.visits, self.source_visits = [], []
        self.g.hook_add(UC_HOOK_MEM_READ, self.read)
        self.g.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def hook(self, guest, address, size, user):
        self.flush_stores()
        self.visits.append(address)
        if address not in self.services:
            self.source_visits.append(address)
        if address == 0xb8a30:
            stack = guest.reg_read(UC_X86_REG_ESP)
            descriptor = self.u(stack + 4)
            self.requests.append(dict(row=self.row,
                original_service='000b8a30', return_value='ffffffff',
                supplied_unavailable=True,
                request_descriptor_hex=bytes(guest.mem_read(descriptor, 0xc4)).hex(),
                stack_arguments=[f'{descriptor:08x}']))
        super().hook(guest, address, size, user)

    def read(self, guest, access, address, size, value, user):
        if address <= GAME + 0x209 < address + size:
            self.option_reads.append(dict(row=self.row,
                instruction=f'{guest.reg_read(UC_X86_REG_EIP):08x}',
                address=f'{address:08x}', size=size))

    def write(self, guest, access, address, size, value, user):
        # Retain every actual original write, including stack/descriptor writes.
        record = dict(row=self.row,
            instruction=f'{guest.reg_read(UC_X86_REG_EIP):08x}',
            address=f'{address:08x}', size=size,
            before_hex=bytes(guest.mem_read(address, size)).hex())
        self.stores.append(record)
        self.pending_stores.append((address, size, record))

    def flush_stores(self):
        # UC_HOOK_MEM_WRITE runs before the write, and its value parameter is
        # insufficient for a full128-bit SIMD store. Read actual retired bytes
        # at the next instruction, rather than manufacture the upper words.
        for address, size, record in self.pending_stores:
            record['after_hex'] = bytes(self.g.mem_read(address, size)).hex()
        self.pending_stores.clear()


def run(value):
    trace = OptionTrace(value)
    cache_rows = trace.cache.block(trace.cache.scenario + 0x120, 0x90)
    rows = []
    for index, address in enumerate(cache_rows):
        trace.row = index
        request_start, store_start, visit_start = (
            len(trace.requests), len(trace.stores), len(trace.source_visits))
        before = bytes(trace.g.mem_read(trace.entries + index * 8, 8)).hex()
        seed_before = f'{trace.u(SEED):08x}'
        trace.action(f'authored_row_{index}', 0x15c110, (trace.entries, index))
        trace.flush_stores()
        rows.append(dict(index=index, file_offset=f'{address:08x}',
            classification=int(trace.cache.data[address + 0x14]),
            authored_record_hex=bytes(trace.cache.data[address:address + 0x90]).hex(),
            before_hex=before,
            after_hex=bytes(trace.g.mem_read(trace.entries + index * 8, 8)).hex(),
            seed_before=seed_before, seed_after=f'{trace.u(SEED):08x}',
            request_range=[request_start, len(trace.requests)],
            store_range=[store_start, len(trace.stores)],
            source_visit_range=[visit_start, len(trace.source_visits)]))
    invoked = {event['service'] for event in trace.events if 'service' in event}
    assert invoked == {'000b8a30'}, invoked
    assert not trace.option_reads
    assert len(rows) == 32 and len(trace.requests) == 14
    assert trace.u(SEED) == 0x12345678
    assert all(row['classification'] != 1 for row in rows)
    result = dict(rows=rows, requests=trace.requests, ordered_stores=trace.stores,
        option_reads=trace.option_reads,
        final_entries_hex=bytes(trace.g.mem_read(trace.entries, 800)).hex(),
        final_seed=f'{trace.u(SEED):08x}',
        source_instruction_visits=[f'{address:08x}' for address in trace.source_visits],
        source_instruction_visit_sha256=hashlib.sha256(
            b''.join(struct.pack('<I', address) for address in trace.source_visits)).hexdigest(),
        invoked_services=[dict(address=address,
            name=trace.services[int(address, 16)][0],
            return_value=f'{trace.services[int(address, 16)][1]:08x}',
            stack_cleanup=trace.services[int(address, 16)][2]) for address in sorted(invoked)])
    return result, trace


def static_image(path, expected, version, chooser, leaf, caller, output):
    assert sha(path) == expected, path
    image = Xbe(path)
    code = Cs(CS_ARCH_X86, CS_MODE_32)
    text = image.section('.text')
    data = image.section_bytes(text)
    # Raw exact rel32 operand inventory; the validated caller listing below
    # distinguishes real calls from the merely possible raw-byte matches.
    calls = []
    for offset in range(len(data) - 4):
        if data[offset] == 0xe8 and (
            text.va + offset + 5 + struct.unpack_from('<i', data, offset + 1)[0]
        ) & 0xffffffff == chooser:
            calls.append(text.va + offset)
    assert calls == [caller + 0x8a], calls
    pointer_refs = []
    needle = struct.pack('<I', chooser)
    for section in image.sections:
        section_bytes = image.section_bytes(section)
        begin = 0
        while (offset := section_bytes.find(needle, begin)) != -1:
            pointer_refs.append(dict(section=section.name, address=f'{section.va + offset:08x}'))
            begin = offset + 1
    assert not pointer_refs
    regions = []
    for name, address, size, table_count in (
        ('chooser', chooser, 0xb6, 7),
        ('primary-light-leaf', leaf, 0xed, 7),
        ('whole-row-spawn', caller, 0x17d, 0),
    ):
        raw = image.read(address, size)
        filename = f'{version}-{name}-{address:08x}'
        (output / (filename + '.bin')).write_bytes(raw)
        listing = ''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'
                          for i in code.disasm(raw, address))
        (output / (filename + '.asm')).write_text(listing)
        region = dict(name=name, address=f'{address:08x}', size=size,
            sha256=hashlib.sha256(raw).hexdigest(),
            raw_file=filename + '.bin', listing_file=filename + '.asm')
        if table_count:
            # Chooser has 2 alignment bytes; primary leaf has 3 alignment bytes.
            table_address = address + size + (2 if name == 'chooser' else 3)
            table = image.read(table_address, table_count * 4)
            (output / (filename + '.table.bin')).write_bytes(table)
            region['switch_table'] = dict(address=f'{table_address:08x}',
                targets=[f'{target:08x}' for target in struct.unpack('<7I', table)],
                sha256=hashlib.sha256(table).hexdigest())
        regions.append(region)
    return dict(version=version, path=str(path), sha256=expected,
        chooser_direct_call_operand_matches=[f'{a:08x}' for a in calls],
        chooser_literal_pointer_references=pointer_refs, regions=regions,
        scope='Read-only original instruction/byte evidence; raw rel32 scan is '
              'validated against the recorded caller. Only TU1.1 whole-row spawn is executed.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-directory', required=True, type=Path)
    parser.add_argument('--u5-xbe', type=Path, default=U5)
    args = parser.parse_args()
    output = args.output_directory.resolve()
    if output.exists() and any(output.iterdir()):
        parser.error('output directory must be absent or empty; previous evidence is immutable')
    output.mkdir(parents=True, exist_ok=True)
    assert sha(ROOT / 'assets/disc/maps/lockout.map') == CACHE_PIN
    menu_path = ROOT / 'assets/disc/maps/mainmenu.map'
    # Original menu identity is retained in the report rather than inferred
    # from the runtime enum or a native default.
    menu = Cache(menu_path)
    variants = menu.inspect_variants()
    parameter = variants['parameters'][76]
    assert [(c['value'], c['label']) for c in parameter['choices']] == [
        (0, 'Map Default'), (1, 'Warthog'), (2, 'Gauss Warthog'), (3, 'Ghost'),
        (5, 'Spectre'), (6, 'Random'), (7, 'None')]
    a, trace = run(0)
    b, _ = run(1)
    assert a == b, 'primary-light option changed a Lockout original row result'
    for value, result in ((0, a), (1, b)):
        (output / f'original-option-{value}.json').write_text(json.dumps(result, indent=2) + '\n')
    listing_dir = output / 'instructions'
    listing_dir.mkdir()
    images = [static_image(
        ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
        PIN, '1.1', 0x15bb10, 0x15b780, 0x15c110, listing_dir),
        static_image(args.u5_xbe, U5_PIN, '1.5', 0x15cef0, 0x15cb60, 0x15d4f0, listing_dir)]
    dependencies = [
        'tools/oracle_mp_primary_light_vehicle_1_1.py',
        'tools/oracle_mp_inventory_1_1.py', 'tools/oracle_smg_firing_1_1.py',
        'tools/inspect_multiplayer.py',
        'tools/reference/halo2-decompiled/tools/xbe.py',
        'tools/reference/halo2-decompiled/tools/functions.py',
        'tools/reference/assembly/src/Assembly/Plugins/Halo2/sily.xml',
        'tools/reference/assembly/src/Assembly/Plugins/Halo2/scnr.xml',
        'tools/reference/halo2-decompiled/src/unknown_236ae6.cpp',
        'tools/reference/halo2-decompiled/src/unknown_19d220.cpp',
        'tools/reference/halo2-decompiled/include/unknown_19d220.h',
        'tests/fixtures/frontend-lobby-game-original.tsv',
        'analysis/frontend/lobby/game-panel-original.json',
    ]
    cache = trace.cache
    report = dict(
        parameter=76, parameter_hex='4c', original_parameter=parameter,
        original_slayer=variants['variants'][0],
        variant_byte_offset='cd', game_options_byte_offset='209',
        classification=1, map='lockout', xbe_images=images,
        cache_sha256=CACHE_PIN, menu_sha256=sha(menu_path),
        scenario_file_offset=f'{cache.scenario:08x}',
        vehicle_placement_count=cache.u(cache.scenario + 0x70),
        vehicle_palette_count=cache.u(cache.scenario + 0x78),
        vehc_tags=[tag for tag in cache.tags.values() if tag['group'] == 'vehc'],
        row_count=len(a['rows']), classifications=[row['classification'] for row in a['rows']],
        primary_light_row_count=sum(row['classification'] == 1 for row in a['rows']),
        vehc_row_count=sum(cache.u(int(row['file_offset'], 16) + 0x58) == 0x76656863
                           for row in a['rows']),
        original_outputs_equal=a == b,
        canonical_output_sha256=hashlib.sha256(canonical(a)).hexdigest(),
        original_option_read_count=len(a['option_reads']) + len(b['option_reads']),
        requests_per_run=len(a['requests']), ordered_stores_per_run=len(a['ordered_stores']),
        instruction_visits_per_run=len(a['source_instruction_visits']),
        final_seed=a['final_seed'],
        dependencies={path: sha(ROOT / path) for path in dependencies},
        boundaries=[
            'Existing InventoryTrace supplies the entire synthetic mapped engine/options/object context. '
            'It is not a source-created ordinary multiplayer world or session.',
            'This experiment supplies an isolated100x8-byte runtime row bank, each datumNONE/timers0, '
            'and the literal option0 or1. Actual32 authored Lockout records and tag metadata are unchanged.',
            'Original15C110 executes to return for every row, including original collection choice, '
            'classification and allocation descriptor construction. The only invoked external service '
            'is B8A30 returning unavailable/NONE, with its fullC4 descriptor retained before redirection.',
            'No allocation success, vehicle control/collision, membership, readiness, driver or platform result is supplied.',
            'Both1.1 and authenticated1.5 chooser/caller bytes establish the same classification1 '
            'option+209 consumer. No1.5 whole-row execution is claimed.',
            'No affected classification1 vehicle path is executed here. The no-row result does not '
            'justify admitting affected maps or all other nondefault vehicle options.',
        ])
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in (
        'row_count', 'primary_light_row_count', 'requests_per_run',
        'ordered_stores_per_run', 'instruction_visits_per_run',
        'original_option_read_count', 'original_outputs_equal', 'canonical_output_sha256')}))


if __name__ == '__main__':
    main()
