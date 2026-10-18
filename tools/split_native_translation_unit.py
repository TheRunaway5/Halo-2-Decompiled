#!/usr/bin/env python3
"""Partition emitted native bodies for bounded-memory C++ compilation.

The generator's canonical source stays unchanged. Only C++ linkage changes:
one named detail namespace shares the state type, inline helpers share their
ODR state, and each recovered body has exactly one definition. Every byte
inside each body is checked against the canonical input before publication.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re


BODY = re.compile(
    r'^static void (run_[0-9a-f]+)\(WorldState &s\) \{\n.*?'
    r'^throw std::runtime_error\("Original solver fell outside authenticated CFG"\);\n}\n',
    re.M | re.S)
HELPER = re.compile(
    r'^(?=(?:long double|__m128|std::uint\d+_t|void|float|double|bool|unsigned|int)\s+\w+\([^;\n]*\)\s*\{)',
    re.M)


def sha(value):
    return hashlib.sha256(value.encode()).hexdigest()


def split(source, directory, count):
    if not 1 <= count <= 128:
        raise ValueError('Native translation partition count must be 1..128')
    text = source.read_text()
    bodies = list(BODY.finditer(text))
    declarations = re.findall(r'^static void (run_[0-9a-f]+)\(WorldState &\);$', text, re.M)
    if not bodies or declarations != [m[1] for m in bodies]:
        raise ValueError('Unsupported native body/declaration correspondence')
    for a, b in zip(bodies, bodies[1:]):
        if text[a.end():b.start()].strip():
            raise ValueError('Unowned source text between recovered bodies')
    prefix, footer = text[:bodies[0].start()], text[bodies[-1].end():]
    if prefix.count('namespace {\n') != 1 or not footer.startswith('} // namespace\n'):
        raise ValueError('Unsupported native namespace boundary')
    if re.search(r'^(?:static (?!void (?:run_|dispatch))|thread_local )', prefix, re.M):
        raise ValueError('Native partition has unowned namespace static state')
    module = 'native_split_' + re.sub(r'\W', '_', source.stem)
    prefix = prefix.replace('namespace {\n', 'namespace ' + module + ' {\n', 1)
    prefix = prefix.replace('static void run_', 'void run_')
    prefix = prefix.replace('static void dispatch(', 'inline void dispatch(')
    prefix = HELPER.sub('inline ', prefix)
    # Helpers are inline rather than separate TU-local copies: a helper's
    # function-local static must remain one object across all body partitions.
    common = '#pragma once\n' + prefix + '\n} // namespace detail\n} // namespace halo2\n'
    buckets, sizes = [[] for _ in range(count)], [0] * count
    rows = []
    for m in sorted(bodies, key=lambda m: (-len(m[0]), m.start())):
        index = min(range(count), key=lambda n: (sizes[n], n))
        body = m[0].removeprefix('static ')
        if body[body.index('\n'):] != m[0][m[0].index('\n'):]:
            raise ValueError('Recovered native instruction body changed')
        buckets[index].append((m.start(), body))
        sizes[index] += len(body)
        rows.append(dict(entry=m[1], chunk=index, body_sha256=sha(body[body.index('\n'):]),
                         original_bytes=len(m[0]), instruction_body_unchanged=True))
    directory.mkdir(parents=True, exist_ok=True)
    (directory/'common.hpp').write_text(common)
    for n, bucket in enumerate(buckets):
        value = '#include "common.hpp"\nnamespace halo2 { namespace ' + module + ' {\n'
        for _, body in sorted(bucket):
            value += body + '\n'
        value += '} }\n'
        (directory/f'body_{n:03d}.cpp').write_text(value)
    router = '#include "common.hpp"\nnamespace halo2 {\nusing namespace ' + module + ';\n'
    router += footer.removeprefix('} // namespace\n')
    (directory/'router.cpp').write_text(router)
    manifest = dict(schema='native-translation-partition-v1', source=str(source),
        source_sha256=sha(text), body_count=len(bodies), chunk_count=count,
        chunk_body_bytes=sizes, shared_prefix_bytes=len(common),
        body_bytes_unchanged=True, entries=sorted(rows, key=lambda r:r['entry']))
    (directory/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    return manifest


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--count', type=int, default=32)
    a = p.parse_args()
    report = split(a.source, a.output, a.count)
    print(json.dumps({k:v for k,v in report.items() if k != 'entries'}))


if __name__ == '__main__':
    main()
