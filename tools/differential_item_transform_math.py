#!/usr/bin/env python3
"""Compare native item matrix operations with unchanged original output packets."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/inventory-source/item-transform-producers'
PINS = {
    ROOT / 'tools/oracle_mp_item_transform_producers_1_1.py': '49a4c7371445aaabc4223264d61f0a4ee03a7f8bcd9adb4635b5a5043713e202',
    ROOT / 'tools/oracle_mp_item_transform_producers_audit_1_1.py': '597a7d37faa9bed09a4164b47c901c8a27a51661730c224035c4d7484058b7c3',
    ROOT / 'docs/MP_ITEM_TRANSFORM_PRODUCERS_RESEARCH.md': '8be6c033dda227e7f1922442fdc06fce4e6902dae50729888de20a4de3502548',
    BASE / 'report.json': '52935310c33d6b5df1f77db7ae5c9bc6b5971f8369cf8a7ae29cd86cc172f255',
    BASE / 'audit.json': '9e683f8ab3cfbb4c718fcd70ae8dd4ab0dcf05019aff32977d2f6cd2dafb54dc',
    BASE / 'manifest.json': '47fe2c930d84a847e682e6b4ed597df850b5087560cbc170992236896552aedb',
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def authority():
    for path, sha in PINS.items():
        require(digest(path) == sha, f'Changed immutable transform authority: {path}')
    report = json.loads((BASE / 'report.json').read_text())
    require((BASE / 'report.json').read_bytes() == (BASE / 'reexecution/report.json').read_bytes(),
            'Independent original transform report differs')
    require((BASE / 'audit.json').read_bytes() == (BASE / 'reexecution/audit.json').read_bytes(),
            'Independent original transform audit differs')
    for path, sha in report['source_pins'].items():
        require(digest(ROOT / path) == sha, f'Changed preceding transform dependency: {path}')
    for listing in report['instructions']:
        path = Path(listing['file'])
        require(digest(BASE / path) == listing['sha256'], f'Changed transform instruction bytes: {path}')
        require((BASE / path).read_bytes() == (BASE / 'reexecution' / path).read_bytes(),
                f'Independent instruction repeat differs: {path}')
    require(not report['instruction_patches'] and not report['new_guest_inputs'] and
            not report['new_supplied_service_results'], 'Changed original transform composition boundary')
    require(len(report['source_pins']) == 179 and len(report['instructions']) == 65 and
            len(report['scopes']) == 7 and len(report['states']) == 55,
            'Incomplete independently frozen transform coverage')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/item_transform_math_tests')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/item-transform-math-differential.json')
    args = parser.parse_args()
    report = authority()
    entries = {'00143960', '001441d0', '000beb30'}
    calls = [c for c in report['calls'] if c['entry'] in entries]
    require(len(calls) == 46, 'Incomplete original math call roster')

    def operand(address, size):
        values = {r['raw_hex'] for r in report['reads']
                  if int(r['address'], 16) == address and r['size'] == size}
        require(len(values) == 1, f'Missing original math operand {address:08x}')
        return next(iter(values))

    def word(raw):
        return f"{int.from_bytes(bytes.fromhex(raw), 'little'):08x}"

    one = word(operand(0x465030, 4))
    mask = operand(0x45a8c0, 8) + operand(0x45a8c8, 8)
    rows = []
    stores = aliases = reciprocals = 0
    for call in calls:
        before = report['states'][call['before_state']]['source_quaternion_lazy_constants']['raw8_hex']
        after = report['states'][call['after_state']]['source_quaternion_lazy_constants']['raw8_hex']
        scale, handedness = call['stack_words'][1:3] if call['entry'] == '000beb30' else ('00000000', '00000000')
        rows.append(['C', f"scope{call['scope']['id']}_call{call['id']}", call['entry'], one,
                     mask, before, after, scale, handedness, 'literal_source'])
        for p, q in zip(call['packets'], call['after_packets']):
            require(p['field'] == q['field'] and p['pointer'] == q['pointer'],
                    'Changed original math packet correlation/aliasing')
            rows.append(['P', p['field'], p['pointer'], p['raw_hex'] or '-', q['raw_hex'] or '-'])
        if call['entry'] == '00143960':
            points = [p for p in report['points'] if call['order'] < p['order'] < call['return_order']
                      and p['instruction'] in {'001439df', '001439e3'}]
            require(len(points) == 2 and [p['instruction'] for p in points] == ['001439df', '001439e3'],
                    'Missing original immediately-before/after RCPSS observations')
            rows.append(['R', points[0]['cpu']['xmm128'][0][-8:], points[1]['cpu']['xmm128'][1][-8:]])
            reciprocals += 1
        output_field = {'00143960': 's0_matrix34_output', '001441d0': 'stack0_matrix34_output',
                        '000beb30': 's4_matrix34_output'}[call['entry']]
        output = int(next(p['pointer'] for p in call['packets'] if p['field'] == output_field), 16)
        if call['entry'] == '001441d0' and any(int(p['pointer'], 16) == output
                for p in call['packets'] if p['field'] != output_field):
            aliases += 1
        owning = [s for s in report['stores'] if call['order'] < s['order'] < call['return_order']
                  and output <= int(s['address'], 16) and
                  int(s['address'], 16) + s['size'] <= output + 0x34]
        require(owning, 'Missing actual original output stores')
        for store in owning:
            rows.append(['W', store['instruction'], f"{int(store['address'], 16) - output:x}",
                         store['before_hex'], store['written_hex']])
        stores += len(owning)
    fixture = ROOT / 'analysis/multiplayer/integration/item-transform-math-fixtures.tsv'
    fixture.write_text('\n'.join(' '.join(map(str, row)) for row in rows) + '\n')
    run = subprocess.run([str(args.binary.resolve()), str(fixture)], text=True, capture_output=True)
    print(run.stdout, end=''); print(run.stderr, end='')
    result = dict(passed=run.returncode == 0, scope=__doc__, original_calls=len(calls),
                  node_calls=16, composition_calls=26, root_calls=4,
                  in_place_compositions=aliases, ordered_matrix_stores=stores,
                  explicit_rcpss_results=reciprocals,
                  compared='Complete pointed matrix/node/up/forward/root buffers with literal pointer aliasing, lazy globals, actual reciprocal inputs and ordered writes including overwritten/no-op bytes.',
                  limits='RCPSS retains independently observed original outputs; Xbox reciprocal/hardware parity unresolved. Original already-initialized quaternion globals and no-parent/low-byte-handedness-false roots only. Full nodes/bounds/location/registry and live frame integration remain separate.',
                  fixture_sha256=digest(fixture), native_binary_sha256=digest(args.binary),
                  tool_sha256=digest(Path(__file__)), stdout=run.stdout, stderr=run.stderr)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    raise SystemExit(run.returncode)


if __name__ == '__main__':
    main()
