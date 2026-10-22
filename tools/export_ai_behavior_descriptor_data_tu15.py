#!/usr/bin/env python3
"""Export the original AI descriptor interface without constructor results.

Use tools/run_test.py. The original131-slot directory identifies each record.
Kind0 owns its20-byte base; kinds1/2 expose callbacks through+40, and kind1
also exposes the child count/pointer at+44/+48 (original1AF9EA..1AF9F6).
The71 real CRT mask destinations are excluded and stay constructor-owned.
The cache words at+8/+C remain writable, with their literal initial -2/0.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_AC_READ, CS_AC_WRITE
from capstone.x86 import X86_OP_MEM, X86_REG_EAX
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from export_final_startup_storage_data_tu15 import regions


def behavior_sentinel_inputs(image, frame_source=None):
    # These original -1/-2/-3 operands are also read by actor startup. Extend
    # that existing literal owner only for the actual compiled frame reads.
    # In particular the last sentinel has a WORD consumer; its two trailing
    # file bytes do not belong to this declaration.
    source = frame_source or Path(__file__).resolve().parents[1] / 'src/campaign/ai/source_ai_frame_tu15_generated.cpp'
    raw = source.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == '0faad001eff14edaaa31df28eddcde059ab855a5a3639fff4b81e3526ee15308'
    code = _Code(image, '.text')
    reads, points = [], set()
    for pc in sorted({int(pc, 16) for pc in re.findall(r'services\.instruction\(0x([0-9a-f]+)u?\)', raw.decode())}):
        if not code.inside(pc):
            continue
        ins = code.at(pc)
        assert ins is not None
        for op in ins.operands:
            if op.type != X86_OP_MEM or not 0x47c1a4 <= op.mem.disp < 0x47c1b0:
                continue
            assert not op.mem.base and not op.mem.index
            assert op.access & CS_AC_READ and not op.access & CS_AC_WRITE
            address, count = op.mem.disp, op.size
            assert address + count <= 0x47c1b0
            points.update(range(address, address + count))
            reads.append(dict(pc=pc, address=address, bytes=count,
                instruction_hex=bytes(ins.bytes).hex(), instruction=ins.mnemonic + ' ' + ins.op_str))
    assert points == set(range(0x47c1a4, 0x47c1ae))
    assert image.read(0x47c1a4, 10).hex() == 'ffff0000feff0000fdff'
    return reads, regions(points)


def descriptor_inputs(image):
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    roots = struct.unpack('<131I', image.read(0x47b478, 131 * 4))
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    constructors = []
    for index in range(71):
        entry = 0x381a60 + index * 16
        assert struct.unpack('<I', image.read(0x46dad8 + index * 4, 4))[0] == entry
        ins = list(md.disasm(image.read(entry, 11), entry))
        assert len(ins) == 3 and [i.mnemonic for i in ins] == ['mov', 'mov', 'ret']
        load, store, ret = ins
        assert load.operands[0].reg == X86_REG_EAX and load.operands[1].type == X86_OP_MEM
        assert store.operands[0].type == X86_OP_MEM and store.operands[1].reg == X86_REG_EAX
        assert not load.operands[1].mem.base and not load.operands[1].mem.index
        assert not store.operands[0].mem.base and not store.operands[0].mem.index
        source = load.operands[1].mem.disp & 0xffffffff
        destination = store.operands[0].mem.disp & 0xffffffff
        assert destination - 4 in roots and image.read(destination, 4) == bytes(4)
        constructors.append(dict(entry=entry, source=source, destination=destination,
            operand_hex=image.read(source, 4).hex(), instruction_hex=b''.join(i.bytes for i in ins).hex()))
    mutable = {p for row in constructors for p in range(row['destination'], row['destination'] + 4)}
    points = set()
    records = []
    unique = sorted(set(roots) - {0})
    for ordinal, root in enumerate(unique):
        kind = struct.unpack('<H', image.read(root + 2, 2))[0]
        assert kind in (0, 1, 2)
        size = 20 if kind == 0 else 76 if kind == 1 else 68
        if ordinal + 1 < len(unique):
            assert root + size <= unique[ordinal + 1]
        raw = image.read(root, size)
        assert raw[8:16] == bytes.fromhex('feffffff00000000')
        points.update(range(root, root + size))
        mutable.update(range(root + 8, root + 16))
        records.append(dict(address=root, kind=kind, bytes=size,
            directory_indices=[i for i, p in enumerate(roots) if p == root],
            original_hex=raw.hex(), sha256=hashlib.sha256(raw).hexdigest()))
    points.difference_update(mutable)
    return records, constructors, regions(points)


def child_selection_inputs(image, records):
    # Original1AF9F1/1AF9F6 pass the literal table/count to whole1AFA90.
    # Its1AFAF6/1AFAFC indexing has a20-byte stride.1B0020 reads the
    # child's kind, timer word and seconds;1B02CD also reads its flag byte.
    # Do not turn that stride into ownership of unread padding/tail storage.
    code = _Code(image, '.text')
    expected = {0x1af9f1: '8b5048', 0x1af9f6: '668b4844',
        0x1afaf6: '8d0c89', 0x1afafc: '8d4c8d00'}
    for pc, bits in expected.items():
        assert bytes(code.at(pc).bytes).hex() == bits, (hex(pc), bytes(code.at(pc).bytes).hex())
    parents, points = [], set()
    for row in records:
        if row['kind'] != 1:
            continue
        root = row['address']
        count = struct.unpack('<h', image.read(root + 0x44, 2))[0]
        table = struct.unpack('<I', image.read(root + 0x48, 4))[0]
        assert 0 <= count <= 131
        children = []
        for index in range(count):
            address = table + index * 20
            data = image.read(address, 20)
            kind = struct.unpack('<h', data[:2])[0]
            assert 0 <= kind < 131
            # Existing startup owns the first DWORD. Preserve its exact bytes
            # and lifetime while completing the selector's two other fields.
            points.update(range(address, address + 6))
            points.update(range(address + 8, address + 12))
            children.append(dict(address=address, kind=kind, original_hex=data.hex()))
        parents.append(dict(address=root, count=count, table=table, children=children))
    assert len(parents) == 16 and sum(row['count'] for row in parents) == 105
    return parents, regions(points)


def property_selection_inputs(image):
    # Whole1AF970 consumes20 eight-byte style entries,12-byte property
    # records and24-byte commands.1AFA90 publishes command+4 as a child.
    # These are authored inputs; the mutable pointer directory is not copied.
    code = _Code(image, '.text')
    assert code.at(0x1af984).op_str == 'edx, 0x47d590'
    assert code.at(0x1afa6c).op_str == 'edx, 8'
    assert code.at(0x1af9ca).op_str == 'eax, 0xc'
    styles, points = [], set()
    for index in range(20):
        address = 0x47d590 + index*8
        pointer, count = struct.unpack('<Ih', image.read(address, 6))
        assert 0 <= count <= 131
        entries = []
        for lane in range(count):
            entry = pointer + lane*12
            kind, commands, command_count = struct.unpack('<H2xIh', image.read(entry, 10))
            assert 0 <= kind < 131 and 0 <= command_count <= 50
            rows = []
            for ordinal in range(command_count):
                command = commands + ordinal*24
                raw = image.read(command, 24)
                operation, parameter, child = struct.unpack('<hhh', raw[:6])
                assert 0 <= operation <= 4 and -1 <= child < 131
                at = command + 4
                points.update(range(at, at+6)); points.update(range(at+8, at+12))
                rows.append(dict(address=command, operation=operation, parameter=parameter,
                    child_address=at, child_kind=child, original_hex=raw.hex()))
            entries.append(dict(address=entry, parent_kind=kind, commands=rows))
        styles.append(dict(style=index, address=address, entries=entries))
    assert sum(len(row['entries']) for row in styles) == 14
    assert sum(len(entry['commands']) for row in styles for entry in row['entries']) == 15
    return styles, regions(points)


def descriptor_callback_tail_inputs(image, records):
    # Kind2's interface continues beyond its first68 bytes. These two
    # distinct guarded callers consume actual callback identities at+44/+48;
    # the following record or alignment at+4C is not part of this declaration.
    code = _Code(image, '.text')
    expected = {0x1b0ef5:'6683790202',0x1b0efd:'8b4944',0x1b0f06:'ffd1',
        0x1b0f45:'6683790202',0x1b0f4d:'8b4948',0x1b0f56:'ffd1'}
    for pc, bits in expected.items():
        assert bytes(code.at(pc).bytes).hex()==bits
    rows=[]
    for row in records:
        if row['kind']!=2:continue
        address=row['address']+0x44
        raw=image.read(address,8)
        callbacks=struct.unpack('<2I',raw)
        assert all(p==0 or code.inside(p) for p in callbacks)
        rows.append(dict(descriptor=row['address'],address=address,bytes=8,
            directory_indices=row['directory_indices'],callbacks=callbacks,data_hex=raw.hex()))
    assert len(rows)==55
    return rows,regions({p for row in rows for p in range(row['address'],row['address']+8)})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--fixture', type=Path)
    parser.add_argument('--child-output', type=Path)
    parser.add_argument('--property-output', type=Path)
    parser.add_argument('--callback-output', type=Path)
    parser.add_argument('--frame-source', type=Path,
        help='Pinned compiled-frame reference when the current frame has independent changes')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.output.resolve().is_relative_to(scratch)
    image = Xbe(NEW_DEFAULT)
    records, constructors, ranges = descriptor_inputs(image)
    sentinel_reads, sentinel_ranges = behavior_sentinel_inputs(image, args.frame_source)
    children, child_ranges = child_selection_inputs(image, records)
    properties, property_ranges = property_selection_inputs(image)
    callback_rows, callback_ranges = descriptor_callback_tail_inputs(image,records)
    lines = ['// Authenticated TU1.5 AI descriptor interfaces; mutable CRT masks are excluded.']
    for address, count in sentinel_ranges + ranges:
        raw = image.read(address, count)
        lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{count}>{{' +
            ','.join(f'0x{value:02x}' for value in raw) + '});')
    for row in records:
        address = row['address'] + 8
        raw = image.read(address, 8)
        lines.append(f'descriptor_cache(memory,0x{address:x}u,std::array<std::uint8_t,8>{{' +
            ','.join(f'0x{value:02x}' for value in raw) + '});')
    args.output.write_text('\n'.join(lines) + '\n')
    if args.child_output:
        assert args.child_output.resolve().is_relative_to(scratch)
        child_lines = ['// Original TU1.5 behavior-child selection inputs; unread row padding/tails stay unowned.']
        # The old startup path exercised only command0. Whole1AFA90 has
        # five guarded command variants; publish its complete original switch.
        child_switch = struct.unpack('<5I', image.read(0x1afb48, 20))
        assert set(child_switch) == {0x1afb31, 0x1afae2, 0x1afb16, 0x1afb2a, 0x1afae8}
        for address, count in [(0x1afb48, 20)] + child_ranges:
            raw = image.read(address, count)
            child_lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{count}>{{' +
                ','.join(f'0x{value:02x}' for value in raw) + '});')
        args.child_output.write_text('\n'.join(child_lines) + '\n')
    if args.property_output:
        assert args.property_output.resolve().is_relative_to(scratch)
        property_lines = ['// Original TU1.5 property-command child selection inputs; no constructed directory values.']
        for address, count in property_ranges:
            raw = image.read(address, count)
            property_lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{count}>{{' +
                ','.join(f'0x{value:02x}' for value in raw) + '});')
        args.property_output.write_text('\n'.join(property_lines) + '\n')
    if args.callback_output:
        assert args.callback_output.resolve().is_relative_to(scratch)
        callback_lines=['// Original kind2 behavior callback identities at+44/+48; following records/padding excluded.']
        for address,count in callback_ranges:
            raw=image.read(address,count)
            callback_lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{count}>{{'+
                ','.join(f'0x{value:02x}' for value in raw)+'});')
        args.callback_output.write_text('\n'.join(callback_lines)+'\n')
    report = dict(original_xbe_sha256=NEW_PIN, records=records, mask_constructors=constructors,
        immutable_ranges=ranges, immutable_bytes=sum(n for _, n in ranges),
        behavior_sentinel_reads=sentinel_reads, behavior_sentinel_ranges=sentinel_ranges,
        behavior_sentinel_frame_sha256='0faad001eff14edaaa31df28eddcde059ab855a5a3639fff4b81e3526ee15308',
        behavior_child_parents=children, behavior_child_selection_ranges=child_ranges,
        behavior_child_selection_bytes=sum(n for _, n in child_ranges),
        behavior_child_constructor_switch_hex=image.read(0x1afb48, 20).hex(),
        behavior_child_data_sha256=hashlib.sha256(args.child_output.read_bytes()).hexdigest() if args.child_output else None,
        behavior_property_styles=properties, behavior_property_child_ranges=property_ranges,
        behavior_property_child_bytes=sum(n for _, n in property_ranges),
        behavior_property_child_sha256=hashlib.sha256(args.property_output.read_bytes()).hexdigest() if args.property_output else None,
        descriptor_callback_rows=callback_rows,descriptor_callback_bytes=sum(n for _,n in callback_ranges),
        descriptor_callback_data_sha256=hashlib.sha256(args.callback_output.read_bytes()).hexdigest() if args.callback_output else None,
        mutable_descriptor_caches=[dict(address=r['address'] + 8, bytes=8,
            initial_hex='feffffff00000000') for r in records],
        mask_results_imported=False, supplied_game_results=False,
        source_instruction_changes=False, data_inc_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        scratch=str(scratch), large_artifacts_retained=[])
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text(json.dumps(report, indent=2) + '\n')
    if args.fixture:
        assert args.fixture.resolve().is_relative_to(scratch)
        fixture = {key: report[key] for key in ('original_xbe_sha256', 'records',
            'mask_constructors', 'mutable_descriptor_caches')}
        fixture['boundary'] = 'Original metadata/constructor inputs, not completed runtime masks or MCC gameplay results'
        args.fixture.write_text(json.dumps(fixture, indent=2) + '\n')
    print(json.dumps(dict(records=len(records), immutable_bytes=report['immutable_bytes'],
        excluded_real_constructor_masks=len(constructors))), flush=True)


if __name__ == '__main__':
    main()
