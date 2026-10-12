#!/usr/bin/env python3
"""Bind actual startup registrations through complete original factory bodies.

No nearby-address or same-return-value selection is permitted. Ambiguous full
body matches stop; selected static records and registration stores must agree.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
import research_mp_sandbox_update_functions as base

CONSTRUCTORS = {0x37f110, 0x37f130, 0x37f150, 0x37f1e0, 0x37f240,
    0x37fc70, 0x37fc90, 0x37fcb0}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--parent', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--initializer', type=lambda value: int(value, 16), action='append',
        help='Restrict evidence to an actual required registration; default checks all eight')
    args = parser.parse_args()
    tool_pin = sha(__file__)
    parent = json.loads(args.parent.read_text())
    assert parent['disc_1_1_sha256'] == PIN and parent['later_owned_xbe_sha256'] == base.NEW_PIN
    old_path = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(old_path) == PIN and sha(base.NEW_DEFAULT) == base.NEW_PIN
    old, new = Xbe(old_path), Xbe(base.NEW_DEFAULT)
    oc, nc = _Code(old, '.text'), _Code(new, '.text')
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    md.skipdata = True
    # Match complete bodies only. All decoded instruction starts are candidates
    # because these factory addresses are held in static data, not direct calls.
    section = new.section('.text')
    candidates = [i.address for i in md.disasm(new.section_bytes(section), section.va)
        if i.id and ((i.mnemonic == 'mov' and len(i.operands) == 2
            and i.operands[1].type == 3 and not i.operands[1].mem.base)
        or (i.mnemonic == 'push' and len(i.operands) == 1
            and i.operands[0].type == 2 and i.operands[0].imm == -1))]
    old_starts = {int(p, 16) for p in parent['entries']}
    new_starts = set(parent['entries'].values())
    mapping, addresses, regions, rows, registrations = {}, {}, [], [], []
    parent_entries = {int(p, 16): q for p, q in parent['entries'].items()}
    head = 0x487694
    later_head = parent['address_map'][f'{head:08x}']

    def body(code, entry, starts):
        instructions = [code.at(p) for p in sorted(_trace(code, entry, starts | {entry}).code)]
        assert all(i is not None for i in instructions), 'Candidate leaves decoded text'
        return instructions

    def has_prefix(candidate, expected):
        cursor, probe = candidate, []
        for _ in expected:
            instruction = nc.at(cursor)
            if instruction is None:
                return False
            probe.append(base.norm(instruction, new))
            cursor += instruction.size
        return probe == expected

    def admit(p, q, oi, ni):
        assert base.full_tokens(oi, old) == base.full_tokens(ni, new)
        if p in parent_entries:
            assert parent_entries[p] == q
            return
        assert p not in mapping or mapping[p] == q
        if p in mapping:
            return
        mapping[p] = q
        for x, y in zip(oi, ni):
            for xo, yo in zip(x.operands, y.operands):
                pairs = [(xo.imm, yo.imm)] if xo.type == yo.type == 2 else (
                    [(xo.mem.disp, yo.mem.disp)] if xo.type == yo.type == 3 else [])
                for a, b in pairs:
                    if base.containing(old, a) and base.containing(new, b):
                        assert a not in addresses or addresses[a] == b
                        addresses[a] = b
        rows.append((p, q, oi, ni))

    def child_evidence(oi, ni):
        children, tables = [], []

        def paired_child(p, q):
            if p in parent_entries:
                assert parent_entries[p] == q
                return
            left, right = body(oc, p, old_starts), body(nc, q, new_starts)
            assert base.full_tokens(left, old) == base.full_tokens(right, new)
            children.append((p, q, left, right))

        for x, y in zip(oi, ni):
            if x.mnemonic in ('call', 'jmp') and x.operands[0].type == y.operands[0].type == 2:
                p, q = x.operands[0].imm, y.operands[0].imm
                if oc.section.va <= p < oc.section.va + oc.section.rsize:
                    paired_child(p, q)
            if x.mnemonic != 'mov' or len(x.operands) != 2:
                continue
            if x.operands[0].type != 3 or x.operands[1].type != y.operands[1].type != 2:
                continue
            if x.operands[1].type != 2:
                continue
            p, q = x.operands[1].imm, y.operands[1].imm
            if not (base.containing(old, p) and base.containing(new, q)):
                continue
            left, right = struct.unpack('<II', old.read(p, 8)), struct.unpack('<II', new.read(q, 8))
            # These are exact data words at tables stored by the paired
            # constructors. Both words must hold original code addresses.
            if not all(oc.section.va <= target < oc.section.va + oc.section.rsize for target in left):
                continue
            assert all(nc.section.va <= target < nc.section.va + nc.section.rsize for target in right)
            for offset, (a, b) in enumerate(zip(left, right)):
                paired_child(a, b)
                tables.append(dict(old_store=x.address, later_store=y.address,
                    old_slot=p + 4 * offset, later_slot=q + 4 * offset,
                    old_entry=a, later_entry=b))
        return children, tables

    crt = [(at, int.from_bytes(old.read(at, 4), 'little')) for at in range(0x468630, 0x468cb8, 4)]
    chosen = [(at, p) for at, p in crt if p in CONSTRUCTORS]
    assert len(chosen) == 8 and {p for _, p in chosen} == CONSTRUCTORS
    if args.initializer:
        selected = set(args.initializer)
        assert selected <= CONSTRUCTORS
        chosen = [(at, p) for at, p in chosen if p in selected]
    for crt_word, initializer in chosen:
        oi = body(oc, initializer, old_starts)
        stores = [i for i in oi if i.mnemonic == 'mov' and len(i.operands) == 2
            and i.operands[0].type == 3 and i.operands[0].mem.disp == head
            and i.operands[1].type == 2]
        assert len(stores) == 1
        record = stores[0].operands[1].imm
        factory, next_record, output = struct.unpack('<III', old.read(record, 12))
        assert next_record == 0 and base.containing(old, output)
        fi = body(oc, factory, old_starts)
        prefix = [base.norm(i, old) for i in fi[:5]]
        matches = []
        for candidate in candidates:
            try:
                if not has_prefix(candidate, prefix):
                    continue
                ni = body(nc, candidate, new_starts)
                if base.full_tokens(fi, old) == base.full_tokens(ni, new):
                    children, tables = child_evidence(fi, ni)
                    matches.append((candidate, ni, children, tables))
            except (AssertionError, KeyError, ValueError):
                continue
        assert len(matches) == 1, ('Factory full body and child evidence are ambiguous', hex(factory), [hex(row[0]) for row in matches])
        later_factory, fn, children, tables = matches[0]
        for child in children:
            admit(*child)
        admit(factory, later_factory, fi, fn)
        records = []
        for data in new.sections:
            if data.name == '.text':
                continue
            raw = new.section_bytes(data)
            needle = struct.pack('<II', later_factory, 0)
            for offset in range(0, len(raw) - 11, 4):
                if raw[offset:offset + 8] == needle:
                    pointer = int.from_bytes(raw[offset + 8:offset + 12], 'little')
                    if base.containing(new, pointer):
                        records.append((data.va + offset, pointer))
        assert len(records) == 1, ('Factory static record is ambiguous', hex(factory), records)
        later_record, later_output = records[0]
        constructors = []
        constructor_prefix = [base.norm(i, old) for i in oi[:5]]
        for candidate in candidates:
            try:
                if not has_prefix(candidate, constructor_prefix):
                    continue
                ni = body(nc, candidate, new_starts)
                if base.full_tokens(oi, old) != base.full_tokens(ni, new):
                    continue
                exact = [(x, y) for x, y in zip(oi, ni) if x.address == stores[0].address]
                assert len(exact) == 1
                y = exact[0][1]
                if y.operands[0].mem.disp == later_head and y.operands[1].imm == later_record:
                    constructors.append((candidate, ni))
            except (AssertionError, KeyError, ValueError):
                continue
        assert len(constructors) == 1, ('Registration store does not uniquely match its static record', hex(initializer))
        later_initializer, ni = constructors[0]
        admit(initializer, later_initializer, oi, ni)
        for a, b in ((record, later_record), (record + 4, later_record + 4),
                (record + 8, later_record + 8), (output, later_output), (head, later_head)):
            assert a not in addresses or addresses[a] == b
            addresses[a] = b
        regions.extend((dict(old=record, later=later_record, size=12), dict(old=output, later=later_output, size=4)))
        registrations.append(dict(original_crt_word=crt_word, old_initializer=initializer,
            later_initializer=later_initializer, old_record=record, later_record=later_record,
            old_factory=factory, later_factory=later_factory, old_output=output, later_output=later_output))
        registrations[-1]['constructor_stored_table_words'] = tables
        print(json.dumps(registrations[-1]), flush=True)
    args.output.mkdir(parents=True, exist_ok=False)
    directory = args.output / 'instructions'
    directory.mkdir()
    routines = []
    for p, q, oi, ni in rows:
        row = dict(old_entry=p, later_entry=q, complete_body_equal=True, instructions=len(oi))
        for label, instructions in (('disc', oi), ('later', ni)):
            path = directory / f'{p:08x}-{label}.asm'
            path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions))
            row[label + '_listing'] = dict(file=path.name, sha256=sha(path))
        routines.append(row)
    assert sha(__file__) == tool_pin
    result = dict(schema=1, tool_sha256=tool_pin, parent_correspondence_sha256=sha(args.parent),
        disc_sha256=PIN, later_sha256=base.NEW_PIN, routines=routines,
        entries={f'{p:08x}': q for p, q in mapping.items()},
        address_map={f'{p:08x}': q for p, q in addresses.items()},
        input_regions=regions, registrations=registrations, guessed_bindings=False,
        all_eight_registrations_bound=len(registrations) == 8,
        live_gameplay_proof=False, scope=__doc__)
    path = args.output / 'report.json'
    path.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(sha256=sha(path), complete_bodies=len(rows))))


if __name__ == '__main__':
    main()
