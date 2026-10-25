#!/usr/bin/env python3
"""Compare native and literal complete objective matches from one source build."""
import argparse
import json
from pathlib import Path


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native',type=Path)
    parser.add_argument('reference',type=Path)
    parser.add_argument('report',type=Path)
    args=parser.parse_args()
    native=json.loads(args.native.read_text());reference=json.loads(args.reference.read_text())
    rows=[]
    if not native['passed'] or not reference['passed']:
        raise ValueError('Both complete-match cohorts must pass before comparison')
    left={row['name']:row for row in native['cases']};right={row['name']:row for row in reference['cases']}
    expected=54 if native.get('edited_rules') else 9
    if native.get('edited_rules')!=reference.get('edited_rules') or len(left)!=expected or left.keys()!=right.keys():
        raise ValueError('Missing original objective preset/rule case')
    returns=bool(native.get('return_flow'))
    for key in ('return_flow','short_return_edit','touch_return_edit'):
        if native.get(key)!=reference.get(key):raise ValueError('Different objective return cohorts')
    for name,row in left.items():
        native_calls={value['entry']:value['calls'] for value in row['native_owner_calls']}
        reference_calls={value['entry']:value['calls'] for value in right[name]['native_owner_calls']}
        if native_calls.get(0x2496b0,0)<=0 or native_calls.get(0x249b20,0)<=0:
            raise ValueError(f'{name} did not execute both native player/object ticks')
        if any(reference_calls.values()):raise ValueError(f'{name} literal control executed native objective owners')
        # Diagnostics count native executions, and therefore differ in the
        # literal control. Every gameplay result and frame digest must match.
        diagnostics={'native_owner_calls'}
        if returns:
            for control in (row,right[name]):
                counters=control['native_kernels']
                if set(counters)!=set(('arming','decay')) or any(type(v)!=int or v<0 for v in counters.values()):
                    raise ValueError('Malformed objective kernel diagnostics')
            if any(right[name]['native_kernels'].values()):raise ValueError('Literal return control executed native scalar kernels')
            diagnostics.add('native_kernels')
        a={k:v for k,v in row.items() if k not in diagnostics}
        b={k:v for k,v in right[name].items() if k not in diagnostics}
        rows.append({'name':name,'passed':a==b,
                     'different_fields':[key for key in sorted(a.keys()|b.keys()) if a.get(key)!=b.get(key)]})
    result={'passed':all(row['passed'] for row in rows),'cases':rows,
            'scope':('All reported return gameplay fields; native/source controls from the same build.' if returns else
                'All observed gameplay-frame digests and complete authored match results; source controls from the same build.')}
    args.report.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
    return 0 if result['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
