#!/usr/bin/env python3
"""Compare the private 1CF0E0 parent with original TU1.5 hardware packets."""
import argparse
import copy
import json
import os
import platform
from pathlib import Path
import shlex
import subprocess

import oracle_native_campaign_screen_lifecycle_tu15 as original
from generate_physics_live_support_tu15 import complete_body
from validate_native_node_descriptor_lookup_tu15 import REGIONS
from validate_native_node_progress_query_source_tu15 import (
    FRAME, INPUTS, PIN, ROOT, UNIT, Xbe, corpus, prepare, sha)
from validate_native_projectile_function_query_frame_tu15 import boundary_packet
from validate_native_tu15_prt_helpers import physical_original

PARENT, CHILD = 0x1cf0e0, 0x1e3150
ENTRIES = (PARENT, CHILD, 0x1e5c80, 0x1e3270, 0x1e5af0)
AFTER_CHILD = 0x1cf10d
DRIVER = 'tests/native_tu15_node_progress_query_frame_tests.cpp'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--compiler', action='append')
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--prefixes', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686'):
        return 77
    if not args.xbe.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True, exist_ok=False)
    names = set(INPUTS) | {FRAME, UNIT, DRIVER, str(Path(__file__).relative_to(ROOT)),
        'tools/validate_native_node_progress_query_source_tu15.py',
        'tools/validate_native_projectile_function_query_frame_tu15.py',
        'tools/oracle_native_campaign_screen_lifecycle_tu15.py'}
    before = {name: sha(ROOT / name) for name in sorted(names)}
    headers = {p.resolve(): sha(p) for tree in ('include', 'src', 'tests') for p in (ROOT / tree).rglob('*')
               if p.suffix in ('.hpp', '.h', '.inc')}
    report = dict(passed=False, production_bound=False, owned_scratch=str(scratch), retained_large_artifacts=[])
    try:
        if sha(args.xbe) != PIN:
            raise ValueError('Wrong original TU1.5 authority')
        image = Xbe(args.xbe)
        code = original._Code(image, '.text')
        parent = set(complete_body(code, PARENT).code)
        call = code.at(0x1cf108)
        if len(parent) != 34 or call.mnemonic != 'call' or call.operands[0].imm != CHILD:
            raise ValueError('Original 1CF0E0 ownership or child call changed')
        metadata = original.metadata
        original.ENTRIES = ENTRIES

        def with_stack_reads(img):
            result = metadata(img)
            for row in result['sites']:
                if code.at(row['instruction']).mnemonic in ('pop', 'ret'):
                    row['operands'].append(dict(base=4, index=-1, scale=1, displacement=0, bytes=4, read=True, write=False))
            return result

        original.metadata = with_stack_reads
        sites = with_stack_reads(image)['sites']
        traces = [dict(entry=row['instruction'], arguments=0, gp=True, xmm=True) for row in sites
                  if row['instruction'] in (CHILD, AFTER_CHILD) or
                  (row['instruction'] in parent and
                   (row['operands'] or code.at(row['instruction']).mnemonic in ('push', 'call')))]

        def observe(rows, folder):
            return physical_original(image, rows, folder, entries=ENTRIES, regions=(*REGIONS, (0x46a000, 4096)),
                                     trace_calls=traces, trace_extended=True, preserve_x87_prefix=True)

        rows = corpus(image)
        normal = len(rows)
        observed, report['original_authority'] = observe(rows, scratch)
        if args.prefixes:
            stopped = []
            for row, actual in zip(rows[:16], observed['cases'][:16], strict=True):
                for phase, key in (('before_read', 'reads'), ('before_write', 'writes')):
                    for index, event in enumerate(actual[key]):
                        if event['instruction'] not in parent:
                            continue
                        candidate = copy.deepcopy(row)
                        candidate.update(id=len(stopped), name=f"{row['name']}-{phase}-{index}",
                                         fault=dict(phase=phase, ordinal=index))
                        stopped.append(candidate)
            folder = scratch / 'prefixes'
            folder.mkdir()
            prefixes, report['prefix_authority'] = observe(stopped, folder)
            rows.extend(stopped)
            observed['cases'].extend(prefixes['cases'])
        visited = {pc for actual in observed['cases'][:normal] for pc in actual['executed_sites']}
        if not parent <= visited:
            raise ValueError('Original parent site coverage missing: '+str(sorted(parent - visited)))
        child_entries = child_returns = 0
        for row, actual in zip(rows, observed['cases'], strict=True):
            if actual['completed'] != ('fault' not in row) or (actual['completed'] and not actual['live_x87_unchanged']):
                raise ValueError('Original completion or live x87 changed: '+row['name'])
            for item in actual['calls']:
                if item.pop('arguments') != []:
                    raise ValueError('Unexpected original packet arguments')
            if actual['completed']:
                child_entries += sum(item['entry'] == CHILD for item in actual['calls'])
                child_returns += sum(item['entry'] == AFTER_CHILD for item in actual['calls'])
            pc = actual['boundary']['instruction'] if 'fault' in row else actual['calls'][-1]['entry']
            row.update(expected=boundary_packet(actual, row, pc), reads=actual['reads'],
                       writes=actual['writes'], calls=actual['calls'])
            for bank, after in zip(row['banks'], actual['banks'], strict=True):
                bank['after'] = after['after']
        if not child_entries or child_entries != child_returns:
            raise ValueError('Genuine original descriptor call/return closure changed')
        report.update(original_returns=normal, original_prefixes=len(rows)-normal, parent_sites=len(parent),
                      child_entries_on_returning_paths=child_entries, child_returns_on_returning_paths=child_returns,
                      child_owner_sites_executed={hex(pc):len(visited & set(complete_body(code, pc).code)) for pc in ENTRIES[1:]},
                      authority_scope='All original parent pre-I/O, actual descriptor entry/return, complete final ABI and banks')
        report['retained_child_authority'] = prepare(scratch)
        fixture = scratch / 'fixture.json'
        fixture.write_text(json.dumps(rows, separators=(',', ':')))
        report['compilers'] = []
        for compiler in args.compiler or ['g++', 'clang++']:
            binary = scratch / Path(compiler).name
            dep = binary.with_suffix('.d')
            command = [compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
                       *('-I'+str(ROOT / tree) for tree in ('include', 'src', 'tests')), '-I'+str(scratch),
                       str(ROOT / DRIVER), '-o', str(binary), '-MD', '-MF', str(dep)]
            if args.sanitizers:
                command.extend(('-fsanitize=address,undefined', '-fno-omit-frame-pointer'))
            built = subprocess.run(command, capture_output=True, text=True, timeout=1200)
            if built.returncode:
                raise ValueError('Node progress frame compile failed: '+built.stderr[-8000:])
            for name in shlex.split(dep.read_text().replace(chr(92)+'\n', ' ').split(':', 1)[1]):
                file = Path(name).resolve()
                if file in headers:
                    before[str(file.relative_to(ROOT))] = headers[file]
            run = subprocess.run([str(binary), str(fixture)], capture_output=True, text=True, timeout=600)
            if run.returncode:
                raise ValueError('Node progress frame comparison failed: '+(run.stdout+run.stderr)[-8000:])
            report['compilers'].append(dict(compiler=compiler, result=json.loads(run.stdout)))
        report['passed'] = True
    except BaseException as error:
        report['error'] = repr(error)
        raise
    finally:
        report['source_before'] = before
        report['source_after'] = {name: sha(ROOT / name) for name in before}
        if report['source_after'] != before:
            report.update(passed=False, source_drift=True)
        (args.output / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
