#!/usr/bin/env python3
"""Compare the complete public contact CPP and a private shared predicate."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

import validate_native_contact_records_tu15 as original

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / 'src/physics/native_tu15_contact_records.cpp'
DRIVER = ROOT / 'tests/native_tu15_contact_records_tests.cpp'
PUBLIC_SHA256 = '8d0655e35e79821e0e507175a3deafc7e2c5d9f8870d99113e971e2cdb8c3aa0'
INCLUDE = '#include "halo2/native_tu15_contact_predicate_kernel.hpp"\n'
VIEW = """struct PredicateValue {
    Storage& storage;
    Record first, second;
    NativeTu15ContactRecordSettings settings;

    std::uint16_t distance_limit() const {
        push(storage.word(second, 0x0c));
        return compare_pop(storage.comparison_limit());
    }
    native_contact_predicate_detail::Pair normals() const {
        const auto a = lanes(storage.vector(first, 0x10));
        return {a, lanes(storage.vector(second, 0x10))};
    }
    native_contact_predicate_detail::Pair positions() const {
        const auto a = lanes(storage.vector(first, 0));
        return {a, lanes(storage.vector(second, 0))};
    }
    std::uint16_t normal_threshold(const native_contact_predicate_detail::Dot& dot) const {
        push(std::bit_cast<std::uint32_t>(_mm_cvtss_f32(dot.z_sum)));
        return compare_pop(storage.setting(settings, 0x10));
    }
    std::uint16_t separation_threshold(const native_contact_predicate_detail::Dot& dot) const {
        push(std::bit_cast<std::uint32_t>(_mm_cvtss_f32(dot.z_sum)));
        return compare_pop(storage.setting(settings, 0x0c));
    }
};
std::uint8_t test(Storage&s,Record a,Record b,NativeTu15ContactRecordSettings settings){
    PredicateValue view{s, a, b, settings};
    return native_contact_predicate_detail::reject(view);
}
"""


def candidate():
    source = CPP.read_text()
    if hashlib.sha256(source.encode()).hexdigest() != PUBLIC_SHA256:
        raise ValueError('Complete public contact record source drift')
    arithmetic = source[source.index('__m128 multiply('):source.index('void push(')]
    predicate = source[source.index('bool parity('):source.index('void publish_pop(')]
    result = INCLUDE + source.replace(arithmetic, '', 1).replace(predicate, VIEW, 1)
    arithmetic_insertion = result.index('void push(')
    recovered = (result[len(INCLUDE):arithmetic_insertion] + arithmetic
                 + result[arithmetic_insertion:]).replace(VIEW, predicate, 1)
    if recovered != source:
        raise ValueError('Private value overlay changes unrelated contact source')
    return result


def physical_inputs(scratch):
    if original.sha(original.NEW_DEFAULT) != original.NEW_PIN:
        raise ValueError('Original TU1.5 image drift')
    image = original.Xbe(original.NEW_DEFAULT)
    metadata = original.metadata(image)
    owned = original.prepare(image, metadata, scratch)
    rows = original.physical(owned, original.inputs(image), 'complete')
    prefixes = []
    for seed in (row for row in rows if 'fault_seed' in row['name']):
        for phase, key in (('before_read', 'reads'), ('before_write', 'writes'), ('after_write', 'writes')):
            for ordinal in range(len(seed[key])):
                row = copy.deepcopy(seed)
                row.update(id=len(rows) + len(prefixes), name=f"{seed['name']}_{phase}_{ordinal}",
                           fault=dict(phase=phase, ordinal=ordinal))
                prefixes.append(row)
    rows += original.physical(owned, prefixes, 'prefixes')
    fixture = scratch / 'fixture.json'
    fixture.write_text(json.dumps(dict(image_sha256=original.NEW_PIN, required_backend=original.BACKEND,
        body_sites=metadata['body_sites'], listing_sha256=metadata['listing_sha256'], cases=rows),
        separators=(',', ':')))
    return fixture, rows, metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--compiler', action='append')
    parser.add_argument('--sanitizers', action='store_true')
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    report = dict(passed=False, private_unbound=True, owned_scratch=str(scratch),
                  retained_large_artifacts=[], compilers=[])
    preimage = {p.resolve(): original.sha(p) for folder in ('include', 'src')
                for p in (ROOT/folder).rglob('*')
                if p.suffix in ('.hpp', '.h', '.inc', '.def') and p.is_file()}
    inputs = {CPP, DRIVER, ROOT/'tools/observe_native_contact_records_tu15.cpp', original.NEW_DEFAULT}
    for module in tuple(sys.modules.values()):
        if name := getattr(module, '__file__', None):
            path = Path(name).resolve()
            if path.is_relative_to(ROOT/'tools') and path.is_file():
                inputs.add(path)
    for name in original.ORIGINAL_INPUT_PINS:
        inputs.add(original.LIFETIME/(name+'-original-lifetime-final')/'report.json')
    preimage.update({p: original.sha(p) for p in inputs})
    try:
        overlay = scratch/'contact-records-shared.cpp'
        overlay.write_text(candidate())
        fixture, rows, metadata = physical_inputs(scratch)
        report.update(physical_cases=len(rows), body_sites=metadata['body_sites'],
            listing_sha256=metadata['listing_sha256'], image_sha256=original.NEW_PIN,
            original_fixture_sha256=original.sha(fixture), private_cpp_sha256=original.sha(overlay),
            source_cpp_sha256=PUBLIC_SHA256,
            genuine_payload_cases=sum(bool(row.get('original_record_input_provenance')) for row in rows),
            scope='Private public-DATA reuse; both complete public variants independently match original '
                  'values, semantic reads/stores/banks, source FP and host state. Existing public '
                  'storage abstraction, compaction and copy bodies are unchanged; no installed route.')
        for compiler in args.compiler or ('g++', 'clang++'):
            compiler_name=Path(compiler).name
            command = [compiler, '-std=c++20', '-O1', '-g0', '-fno-fast-math', '-ffp-contract=off',
                       '-frounding-math', '-I'+str(ROOT/'include')]
            if args.sanitizers:
                command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
            record = dict(compiler=compiler, commands=[], variants=[])
            report['compilers'].append(record)

            def compile_source(source, destination):
                dep = destination.with_suffix('.d')
                compile_command = [*command, '-MMD', '-MF', str(dep), '-c', str(source), '-o', str(destination)]
                record['commands'].append(compile_command)
                result = subprocess.run(compile_command, cwd=ROOT, capture_output=True, text=True, timeout=180)
                if result.returncode:
                    raise ValueError('Private value compile failed: '+result.stderr[-6000:])
                for name in shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1]):
                    path = Path(name).resolve()
                    if path.is_relative_to(ROOT) and not path.is_relative_to(scratch):
                        inputs.add(path)

            driver = scratch/(compiler_name+'-driver.o')
            compile_source(DRIVER, driver)
            for label, source in (('existing-public', CPP), ('private-shared', overlay)):
                native = scratch/(compiler_name+'-'+label+'.o')
                binary = native.with_suffix('')
                compile_source(source, native)
                link = [*command, str(driver), str(native), '-o', str(binary)]
                record['commands'].append(link)
                built = subprocess.run(link, cwd=ROOT, capture_output=True, text=True, timeout=180)
                if built.returncode:
                    raise ValueError('Private value link failed: '+built.stderr[-6000:])
                result = subprocess.run([str(binary), str(fixture)], capture_output=True, text=True, timeout=180)
                record['variants'].append(dict(mode=label, exit_code=result.returncode, output=result.stdout.strip()))
                if result.returncode:
                    raise ValueError(label+' differs from original: '+(result.stdout+result.stderr)[-6000:])
            if record['variants'][0]['output'] != record['variants'][1]['output']:
                raise ValueError('Public value matrix coverage differs')
        report['passed'] = True
    except BaseException as error:
        report['error'] = repr(error)
        raise
    finally:
        report['source_before'] = {str(p): preimage.get(p) for p in sorted(inputs)}
        report['source_after'] = {str(p): original.sha(p) for p in sorted(inputs)}
        if report['source_after'] != report['source_before']:
            report.update(passed=False, source_drift=True)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
