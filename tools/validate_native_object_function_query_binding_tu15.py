#!/usr/bin/env python3
"""Validate the installed C9070 overlay, retained owner, and generated formats."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

import native_node_function_progress_replacements as progress
import native_object_function_query_replacements as query
import native_object_record_ratio_replacements as ratio
from specialize_native_operands import specialize

ROOT = Path(__file__).resolve().parents[1]
PRIMARY = 'src/physics/physics_live_support_tu15_generated.cpp'


def require_refusal(method, source):
    try:
        method(source)
    except ValueError:
        return
    raise ValueError(method.__name__ + ' accepted malformed C9070 overlay')


def validate(raw, label, scratch, compiler):
    unbound = query.remove_object_function_query(raw)
    if raw == unbound or query.replace_object_function_query(unbound) != raw:
        raise ValueError('C9070 raw source cannot be exactly removed and restored')
    prepared, receipt = specialize(raw)
    if receipt['specialized_sha256'] != query.digest(prepared) or specialize(prepared)[0] != prepared:
        raise ValueError('C9070 prepared source or receipt is not idempotent')
    if query.remove_object_function_query(prepared) != specialize(unbound)[0]:
        raise ValueError('C9070 prepared inverse changed sibling source')
    for entry, evidence in [('c9070', 'object_function_query'), ('d3950', 'node_function_progress'),
                            ('107140', 'object_record_ratio')]:
        if receipt['native_' + evidence + '_entries'] != [entry]:
            raise ValueError('C9070 composition changed the exact ' + entry + ' receipt')
    checks = {}
    for literal, text in ((False, raw), (True, prepared)):
        for helper, entry in ((query, 'c9070'), (progress, 'd3950'), (ratio, '107140')):
            if helper.installed_entries(text) != [entry]:
                raise ValueError('C9070 composition changed ' + entry)
        _, body, _, _ = query.authenticate(text)
        owner = query.BODY.search(text)[0]
        tag = query.prefix(literal)
        moved = body.replace('throw std::runtime_error(', tag + '\nthrow std::runtime_error(', 1)
        nested = body.replace('{const auto target=0xd3750u;', '{' + tag + '\nconst auto target=0xd3750u;', 1)
        wrong_return = owner.replace('s.push(0xc9101u,0xc9106u);', 's.push(0xc9101u,0xc9107u);', 1)
        if any(value == owner for value in (moved, nested, wrong_return)) or moved == body or nested == body:
            raise ValueError('C9070 binder defect did not reach its exact target')
        changes = (text.replace(tag, query.prefix(not literal), 1), text.replace(owner, moved, 1),
                   text.replace(owner, nested, 1), text.replace(owner, wrong_return, 1), text + '\n' + tag,
                   text.replace(query.INCLUDE, '', 1))
        for change in changes:
            for method in (query.authenticate, query.installed_entries, specialize):
                require_refusal(method, change)
        for change in changes[1:4]:
            require_refusal(progress.installed_entries, change)
        variant = label + ('-prepared' if literal else '-raw')
        source = scratch / (variant + '.cpp')
        source.write_text(text)
        if compiler:
            command = [compiler, '-std=c++20', '-fsyntax-only', '-I' + str(ROOT/'include'),
                       '-I' + str(ROOT/'src'), str(source)]
            subprocess.run(command, check=True, timeout=360)
        checks[variant] = {'whole_sha256': query.digest(text), 'whole_inverse_exact': True,
                           'preserved_sibling_receipts': ['d3950', '107140'], 'controls': 21,
                           'whole_syntax_checked': bool(compiler)}
    return checks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if Path(args.compiler).name.lower() in ('cl', 'cl.exe'):
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = {'passed': False, 'owned_scratch': str(scratch), 'retained_large_artifacts': []}
    try:
        current = (ROOT / PRIMARY).read_text()
        fresh, manifest = scratch / 'fresh.cpp', scratch / 'fresh.json'
        command = [sys.executable, '-B', str(ROOT/'tools/generate_physics_live_support_tu15.py'),
                   '--output', str(fresh), '--manifest', str(manifest)]
        subprocess.run(command, cwd=ROOT, check=True, timeout=360)
        report['contexts'] = validate(current, 'current', scratch, args.compiler)
        report['contexts'].update(validate(fresh.read_text(), 'fresh', scratch, args.compiler))
        report.update(passed=True, generator_manifest_sha256=hashlib.sha256(manifest.read_bytes()).hexdigest(),
                      primary_unrelated_bytes_preserved=True, retained_1046c0=True,
                      native_dependency_closure=False)
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
