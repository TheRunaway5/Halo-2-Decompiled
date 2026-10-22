#!/usr/bin/env python3
"""Repack authenticated original aspect readbacks inside an owned test tree."""
from __future__ import annotations
import hashlib
import json
import os
from pathlib import Path
import re
import sys

ROOT=Path(__file__).resolve().parents[3]

def prepare(destination: Path):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    destination=destination.resolve()
    if not destination.is_relative_to(scratch) or destination==scratch:
        raise ValueError('Reference copies must be inside owned HALO2_TEST_SCRATCH')
    entries=re.findall(r'"(analysis/[^"|]+)\|(\d+)\|([0-9a-f]{64})"',
                       (ROOT/'cmake/SourceSkyStencilReference.cmake').read_text())
    if len(entries)!=12:
        raise ValueError('Expected exactly twelve pinned source reference files')
    receipt=[]
    for relative,size,expected in entries:
        original=ROOT/relative
        if original.is_file():
            data=original.read_bytes();source=relative
        else:
            match=re.fullmatch(r'(8097|8102|8110)-(before|after)-packed-d24s8.bin',original.name)
            if not match:
                raise FileNotFoundError(original)
            event,phase=match.groups()
            directory='original20-moon-stencil' if event=='8110' else 'original20-sky-stencil'
            source=f'analysis/multiplayer-shading/{directory}/{event}-{phase}-depth-d24s8.raw'
            report=json.loads((ROOT/f'analysis/multiplayer-shading/{directory}/report.json').read_text())
            metadata=next(row for row in report['snapshots'] if row['file']==Path(source).name)
            raw=(ROOT/source).read_bytes()
            if hashlib.sha256(raw).hexdigest()!=metadata['sha256'] or len(raw)!=640*480*5 or any(raw[640*480*4:]):
                raise ValueError('Original replay readback identity/layout changed: '+source)
            data=raw[:640*480*4]
        if len(data)!=int(size) or hashlib.sha256(data).hexdigest()!=expected:
            raise ValueError('Pinned sky reference identity changed: '+relative)
        target=destination/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
        receipt.append({'path':relative,'bytes':len(data),'sha256':expected,'source':source})
    (destination/'reference-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return receipt

if __name__=='__main__':
    prepare(Path(sys.argv[1]))
