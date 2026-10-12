#!/usr/bin/env python3
"""Compare native item current-node and bounds producers with original full storage."""
import argparse
import json
from pathlib import Path
import subprocess
from differential_item_transform_math import ROOT, BASE, authority, digest, require


def raw_fields(value, path=()):
    if isinstance(value, dict):
        for key, child in sorted(value.items()):
            if key == 'objects':
                continue  # Bodies/nodes are full separate owned banks; header pool is full.
            if key.endswith('_hex') and isinstance(child, str) and child:
                yield '/'.join(path + (key,)), child
            else:
                yield from raw_fields(child, path + (key,))
    elif isinstance(value, list):
        for i, child in enumerate(value):
            yield from raw_fields(child, path + (str(i),))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/item_node_bounds_tests')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/item-node-bounds-differential.json')
    args = parser.parse_args()
    report = authority()
    calls = [c for c in report['calls'] if c['entry'] in {'000bdf80', '000bede0'}]
    require(len(calls) == 8, 'Incomplete original node/bounds call roster')
    selected = sorted({c[k] for c in calls for k in ('before_state', 'after_state')})
    rows = []
    for sid in selected:
        state = report['states'][sid]
        for obj in state['current']['current']['objects']:
            duplicate = next(o for o in state['current']['world']['objects'] if o['datum'] == obj['datum'])
            require(obj['body_hex'] == duplicate['raw_hex'], 'Original full body/node alias differs')
            rows.append(['B', f'{sid:x}', obj['datum'], obj['body_hex']])
        for path, raw in raw_fields(state):
            rows.append(['E', f'{sid:x}', path, raw])
    for tag in report['states'][0]['tags']:
        for sid in selected:
            require(next(t for t in report['states'][sid]['tags'] if t['datum'] == tag['datum']) == tag,
                    'Original complete cache tag changed across producers')
        rows.append(['D', tag['datum'], tag['address'], tag['raw_hex']])

    def operand(address, size):
        values = {r['raw_hex'] for r in report['reads']
                  if int(r['address'], 16) == address and r['size'] == size}
        require(len(values) == 1, f'Missing original producer operand {address:08x}')
        return next(iter(values))

    def word(raw):
        return f"{int.from_bytes(bytes.fromhex(raw), 'little'):08x}"

    one, zero, movement = [word(operand(address, 4)) for address in (0x465030, 0x465048, 0x4654c0)]
    mask = operand(0x45a8c0, 8) + operand(0x45a8c8, 8)
    body_stores = fallback_stores = reciprocal_count = 0
    for call in calls:
        rows.append(['C', f"scope{call['scope']['id']}_call{call['id']}", call['entry'], call['scope']['datum'],
                     f"{call['before_state']:x}", f"{call['after_state']:x}", call['return_cpu']['registers']['eax'],
                     one, zero, movement, mask, 'literal_source'])
        if call['entry'] == '000bdf80':
            points = [p for p in report['points'] if call['order'] < p['order'] < call['return_order']
                      and p['instruction'] in {'001439df', '001439e3'}]
            require(len(points) == 8 and [p['instruction'] for p in points] == ['001439df', '001439e3'] * 4,
                    'Missing original node reciprocal instruction checkpoints')
            for before, after in zip(points[::2], points[1::2]):
                rows.append(['R', before['cpu']['xmm128'][0][-8:], after['cpu']['xmm128'][1][-8:]])
                reciprocal_count += 1
            callbacks = [r for r in report['reads'] if call['order'] < r['order'] < call['return_order']
                         and r['instruction'] == '0010d2a2']
            require(len(callbacks) == 3 and all(r['size'] == 4 for r in callbacks),
                    'Missing actual source type callback roster')
            for callback in callbacks:
                rows.append(['K', word(callback['raw_hex'])])
        body = int(call['scope']['body'], 16)
        state = report['states'][call['before_state']]
        fallback = state['source_fallback_node_bank']
        fb = int(fallback['address'], 16)
        for store in report['stores']:
            if not call['order'] < store['order'] < call['return_order']:
                continue
            address = int(store['address'], 16)
            if body <= address and address + store['size'] <= body + call['scope']['body_size']:
                rows.append(['W', 'B', store['instruction'], f'{address - body:x}', store['before_hex'], store['written_hex']])
                body_stores += 1
            elif fb <= address and address + store['size'] <= fb + fallback['size']:
                rows.append(['W', 'F', store['instruction'], f'{address - fb:x}', store['before_hex'], store['written_hex']])
                fallback_stores += 1
    fixture = ROOT / 'analysis/multiplayer/integration/item-node-bounds-fixtures.tsv'
    fixture.write_text('\n'.join(' '.join(map(str, row)) for row in rows) + '\n')
    run = subprocess.run([str(args.binary.resolve()), str(fixture)], text=True, capture_output=True)
    print(run.stdout, end=''); print(run.stderr, end='')
    result = dict(passed=run.returncode == 0, scope=__doc__, original_calls=8,
                  node_calls=4, bounds_calls=4, complete_states=len(selected),
                  ordered_body_stores=body_stores, ordered_fallback_stores=fallback_stores,
                  explicit_rcpss_results=reciprocal_count,
                  compared='Complete five occupied bodies including local/current node banks; all captured raw header/registry/pool/bitmap/world/player/context/fallback/lazy/cache buffers; defined bounds AL; ordered body/fallback stores.',
                  limits='First source schedule, unparented entity/physical-NONE weapons. RCPSS and actual NULL type callback roster remain explicit; animated local poses, positive callbacks, changed registered-sphere/location/registry producers and live original frame remain separate.',
                  fixture_sha256=digest(fixture), native_binary_sha256=digest(args.binary),
                  tool_sha256=digest(Path(__file__)), stdout=run.stdout, stderr=run.stderr)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    raise SystemExit(run.returncode)


if __name__ == '__main__':
    main()
