#!/usr/bin/env python3
"""Check actual ordinary423B0 expected/copy/draw observations; no pixel claim.

The runtime owns both observations: the complete native descriptor handoff and
the callbacks after the actual scene copy and Vulkan draws. A missing stream,
missing layer21, missing copy, dropped range or reordered body is a failure.
Use run_test.py for any capture and keep its raw JSONL in HALO2_TEST_SCRATCH.
"""
import argparse
from collections import defaultdict
import hashlib
import json
import lzma
from pathlib import Path


def check(records):
    expected = {}
    submitted = defaultdict(list)
    for row in records:
        if row.get('event') == 'hologram_expected':
            if row['frame'] in expected:
                raise ValueError('Duplicate expected group bank for one ordinary frame')
            expected[row['frame']] = row['groups']
        elif row.get('event') == 'hologram_submission':
            submitted[row['frame']].append(row)
    if not expected:
        raise ValueError('No natural source hologram group reached the render handoff')
    counts = dict(frames=0, groups=0, copies=0, body_descriptors=0,
                  hologram_descriptors=0, hologram_draws=0, hologram_indices=0,
                  hologram_source_indices=0)
    for frame, groups in expected.items():
        actual = submitted.pop(frame, [])
        finished = set()
        active = None
        for event in actual:
            owner = event['group']
            if not 0 <= owner < len(groups):
                raise ValueError(f'Frame {frame}: submission references an unowned group')
            if active != owner:
                if owner in finished:
                    raise ValueError(f'Frame {frame}: another group interrupted its body/override schedule')
                finished.add(owner)
                active = owner
        for g, group in enumerate(groups):
            rows = [r for r in actual if r['group'] == g]
            if not rows or rows[0]['kind'] != 0:
                raise ValueError(f'Frame {frame} group {g}: target19 copy did not precede its body')
            if sum(r['kind'] == 0 for r in rows) != 1:
                raise ValueError(f'Frame {frame} group {g}: scene copy repeated inside one group')
            counts['copies'] += 1
            cursor = 1
            for d, part in enumerate(group['parts']):
                ranges = [None]
                if part['layer'] == 21:
                    source = part['source_index_ranges']
                    ranges = part['submitted_index_counts']
                    if len(source) != len(ranges):
                        raise ValueError(f'Frame {frame} group {g}: source and submitted range banks differ at {d}')
                    for raw, submitted_count in zip(source, ranges):
                        if raw['first'] < 0 or raw['count'] < 0:
                            raise ValueError(f'Frame {frame} group {g}: invalid source index range at {d}')
                        if raw['topology'] == 'triangle_strip':
                            triangles = max(raw['count'] - 2, 0)
                        elif raw['topology'] == 'triangle_list' and raw['count'] % 3 == 0:
                            triangles = raw['count'] // 3
                        else:
                            raise ValueError(f'Frame {frame} group {g}: unsupported source topology/count at {d}')
                        if submitted_count != triangles * 3:
                            raise ValueError(f'Frame {frame} group {g}: source triangulation count differs at {d}')
                        counts['hologram_source_indices'] += raw['count']
                ranges = ranges or [0]
                for r, indices in enumerate(ranges):
                    if cursor >= len(rows):
                        raise ValueError(f'Frame {frame} group {g}: dropped descriptor {d} range {r}')
                    event = rows[cursor]
                    wanted = (1, d, part['layer'], r, part['owner'], part['shader'])
                    observed = tuple(event[k] for k in ('kind', 'descriptor', 'layer', 'range', 'owner', 'shader'))
                    if observed != wanted:
                        raise ValueError(f'Frame {frame} group {g}: actual descriptor order/owner differs at {d}/{r}')
                    if indices is not None and event['indices'] != indices:
                        raise ValueError(f'Frame {frame} group {g}: actual submitted triangle-list count differs at {d}/{r}')
                    if indices is None and event['indices'] != part['body_vertices']:
                        raise ValueError(f'Frame {frame} group {g}: body draw exceeds its exact unfiltered source part')
                    if indices != 0 and (not event['indices'] or not event['pipeline'] or not event['descriptor_set']):
                        raise ValueError(f'Frame {frame} group {g}: descriptor {d}/{r} has no actual GPU draw binding')
                    if part['layer'] == 21 and event['indices']:
                        if (not event['indexed'] or not event['index_buffer'] or event['index_offset'] % 2 or
                                event['first_index'] != 0 or event['vertex_offset'] != 0):
                            raise ValueError(f'Frame {frame} group {g}: actual indexed draw binding differs at {d}/{r}')
                        counts['hologram_draws'] += 1
                        counts['hologram_indices'] += event['indices']
                    cursor += 1
                counts['hologram_descriptors' if part['layer'] == 21 else 'body_descriptors'] += 1
            if cursor != len(rows):
                raise ValueError(f'Frame {frame} group {g}: extra unowned draws')
            counts['groups'] += 1
        counts['frames'] += 1
    if submitted or not counts['hologram_draws']:
        raise ValueError('Unowned submissions or no actual nonempty layer21 draw')
    return counts


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--trace', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    args = ap.parse_args()
    opener = lzma.open if args.trace.suffix == '.xz' else open
    report = dict(passed=False, trace=str(args.trace),
                  trace_sha256=hashlib.sha256(args.trace.read_bytes()).hexdigest(),
                  scope='actual Vulkan command inputs and copy-before-body order; no GPU completion or Xbox framebuffer claim',
                  retained_large_artifacts=[])
    try:
        with opener(args.trace, 'rt') as stream:
            report.update(check(json.loads(line) for line in stream if line.strip()))
        report['passed'] = True
    except (ValueError, KeyError) as error:
        report['error'] = str(error)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
