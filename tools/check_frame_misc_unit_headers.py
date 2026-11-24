#!/usr/bin/env python3
"""Compile, per generated unit, every adapter header the binders prepend.

The frame misc binder (tools/native_frame_misc_replacements.py) prepends
each bound owner's adapter header to its generated unit; other binders
prepend theirs too. Two ports that share a namespace, a constant or a
switch name only fail when they land in the same unit, which a full perf
build finds late. This check compiles, for each generated unit, the frame
misc headers of its bound owners together with the other binders' headers
the last perf build prepended to it (build-perf-4k60/native-operands), in
one translation unit with -fsyntax-only.

Usage: tools/check_frame_misc_unit_headers.py [--build build-perf-4k60]
Exit status 1 when any unit fails.
"""
import argparse
import pathlib
import subprocess
import sys
import tempfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import native_frame_misc_replacements as binder  # noqa: E402

MAPPING = 'std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image)'


def unit_headers(unit, built):
    """The include lines the binders prepend to one generated unit."""
    text = unit.read_text(errors='replace')
    if MAPPING not in text:
        return [], []
    names = [name for name in binder.BOUND if binder.admitted(text, name)]
    includes = []
    copy = built / unit.relative_to('src')
    if copy.exists():
        for line in copy.read_text(errors='replace').split('\n')[:400]:
            if not line.startswith('#include "') or line.startswith('#include "halo2/'):
                continue
            header = line[len('#include "'):].split('"')[0]
            if (pathlib.Path('src') / header).exists() and line + '\n' not in includes:
                includes.append(line + '\n')
    for name in names:
        include = binder.OWNERS[name]['include']
        if include not in includes:
            includes.append(include)
    return names, includes


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--build', default='build-perf-4k60')
    args = parser.parse_args()
    built = pathlib.Path(args.build) / 'native-operands'
    failing = 0
    with tempfile.TemporaryDirectory(prefix='frame-misc-unit-headers-') as scratch:
        unit_check = pathlib.Path(scratch) / 'unit_check.cpp'
        for unit in sorted(pathlib.Path('src').rglob('*_generated.cpp')):
            names, includes = unit_headers(unit, built)
            if not names:
                continue
            unit_check.write_text(''.join(includes))
            result = subprocess.run(['c++', '-std=c++20', '-fsyntax-only', '-Iinclude', '-Isrc', str(unit_check)],
                                    capture_output=True, text=True)
            status = 'FAIL' if result.returncode else 'ok  '
            print(f'{status} {unit} ({len(names)} owners, {len(includes)} headers)')
            if result.returncode:
                failing += 1
                for line in [l for l in result.stderr.split('\n') if ' error' in l][:4]:
                    print('   ', line[:220])
    print('units failing:', failing)
    return 1 if failing else 0


if __name__ == '__main__':
    sys.exit(main())
