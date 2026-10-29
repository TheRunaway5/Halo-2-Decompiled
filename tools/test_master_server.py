#!/usr/bin/env python3
"""Run through run_test.py: build the Rust service ABI and test the backend."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def freeze_source(scratch, output):
    """Own one small backend/Rust cohort; borrow no mutable service modules."""
    source = scratch/'master-source'
    source.mkdir()
    for relative in ('multiplayer/master_server', 'rust/online_protocol', 'rust/online_security_ffi'):
        shutil.copytree(ROOT/relative, source/relative,
                        ignore=shutil.ignore_patterns('__pycache__', 'target', '*.so', '*.dylib', '*.dll'))
    # These numerical original fixtures are read by the actual backend tests.
    fixtures = ('tu15_party_privacy_original.json', 'multiplayer_engines_original_tu15.json',
                'tu15_matchmaking_readiness_original.json', 'tu15_party_countdown_original.json',
                'team_balance_tu15_original.json', 'matchmaking_assignment_original.json',
                'matchmaking_ratings_original.json', 'matchmaking_lifecycle_original.json',
                'matchmaking_skill_original.json', 'matchmaking_countdown_writer_original.json',
                'tu15_matchmaking_start_original.json', 'hopper_selection_tu15_original.json')
    for name in fixtures:
        target = source/'tests/fixtures'/name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT/'tests/fixtures'/name, target)
    for name in ('test_master_server.py', 'oracle_session_candidates.py', 'oracle_party_search_ranks.py'):
        target = source/'tools'/name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT/'tools'/name, target)
    files = {str(p.relative_to(source)): hashlib.sha256(p.read_bytes()).hexdigest()
             for p in source.rglob('*') if p.is_file()}
    drift = [name for name, pin in files.items() if hashlib.sha256((ROOT/name).read_bytes()).hexdigest() != pin]
    (output/'source-manifest.json').write_text(json.dumps({
        'source': str(source), 'scratch': str(scratch), 'files': files,
        'drift_during_freeze': drift, 'borrowed_service_modules': False,
        'retained_large_artifacts': []}, indent=2)+'\n')
    if drift:
        raise RuntimeError('Source changed while freezing backend cohort: '+', '.join(drift))
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'build/test-results/master-server')
    parser.add_argument('--freeze-source', action='store_true',
                        help='Test an owned copy of the backend and Rust sources with recorded hashes')
    parser.add_argument('--production-abi', action='store_true',
                        help='Exclude diagnostic input restoration; select tests using production entry paths only')
    parser.add_argument('--rust-test', action='append', default=[], metavar='NAME',
                        help='Run this online_protocol Rust integration test in the same source cohort')
    parser.add_argument('--rust-lib', action='store_true',
                        help='Also run the online_protocol private owner/unit tests in the same source cohort')
    parser.add_argument('tests', nargs='*')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    env = dict(os.environ, CARGO_TARGET_DIR=str(scratch/'cargo'), CARGO_INCREMENTAL='0',
               RUSTUP_TOOLCHAIN='1.96.0', RUSTUP_AUTO_INSTALL='0')
    env['HALO2_MATCHMAKING_TEST_INPUTS'] = '0' if args.production_abi else '1'
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    source = freeze_source(scratch, args.output) if args.freeze_source else ROOT
    commands = []
    if args.rust_test or args.rust_lib:
        command=['cargo','test','--locked','--offline','--jobs','2','--manifest-path',
                 str(source/'rust/online_protocol/Cargo.toml')]
        if not args.production_abi:
            command.extend(('--features','matchmaking-test-inputs'))
        if args.rust_lib:
            command.append('--lib')
        for name in args.rust_test:
            command.extend(('--test',name))
        commands.append(command)
    if not env.get('H2_SECURITY_LIBRARY'):
        command=['cargo','build','--locked','--release','--manifest-path',str(source/'rust/online_security_ffi/Cargo.toml')]
        if not args.production_abi:
            command.extend(('--features','matchmaking-test-inputs'))
        commands.append(command)
        filename = 'halo2_online_security_ffi.dll' if sys.platform=='win32' else 'libhalo2_online_security_ffi.'+('dylib' if sys.platform=='darwin' else 'so')
        env['H2_SECURITY_LIBRARY'] = str(scratch/'cargo/release'/filename)
    commands.append([sys.executable,'-B','-m','unittest',*args.tests] if args.tests else
                    [sys.executable,'-B','-m','unittest','discover','-s','multiplayer/master_server/tests','-t','.'])
    report = {'commands': [], 'passed': False, 'retained_large_artifacts': [],
              'source': str(source), 'scratch': str(scratch), 'source_frozen': args.freeze_source,
              'matchmaking_test_inputs': not args.production_abi}
    compiler = subprocess.run(['rustc','--version','--verbose'], cwd=source, env=env,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    report['compiler'] = {'returncode':compiler.returncode, 'identity':compiler.stdout.decode(errors='replace').strip()}
    for index, command in enumerate(commands):
        print('+ '+' '.join(command), flush=True)
        result = subprocess.run(command, cwd=source, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        output = result.stdout[-1024*1024:]
        (args.output/f'step-{index}.log').write_bytes(output)
        print(output.decode(errors='replace'), end='', flush=True)
        report['commands'].append({'argv': command, 'returncode': result.returncode})
        library = Path(env['H2_SECURITY_LIBRARY'])
        if library.is_file():
            report['rust_library'] = {'path':str(library), 'sha256':hashlib.sha256(library.read_bytes()).hexdigest(),
                                      'bytes':library.stat().st_size, 'retained':False,
                                      'borrowed_external':bool(os.environ.get('H2_SECURITY_LIBRARY'))}
        if result.returncode:
            (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
            return result.returncode
    report['passed']=True
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0


if __name__=='__main__':
    raise SystemExit(main())
