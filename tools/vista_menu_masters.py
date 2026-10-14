#!/usr/bin/env python3
"""Package authored menu SVG masters and replay their high-resolution DDS files.

This source package contains the new vector artwork, generated bitmaps and
source identity records. It does not contain original raster references,
MCC imports, fonts or an installation/activation command. Drafts remain drafts.
"""
import argparse
import json
import shutil
from pathlib import Path

import numpy as np
from PIL import Image

from audit_rendering import file_sha
from verify_vista_menus import local,svg_shapes
from vista_prompt_source import write_dds
from vista_prompt_svg import render

ROOT=Path(__file__).resolve().parents[1]
DEFAULT=ROOT/'resources/ui-menu-masters'


def export(pack,destination):
    if destination.exists():
        raise ValueError('Use a fresh destination; source masters are never deleted or overwritten implicitly')
    reference=json.loads((pack/'reference.json').read_text())
    report=json.loads((pack/'reconstruction.json').read_text())
    assert report['reference_sha256']==file_sha(pack/'reference.json')
    assert report['complete'] is False
    pages={};paused=set()
    for row in report['records']:
        record=row['reconstruction']
        if not record:continue
        if record['family'].startswith('emblem-'):
            paused.add(row['page']);continue
        key=row['page'];source=reference['pages'][key]
        if key not in pages:
            item={k:record[k] for k in ('family','density','physical','vector_sha256','dds_sha256')}
            item.update(logical=[source['width'],source['height']],source_png_sha256=source['png_sha256'],
                        vector=f'svg/{key}.svg',dds=f'bitmaps/{key}.dds',state='authored-needs-review',bindings=[])
            if record.get('art_references'):item['art_references']=record['art_references']
            for field in ('vector','dds'):
                original=local(pack,record[field])
                assert file_sha(original)==record[field+'_sha256']
                target=local(destination,item[field]);target.parent.mkdir(parents=True,exist_ok=True)
                shutil.copyfile(original,target)
            pages[key]=item
        bank=next(b for b in reference['owners'][row['cache']] if b['name']==row['name'])
        frame=next(f for f in bank['frames'] if f['frame']==row['frame'])
        pages[key]['bindings'].append({k:row[k] for k in ('cache','name','frame')}|
            {k:frame[k] for k in ('dds_sha256','registration') if k in frame})
    manifest=dict(schema='halo2-vista-menu-masters-v1',complete=False,
        reference_inventory_sha256=report['reference_sha256'],source_caches=reference['sources'],
        pages=pages,coverage=dict(source_unique_pages=len(reference['pages']),
            bundled_unique_pages=len(pages),paused_emblem_drafts=len(paused),
            unfinished_unique_pages=report['summary']['unfinished_unique_pages']),
        policy='Authored review drafts only. No raster references, embedded images, autotrace or font substitution. No automatic activation.',
        rights_review='Required. Faithful design recreation; this package does not claim redistribution clearance.')
    (destination/'masters.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest


def verify(package,replay=True):
    manifest=json.loads((package/'masters.json').read_text())
    assert manifest['schema']=='halo2-vista-menu-masters-v1' and manifest['complete'] is False
    expected={'masters.json'};shapes=0
    for key,page in manifest['pages'].items():
        assert page['state']=='authored-needs-review'
        assert not page['family'].startswith('emblem-'),'Paused emblem drafts must stay outside this package'
        for field in ('vector','dds'):
            expected.add(page[field]);assert file_sha(local(package,page[field]))==page[field+'_sha256']
        vector=local(package,page['vector']);shapes+=svg_shapes(vector)
        with Image.open(local(package,page['dds'])) as stored:
            assert stored.size==tuple(page['physical'])
            if replay:
                result=render(vector,page['density'])
                assert np.array_equal(np.asarray(stored.convert('RGBA')),np.asarray(result)),key+' raster replay differs'
    actual={str(p.relative_to(package)) for p in package.rglob('*') if p.is_file()}
    assert actual==expected,'Unexpected or missing source package files'
    assert len(manifest['pages'])==manifest['coverage']['bundled_unique_pages']
    return dict(passed=True,complete=False,masters=len(manifest['pages']),vector_shapes=shapes,
                replayed_dds=len(manifest['pages']) if replay else 0,coverage=manifest['coverage'],
                claim='File identity and exact standard SVG raster replay; visual fidelity and full-menu coverage remain open.')


def rasterize(package,output):
    verify(package,replay=False)
    if output.resolve()==package.resolve():raise ValueError('Rasterize to a separate output')
    manifest=json.loads((package/'masters.json').read_text())
    for page in manifest['pages'].values():
        result=render(local(package,page['vector']),page['density'])
        bitmap=local(output,page['dds']);bitmap.parent.mkdir(parents=True,exist_ok=True)
        write_dds(bitmap,result)
        assert file_sha(bitmap)==page['dds_sha256'],'Generated DDS does not match the recorded master'
    return dict(passed=True,complete=False,dds_written=len(manifest['pages']),output=str(output))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    commands=parser.add_subparsers(dest='command',required=True)
    prepare=commands.add_parser('export');prepare.add_argument('--pack',type=Path,required=True)
    prepare.add_argument('--output',type=Path,default=DEFAULT)
    check=commands.add_parser('verify');check.add_argument('--package',type=Path,default=DEFAULT)
    rebuild=commands.add_parser('rasterize');rebuild.add_argument('--package',type=Path,default=DEFAULT)
    rebuild.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if args.command=='export':result=export(args.pack,args.output)['coverage']
    elif args.command=='verify':result=verify(args.package)
    else:result=rasterize(args.package,args.output)
    print(json.dumps(result,indent=2))


if __name__=='__main__':main()
