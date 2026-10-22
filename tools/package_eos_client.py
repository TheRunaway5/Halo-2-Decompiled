#!/usr/bin/env python3
"""Stage shared untrusted EOS settings; device storage stays on each PC."""
import argparse
import os
from pathlib import Path
import tempfile

KEYS = ('H2_EOS_PRODUCT_ID', 'H2_EOS_SANDBOX_ID', 'H2_EOS_DEPLOYMENT_ID',
        'H2_EOS_CLIENT_ID', 'H2_EOS_CLIENT_SECRET')


def package(source, destination):
    contents = Path(source).read_text()
    if len(contents) > 16384:
        raise ValueError('EOS client configuration is too large')
    values = {}
    for line in contents.splitlines():
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        name, separator, value = line.partition('=')
        if (not separator or name not in (*KEYS, 'H2_EOS_CACHE_DIR', 'H2_EOS_ENABLED', 'H2_EOS_PROFILES')
                or name in values or not value or len(value) > 4096
                or any(ord(c) < 32 or ord(c) == 127 for c in value)):
            raise ValueError('Invalid untrusted EOS client configuration')
        values[name] = value
    if not all(name in values for name in KEYS):
        raise ValueError('Missing shared EOS game client settings')
    for name in KEYS[:3]:
        if len(values[name]) != 32 or any(c not in '0123456789abcdef' for c in values[name]):
            raise ValueError('Invalid EOS product identifiers')
    encoded = '# Shared untrusted game client. Never place publisher credentials here.\n'
    encoded += ''.join(name + '=' + values[name] + '\n' for name in KEYS)
    encoded += 'H2_EOS_ENABLED=1\nH2_EOS_PROFILES=1\n'
    destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    name = None
    try:
        with tempfile.NamedTemporaryFile(dir=destination.parent, prefix='.eos-client-', delete=False) as stream:
            name = Path(stream.name)
            stream.write(encoded.encode())
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(name, destination)
    finally:
        if name is not None:
            name.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        package(args.source, args.output)
    except (OSError, ValueError):
        parser.exit(1, 'Unable to package the untrusted EOS game client settings.\n')


if __name__ == '__main__':
    main()
