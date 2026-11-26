#!/usr/bin/env python3
"""Qualify the live shape's typed written-byte observer against original C3AB0."""

import argparse
import contextlib
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
UNIT = 'src/pstencil/native_light_shape_tu15_generated.cpp'
CALLER = 'src/pstencil/native_light_shape.cpp'
ADAPTER = 'src/rendering/native_tu15_light_shape.hpp'
OBSERVER = 'src/pstencil/native_tu15_light_shape_written.hpp'
FIXTURE = 'tests/native_tu15_light_shape_written_tests.cpp'


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def run(argv, scratch, *, env=None, capture=False):
    words = [str(arg) for arg in argv]
    result = subprocess.run(words, cwd=scratch, env=env, text=True, capture_output=capture)
    if result.returncode:
        raise RuntimeError(f'{words[0]} failed ({result.returncode})\n{result.stdout or ""}{result.stderr or ""}')
    return result


def assemble(scratch, patch):
    if patch is None:
        return ROOT
    changed = {Path(path) for path in re.findall(r'^\+\+\+ b/([\w./-]+)$', patch.read_text(), re.M)}
    if changed != {Path(CALLER), Path(ADAPTER)}:
        raise ValueError('Expected only the C3AB0 caller and adapter changes')
    overlay = scratch / 'overlay'
    overlay.mkdir()

    def mirror(path):
        source, target = ROOT / path, overlay / path
        if path in changed:
            shutil.copyfile(source,target)
        elif any(path in item.parents for item in changed):
            target.mkdir()
            for child in source.iterdir():
                mirror(path / child.name)
        else:
            target.symlink_to(source)

    for prefix in ('include', 'src', 'tests', 'tools'):
        mirror(Path(prefix))
    run(['patch', '--batch', '--forward', '--fuzz=0', '-p1', '-i', patch.resolve()], overlay, capture=True)
    return overlay


def shape_route(caller):
    selection = [
        'const LightShapeWrittenObserver written(output,out.written);',
        'const auto observe=written.observer();',
        'world.run_actual(0xc3a20u,parameters,observe);',
        'world.run_actual(0xc3ab0u,transform,observe);',
    ]
    positions = [caller.index(line) for line in selection]
    if positions != sorted(positions) or any(caller.count(line) != 1 for line in selection):
        raise ValueError('Live light-shape observer route changed')


def prepare_fixture(source, normal, counted, scratch, binding, function_bodies):
    _, canonical = binding.authenticate(source)
    all_owners = {match[1]: body for match, body in function_bodies(counted)}
    measured = all_owners['c3ab0']
    tick = '::halo2::execution_accounting::record_literal(halo2_execution_counter_8ef3);'
    if measured.count(tick) != 194:
        raise ValueError('Did not account for all retained C3AB0 sites')
    measured = measured.replace(tick, '::retained_statement();' + tick)
    solutions = [measured]
    for helper in ('native_single_spill', 'native_fptan', 'sign_extend'):
        result = re.search(rf'^(?:[\w:]+ )+{helper}\(.*?^\}}', source, re.M | re.S)
        if not result:
            raise ValueError(f'Actual pStencil shape helper {helper} changed')
        solutions.insert(0, result[0])
    resolver = re.search(r'^std::uint32_t selected\([^\n]+$', source, re.M)
    if not resolver:
        raise ValueError('Missing pStencil selected resolver')
    solutions.insert(0, resolver[0])
    (scratch / 'light_shape_written_owner.inc').write_text('\n'.join(solutions) + '\n')
    return {'canonical_body_sha256': digest(canonical), 'prepared_body_sha256': digest(binding.BODY.search(normal)[0])}


def check_execution(binary, compiled, scratch, force=False):
    report = scratch / f'{"reference" if force else "default"}-census.json'
    env = dict(os.environ, HALO2_NATIVE_EXECUTION_REPORT=str(report), ASAN_OPTIONS='detect_leaks=0')
    if force:
        env['HALO2_REFERENCE_LIGHT_SHAPE'] = '1'
    result = json.loads(run([binary, compiled], scratch, env=env, capture=True).stdout)
    census = json.loads(report.read_text())
    rows = [dict(zip(census['row_fields'], row, strict=True)) for row in census['rows']]
    body, = [row for row in rows if row['symbol'] == 'c3ab0' and row['image'] == census['image_names'].index('tu15')]
    if (body['entries'] != result['native'] + result['retained'] or
            body['returns_without_body'] != result['native'] or body['body_entries'] != result['retained'] or
            bool(result['native']) == force or not result['retained']):
        raise ValueError('Ledgers differ for the actual counted C3AB0 route')
    result['owner_accounting'] = body
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--staged', type=Path)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    owned = os.environ.get('HALO2_TEST_SCRATCH')
    if not owned or not Path(owned).is_dir():
        parser.error('Run through tools/run_test.py for owned scratch storage')
    scratch = Path(owned)
    overlay = assemble(scratch,args.staged)
    for name in ('HALO2_REFERENCE_LIGHT_SHAPE','HALO2_LIGHT_SHAPE_REFERENCE_TEST',
                 'HALO2_LIGHT_SHAPE_CENSUS_TEST','HALO2_CENSUS_NATIVE_BRIDGES'):
        os.environ.pop(name,None)
    import native_light_shape_replacements as binding
    import native_shadow_frame_oracle as physical
    from generated_routine_ownership import function_bodies
    from instrument_native_execution import instrument_translation, unit_declaration
    from specialize_native_operands import specialize
    from validate_native_light_shape_tu15 import corpus, SPEC
    shape_route((overlay / CALLER).read_text())
    source = (ROOT / UNIT).read_text()
    normal, _ = specialize(source)
    counted, manifest, _ = instrument_translation(normal, source, UNIT, ROOT)
    body, = [row for row in manifest['functions'] if row['symbol'] == 'c3ab0']
    if body['literal_sites'] != 194:
        raise ValueError('The actual pStencil C3AB0 owner changed')
    prepared = prepare_fixture(source, normal, counted, scratch, binding, function_bodies)

    includes = [f'-I{path}' for path in (scratch, overlay / 'include', overlay / 'src', overlay / 'tests')]
    flags = ['-std=c++20','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',*includes]
    for name, text in (('normal',normal),('counted',counted)):
        file = scratch / f'{name}.cpp'
        file.write_text(text)
        run([args.compiler,*flags,'-fsyntax-only',file],scratch)
    run([args.compiler,*flags,'-fsyntax-only',overlay / CALLER],scratch)

    original = scratch / 'physical.json'
    physical.ROOT = overlay
    with contextlib.redirect_stdout(sys.stderr):
        baseline = physical.main(SPEC,corpus,['--xbe',str(args.xbe.resolve()),'--report',str(original),
                                              '--compiler',args.compiler,'--require-binding'])
    if baseline:
        raise RuntimeError(f'Original C3AB0 ABI proof did not pass ({baseline}): {original.read_text() if original.exists() else ""}')
    corpus_file = scratch / 'fixture.json'
    report = json.loads(original.read_text())
    if not report['passed']:
        raise ValueError('Original C3AB0 corpus was incomplete')
    accounting = ('#include "halo2/native_execution_accounting.hpp"\n' + unit_declaration(UNIT, source, normal))
    (scratch / 'light_shape_written_accounting.inc').write_text(accounting)
    binary = scratch / 'light-shape-written'
    extra = ['-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie'] if args.sanitize else ['-O2']
    run([args.compiler,*flags,*extra,'-pthread','-include',scratch / 'light_shape_written_accounting.inc',
         overlay / FIXTURE, ROOT / 'src/runtime/native_execution_accounting.cpp', '-o',binary],scratch)
    print(json.dumps({'header_sha256': digest((overlay / OBSERVER).read_text()),
                      'adapter_sha256': digest((overlay / ADAPTER).read_text()),
                      'caller_sha256': digest((overlay / CALLER).read_text()),
                      'production_aliases_parsed': True, 'authority': prepared,
                      'physical_cases': report['physical_cases'], 'original_instructions': report['original_instructions'],
                      'compiler': run([args.compiler,'--version'],scratch,capture=True).stdout.splitlines()[0],
                      'sanitizer': args.sanitize,
                      'qualified_observer': check_execution(binary,corpus_file,scratch),
                      'forced_reference': check_execution(binary,corpus_file,scratch,force=True)}, indent=2))


if __name__ == '__main__':
    main()
