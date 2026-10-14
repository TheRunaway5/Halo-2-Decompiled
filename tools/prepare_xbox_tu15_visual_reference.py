#!/usr/bin/env python3
"""Package owned Xbox disc assets with the unchanged owned TU1.5 executable.

The output is an isolated emulator reference disc, not a physical Xbox capture.
extract-xiso's media patch is explicitly disabled. Every packaged file is read
back and hashed against its declared input. No Vista resource is accepted.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
from disc import Disc, inspect_xbe

TU15_SHA256='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'

def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disc',type=Path,required=True)
    parser.add_argument('--tu15',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    args.output=args.output.resolve()
    args.disc=args.disc.resolve()
    args.tu15=args.tu15.resolve()
    assert sha(args.tu15)==TU15_SHA256,'Not the authenticated Xbox TU1.5 executable'
    original=json.loads((args.disc/'disc-manifest.json').read_text())
    args.output.mkdir(parents=True,exist_ok=False)
    staging=args.output/'disc'
    staging.mkdir()
    inputs={}
    for entry in original['files']:
        relative=Path(entry['path'])
        assert not relative.is_absolute() and '..' not in relative.parts
        source=args.tu15 if str(relative).lower()=='default.xbe' else args.disc/relative
        expected=TU15_SHA256 if source==args.tu15 else entry['sha256']
        assert sha(source)==expected,relative
        target=staging/relative
        target.parent.mkdir(parents=True,exist_ok=True)
        subprocess.run(['cp','--reflink=auto','--',str(source),str(target)],check=True)
        assert sha(target)==expected
        inputs[str(relative).casefold()]={'source':str(source),'sha256':expected,'bytes':target.stat().st_size}
    image=args.output/'xbox-tu15.iso'
    subprocess.run(['extract-xiso','-m','-q','-c',str(staging),str(image)],check=True)
    with Disc(image) as disc:
        entries=disc.entries()
        assert set(inputs)=={e['path'].casefold() for e in entries}
        for entry in entries:
            expected=inputs[entry['path'].casefold()]
            assert entry['size']==expected['bytes']
            disc.file.seek(entry['offset'])
            digest=hashlib.sha256()
            remaining=entry['size']
            while remaining:
                raw=disc.file.read(min(1024*1024,remaining))
                assert raw
                digest.update(raw)
                remaining-=len(raw)
            assert digest.hexdigest()==expected['sha256'],entry['path']
    report=dict(reference='OG Xbox TU1.5',purpose='private original-execution visual reference',
        physical_xbox_capture=False,vista_resources=False,xbe_media_patched=False,
        original_disc_manifest_sha256=sha(args.disc/'disc-manifest.json'),
        executable=inspect_xbe(args.tu15),image=str(image),image_sha256=sha(image),
        files=inputs,all_packaged_bytes_verified=True,tool_sha256=sha(Path(__file__)))
    (args.output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(files=len(inputs),executable_sha256=TU15_SHA256,image=str(image),all_packaged_bytes_verified=True)))

if __name__=='__main__':
    main()
