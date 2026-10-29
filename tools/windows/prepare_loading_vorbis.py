#!/usr/bin/env python3
"""Prepare the private, source-pinned loading decoder in its CMake build tree."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import signal
import sys
import tarfile
import tempfile
import urllib.request

SOURCE_SHA = 'b33cc4934322bcbf6efcbacf49e3ca01aadbea4114ec9589d1b1e9d20f72954b'
SOURCE_NAME = 'libvorbis-1.3.7'
SOURCE_URLS = (
    'https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.xz',
    'https://ftp.osuosl.org/pub/xiph/releases/vorbis/libvorbis-1.3.7.tar.xz',
)
HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def patch_block(text):
    # Keep upstream's state machine and native struct layout. In particular,
    # the small/large branch continues copying from i=n0 after overlap.
    pattern = (r'for\(i=0;i<(n[01]);i\+\+\)\n(\s*)'
               r'pcm\[i\]=pcm\[i\]\*w\[\1-i-1\] \+ ?p\[i\]\*w\[i\];')

    def replacement(match):
        count, indent = match.groups()
        return (f'h2_canonical_vorbis_overlap(pcm, p, w, {count});\n'
                f'{indent}i={count};')

    result, count = re.subn(pattern, replacement, text)
    if count != 4:
        raise RuntimeError('Pinned Vorbis overlap source differs from the reviewed four loops')
    return '#include "canonical_vorbis_kernels.h"\n' + result


def patch_mdct(text):
    # The replacement reads an ordinary scalar lookup. Copy fields explicitly
    # rather than aliasing unrelated struct types or assuming a host ABI.
    if text.count('void mdct_backward(') != 1:
        raise RuntimeError('Pinned Vorbis inverse transform declaration differs')
    return ('#define mdct_backward h2_original_mdct_backward\n' + text + '''
#undef mdct_backward
#include "canonical_vorbis_kernels.h"
void mdct_backward(mdct_lookup *init, DATA_TYPE *in, DATA_TYPE *out) {
    H2CanonicalVorbisMdctLookup lookup = {
        init->n, init->log2n, init->trig, init->bitrev, init->scale
    };
    h2_canonical_vorbis_mdct_backward(&lookup, in, out);
}
''')


def prepare(output):
    if output.is_symlink():
        raise RuntimeError('Loading Vorbis source output must not be a symbolic link')
    output = output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists():
        marker = output / 'manifest.json'
        # CMake/Ninja can create the generated include-directory skeleton
        # before the preparation command runs. Empty directories own no data.
        empty_skeleton = not any(not entry.is_dir() or entry.is_symlink()
                                 for entry in output.rglob('*'))
        if not empty_skeleton and (not marker.is_file() or
                json.loads(marker.read_text()).get('schema') != 'H2_WINDOWS_LOADING_VORBIS_SOURCE_V1'):
            raise RuntimeError('Refusing to replace an unowned loading Vorbis source directory')
    with tempfile.TemporaryDirectory(prefix='loading-vorbis-', dir=output.parent) as owned:
        owned = Path(owned)
        archive = owned / (SOURCE_NAME + '.tar.xz')
        downloaded = None
        for url in SOURCE_URLS:
            try:
                request = urllib.request.Request(url, headers={'User-Agent': 'Halo2-loading-decoder-build'})
                with urllib.request.urlopen(request, timeout=45) as response, archive.open('wb') as stream:
                    shutil.copyfileobj(response, stream)
                downloaded = url
                break
            except (OSError, TimeoutError):
                archive.unlink(missing_ok=True)
                if url == SOURCE_URLS[-1]:
                    raise
        if sha(archive) != SOURCE_SHA:
            raise RuntimeError('Pinned loading Vorbis source checksum mismatch')
        with tarfile.open(archive) as stream:
            stream.extractall(owned, filter='data')
        source = owned / SOURCE_NAME
        original = {name: sha(source / 'lib' / name) for name in ('mdct.c', 'block.c')}
        for name, patch in (('mdct.c', patch_mdct), ('block.c', patch_block)):
            path = source / 'lib' / name
            path.write_text(patch(path.read_text()))
        helpers = {}
        for name in ('canonical_vorbis_kernels.c', 'canonical_vorbis_kernels_mdct.c',
                     'canonical_vorbis_kernels.h'):
            shutil.copyfile(HERE / name, source / 'lib' / name)
            helpers[name] = sha(source / 'lib' / name)
        manifest = dict(schema='H2_WINDOWS_LOADING_VORBIS_SOURCE_V1',
                        version='1.3.7', source=dict(url=downloaded, sha256=SOURCE_SHA),
                        original_sources=original, helpers=helpers,
                        patched_sources={name: sha(source / 'lib' / name) for name in original},
                        preparer_sha256=sha(Path(__file__)),
                        arithmetic='explicit binary32 operation order; only explicit fmaf contracts')
        (source / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
        if output.exists():
            shutil.rmtree(output)
        source.rename(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(143))
    prepare(args.output)


if __name__ == '__main__':
    main()
