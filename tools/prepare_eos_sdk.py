#!/usr/bin/env python3
"""Extract only official EOS headers and the requested platform binary into a build."""
import argparse
from pathlib import Path
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--binary', required=True)
    parser.add_argument('--import-library', default='')
    args = parser.parse_args()
    with zipfile.ZipFile(args.archive) as archive:
        wanted = [item for item in archive.infolist() if not item.is_dir() and (
            item.filename.startswith('SDK/Include/') or item.filename == 'SDK/Bin/' + args.binary or
            (args.import_library and item.filename == 'SDK/Lib/' + args.import_library))]
        for item in wanted:
            path = args.output / item.filename
            if not path.resolve().is_relative_to(args.output.resolve()):
                raise ValueError('Unsafe SDK archive path')
            path.parent.mkdir(parents=True, exist_ok=True)
            if not path.exists() or path.stat().st_size != item.file_size:
                path.write_bytes(archive.read(item))


if __name__ == '__main__':
    main()
