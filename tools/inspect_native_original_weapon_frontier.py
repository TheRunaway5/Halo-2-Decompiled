#!/usr/bin/env python3
"""Inspect a native firing continuation from unchanged original entry state.

This diagnostic permits the declared synchronous native sound-resource policy.
It does not equate that policy to the original queue, scheduler or device path.
An unavailable owner remains a real failed full-call result.
"""
import argparse
import hashlib
import json
import mmap
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time
from original_xbox_pose_case import ROOT, cpu, physical_ram, PageTableMemory, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', type=Path, required=True)
    parser.add_argument('--library-build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--extra-source', type=Path, action='append', default=[])
    parser.add_argument('--replay-source', type=Path,
        help='Use an explicitly retained diagnostic adapter; default is the current production-owner adapter')
    parser.add_argument('--wait-for-application-link', type=int, default=0,
        help='Wait at most this many seconds for a stable application ELF before copying disposable linker inputs')
    parser.add_argument('--build-report', type=Path,
        help='Observe a parent runner report and stop promptly if its build fails')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    output = args.output.absolute()
    output.mkdir(exist_ok=False)
    (output / 'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch))) + '\n')
    case = args.case.absolute()
    call = json.loads((case / 'call.json').read_text())
    assert call['entry'] == 0x1030f0 and not call['guest_code_patches']
    assert not call['guest_memory_writes'] and not call['guest_register_writes']
    initial = cpu(case, 'before')
    (scratch / 'cpu.json').write_text(json.dumps(initial) + '\n')
    build = args.library_build.absolute()
    if args.wait_for_application_link:
        from validate_disposable_weapon_runtime_tu15 import has_writer
        application = build / 'halo2_native'
        deadline = time.monotonic() + args.wait_for_application_link
        previous = None
        while time.monotonic() < deadline:
            if args.build_report and args.build_report.is_file():
                parent = json.loads(args.build_report.read_text())
                if parent.get('status') in ('failed', 'interrupted'):
                    failure = dict(passed=False, status='cancelled after parent build failure',
                        parent_report=str(args.build_report.absolute()), parent_status=parent['status'],
                        runner_scratch=str(scratch), native_probe_executed=False, large_artifacts_retained=[])
                    (output / 'report.json').write_text(json.dumps(failure, indent=2) + '\n')
                    raise RuntimeError('The parent build failed before native firing validation')
            if application.is_file() and os.access(application, os.X_OK):
                stat = application.stat()
                observed = (stat.st_size, stat.st_mtime_ns)
                if observed == previous and stat.st_size and not has_writer(application):
                    break
                previous = observed
            time.sleep(1)
        else:
            raise TimeoutError('The declared current application link wait expired')
    recipe = build / 'CMakeFiles/halo2_native.dir/link.txt'
    tokens = shlex.split(recipe.read_text())
    libraries = []
    missing = []
    for token in tokens[tokens.index('-o') + 2:]:
        if token.startswith('-Wl,-rpath'):
            continue
        if not token.startswith('-') and not Path(token).is_absolute():
            token = str(build / token)
        if token.endswith('.a') and not Path(token).is_file():
            missing.append(token)
            continue
        libraries.append(token)
    files = sorted({Path(token) for token in libraries if not token.startswith('-') and Path(token).is_file()})
    pins = {str(path): sha(path) for path in files}
    # The parent CMake runner may finish and remove its build during this probe.
    # Own only its referenced linker inputs here, under the same cleanup lease.
    private = scratch / 'link-inputs'
    private.mkdir()
    copies = {}
    for index, path in enumerate(files):
        if path.is_relative_to(build):
            target = private / str(index) / path.name
            target.parent.mkdir()
            shutil.copyfile(path, target)
            assert sha(target) == pins[str(path)] == sha(path), path
            copies[str(path)] = str(target)
    libraries = [copies.get(token, token) for token in libraries]
    with physical_ram(case, 'before') as original:
        (scratch / 'ram.bin').write_bytes(original[:])
    source = args.replay_source.absolute() if args.replay_source else ROOT / 'tools/replay_original_live_first_person.cpp'
    dependency = scratch / 'probe.d'
    binary = scratch / 'native-firing-frontier'
    command = ['c++', '-std=c++20', '-O2', '-ffp-contract=off', '-ffunction-sections', '-fdata-sections',
        '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), '-MMD', '-MF', str(dependency), str(source),
        *[str(path.absolute()) for path in args.extra_source],
        '-Wl,--gc-sections', '-Wl,--as-needed', '-Wl,--start-group', *libraries, '-Wl,--end-group', '-o', str(binary)]
    (output / 'build-command.json').write_text(json.dumps(dict(command=command, absent_archives=missing,
        policy='Missing archive names omitted; every referenced native symbol must link without a replacement'), indent=2) + '\n')
    compiled = subprocess.run(command, capture_output=True, text=True)
    (output / 'compile.log').write_text((compiled.stdout + compiled.stderr)[-65536:])
    assert compiled.returncode == 0, compiled.stderr[-3000:]
    environment = os.environ.copy()
    environment['LD_LIBRARY_PATH'] = ':'.join(sorted({str(Path(copies.get(str(path), str(path))).parent) for path in files if '.so' in path.name}) + [environment.get('LD_LIBRARY_PATH', '')])
    run_command = [str(binary), str(scratch / 'cpu.json'), str(scratch / 'ram.bin'),
        str(scratch / 'native.json'), str(scratch / 'native-ram.bin'), hex(call['entry']),
        str(ROOT / 'assets/disc/maps/lockout.map'), str(ROOT / 'assets/disc/maps')]
    completed = subprocess.run(run_command, env=environment, capture_output=True, text=True, timeout=90)
    (output / 'run.log').write_text((completed.stdout + completed.stderr)[-65536:])
    native = json.loads((scratch / 'native.json').read_text())
    comparisons = []
    with (scratch / 'native-ram.bin').open('rb') as file, mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as bank, physical_ram(case, 'after') as reference:
        actual = PageTableMemory(bank, initial)
        expected = PageTableMemory(reference, cpu(case, 'after'))
        spans = [(row['name'], row['address'], row['bytes']) for row in call['observed_objects']]
        fp = call['first_person_state']
        spans.append(('complete_first_person_state', fp['address'], fp['bytes']))
        for name, address, size in spans:
            left, right = actual.read(address, size), expected.read(address, size)
            comparisons.append(dict(name=name, address=address, bytes=size,
                equal=left == right, differing_bytes=sum(a != b for a, b in zip(left, right)),
                native_sha256=hashlib.sha256(left).hexdigest(), original_sha256=hashlib.sha256(right).hexdigest()))
    for path, pin in pins.items():
        assert sha(copies.get(path, path)) == pin, path
    source_files = dependency.read_text().replace('\\\n', ' ').split(':', 1)[1].split()
    dependencies = {str(Path(path).absolute()): sha(path) for path in source_files if Path(path).is_file()}
    for path in [source, *args.extra_source]:
        dependencies[str(path.absolute())] = sha(path)
    result = dict(passed=native['returned'] and all(row['equal'] for row in comparisons), diagnostic_completed=True,
        full_native_call_returned=native['returned'], native_error=native['error'], last_instruction=native['last_instruction'],
        last_external_calls=native['external_calls'][-12:], native_instruction_count=len(native['instructions']),
        native_store_count=len(native['memory_writes']), comparisons=comparisons,
        actual_resource_reads=native['actual_resource_reads'], completed_resource_receipts=native['native_completed_resource_receipts'],
        declared_native_policy='Actual SourceRuntimeResources snapshots, production sound-cache read owner, native completed receipts; synchronous native adaptation rather than Xbox async timing',
        source_or_original_entry_state_patches=[], expected_bytes_or_original_gameplay_returns_supplied=False,
        original_call_sha256=sha(case / 'call.json'), current_library_files_sha256=pins,
        source_files_sha256=dependencies, tool_sha256=sha(Path(__file__)),
        absent_unreferenced_archives=missing, compile_command=command, run_command=run_command,
        exit_code=completed.returncode, runner_scratch=str(scratch),
        whole_current_runtime_verified=False, original_independent_full_call_verified=False,
        physical_xbox_verified=False, full_weapon_parity=False, large_artifacts_retained=[])
    (output / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    raw = (scratch / 'native.json').read_bytes()
    if len(raw) <= 8 * 1024 * 1024:
        (output / 'native-observations.json').write_bytes(raw)
    else:
        (output / 'native-observation-size.json').write_text(json.dumps(dict(
            bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest(), retained=False, bound=8*1024*1024)) + '\n')
    for path in (Path(__file__), source, ROOT / 'tools/original_xbox_pose_case.py'):
        shutil.copyfile(path, output / path.name)
    print(json.dumps({key: result[key] for key in ('passed', 'diagnostic_completed', 'full_native_call_returned',
        'native_error', 'last_instruction', 'native_instruction_count', 'actual_resource_reads', 'runner_scratch')}), flush=True)
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
