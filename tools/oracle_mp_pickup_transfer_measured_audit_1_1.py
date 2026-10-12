#!/usr/bin/env python3
"""Read-only original-byte/order/state audit of the separate transfer capture."""
import argparse
import json
from pathlib import Path
import re
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

from oracle_mp_pickup_transfer_measured_1_1 import ROOT, BASE, OUT, ENTRIES, sha, require, encoded, frozen_write
from oracle_mp_inventory_equipped_1_1 import ENTRIES as EQUIPPED_ENTRIES
from oracle_mp_weapon_attachment_1_1 import ENTRIES as ATTACHMENT_ENTRIES
from xbe import Xbe

DOC = ROOT / 'docs/MP_PICKUP_TRANSFER_MEASURED_RESEARCH.md'


def u(raw, offset, size=4):
    return int.from_bytes(bytes.fromhex(raw)[offset:offset + size], 'little')


def item(state, datum):
    return next(i for i in state['items'] if i['datum'] == datum)


def main():
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--output-dir', type=Path, default=OUT)
    args = parser.parse_args(); out = args.output_dir.resolve()
    report = json.loads((out / 'report.json').read_text())
    require(sha(ROOT / 'tools/oracle_mp_pickup_transfer_measured_1_1.py') == report['tool_sha256'], 'Measurement tool differs')
    for name, pin in report['source_pins'].items():
        require(sha(ROOT / name) == pin, f'Prior freeze changed: {name}')
    baseline = BASE / 'weapon-attachment/report.json'
    for row in (report['preflight'], report['measured_baseline']):
        require(row['byte_identical'] and row['frozen_listing_writes_compared_and_suppressed'], 'Original repeat unverified')
        require((ROOT / row['path']).read_bytes() == baseline.read_bytes(), 'Whole original repeat differs')
    reproduction = json.loads((out / 'reproducibility.json').read_text())
    require(reproduction['byte_identical_report'] and reproduction['instruction_listing_hashes_match'], 'Transfer repeat unverified')
    require(sha(out / 'report.json') == reproduction['report_sha256'] == reproduction['reexecution_sha256'], 'Repeat pins differ')
    require((ROOT / reproduction['reexecution_report']).read_bytes() == (out / 'report.json').read_bytes(), 'Transfer report repeat differs')
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    rows = executed = 0
    for listing in report['instructions']:
        path = ROOT / listing['path']; require(sha(path) == listing['sha256'], 'Measured listing differs')
        lines = path.read_text().splitlines(); require(len(lines) == listing['rows'], 'Listing row count differs')
        for line in lines:
            address, raw, _ = line.split(' ', 2); raw = bytes.fromhex(raw)
            require(image.read(int(address, 16), len(raw)) == raw, f'Original instruction differs: {address}')
        rows += len(lines); executed += listing['executed_rows']
    old = json.loads(baseline.read_text()); correlated = states = stores = requests = 0
    store_sites = set(); registry_checks = 0
    for case, original in zip(report['cases'], old['cases']):
        require(case['name'] == original['name'], 'Original case order differs')
        state_map = {s['id']: s for s in case['states']}; require(len(state_map) == len(case['states']), 'Duplicate state ID')
        for s in case['states']:
            raw = {k: v for k, v in s.items() if k not in ('id', 'sha256')}
            import hashlib
            require(hashlib.sha256(encoded(raw)).hexdigest() == s['sha256'], 'Stored state checksum differs')
            unit = s['unit']; prefix = unit['body_prefix_hex']
            require(bytes.fromhex(prefix)[0x210:0x240].hex() == unit['inventory_hex'], 'Inventory prefix differs')
            require(f'{u(prefix, 0x134):08x}' == unit['controlled_unit_flags'], 'Unit admission differs')
            require(f'{u(prefix, 0xd4):08x}' == unit['network_entity'], 'Unit network entity differs')
            headers = bytes.fromhex(s['headers_hex'])
            for i in s['items']:
                datum = int(i['datum'], 16); header = bytes.fromhex(i['header_hex']); raw = i['body_prefix_hex']
                require(headers[(datum & 0xffff) * 12:(datum & 0xffff) * 12 + 12] == header, 'Header identity differs')
                require(struct.unpack_from('<H', header)[0] == datum >> 16 and header[3] == 2, 'Item salt/type differs')
                require(f'{struct.unpack_from("<I", header, 8)[0]:08x}' == i['body_address'], 'Item body identity differs')
                for field, offset in (('definition', 0), ('owner', 0x154), ('parent', 0x14), ('sibling', 0xc), ('heat_bits', 0x184)):
                    require(f'{u(raw, offset):08x}' == i[field], f'Item {field} differs')
                require(u(raw, 0x18, 1) == i['parent_node'] and u(raw, 0xaf, 1) == i['originating_row'], 'Item byte fields differ')
                require(u(raw, 0x22c, 2) == i['loaded'] and u(raw, 0x22a, 2) == i['reserve'], 'Item ammo differs')
            states += 1
        timeline = case['timeline']; order = [e['order'] for e in timeline]
        require(order == sorted(order) and len(order) == len(set(order)), 'Measured event order differs')
        calls = {c['id']: c for c in case['calls']}
        for event in timeline:
            if event['kind'] == 'store':
                ins = int(event['instruction'], 16); require(image.read(ins, 1), 'Invalid store instruction')
                store_sites.add(ins)
                require(len(bytes.fromhex(event['before_hex'])) == event['size'] == len(bytes.fromhex(event['written_hex'])), 'Store size differs')
                stores += 1
            else:
                call = calls[event['call_id']]; before = event['kind'] == 'entry'
                require(event['state'] == call['before_state' if before else 'after_state'], 'Call state correlation differs')
                require(event['order'] == call['order' if before else 'return_order'], 'Call order correlation differs')
        for source_key, entries in (('source_calls', EQUIPPED_ENTRIES), ('calls', ATTACHMENT_ENTRIES)):
            observed = [c for c in case['calls'] if int(c['entry'], 16) in entries]
            expected = [c for c in original[source_key] if int(c['entry'], 16) in ENTRIES]
            require(len(observed) == len(expected), f'Original call coverage differs: {source_key}')
            for a, b in zip(observed, expected):
                require(a['entry'] == b['entry'] and a['registers'] == b['registers']
                    and a['stack'][:4] == b['stack'][:4] and a['return_eax'] == b['return_eax'], 'Original call inputs/output differ')
                if 'output_hex' in a and 'output_hex' in b:
                    require(a['output_hex'] == b['output_hex'], 'Original admission/control output differs')
                correlated += 1
        for call in case['calls']:
            if call['entry'] == '000e7f30' and u(call.get('request_hex', ''), 0) == 20:
                require(u(call['request_hex'], 4) == 0x20003 and u(call['request_hex'], 8, 2) in (3, 5), 'Ordinary request identity/mode differs')
                requests += 1
        final = state_map[case['final_state']]; original_items = {i['datum']: i for i in original['final']['items']}
        for i in final['items']:
            reference = original_items[i['datum']]
            for key in ('owner', 'parent', 'parent_node', 'sibling', 'definition', 'loaded', 'reserve', 'heat_bits'):
                require(i[key] == reference[key], f'Original final retained field differs: {key}')
        require(final['shared_rng'] == original['shared_rng'], 'Original shared RNG differs')
        used = {e['service'] for e in case['actual_services']}
        require(not used & {'000cec00', '000cea70', '000ce510', '000cfed0', '000fb120', '000fb3f0', '000fb560',
            '000b96a0', '000c57a0', '00111490', '001cfde0'}, 'Supplied admission/transfer success encountered')
        if case['name'] in ('primary_br', 'secondary_smg'):
            require('00246a50' not in used, 'Ordinary pickup used supplied Havok')
            target = item(final, '00020003')
            require(target['owner'] == target['parent'] == '00010001' and target['parent_node'] == (20 if case['name'] == 'primary_br' else 19), 'Ordinary carry differs')
            phase = 'original_233ms_pickup_bounded_original_player_phase'
            ordinary = [c for c in case['calls'] if c['phase'] == phase]
            def call(entry): return next(c for c in ordinary if c['entry'] == entry)
            def registered(value):
                return [r['object'] for cluster in value for key in ('collidable', 'noncollidable') for r in cluster[key]]
            owner = call('000fb120'); hidden = call('000fb3f0'); carry = call('000cfed0')
            require(registered(owner['registry_before']) == registered(owner['registry_after']) == ['00020003'], 'Owner update must retain registry membership')
            require(registered(hidden['registry_before']) == ['00020003'] and registered(hidden['registry_after']) == [], 'Holster removal differs')
            registry_checks += 4
            require(call('000cea70')['return_eax'] == '00000001', 'Original free slot differs')
            require(carry['registers']['ecx'] == '00020003' and int(carry['registers']['edx'], 16) & 0xffff == 1
                and carry['registers']['eax'] == 'ffffffff' and carry['stack'][0] == '00010001', 'CFED0 ABI differs')
            before = state_map[carry['before_state']]; after = state_map[carry['after_state']]
            require(u(before['unit']['body_prefix_hex'], 0x21c) == 0xffffffff and u(after['unit']['body_prefix_hex'], 0x21c) == 0x20003, 'Original slot store differs')
            require(item(after, '00020003')['owner'] == '00010001' and item(after, '00020003')['parent'] == 'ffffffff', 'Slot storage must precede hand attachment')
            seq = call('000cfe60')
            require(u(state_map[seq['before_state']]['unit']['body_prefix_hex'], 0x214, 2) == 1
                and u(state_map[seq['after_state']]['unit']['body_prefix_hex'], 0x214, 2) == 2, 'Source sequence advance differs')
            admission = [c for c in ordinary if c['entry'] == '000cec00']
            require(len(admission) == 3 and all(int(c['return_eax'], 16) & 255 for c in admission), 'Original repeated admission differs')
            expected = '01000100' if case['name'] == 'primary_br' else '00010000'
            require(all(c['output_hex'] == expected for c in admission), 'Actual admission output flags differ')
            incoming = call('000c8140')['unit_control_input_hex']
            require(len(bytes.fromhex(incoming)) == 0x7c, 'Incomplete C8140 marshalling input')
            require(u(incoming, 6, 2) == (1 if case['name'] == 'primary_br' else 2), 'Primary/secondary incoming sequence differs')
            require(u(incoming, 8, 2) == (0xff00 if case['name'] == 'primary_br' else 0x0100), 'Original incoming selected slots differ')
    require(requests == 4 and len(report['cases']) == 5, 'Measured ordinary coverage differs')
    disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
    site_rows = []
    for address in sorted(store_sites):
        ins = next(disassembler.disasm(image.read(address, 15), address, count=1))
        site_rows.append(f'{address:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}')
    store_path = out / 'observed-store-sites.asm'
    frozen_write(store_path, ('\n'.join(site_rows) + '\n').encode())
    links = []
    for link in re.findall(r'\]\(([^)]+)\)', DOC.read_text()):
        if '://' in link: continue
        path = (DOC.parent / link.partition('#')[0]).resolve()
        require(path.exists() or path in (out / 'audit.json', out / 'manifest.json'), f'Document link missing: {link}')
        links.append(link)
    audit = dict(tool_sha256=sha(Path(__file__)), document_sha256=sha(DOC), report_sha256=sha(out / 'report.json'),
        reproduction_sha256=sha(out / 'reproducibility.json'), preserved_pins=len(report['source_pins']),
        cases=len(report['cases']), correlated_original_calls=correlated, measured_states=states, ordered_stores=stores,
        ordinary_requests=requests, byte_verified_instruction_rows=rows, executed_instruction_rows=executed,
        original_store_instruction_sites=len(site_rows), original_store_instruction_sites_sha256=sha(store_path),
        owner_holster_registry_chain_checks=registry_checks,
        document_links=len(links), scope='Read-only original bytes, whole baseline/reexecution identity, source-call input/output correlation, captured state/identity/order and documented boundaries.')
    frozen_write(out / 'audit.json', encoded(audit)); print(json.dumps(audit))


if __name__ == '__main__': main()
