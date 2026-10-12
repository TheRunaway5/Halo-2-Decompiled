#!/usr/bin/env python3
"""Build and test a disposable source copy containing no original game files."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/code-only')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    report = {'scope': 'Fresh disposable source copy without assets, original executables, '
                       'analysis exports or downloaded reference tools.', 'files': {}}
    with tempfile.TemporaryDirectory(prefix='halo2-code-only-') as directory:
        source = Path(directory) / 'source'
        source.mkdir()
        for name in ('CMakeLists.txt', 'README.md'):
            shutil.copy2(ROOT / name, source / name)
        for name in ('src', 'include', 'shaders', 'resources', 'tests', 'tools'):
            shutil.copytree(ROOT / name, source / name,
                            ignore=shutil.ignore_patterns('reference', '__pycache__', '*.bin', '*.vp'))
        forbidden = {'.iso', '.xbe', '.map', '.lvl', '.bik', '.wav', '.zip', '.7z', '.bin', '.vp'}
        for path in sorted(source.rglob('*')):
            if path.is_file():
                if path.suffix.lower() in forbidden:
                    raise RuntimeError('Game-data blob in source copy: ' + str(path))
                report['files'][str(path.relative_to(source))] = hashlib.sha256(path.read_bytes()).hexdigest()
        assert not (source / 'assets').exists() and not (source / 'analysis').exists()
        build = Path(directory) / 'build'
        commands = [
            ('configure', ['cmake', '-S', str(source), '-B', str(build), '-DCMAKE_BUILD_TYPE=RelWithDebInfo']),
            ('build', ['cmake', '--build', str(build), '-j', '4']),
            ('tests', ['ctest', '--test-dir', str(build), '--output-on-failure']),
        ]
        env = os.environ.copy()
        env.setdefault('DISPLAY', ':0')
        report['steps'] = []
        for name, command in commands:
            log = output / (name + '.log')
            with log.open('w') as stream:
                result = subprocess.run(command, env=env, stdout=stream, stderr=subprocess.STDOUT)
            report['steps'].append({'name': name, 'command': command,
                                    'exit_code': result.returncode, 'log': str(log)})
            report['passed'] = result.returncode == 0
            (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
            print(name + ': ' + ('passed' if report['passed'] else 'FAILED'), flush=True)
            if not report['passed']:
                return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
