#!/usr/bin/env python3
"""Check the active TU1.5 bitmap residency binder and its provider/trace ABI."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from instrument_native_execution import instrument_translation
import native_bitmap_residency_replacements as binding
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'src/physics/physics_live_support_tu15_generated.cpp'
FIXTURE = 'tests/native_tu15_bitmap_residency_tests.cpp'
HEADER = 'src/' + binding.HEADER


def run(command, directory):
    result = subprocess.run([str(value) for value in command], cwd=directory, capture_output=True, text=True,
                            env={name: value for name, value in os.environ.items()
                                 if name != 'HALO2_REFERENCE_BITMAP_RESIDENCY'})
    if result.returncode:
        raise ValueError(f'{command[0]} failed with {result.returncode}\n{result.stdout[-2000:]}{result.stderr[-4000:]}')
    return result.stdout


def export(body, name):
    assert body.startswith('static void run_12f910(WorldState &s)')
    body = body.replace('static void run_12f910(WorldState &s)', f'void {name}(WorldState &s, ChildContext &context)', 1)
    runs = ''.join(f'        case 0x{target}u: run_{target}(v); return;\n' for _, target, _ in binding.CHILDREN)
    if name == 'selected':
        assert body.count(runs) == 1
        body = body.replace('bridge(s,[](auto &v,std::uint32_t target)', 'bridge(s,[&](auto &v,std::uint32_t target)')
        for _, target, _ in binding.CHILDREN:
            body = body.replace(f'run_{target}(v);', f'child(v,0x{target}u,context);')
        # Fail if the intended test configuration unexpectedly selects rollback.
        body = body.replace('    }))return;\n', '    }))return;\n    throw std::logic_error("Bitmap residency alias refused native fixture");\n')
    for _, target, _ in binding.CHILDREN:
        needle = f'run_{target}(s);'
        assert body.count(needle) == 1
        body = body.replace(needle, f'child(s,0x{target}u,context);')
    return body


def qualify(args, scratch, report):
    canonical = (ROOT / SOURCE).read_text()
    original = binding.BODY.search(canonical)[0]
    alias, _ = specialize(canonical)
    prepared = binding.BODY.search(alias)[0]
    if alias.count(binding.INCLUDE) != 1 or prepared.count(binding.HOOK) != 1:
        raise ValueError('Complete native bitmap residency production binder is missing')
    unbound = prepared.replace(binding.HOOK, '').replace('halo2::native_world_leaves_detail::ordered_add_ss(', '_mm_add_ss(')
    unbound = unbound.replace('halo2::native_tu15_leaves::ordered_mul_ss(', '_mm_mul_ss(')
    unbound, pushes = re.subn(re.escape(binding.ATOMIC_PUSH) + r'[^;\n]+\);else (?=s\.push\()', '', unbound)
    if (binding.digest(original) != binding.PIN or binding.digest(unbound) != binding.PREPARED or
            pushes != len(binding.PUSH_SITES) or binding.bound_body(unbound) != prepared):
        raise ValueError('Bitmap canonical or active binding changed')
    retained = re.sub(r'if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+u)\);\}',
                      r's.instruction_literal(\1);', binding.retained(original))
    (scratch / 'bitmap_residency_alias.inc').write_text(export(retained, 'retained') + '\n' + export(prepared, 'selected'))
    sites = {int(pc, 16) for pc in re.findall(r'instruction_literal\(0x([0-9a-f]+)u\)', retained)}
    accounted, manifest, _ = instrument_translation(alias, canonical, SOURCE, ROOT)
    owner, = (row for row in manifest['functions'] if row['symbol'] == '12f910')
    if len(sites) != 223 or owner['literal_sites'] != len(sites) or owner['dynamic_instruction_forwarders']:
        raise ValueError('Bitmap residency production literal accounting changed')
    report.update(canonical_binding_sha256=binding.PIN, prepared_body_sha256=binding.PREPARED,
                  bound_body_sha256=binding.digest(prepared), literal_sites=len(sites), atomic_stack_sites=pushes,
                  sources={name: binding.digest((ROOT / name).read_text())
                           for name in (SOURCE, HEADER, FIXTURE, 'tools/native_bitmap_residency_replacements.py')},
                  profiles={})
    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             *(f'-I{path}' for path in (ROOT / 'include', ROOT / 'src', ROOT / 'tests', scratch))]
    if args.full_syntax:
        for name, source in (('ordinary', alias), ('accounted', accounted)):
            path = scratch / f'{name}.cpp'
            path.write_text(source)
            run([args.compiler, *flags, '-fsyntax-only', path], scratch)
        report['full_production_aliases'] = ['ordinary', 'accounted']
    profiles = [('release', []), ('asan_ubsan', ['-O1', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                                              '-fno-sanitize-recover=all', '-fno-pie', '-no-pie'])]
    for name, sanitizer in profiles:
        exe = scratch / name
        run([args.compiler, *flags, *sanitizer, ROOT / FIXTURE, '-o', exe], scratch)
        result = json.loads(run([exe], scratch))
        observed = set(result.pop('sites'))
        result['observed_sites'] = len(observed)
        result['omitted_sites'] = sorted(sites - observed)
        report['profiles'][name] = result
        print(name, result, flush=True)
        if observed != sites or set(result['child_entries']) != {int(target, 16) for _, target, _ in binding.CHILDREN}:
            raise ValueError('Bitmap residency fixture omitted an original parent site or real child entry')
        stack_stores = {int(pc, 16) for pc in binding.PUSH_SITES}
        if set(result['stack_store_denials']) != stack_stores or set(result['committed_stack_observer_rejections']) != stack_stores:
            raise ValueError('Bitmap residency fixture omitted a pre-store or committed stack fault boundary')
    report['passed'] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--full-syntax', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = dict(passed=False, scratch=str(scratch), compiler=args.compiler)
    try:
        qualify(args, scratch, report)
    except BaseException as error:
        report['error'] = str(error)
        raise
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
