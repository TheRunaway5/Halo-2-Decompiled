#!/usr/bin/env python3
"""Independent unchanged-i386 AI awareness reference; never a runtime backend.

The two gameplay roots execute their complete original iterator/membership
closure. Declared actor/clump/group/property inputs provide no callee outcomes.
Physical ptrace, rather than Unicorn arithmetic, supplies every retained result.
"""
import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import struct
import subprocess

from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry

ENTRIES = (0x27d7d0, 0x27da40, 0x27caa0, 0x27cb30, 0x27c950,
           0x27c880, 0x20d590, 0x20cff0, 0x20d070, 0x20d0d0, 0x20d030)
NONE = 0xffffffff
AI, ACTORS, SQUADS, SQUAD_GROUPS, CLUMPS, GROUPS, PROPERTIES, CLOCK = (
    0x1000000, 0x1001000, 0x1001100, 0x1001200, 0x1001300,
    0x1001400, 0x1001500, 0x1001800)
SCENARIO, ACTOR_ROWS, SQUAD_ROWS, SQUAD_GROUP_ROWS = (
    0x1002000, 0x1010000, 0x1020000, 0x1030000)
CLUMP_ROWS, GROUP_ROWS, PROPERTY_ROWS = 0x1040000, 0x1050000, 0x1060000
DEFINITIONS, STARTS = 0x1070000, 0x1071000
OBJECT = 0xabcd0007
sha = common.sha


def actor(index):
    return ((0xe100 + index) << 16) | index


def prepare(image, meta, scratch):
    owned = scratch / 'physical-original'
    owned.mkdir()
    section = image.section('.text')
    (owned / 'text.bin').write_bytes(image.section_bytes(section))
    (owned / 'metadata.json').write_text(json.dumps(meta))
    regions = {
        'inputs': (0x1000000, 0x80000), 'constants': (0x46a000, 4096),
        'scenario': (0x4ec000, 4096), 'ai': (0x502000, 4096),
        'perception': (0x50e000, 4096), 'clock': (0x51d000, 0x3000),
        'squads': (0x52b000, 4096), 'fault_edge': (0x1100000, 4096),
        'stack': (geometry.STACK, 65536)}
    assembly = f'.code32\n.section .original,"ax"\n.incbin "{owned / "text.bin"}"\n'
    for name, (_, size) in regions.items():
        assembly += f'.section .{name},"aw"\n.space {size}\n'
    assembly += '''.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
'''
    (owned / 'reference.s').write_text(assembly)
    (owned / 'link.ld').write_text(
        f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} ' +
        ''.join(f'.{name} 0x{address:x}:{{*(.{name})}} '
                for name, (address, _) in regions.items()) +
        f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
    for command in (
            ['as', '--32', str(owned / 'reference.s'), '-o', str(owned / 'reference.o')],
            ['ld', '-melf_i386', '-T', str(owned / 'link.ld'),
             str(owned / 'reference.o'), '-o', str(owned / 'reference')],
            ['c++', '-std=c++20', '-O2',
             str(ROOT / 'tools/observe_native_campaign_ai_awareness_tu15.cpp'),
             '-o', str(owned / 'observer')]):
        result = subprocess.run(command, text=True, capture_output=True)
        if result.returncode:
            raise RuntimeError(result.stderr)
    return owned


def inputs(image):
    rows = []

    def make(name, entry=0x27d7d0, first=0, second=1, enabled=1,
             rate=30, twenty=False, property_base=PROPERTY_ROWS,
             changes=(), mutations=(), clock=CLOCK, depth=None, top=None, **fields):
        n = len(rows)
        banks = {}

        def bank(address, size):
            begin, end = address, address + size
            overlaps = [(p, b) for p, b in banks.items()
                        if p < end and begin < p + len(b)]
            if overlaps:
                begin = min(begin, *(p for p, _ in overlaps))
                end = max(end, *(p + len(b) for p, b in overlaps))
            data = bytearray(end - begin)
            for p, old in overlaps:
                data[p - begin:p - begin + len(old)] = old
                del banks[p]
            banks[begin] = data

        def put(address, value, width=4):
            raw = (value & ((1 << (8 * width)) - 1)).to_bytes(width, 'little')
            for start, data in banks.items():
                if start <= address and address + width <= start + len(data):
                    data[address - start:address - start + width] = raw
                    return
            bank(address, width)
            put(address, value, width)

        for p, size in ((AI, 0x400), (ACTORS, 0x600), (SCENARIO, 0x168),
                        (ACTOR_ROWS, 20 * 0x888), (SQUAD_ROWS, 4 * 0x98),
                        (SQUAD_GROUP_ROWS, 4 * 0x38), (CLUMP_ROWS, 20 * 0x50),
                        (GROUP_ROWS, 20 * 0xc4), (property_base, 40 * 0x3c),
                        (CLOCK, 0x20), (DEFINITIONS, 4 * 0x74),
                        (STARTS, 4 * 2 * 0x64)):
            bank(p, size)
        pointers = ((0x5020a8, AI), (0x5020c8, ACTORS), (0x52be7c, SQUADS),
                    (0x52be80, SQUAD_GROUPS), (0x50eef8, CLUMPS),
                    (0x50eef4, GROUPS), (0x50eef0, PROPERTIES),
                    (0x51df54, clock), (0x4ecc20, SCENARIO))
        for p, target in pointers:
            bank(p, 4)
            put(p, target)
        for pool, records, stride, capacity in (
                (ACTORS, ACTOR_ROWS, 0x888, 20), (SQUADS, SQUAD_ROWS, 0x98, 4),
                (SQUAD_GROUPS, SQUAD_GROUP_ROWS, 0x38, 4),
                (CLUMPS, CLUMP_ROWS, 0x50, 20), (GROUPS, GROUP_ROWS, 0xc4, 20),
                (PROPERTIES, property_base, 0x3c, 40)):
            put(pool + 0x20, capacity)
            put(pool + 0x24, stride)
            put(pool + 0x44, records)
        put(AI + 1, enabled, 1)
        put(AI + 0x14, actor(0))
        put(CLOCK + 2, rate, 2)
        put(SCENARIO + 0x160, 4)
        put(SCENARIO + 0x164, DEFINITIONS)
        count = 20 if twenty else 8
        per_squad = 5 if twenty else 2
        for squad in range(4):
            put(SQUAD_ROWS + squad * 0x98 + 0x68, actor(squad * per_squad))
            put(SQUAD_ROWS + squad * 0x98 + 0x6c, squad)
            put(SQUAD_ROWS + squad * 0x98 + 0x74, NONE, 2)
            put(DEFINITIONS + squad * 0x74 + 0x48, 2)
            put(DEFINITIONS + squad * 0x74 + 0x4c, STARTS + squad * 0xc8)
            put(STARTS + squad * 0xc8, 0x1000 + squad)
            put(STARTS + squad * 0xc8 + 0x64, 0x2000 + squad)
        for group in range(4):
            record = SQUAD_GROUP_ROWS + group * 0x38
            for offset in (4, 12, 16):
                put(record + offset, NONE)
            put(record + 8, group)
        put(SQUAD_GROUP_ROWS + 4, 1)
        put(SQUAD_GROUP_ROWS + 0x38 + 12, 2)
        put(SQUAD_GROUP_ROWS + 0x38 + 16, 0)
        put(SQUAD_GROUP_ROWS + 2 * 0x38 + 4, 3)
        put(SQUAD_GROUP_ROWS + 2 * 0x38 + 16, 0)
        put(SQUAD_GROUP_ROWS + 3 * 0x38 + 16, 2)
        for index in range(20):
            record = ACTOR_ROWS + index * 0x888
            put(record, 0xe100 + index, 2)
            put(record + 0x20, actor(index + 1)
                if index + 1 < count and (index + 1) % per_squad else NONE)
            put(record + 0x30, index // per_squad if index < count else NONE)
            put(record + 0x38, (0x1000 if index % per_squad == 0 else 0x2000) + index // per_squad)
            put(record + 0x7c, index if twenty else (0, 0, 1, 2, 3, NONE, 4, 5)[index % 8])
        for clump in range(20):
            put(CLUMP_ROWS + clump * 0x50 + 0x14, clump)
        for group in range(20):
            record = GROUP_ROWS + group * 0xc4
            put(record + 8, OBJECT if group % 2 == 0 else OBJECT + 1)
            put(record + 0x14, NONE)
            put(record + 0x18, group * 2)
            put(record + 0x1c, actor((group + 2) % count))
            for member in range(2):
                prop = property_base + (group * 2 + member) * 0x3c
                put(prop + 4, actor(group if twenty else (group + member) % count))
                put(prop + 0x1c, 0x6130 + member, 2)
                put(prop + 0x1e, 0x7130 + group, 2)
                put(prop + 0x34, group * 2 + 1 if member == 0 else NONE)
        # The ordinary first squad shares clump0. Its two groups select one
        # matching and one nonmatching target actor/squad.
        if not twenty:
            put(GROUP_ROWS + 0x14, 1)
            put(GROUP_ROWS + 0xc4 + 0x1c, actor(6))
        put(0x46a100, int.from_bytes(image.read(0x46a100, 4), 'little'))
        for p, value, width in changes:
            put(p, value, width)
        gp = [0x5a010000 + n * 31 + k * 7 for k in range(8)]
        gp[0] = first
        gp[4] = geometry.SP + (n % 4) * 4
        occupied = (n % 8) if depth is None else depth
        rows.append(dict(
            id=n, name=name, entry=entry, first_ai=first, second_reference=second,
            gp=gp, banks=[dict(address=p, before=b.hex()) for p, b in sorted(banks.items())],
            before_fp=[0x7f | ((0, 2, 3)[n % 3] << 8) | ((n % 4) << 10),
                       (((n % 8) if top is None else top) << 11) |
                       ((n * 0x100) & 0x4700) | (n * 3 % 64),
                       0x1f80 | ((n % 4) << 13) | (0, 0x40, 0x8000, 0x8040)[n % 4] | (n * 7 % 64)],
            live_x87=[struct.pack('<QH', 0x8000000000000000 + (k << 58),
                                  (0x3fff + k % 3) | (0x8000 if k % 2 else 0)).hex()
                      for k in range(occupied)],
            input_changes=list(mutations), **fields))

    refs = (0, 1, 0x40000000, 0x40000001, 0x40000002,
            0x80000000, 0x80000002, 0xc0000000, 0xc0010001, NONE)
    for first in refs:
        for second in (0, 1, 0x40000000, 0x40000001,
                       0x80000002, 0x80000006, 0xc0010000, NONE):
            make(f'ai_{first:x}_{second:x}', first=first, second=second)
    for first in (0, 1, 2, 3, 0x40000000, 0x80000000, 0xc0000000, NONE):
        for second in (OBJECT, OBJECT + 1, NONE):
            make(f'object_{first:x}_{second:x}', entry=0x27da40, first=first, second=second)
    for entry in (0x27d7d0, 0x27da40):
        make(f'disabled_{entry:x}', entry=entry, enabled=0,
             second=1 if entry == 0x27d7d0 else OBJECT)
        for rate in (-32768, -31, -1, 0, 1, 29, 32767):
            make(f'rate_{entry:x}_{rate}', entry=entry, rate=rate,
                 second=1 if entry == 0x27d7d0 else OBJECT)
    for depth in range(8):
        make(f'live_x87_{depth}', depth=depth)
    for top in range(8):
        for depth in range(8):
            make(f'live_top_{top}_depth_{depth}', top=top, depth=depth)
    for name, changes in (
            ('no_first_squad_members', ((SQUAD_ROWS + 0x68, NONE, 4),)),
            ('no_clump', ((ACTOR_ROWS + 0x7c, NONE, 4),)),
            ('empty_clump', ((CLUMP_ROWS + 0x14, NONE, 4),)),
            ('group_owner_none', ((GROUP_ROWS + 0x1c, NONE, 4),)),
            ('group_properties_none', ((GROUP_ROWS + 0x18, NONE, 4),)),
            ('property_owner_other_squad', ((PROPERTY_ROWS + 4, actor(4), 4),)),
            ('property_owner_no_squad', ((ACTOR_ROWS + 0x30, NONE, 4),)),
            ('empty_group_tree', ((SQUAD_GROUP_ROWS + 4, NONE, 4),
                                  (SQUAD_GROUP_ROWS + 8, NONE, 4))),
            ('location_name_missing', ((STARTS + 0xc8, 0x98765432, 4),)),
            ('location_negative', ())):
        make(name, first=0x40000000 if name == 'empty_group_tree' else 0,
             second=0xc001ffff if name == 'location_negative' else
                    0xc0010000 if name == 'location_name_missing' else 1,
             changes=changes)
    make('twenty_real_clumps', first=0x40000000, second=0x40000000, twenty=True)
    all_members = tuple(change for index in range(20) for change in (
        (ACTOR_ROWS + index * 0x888 + 0x20, actor(index + 1) if index < 19 else NONE, 4),
        (ACTOR_ROWS + index * 0x888 + 0x30, 0, 4)))
    make('object_twenty_real_clumps', entry=0x27da40, first=0, second=OBJECT,
         twenty=True, changes=all_members)
    make('clock_alias_state_word', clock=PROPERTY_ROWS + 0x1a)
    make('object_clock_alias_state_word', entry=0x27da40, second=OBJECT,
         clock=PROPERTY_ROWS + 0x1a)
    # Capture the actor iterator's next member before a real property store
    # aliases that actor's linked-list field. Keep its timer source ordinary.
    make('property_alias_actor_next', property_base=ACTOR_ROWS + 4,
         changes=((GROUP_ROWS + 0x18, 0, 4), (GROUP_ROWS + 0x14, NONE, 4),
                  (ACTOR_ROWS + 0x20, actor(1), 4), (ACTOR_ROWS + 0x30, 0, 4),
                  (ACTOR_ROWS + 0x38, NONE, 4), (ACTOR_ROWS + 0x7c, 0, 4),
                  (ACTOR_ROWS + 0x888 + 0x20, NONE, 4),
                  (ACTOR_ROWS + 0x888 + 0x30, 0, 4),
                  (ACTOR_ROWS + 0x888 + 0x7c, 0, 4),
                  (ACTOR_ROWS + 4 + 4, actor(0), 4),
                  (ACTOR_ROWS + 4 + 0x34, NONE, 4)))
    # Only one linked property is required for the true clock-pointer overlap:
    # the pointer is fetched before publishing state3 through its low WORD.
    make('property_alias_clock_pointer', property_base=0x51df38,
         changes=((0x51df54, CLOCK, 4), (0x51df38 + 0x34, NONE, 4),
                  (GROUP_ROWS + 0x14, NONE, 4)))
    for name, first, second in (
            ('bad_first_starting_squad', 0xffff0000, 1),
            ('bad_second_starting_squad', 0, 0xffff0000),
            ('bad_second_starting_index', 0, 0xc0010002),
            ('bad_second_negative_start', 0, 0xc001ffff)):
        make(name, first=first, second=second)
    for entry in (0x27d7d0, 0x27da40):
        second = 1 if entry == 0x27d7d0 else OBJECT
        make(f'actual_clock_fault_{entry:x}', entry=entry, second=second,
             clock=0x40000000, actual_read_fault=True)
        make(f'actual_actor_rows_fault_{entry:x}', entry=entry, second=second,
             changes=((ACTORS + 0x44, 0x40000000, 4),), actual_read_fault=True)
        make(f'actual_property_rows_fault_{entry:x}', entry=entry, second=second,
             changes=((PROPERTIES + 0x44, 0x40000000, 4),), actual_read_fault=True)
        make(f'actual_property_state_write_fault_{entry:x}', entry=entry,
             second=second, property_base=0x1100fe8,
             changes=((GROUP_ROWS + 0x14, NONE, 4),), actual_write_fault=True)
        row = rows[-1]
        row['banks'] = [b for b in row['banks']
                        if not 0x1100000 <= b['address'] < 0x1200000]
        row['banks'].append(dict(address=0x1100fec, before=actor(0).to_bytes(4, 'little').hex()))
        row['banks'].sort(key=lambda b: b['address'])
    make('fault_seed_ai_group', first=0x40000000, second=0x40000000)
    make('fault_seed_ai_actor', first=0x80000000, second=0x80000002)
    make('fault_seed_object', entry=0x27da40, first=0, second=OBJECT)
    make('fault_seed_clock_alias', clock=PROPERTY_ROWS + 0x1a)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--verify', type=Path)
    parser.add_argument('--complete-only', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    assert scratch.is_dir() and not args.output.exists()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    common.ENTRIES = ENTRIES
    meta = common.metadata(image)
    meta['required_owners'] = {}
    assert sum(row['sites'] for row in meta['owners'].values()) == 590
    owned = prepare(image, meta, scratch)
    rows = geometry.physical(owned, inputs(image), 'complete')
    prefix = []
    if not args.complete_only:
        for seed in (r for r in rows if r['name'].startswith('fault_seed')):
            for phase, events in (('before_read', seed['reads']),
                                  ('before_write', seed['writes']),
                                  ('after_write', seed['writes'])):
                for n in range(len(events)):
                    row = copy.deepcopy(seed)
                    row.update(id=len(rows) + len(prefix),
                               name=seed['name'] + '_' + phase + '_' + str(n),
                               fault=dict(phase=phase, ordinal=n))
                    prefix.append(row)
        rows += geometry.physical(owned, prefix, 'prefix')
    visited = set().union(*(set(r['executed_sites']) for r in rows))
    uncovered = [row['instruction'] for row in meta['sites']
                 if row['instruction'] not in visited]
    fixture = dict(image_sha256=NEW_PIN, backend='physical-unchanged-i386-offline',
                   owners=meta['owners'], instruction_patches=[],
                   supplied_original_outcomes=False, cases=rows)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(gzip.compress(json.dumps(fixture, separators=(',', ':')).encode(), mtime=0))
    repeat = None
    if args.verify:
        repeat = args.output.read_bytes() == args.verify.read_bytes()
        assert repeat, 'Independent physical awareness repeat differs'
    report = dict(passed=True, scope=__doc__, cases=len(rows),
                  complete_calls=sum(row['completed'] for row in rows),
                  fault_prefixes=len(prefix), original_sites=len(visited),
                  total_original_sites=len(meta['sites']),
                  uncovered_sites=[hex(pc) for pc in uncovered],
                  original_instructions=sum(row['original_instructions'] for row in rows),
                  ordered_reads=sum(len(row['reads']) for row in rows),
                  ordered_stores=sum(len(row['writes']) for row in rows),
                  owners=meta['owners'], fixture=str(args.output),
                  fixture_bytes=args.output.stat().st_size, fixture_sha256=sha(args.output),
                  original_sha256=NEW_PIN, repeat_identical=repeat,
                  source_hashes={str(Path(__file__).relative_to(ROOT)): sha(__file__),
                                 'tools/observe_native_campaign_ai_awareness_tu15.cpp':
                                 sha(ROOT / 'tools/observe_native_campaign_ai_awareness_tu15.cpp')},
                  owned_scratch=str(scratch), retained_large_artifacts=[],
                  scope_limits=['Declared genuine20-slot clump domain; no invalid bitmap index profile',
                                'Original masked FP policy with0..7 older live x87 operands',
                                'HS evaluator glue is separately authenticated caller data flow',
                                'No native runtime, visibility/raycast result or complete campaign qualification'])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(rows)} physical awareness cases/{len(visited)} of{len(meta["sites"])} sites')


if __name__ == '__main__':
    main()
