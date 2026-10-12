#!/usr/bin/env python3
"""Recheck complete vehicle update/controller bodies and their real TU bindings.

Extends the sealed vehicle controller correspondence without changing that evidence.
Every instruction, branch target, jump table and encoded image reference comes
from the two owned executables. Unported callees remain actual required owners.
"""
import argparse
import collections
import csv
import json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, full_tokens, containing
from research_vehicle_control_20261011 import sha

PARENT = ROOT / 'analysis/multiplayer/sandbox-source/vehicle-runtime-20261011/correspondence-verified/report.json'
PARENT_PIN = '720087ac03bd5f7c01f54d45d9c66997f46cd79dc7377508b5fea9f06e0c59ab'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert sha(PARENT) == PARENT_PIN
    parent = json.loads(PARENT.read_text())
    paths = (ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03', NEW_DEFAULT)
    assert tuple(map(sha, paths)) == (PIN, NEW_PIN)
    images = tuple(map(Xbe, paths))
    codes = tuple(_Code(image, '.text') for image in images)
    inventories = (ROOT / 'analysis/map-pack-update/functions.tsv',
                   ROOT / 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')
    starts = [{int(row['address'], 16) for row in csv.DictReader(path.open(), delimiter='\t')}
              for path in inventories]
    mapping = {int(p, 16): q for p, q in parent['entries'].items()}
    parent_entries = sorted(mapping)
    # Admit these callees only through matching calls in the already paired
    # callers. Their TU addresses are read from instructions, never inferred
    # from a section delta or from a nearby symbol.
    pending = {0xd26c0, 0xf2960, 0xf0eb0, 0xf11a0, 0xf1270, 0xf1d00,
               0xbb0c0, 0x208ed0, 0x115600, 0x1105e0, 0xe7e30,
               0xb86c0, 0xb8630, 0x1c8430, 0xb8250, 0xbc910, 0xbaa80,
               0xbd6a0, 0xbbe70, 0x1d43c0, 0x1d44c0, 0xe7e60}
    while pending:
        added = {}
        for entry, target in mapping.items():
            a = [codes[0].at(p) for p in sorted(_trace(codes[0], entry, starts[0]).code)]
            b = [codes[1].at(p) for p in sorted(_trace(codes[1], target, starts[1]).code)]
            assert full_tokens(a, images[0]) == full_tokens(b, images[1])
            for old, new in zip(a, b):
                if old.mnemonic in ('call', 'jmp') and old.operands[0].type == 2 and old.operands[0].imm in pending:
                    p, q = old.operands[0].imm, new.operands[0].imm
                    assert p in starts[0] and q in starts[1]
                    assert p not in added or added[p] == q
                    added[p] = q
        assert added, sorted(pending)
        mapping.update(added)
        pending.difference_update(added)
    instructions = {}
    traces = {}
    sites = {}
    addresses = {}
    mnemonics = collections.Counter()
    outside = collections.defaultdict(list)
    indirect = []

    def bind(p, q):
        key = f'{p:08x}'
        assert key not in addresses or addresses[key] == q, (key, q, addresses.get(key))
        addresses[key] = q

    for entry, target in sorted(mapping.items()):
        left_trace, right_trace = _trace(codes[0], entry, starts[0]), _trace(codes[1], target, starts[1])
        left = [codes[0].at(p) for p in sorted(left_trace.code)]
        right = [codes[1].at(p) for p in sorted(right_trace.code)]
        assert full_tokens(left, images[0]) == full_tokens(right, images[1])
        instructions[entry] = left, right
        traces[entry] = left_trace, right_trace
        for old, new in zip(left, right):
            key = f'{old.address:08x}'
            assert key not in sites or sites[key] == new.address
            sites[key] = new.address
            mnemonics[old.mnemonic] += 1
            for a, b in zip(old.operands, new.operands):
                refs = []
                if a.type == b.type == 3:
                    refs.append((a.mem.disp, b.mem.disp))
                elif a.type == b.type == 2:
                    refs.append((a.imm, b.imm))
                for p, q in refs:
                    if containing(images[0], p) and containing(images[1], q):
                        bind(p, q)
            if old.mnemonic == 'call':
                if old.operands[0].type == 2:
                    p, q = old.operands[0].imm, new.operands[0].imm
                    if p in mapping:
                        assert mapping[p] == q
                    else:
                        outside[f'{p:08x}'].append(dict(site=old.address, later_target=q, owner=entry))
                else:
                    indirect.append(dict(site=old.address, owner=entry, operand=old.op_str))
    args.output.mkdir(parents=True, exist_ok=False)
    directory = args.output / 'instructions'
    directory.mkdir()
    rows = []
    for entry, target in sorted(mapping.items()):
        left, right = instructions[entry]
        a, b = traces[entry]
        row = dict(disc_1_1_entry=f'{entry:08x}', later_entry=f'{target:08x}',
                   instructions=len(left), complete_paired_body=True, tables=[])
        assert len(a.tables) == len(b.tables)
        for (p, width, count), (q, other_width, other_count) in zip(a.tables, b.tables):
            assert (width, count) == (other_width, other_count)
            old, new = images[0].read(p, width * count), images[1].read(q, width * count)
            if width == 4:
                for n in range(count):
                    x = int.from_bytes(old[n * 4:n * 4 + 4], 'little')
                    y = int.from_bytes(new[n * 4:n * 4 + 4], 'little')
                    assert sites[f'{x:08x}'] == y
            else:
                assert old == new
            bind(p, q)
            row['tables'].append(dict(disc=p, later=q, width=width, count=count))
        for label, ins in (('disc', left), ('later', right)):
            path = directory / f'{entry:08x}-{label}.asm'
            path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
            row[label + '_listing'] = dict(file=path.name, sha256=sha(path))
        rows.append(row)
    # The normalizer's actual immutable pointer owns its fallback vector.
    pointer = 0x46fc18
    later_pointer = addresses[f'{pointer:08x}']
    old_default = int.from_bytes(images[0].read(pointer, 4), 'little')
    new_default = int.from_bytes(images[1].read(later_pointer, 4), 'little')
    assert containing(images[0], old_default) and containing(images[1], new_default)
    bind(old_default, new_default)
    assert images[0].read(old_default, 8) == images[1].read(new_default, 8)
    # The velocity owner's immutable zero-vector pointer is a real image
    # reference just like the steering normalizer's fallback.
    pointer_bindings = []
    for p, width in ((0x46fc44, 12),):
        q = addresses[f'{p:08x}']
        x = int.from_bytes(images[0].read(p, 4), 'little')
        y = int.from_bytes(images[1].read(q, 4), 'little')
        assert containing(images[0], x) and containing(images[1], y)
        assert images[0].read(x, width) == images[1].read(y, width)
        bind(x, y)
        pointer_bindings.append(dict(pointer=p, later_pointer=q, target=x, later_target=y, bytes=width))
    report = dict(schema=1, extension_tool_sha256=sha(__file__), parent_sha256=PARENT_PIN,
        parent_native_entries=parent_entries, additional_native_entries=sorted(set(mapping)-set(parent_entries)),
        pointer_bindings=pointer_bindings,
        disc_1_1_sha256=PIN, later_owned_xbe_sha256=NEW_PIN, complete_bodies=len(mapping),
        entries={f'{p:08x}': q for p, q in mapping.items()}, routines=rows,
        instruction_map=sites, address_map=addresses, mnemonics=dict(mnemonics),
        outside_direct_calls=dict(outside), indirect_calls=indirect,
        fallback_vector=dict(old_pointer=pointer, later_pointer=later_pointer,
            old_target=old_default, later_target=new_default,
            raw_hex=images[0].read(old_default, 8).hex()),
        production_original_execution=False, live_vehicle_physics_integrated=False)
    path = args.output / 'report.json'
    path.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(dict(complete_bodies=len(mapping), instructions=sum(mnemonics.values()),
                          report_sha256=sha(path))))


if __name__ == '__main__':
    main()
