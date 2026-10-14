#!/usr/bin/env python3
"""Verify Vista menu reference coverage and standard SVG/DDS replay.

Passing this check proves reproducibility and recorded coverage. It does not
turn an unfinished reconstruction into completed or visually proven artwork.
"""
import argparse
import json
from pathlib import Path, PurePosixPath
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image

from audit_rendering import file_sha
from vista_prompt_svg import render

ROOT=Path(__file__).resolve().parents[1]


def local(pack,relative):
    path=PurePosixPath(relative)
    if path.is_absolute() or '..' in path.parts or '\\' in relative:
        raise ValueError('Unsafe menu artifact path: '+relative)
    result=pack/path
    if not result.resolve().is_relative_to(pack.resolve()):raise ValueError('Escaping menu artifact path')
    return result


def svg_shapes(vector):
    shapes=0
    for node in ET.parse(vector).getroot().iter():
        tag=node.tag.rsplit('}',1)[-1]
        assert tag not in ('image','foreignObject','text','script'),'Raster/font/script substitution in SVG'
        assert all(not name.endswith('href') for name in node.attrib),'External SVG reference'
        assert all('data:' not in str(v) for v in node.attrib.values()),'Embedded pixel payload'
        if tag in ('path','rect','ellipse','circle','polygon'):shapes+=1
        if tag in ('linearGradient','radialGradient'):assert len(node)<=16,'Pixel palette disguised as gradient'
    return shapes


def verify(pack):
    reference=json.loads((pack/'reference.json').read_text())
    report=json.loads((pack/'reconstruction.json').read_text())
    assert report['reference_sha256']==file_sha(pack/'reference.json'),'Reference inventory changed'
    expected={(cache,b['name'],f['frame'],f['page']) for cache,bs in reference['owners'].items() for b in bs for f in b['frames']}
    actual={(r['cache'],r['name'],r['frame'],r['page']) for r in report['records']}
    assert actual==expected and len(actual)==len(report['records']),'Source bank/frame coverage changed'
    for key,page in reference['pages'].items():
        source=local(pack,page['png'])
        assert file_sha(source)==page['png_sha256'],'Original reference changed'
        with Image.open(source) as image:assert image.size==(page['width'],page['height'])
    replayed=set();unfinished=set();shapes=0
    for row in report['records']:
        key=row['page'];record=row['reconstruction']
        if record is None:
            assert row['state']=='unfinished'
            unfinished.add(key);continue
        assert row['state']=='authored-needs-review','Draft incorrectly marked final'
        if key in replayed:continue
        replayed.add(key)
        vector=local(pack,record['vector']);dds=local(pack,record['dds'])
        assert file_sha(vector)==record['vector_sha256'] and file_sha(dds)==record['dds_sha256']
        shapes+=svg_shapes(vector)
        raster=render(vector,record['density'])
        with Image.open(dds) as stored:
            assert stored.size==tuple(record['physical'])==raster.size
            assert np.array_equal(np.asarray(stored.convert('RGBA')),np.asarray(raster)),'DDS differs from standard SVG replay'
    assert not report['complete'],'Unreviewed draft must not be declared complete'
    return dict(passed=True,complete=False,source_caches=len(reference['sources']),source_bindings=len(actual),
                unique_reference_pages=len(reference['pages']),svg_dds_pairs_replayed=len(replayed),
                unfinished_unique_pages=len(unfinished),vector_shapes=shapes,
                unavailable_caches=reference.get('unavailable_caches',{}),
                claim='Reference coverage and exact standard SVG/DDS replay only; visual review and full reconstruction remain open.')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pack',type=Path,default=ROOT/'assets/vista-menu-svg-4k')
    p.add_argument('--report',type=Path)
    args=p.parse_args();result=verify(args.pack)
    text=json.dumps(result,indent=2)+'\n'
    if args.report:args.report.write_text(text)
    print(text,end='')


if __name__=='__main__':main()
