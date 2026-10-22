#!/usr/bin/env python3
"""Exercise the production prebuilt DSP architecture gate with real archives."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]


def run(command):
    return subprocess.run(command, check=True, text=True, capture_output=True)


def main():
    with tempfile.TemporaryDirectory(prefix='dsp-archives-', dir=os.environ['HALO2_TEST_SCRATCH']) as directory:
        work = Path(directory)
        source = work / 'symbol.c'
        source.write_text('int halo2_dsp_archive_probe(void){return 1;}\n')
        for arch in ('arm64', 'x86_64'):
            run(['cc', '-arch', arch, '-c', str(source), '-o', str(work / (arch + '.o'))])
            run(['ar', 'rcs', str(work / (arch + '.a')), str(work / (arch + '.o'))])
        universal = work / 'universal.a'
        run(['lipo', '-create', str(work / 'arm64.a'), str(work / 'x86_64.a'), '-output', str(universal)])
        for name, accepted in (('universal.a', True), ('arm64.a', False), ('x86_64.a', False)):
            script = work / 'verify.cmake'
            script.write_text('set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")\n'
                f'include("{ROOT}/cmake/macos/Dsp.cmake")\n'
                f'halo2_macos_verify_dsp_archive("{work / name}")\n')
            result = subprocess.run(['cmake', '-P', str(script)], text=True, capture_output=True)
            if (result.returncode == 0) != accepted:
                details = []
                for arguments in (['-archs'], ['-verify_arch', 'arm64'], ['-verify_arch', 'x86_64'],
                                  ['-verify_arch', 'arm64', 'x86_64']):
                    probe = subprocess.run(['lipo', str(work / name), *arguments], text=True, capture_output=True)
                    details.append(dict(arguments=arguments, status=probe.returncode,
                                        stdout=probe.stdout, stderr=probe.stderr))
                raise RuntimeError(f'Production DSP architecture admission differs for {name}: {result.stderr}; native diagnostics: {details}')
            if not accepted and 'DSP archive lacks the requested macOS architectures' not in result.stderr:
                raise RuntimeError('Thin DSP rejection came from an unrelated failure: ' + result.stderr)
        print('PASS production DSP architecture gate: universal static archive admitted; each thin archive rejected')


if __name__ == '__main__':
    main()
