#!/usr/bin/env python3
"""Pair complete DSOUND owners and their literal uses in three owned images.

Prefix signatures only find candidates. Complete instruction/branch topology,
encoded bytes, direct targets and changed image-address operands are retained.
This is static correspondence, not execution of either title update.
"""
import argparse
import csv
from pathlib import Path

from oracle_mp_title_update_display import ROOT, OLD, NEW, PINS, Xbe, _Code, _trace, sha, save, encoded
from research_mp_inventory_switch_u5 import prefix, raw_prefix_candidates, norm

BASE = ROOT / 'assets/disc/default.xbe'
INVENTORIES = [ROOT / 'analysis/retail/functions.tsv', ROOT / 'analysis/map-pack-update/functions.tsv',
               ROOT / 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv']
# Original helper plus its actual conversion/normalization callees, wrapper,
# algorithm selector, retained calculation and device application owners.
ENTRIES = (0x379d4b, 0x37debf, 0x380168, 0x37def9, 0x37dfe1, 0x37db3e,
           0x37dbc4, 0x37dc5a, 0x37dca5, 0x37ddb6, 0x37de46, 0x37e112,
           0x37e098, 0x3812e0, 0x3808c2, 0x37e126, 0x37ae57, 0x37ca0a,
           0x381e2a, 0x382032, 0x37db87, 0x37dae0, 0x379da1, 0x379cc9, 0x379cee,
           0x379dca, 0x379e01, 0x379e21, 0x379e55, 0x379e77, 0x37db07,
           0x379d54, 0x3801df, 0x38368b, 0x3834f3, 0x383167,
           0x381ce4, 0x381d7c, 0x381710, 0x380b97, 0x380deb, 0x379c90, 0x379cb2,
           0x379e9e, 0x37a669, 0x37a616, 0x37c031, 0x37c0e9,
           0x37c3bf, 0x37c4d2, 0x37b1e6, 0x37c668, 0x37c6e5, 0x37b68b,
           0x379d0c, 0x37a14f, 0x37b16f, 0x37a110, 0x37a132, 0x37be27,
           0x37c1d7, 0x381e93, 0x382b22, 0x37be6c, 0x37cc6a, 0x37cf56, 0x37e63c,
           0x37cdca, 0x37cef1, 0x37c825, 0x37bc89, 0x37c8ab, 0x37da87,
           0x37da3e, 0x37da17, 0x37d9ee, 0x379f7a, 0x37a500, 0x37c1b3, 0x379d9d,
           0x381aa9, 0x37e737, 0x381a92, 0x381a74, 0x3808e4, 0x380ae6, 0x3808be,
           0x382f58, 0x380900, 0x382fe3, 0x38094b, 0x382cd4,
           0x38025a, 0x38029c, 0x3806be, 0x380327, 0x3801fd, 0x379c60, 0x380419,
           0x38085d, 0x38074d, 0x3803f8, 0x37e612, 0x380486,
           0x382b7e, 0x382be3, 0x3824ee, 0x382560, 0x3824ea, 0x37f43e,
           0x3811a6, 0x381e0e, 0x382b08, 0x382ad9, 0x37f068, 0x37f245,
           0x37f11a, 0x37eb07, 0x37eae2, 0x37f407,
           0x384a36, 0x380ee1, 0x3810bd, 0x3829f7,
           0x381ef8, 0x3811fa, 0x381122, 0x37ed0f, 0x37ed36)


def body(code, entry, starts):
    traced = _trace(code, entry, starts)
    assert not traced.tables, (hex(entry), traced.tables)
    rows = [code.at(pc) for pc in sorted(traced.code)]
    assert rows and rows[-1].mnemonic.startswith(('ret', 'jmp'))
    return rows


def tokens(rows, image):
    positions = {i.address: index for index, i in enumerate(rows)}
    return [norm(i, image, positions) for i in rows]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    pins = {BASE: '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d', **PINS}
    for path, pin in pins.items():
        assert sha(path) == pin
    images = [Xbe(path) for path in (BASE, OLD, NEW)]
    codes = [_Code(image, 'DSOUND') for image in images]
    starts = [{int(row['address'], 16) for row in csv.DictReader(path.open(), delimiter='\t')
               if code.inside(int(row['address'], 16))} for path, code in zip(INVENTORIES, codes)]
    buckets = [{}, {}, {}]
    for n in (1, 2):
        for start in sorted(starts[n]):
            buckets[n].setdefault(tuple(prefix(codes[n], images[n], start)), []).append(start)
    paired = []
    for entry in ENTRIES:
        before = body(codes[0], entry, starts[0])
        expected = tokens(before, images[0])
        row = dict(base_entry=f'{entry:08x}', instructions=len(before), variants=[])
        for n, label in enumerate(('base', 'map_pack_11', 'title_update_15')):
            if n == 0:
                target, after = entry, before
            else:
                candidates = set(buckets[n].get(tuple(prefix(codes[0], images[0], entry)), []))
                candidates.update(raw_prefix_candidates(codes[0], images[0], codes[n], entry))
                matches = []
                for candidate in sorted(candidates):
                    try:
                        after = body(codes[n], candidate, starts[n])
                        if tokens(after, images[n]) == expected:
                            matches.append((candidate, after))
                    except (AssertionError, AttributeError, ValueError):
                        continue
                if len(matches) > 1:
                    # Identical SDK helpers/wrappers are emitted more than
                    # once. Require a separately paired complete caller or
                    # the exact already-paired callees at corresponding sites.
                    reached = {int(call['target'], 16) for previous in paired
                               for call in previous['variants'][n]['calls']
                               if call['base_target'] == f'{entry:08x}'}
                    if reached:
                        matches = [match for match in matches if match[0] in reached]
                    else:
                        known = {int(previous['base_entry'], 16):
                                 int(previous['variants'][n]['entry'], 16) for previous in paired}
                        sites = [(index, known[i.operands[0].imm])
                                 for index, i in enumerate(before)
                                 if i.mnemonic == 'call' and i.operands[0].type == 2
                                 and i.operands[0].imm in known]
                        assert sites, (hex(entry), label, 'No complete caller/callee identity evidence')
                        matches = [match for match in matches if all(
                            match[1][index].operands[0].imm == target for index, target in sites)]
                assert len(matches) == 1, (hex(entry), label, [hex(v[0]) for v in matches])
                target, after = matches[0]
            calls, operands = [], []
            for a, b in zip(before, after):
                assert images[n].read(b.address, b.size) == b.bytes
                if a.mnemonic == 'call' and a.operands[0].type == 2:
                    calls.append(dict(base_target=f'{a.operands[0].imm:08x}', target=f'{b.operands[0].imm:08x}'))
                for index, (left, right) in enumerate(zip(a.operands, b.operands)):
                    if left.type == right.type == 2 and not (a.mnemonic.startswith('j') or a.mnemonic == 'call'):
                        x, y = left.imm, right.imm
                        if x != y or any(s.va <= x < s.va + s.vsize for s in images[0].sections):
                            assert all(any(s.va <= value < s.va + s.vsize for s in image.sections)
                                       for value, image in ((x, images[0]), (y, images[n])))
                            operands.append(dict(kind='changed_image_immediate' if x != y else 'equal_image_immediate', base_site=f'{a.address:08x}',
                                site=f'{b.address:08x}', operand=index, base_address=f'{x:08x}', address=f'{y:08x}'))
                    if left.type == right.type == 3 and left.mem.disp and right.mem.disp:
                        x, y = left.mem.disp, right.mem.disp
                        if any(s.va <= x < s.va + s.vsize for s in images[0].sections):
                            assert left.size == right.size
                            # LEA kernel bases and indexed tables name data; a
                            # read width alone does not establish table extent.
                            operands.append(dict(kind='image_displacement', base_site=f'{a.address:08x}',
                                site=f'{b.address:08x}', operand=index, base_address=f'{x:08x}', address=f'{y:08x}',
                                width=left.size, base_hex=images[0].read(x, left.size).hex(),
                                hex=images[n].read(y, right.size).hex()))
                        elif x != y:
                            raise AssertionError((hex(a.address), a.op_str, b.op_str))
            listing = args.output / 'instructions' / f'{entry:08x}-{label}.asm'
            save(listing, ''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in after).encode())
            row['variants'].append(dict(label=label, entry=f'{target:08x}', complete_topology_equal=True,
                calls=calls, image_operands=operands, listing_sha256=sha(listing)))
        paired.append(row)
        print(f'{entry:08x}: ' + ', '.join(v['entry'] for v in row['variants']), flush=True)
    # The curve-index conversion is in .text. Its exact target is established
    # by each separately paired complete SDK distance caller, not by a section
    # displacement or an identical compiler-helper signature elsewhere.
    caller = next(row for row in paired if row['base_entry'] == '0037dca5')
    targets = [next(int(call['target'], 16) for call in variant['calls']
                    if call['base_target'] == '00322b0c') for variant in caller['variants']]
    external_codes = [_Code(image, '.text') for image in images]
    external_starts = [{int(row['address'], 16) for row in csv.DictReader(path.open(), delimiter='\t')
                        if code.inside(int(row['address'], 16))}
                       for path, code in zip(INVENTORIES, external_codes)]
    before = body(external_codes[0], targets[0], external_starts[0])
    expected = tokens(before, images[0])
    row = dict(base_entry='00322b0c', instructions=len(before),
               pairing_via_complete_caller='0037dca5', variants=[])
    for n, label in enumerate(('base', 'map_pack_11', 'title_update_15')):
        after = body(external_codes[n], targets[n], external_starts[n])
        assert tokens(after, images[n]) == expected
        assert all(i.mnemonic != 'call' for i in after)
        assert all(images[n].read(i.address, i.size) == i.bytes for i in after)
        listing = args.output / 'instructions' / f'00322b0c-{label}.asm'
        save(listing, ''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in after).encode())
        row['variants'].append(dict(label=label, entry=f'{targets[n]:08x}', complete_topology_equal=True,
                                    calls=[], image_operands=[], listing_sha256=sha(listing)))
    paired.append(row)
    print('00322b0c: ' + ', '.join(v['entry'] for v in row['variants']), flush=True)
    mappings = [{row['base_entry']: row['variants'][n]['entry'] for row in paired} for n in range(3)]
    for row in paired:
        for n, variant in enumerate(row['variants']):
            for call in variant['calls']:
                call['complete_callee_paired_here'] = call['base_target'] in mappings[n]
                if call['complete_callee_paired_here']:
                    assert call['target'] == mappings[n][call['base_target']]
            for operand in variant['image_operands']:
                if operand['kind'] != 'changed_image_immediate':
                    continue
                x, y = int(operand['base_address'], 16), int(operand['address'], 16)
                if operand['base_address'] in mappings[n]:
                    assert mappings[n][operand['base_address']] == operand['address']
                    operand['paired_function_pointer'] = True
                elif x == 0x417240 and row['base_entry'] == '0038025a':
                    # The constructor publishes this table identity. Compare
                    # five adjacent executable-pointer words as static evidence;
                    # their membership and the table length are not established.
                    # Importing an identity implements no Release/platform owner.
                    methods=[]
                    for slot in range(5):
                        base_target=int.from_bytes(images[0].read(x+slot*4,4),'little')
                        target=int.from_bytes(images[n].read(y+slot*4,4),'little')
                        before=body(codes[0],base_target,starts[0]);after=body(codes[n],target,starts[n])
                        assert tokens(before,images[0]) == tokens(after,images[n])
                        if f'{base_target:08x}' in mappings[n]:assert mappings[n][f'{base_target:08x}']==f'{target:08x}'
                        methods.append(dict(slot=slot,base_entry=f'{base_target:08x}',entry=f'{target:08x}',complete_topology_equal=True,instructions=len(before)))
                    operand['sample_page_identity_adjacent_function_words']=methods
                elif (x in (0x386af0, 0x386af4) and row['base_entry'] == '0037e63c') or (x == 0x386b18 and row['base_entry'] in (
                        '00379e9e', '0037c3bf', '0037c4d2', '0037b1e6', '0037a14f', '0037cf56')):
                    # The complete paired access/release callers pass this
                    # mutable synchronization object. Its image bytes are not
                    # an immutable initialized lock or a default lock result.
                    operand['mutable_state_lock_identity'] = True
                    operand['base_initial12_hex'] = images[0].read(x, 12).hex()
                    operand['initial12_hex'] = images[n].read(y, 12).hex()
                elif x in (0x417120, 0x4170e4, 0x417140, 0x417150, 0x417254,
                           0x41727c, 0x41718c, 0x417200):
                    # Constructors publish an original vtable identity. Its
                    # reached reference slot is checked against complete paired
                    # method owners; relocated pointer bytes are not immutable
                    # equal defaults. Other vtable slots remain outside scope.
                    base_method = int.from_bytes(images[0].read(x+4, 4), 'little')
                    method = int.from_bytes(images[n].read(y+4, 4), 'little')
                    assert base_method in (0x379d0c, 0x37a14f)
                    assert mappings[n][f'{base_method:08x}'] == f'{method:08x}'
                    operand['paired_reference_vtable'] = True
                    operand['base_reference_method'] = f'{base_method:08x}'
                    operand['reference_method'] = f'{method:08x}'
                    if x == 0x41727c:
                        # The derived constructor's exact table identity and
                        # separately paired complete notification wrappers
                        # establish only these two reached method slots.
                        methods=[]
                        for slot,base_target in ((0x18,0x3829f7),(0x1c,0x382ad9),(0x20,0x382b08)):
                            target=int.from_bytes(images[n].read(y+slot,4),'little')
                            assert int.from_bytes(images[0].read(x+slot,4),'little')==base_target
                            assert mappings[n][f'{base_target:08x}']==f'{target:08x}'
                            methods.append(dict(slot=slot,base_entry=f'{base_target:08x}',entry=f'{target:08x}'))
                        operand['paired_notification_methods']=methods
                elif x == 0x4171fc and row['base_entry'] == '0037e63c':
                    # Separate embedded-interface table, not the object's
                    # reference-count table at417200. Pair its complete first
                    # method through the two encoded table identities.
                    base_target = int.from_bytes(images[0].read(x, 4), 'little')
                    target = int.from_bytes(images[n].read(y, 4), 'little')
                    before = body(external_codes[0], base_target, external_starts[0])
                    after = body(external_codes[n], target, external_starts[n])
                    assert tokens(before, images[0]) == tokens(after, images[n])
                    operand['embedded_vtable_first_method'] = dict(
                        base_entry=f'{base_target:08x}', entry=f'{target:08x}',
                        complete_topology_equal=True, instructions=len(before))
                elif x in (0x38589c, 0x38577c) and row['base_entry'] == '0037bc89':
                    # Headers contain relocated record pointers. Compare the
                    # exact counts and all reached records, not pointer bytes.
                    count = int.from_bytes(images[0].read(x, 4), 'little')
                    assert count == int.from_bytes(images[n].read(y, 4), 'little')
                    a = int.from_bytes(images[0].read(x+4, 4), 'little')
                    b = int.from_bytes(images[n].read(y+4, 4), 'little')
                    assert images[0].read(a, count*8) == images[n].read(b, count*8) if count else a == b == 0
                    operand['paired_bin_header'] = dict(count=count,
                        base_records=f'{a:08x}', records=f'{b:08x}', complete_records_equal=True)
                else:
                    assert images[0].read(x, 12) == images[n].read(y, 12), operand
                    operand['equal_first12_bytes_hex'] = images[0].read(x, 12).hex()
    ranges = []
    for address, size in [(0x45dbd8, 4), (0x45dbc0, 4), (0x45dbbc, 4), (0x417238, 4),
                          (0x4171f8, 4), (0x4171e8, 4), (0x4171ec, 4), (0x45dfcc, 4),
                          (0x45e320, 4), (0x45dccc, 4), (0x385a48, 12), (0x385804, 12),
                          (0x385a58, 61 * 64), (0x385a78, 32), (0x3869c0, 32), (0x45dc10, 4),
                          (0x45dbb8, 4), (0x4171e4, 4), (0x45dde8, 4), (0x385a44, 4),
                          (0x4171f4, 4), (0x4171f0, 4), (0x4122dc, 4), (0x4172b4, 4),
                          (0x4123e8, 4), (0x3869e4, 4), (0x45e4f0, 4), (0x45dcc0, 8),
                          (0x3857ec, 60), (0x3857b8, 48), (0x385854, 72), (0x385828, 36),
                          (0x385798, 16), (0x3857a8, 16), (0x385748, 18), (0x45dde0, 4),
                          (0x38699c, 36)]:
        raw = images[0].read(address, size)
        variants = []
        for n, label in enumerate(('base', 'map_pack_11', 'title_update_15')):
            uses = [op for row in paired for op in row['variants'][n]['image_operands']
                    if op['base_address'] == f'{address:08x}']
            addresses = {int(op['address'], 16) for op in uses}
            assert len(addresses) == 1, (hex(address), label, addresses)
            target = next(iter(addresses))
            assert raw == images[n].read(target, size), (hex(address), label)
            variants.append(dict(label=label, address=f'{target:08x}', complete_bytes_equal=True,
                                 encoded_consumer_sites=[op['site'] for op in uses]))
        payload = args.output / 'immutable' / f'{address:08x}.bin'
        save(payload, raw)
        ranges.append(dict(base_address=f'{address:08x}', bytes=size, sha256=sha(payload), variants=variants))
    bin_data=[]
    for n,label in enumerate(('base','map_pack_11','title_update_15')):
        def address(base):
            values={int(op['address'],16) for row in paired for op in row['variants'][n]['image_operands']
                    if int(op['base_address'],16)==base}
            assert len(values)==1,(hex(base),label,values)
            return next(iter(values))
        table=address(0x38576c);headers=[address(0x38589c),address(0x38577c)]
        headers.extend(int.from_bytes(images[n].read(table+i*4,4),'little')for i in range(4))
        regions=[dict(address=f'{table:08x}',hex=images[n].read(table,16).hex())]
        shapes=[]
        for header in dict.fromkeys(headers):
            raw=images[n].read(header,8);count=int.from_bytes(raw[:4],'little');records=int.from_bytes(raw[4:],'little')
            regions.append(dict(address=f'{header:08x}',hex=raw.hex()))
            payload=images[n].read(records,count*8) if count else b''
            if count:regions.append(dict(address=f'{records:08x}',hex=payload.hex()))
            shapes.append((count,payload.hex()))
        if n==0:base_shapes=shapes
        else:assert shapes==base_shapes
        bin_data.append(dict(label=label,table_address=f'{table:08x}',regions=regions,complete_record_shapes_equal=True))
    save(args.output / 'report.json', encoded(dict(boundary=__doc__, tool_sha256=sha(__file__),
        source_pins={str(p): pin for p, pin in pins.items()},
        inventory_pins={str(p): sha(p) for p in INVENTORIES}, owners=paired, immutable_ranges=ranges,
        voice_initialization_bins=bin_data,
        instruction_patches=[], supplied_callee_returns=[], title_update_execution_claimed=False)))


if __name__ == '__main__':
    main()
