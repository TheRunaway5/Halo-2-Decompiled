#!/usr/bin/env python3
"""Compare native2F0EB0 against unchanged physical English Xbox TU1.5.

Uses the established wrapped-constructor physical harness with the normal
BV-first argument order, the original allocator return PC and size store.
Child owners execute genuine original CPU code on each side's current data;
this is component qualification, not a production native-only closure claim.
Run under tools/run_test.py; CMake builds use tools/run_tests.py.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

def replace_once(text, old, new):
    if text.count(old) != 1:
        raise ValueError('Physical BV harness requires unique inherited context: '+old[:90])
    return text.replace(old, new)


def main():
    ap = argparse.ArgumentParser(add_help=False)
    ap.add_argument('--source-root', type=Path)
    args, remaining = ap.parse_known_args()
    source = args.source_root.resolve(strict=True) if args.source_root else Path(__file__).resolve().parents[1]
    # Load the inherited harness from the qualified cohort itself. Its helper
    # source pin and __file__ must identify the same tree as its candidate.
    sys.path.insert(0, str(source/'tools'))
    spec = importlib.util.spec_from_file_location('bv_wrapped_physical',
        source/'tools/validate_native_wrapped_contact_agent_tu15.py')
    wrapped = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = wrapped
    spec.loader.exec_module(wrapped)
    wrapped.ROOT = source
    sys.argv = [sys.argv[0], *remaining]
    driver = wrapped.DRIVER
    # A real captured first shape is wrapped as declared input DATA. The
    # original and candidate both receive that first payload, without the
    # reversed factory's B->A swap. No class, child pointer or result is supplied.
    driver = replace_once(driver, 'original.word(b)};', 'original.word(a)};')
    driver = replace_once(driver, 'original.word(b+4u),original.word(b+8u)',
                          'original.word(a+4u),original.word(a+8u)')
    driver = replace_once(driver, 'b=wrapper_payload;', 'a=wrapper_payload;')
    for old, new in [('0x2f1cc0', '0x2f0eb0'), ('0x2f1cd6', '0x2f0ec6'),
                     ('0x2f1cdc', '0x2f0ecc'), ('0x2f1ce3', '0x2f0ed2'),
                     ('0x2f1ce6', '0x2f0ed5'), ('0x2f1ce8', '0x2f0ed5')]:
        if old not in driver:
            raise ValueError('Missing inherited original-PC context '+old)
        driver = driver.replace(old, new)
    driver = replace_once(driver, 'native_tu15_create_wrapped_contact_agent(memory,arguments,',
                          'native_tu15_create_bv_contact_agent(memory,arguments,')
    driver = replace_once(driver, 'case 0x2f0ed5:address=r.rsi+4u;width=2;write=true;break;',
                          'case 0x2f0ed5:address=r.rax+4u;width=2;write=true;break;')
    driver = replace_once(driver, 'case 0x2f1d19:address=r.rsi;write=true;break;', '')
    driver = replace_once(driver,
        '// Actual2F1D04 pushes A, then2F1D05 pushes B: ctor first=B, second=A.',
        '// Actual2F0EB0 preserves A/B order; declared BV payload is first.')
    variants = (
        ('wrong_bv_size', 'm.word(0x2f0ed5u,result.identity+4u,20u,2)',
         'm.word(0x2f0ed5u,result.identity+4u,24u,2)'),
        ('wrong_bv_order', 'current{[&]{return first;},[&]{return second;}',
         'current{[&]{return second;},[&]{return first;}'),
        ('wrong_bv_child', 'm.word(shape+12u)', 'm.word(shape+8u)'),
        ('wrong_bv_stride', '(first_class<<5u)+second_class', '(first_class<<4u)+second_class'),
        ('wrong_bv_vtable', 'm.word(0x2f0e21u,object,0x41ca9cu)',
         'm.word(0x2f0e21u,object,0x41caf0u)'),
    )
    result = wrapped.main(driver_source=driver, source_scope='wrapped', variants_spec=variants)
    # The common harness retains its exact commands and source/driver hashes.
    # Add the adapter identity without retaining any executable or raw bank.
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--output', type=Path, required=True)
    options, _ = parser.parse_known_args(remaining)
    summary = options.output.resolve()/'report.json'
    if summary.is_file():
        report = json.loads(summary.read_text())
        with Path(__file__).open('rb') as stream:
            report['bv_first_adapter_sha256'] = hashlib.file_digest(stream, 'sha256').hexdigest()
        report['scope'] = __doc__
        report['production_native_only_closure'] = False
        summary.write_text(json.dumps(report, indent=2)+'\n')
    return result


if __name__ == '__main__':
    raise SystemExit(main())
