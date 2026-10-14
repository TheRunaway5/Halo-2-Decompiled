#!/usr/bin/env python3
"""Package newly authored custom Slayer definitions with reproducible hashes."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile


def bundle(root, output):
    files={p.relative_to(root).as_posix():p.read_bytes() for p in sorted((root/'data/tags/variants').glob('*.json'))}
    if len(files)!=2:raise ValueError('Expected Fiesta Slayer and Infection Slayer definitions')
    for raw in files.values():
        row=json.loads(raw)
        if row['schema']!='halo2-native-slayer-variant-v1' or row['engine']!='slayer':raise ValueError('Invalid custom variant')
    manifest=dict(schema='halo2-custom-slayer-bundle-v1',files={name:hashlib.sha256(raw).hexdigest() for name,raw in files.items()},
        contents='Newly authored JSON definitions. Game maps/models/textures/code are supplied by the user installation.',
        installation='Extract beside the matching halo2_native executable, preserving data/tags/variants.',
        native_compatibility='halo2-native-queue-v10')
    files['data/tags/custom-slayer-bundle.json']=(json.dumps(manifest,indent=2,sort_keys=True)+'\n').encode()
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for name,raw in sorted(files.items()):
            record=zipfile.ZipInfo(name,date_time=(2026,10,13,0,0,0));record.compress_type=zipfile.ZIP_DEFLATED
            record.external_attr=0o100644<<16;archive.writestr(record,raw)
    verify(output)


def verify(path):
    with zipfile.ZipFile(path) as archive:
        manifest=json.loads(archive.read('data/tags/custom-slayer-bundle.json'))
        if set(archive.namelist())!=set(manifest['files'])|{'data/tags/custom-slayer-bundle.json'}:raise ValueError('Unexpected bundle payload')
        for name,digest in manifest['files'].items():
            if not name.startswith('data/tags/variants/') or '..' in Path(name).parts:raise ValueError('Invalid bundle path')
            if hashlib.sha256(archive.read(name)).hexdigest()!=digest:raise ValueError('Bundle hash mismatch')
    print('PASS custom Slayer bundle: Fiesta Slayer and Infection Slayer; authored definitions only')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
    parser.add_argument('--output',type=Path)
    parser.add_argument('--verify',type=Path)
    args=parser.parse_args()
    if args.verify:verify(args.verify)
    elif args.output:bundle(args.root.resolve(),args.output.resolve())
    else:parser.error('Use --output or --verify')


if __name__=='__main__':main()
