#!/usr/bin/env python3
"""Compare native CFED0/CFE60 orchestration with immutable original observations.

FB120/FB3F0/CF8C0 are explicit literal-output leaf boundaries in this check,
not implemented native ownership, registry removal or physical drop services.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from differential_unit_input import verify_authority

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/inventory-source'
OPTIONS = BASE / 'pickup-transfer-options'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def verify_options():
    pins = {
        OPTIONS / 'manifest.json': 'fa3c56dd60ed6f6ab591a2c68224534c3809b06c0b678de2dd69406340a652a6',
        ROOT / 'docs/MP_PICKUP_TRANSFER_OPTIONS_RESEARCH.md': '9a339d7082e7d4438309d8cd1e982cf800a1c22f5f9c85a8ba9ac8be05de4d2b',
        ROOT / 'tools/differential_unit_input.py': 'cbc20da5c5d67e3b71477524a8face6828b462b6c504da8cd895b2d77a58f94d',
        OPTIONS / 'report.json': 'db8b94107fe570e29e0a75d0fc62c91f667fce565a116efd2d9216a9ad94f411',
        OPTIONS / 'audit.json': '87183d476543d7e2dcf7798aad83b9484bca9ecd19375b4517d2b4c9e7254b52',
        ROOT / 'tools/oracle_mp_pickup_transfer_options_1_1.py': '48deb72187702c3660fd3d018cabbe22d2647c6fb9fa94118213d3437904022d',
        ROOT / 'tools/oracle_mp_pickup_transfer_options_audit_1_1.py': '361b8697415c9894a030d3d5df8e9be0c855c2bd45f390e749a9ddcb20ba6ce6',
    }
    for path, sha in pins.items():
        require(digest(path) == sha, f'Changed original authority: {path}')
    verify_authority()
    manifest = json.loads((OPTIONS / 'manifest.json').read_text())
    for record in manifest['payloads']:
        require(digest(OPTIONS / record['path']) == record['sha256'],
                f"Changed original options payload: {record['path']}")
    for name, sha in manifest['tools'].items():
        require(digest(ROOT / name) == sha, f'Changed original options tool: {name}')
    for name, sha in manifest['source_pins'].items():
        require(digest(ROOT / name) == sha, f'Changed original options manifest prerequisite: {name}')
    for name in ('report.json', 'audit.json'):
        require((OPTIONS / name).read_bytes() == (OPTIONS / 'reexecution' / name).read_bytes(),
                f'Original options repeat differs: {name}')
    options = json.loads((OPTIONS / 'report.json').read_text())
    for name, sha in options['source_pins'].items():
        require(digest(ROOT / name) == sha, f'Changed original options prerequisite: {name}')
    return options


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/unit_inventory_tests')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/unit-inventory-differential.json')
    args = parser.parse_args()
    options = verify_options()
    original = json.loads((BASE / 'pickup-transfer-measured/report.json').read_text())
    rows = []
    for case, recorded in zip(original['cases'], options['cases']):
        require(case['name'] == recorded['name'], 'Original case order differs')
        observed = {c['frozen_transfer_call_id']: c for c in recorded['calls'] if 'frozen_transfer_call_id' in c}
        sites = {s['cfe60_call_id']: s for s in recorded['sites'] if s['instruction'] == '000cfe66'}
        def prefix(call, phase):
            return case['states'][call[phase + '_state']]['unit']['body_prefix_hex']
        for call in case['calls']:
            if call['entry'] not in ('000cfed0', '000cfe60'):
                continue
            if call['entry'] == '000cfed0':
                sequence = [c for c in case['calls'] if c['entry'] == '000cfe60' and
                            call['order'] < c['order'] < call['return_order']]
                require(len(sequence) == 1, 'Missing owning sequence consumer')
                sequence = sequence[0]
                regs = call['registers']
                arguments = [call['stack'][0], regs['ecx'], f"{int(regs['edx'],16) & 65535:04x}", regs['eax']]
                kind = 'T'
            else:
                sequence = call
                arguments = [call['registers']['eax'], 'ffffffff', 'ffff', 'ffffffff']
                kind = 'S'
            captured = observed[sequence['id']]
            site = sites[captured['id']]
            mode = site['consumed_options_byte_0c']
            require(bytes.fromhex(site['options_prefix_hex'])[12] == int(mode, 16), 'Consumed original byte differs')
            require(site['options_global_pointer'] == site['consumed_options_pointer'] == site['registers']['ecx'],
                    'Original options pointer correlation differs')
            for c in (call, sequence):
                require(observed[c['id']]['unit_before_hex'] == prefix(c, 'before') and
                        observed[c['id']]['unit_after_hex'] == prefix(c, 'after'),
                        'Independent full-unit call correlation differs')
            rows.append([kind, case['id'], call['id'], *arguments, mode,
                         prefix(call, 'before'), prefix(call, 'after'), prefix(sequence, 'before'), 'literal'])
            if kind == 'T':
                for leaf in case['calls']:
                    if not call['order'] < leaf['order'] < call['return_order']:
                        continue
                    if leaf['entry'] == '000fb120' and leaf['caller'] == '000cff05':
                        leaf_args = ['A', leaf['registers']['eax'], leaf['stack'][0], 'ffffffff']
                    elif leaf['entry'] == '000fb3f0' and leaf['caller'] == '000cff0c':
                        leaf_args = ['R', leaf['registers']['eax'], 'ffffffff', 'ffffffff']
                    elif leaf['entry'] == '000cf8c0' and leaf['caller'] == '000cff46':
                        r = leaf['registers']; leaf_args = ['D', r['ebx'], r['esi'], r['eax']]
                    else:
                        continue
                    rows.append(['K', *leaf_args, prefix(leaf, 'before'), prefix(leaf, 'after')])
            for store in case['timeline']:
                if store['kind'] != 'store' or not call['order'] < store['order'] < call['return_order']:
                    continue
                if store['instruction'] not in ('000cff0f', '000cff16', '000cff30', '000cfe9b', '000cfea2'):
                    continue
                require(store['target']['datum'] == arguments[0], 'Owning store unit differs')
                rows.append(['W', store['instruction'], store['target']['offset'], f"{store['size']:x}",
                             f"{int.from_bytes(bytes.fromhex(store['before_hex']), 'little'):08x}",
                             f"{int.from_bytes(bytes.fromhex(store['written_hex']), 'little'):08x}"])
    fixture = ROOT / 'analysis/multiplayer/integration/unit-inventory-fixtures.tsv'
    fixture.write_text('\n'.join(' '.join(map(str, row)) for row in rows) + '\n')
    run = subprocess.run([str(args.binary.resolve()), str(fixture)], text=True, capture_output=True)
    print(run.stdout, end=''); print(run.stderr, end='')
    report = dict(passed=run.returncode == 0, scope=__doc__, original_cases=5,
                  slot_orchestrations=10, sequence_consumers=10, supplied_original_leaf_boundaries=19,
                  native_ordered_stores=59, complete_unit_prefixes_compared=True,
                  live_pickup_transfer_integration=False, executed_network_dirty_callbacks=0,
                  mode4_or_full_inventory_coverage=False,
                  limits='Actual consumed options byte 01 and network entity NONE. FB120/FB3F0/CF8C0 outputs are explicit original leaf boundaries, not native owner/removal/drop integration. Original live world, controls, admission, contacts, full startup and non-NONE network-dirty producers remain open.',
                  original_options_report_sha256=digest(OPTIONS / 'report.json'),
                  original_transfer_report_sha256=digest(BASE / 'pickup-transfer-measured/report.json'),
                  fixture_sha256=digest(fixture), native_binary_sha256=digest(args.binary),
                  tool_sha256=digest(Path(__file__)), stdout=run.stdout, stderr=run.stderr)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    raise SystemExit(run.returncode)


if __name__ == '__main__':
    main()
