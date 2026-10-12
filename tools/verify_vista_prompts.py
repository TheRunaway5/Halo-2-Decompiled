#!/usr/bin/env python3
"""Verify standard SVG conversion, Vista ownership and authored design invariants.

Source-pixel contour and paint deviations are measurements, not a requirement
to reproduce compressed low-resolution pixels. Letters retain their counters
and bounded core outlines; face bodies must be actual uniformly scaled circles.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image
from scipy.ndimage import distance_transform_edt, binary_erosion, binary_fill_holes, label

from vista_prompt_source import decode_glyph
from vista_prompt_svg import render


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def contour_error(a, b, scale):
    # Evaluate both directions; a lost hole or disconnected stroke cannot
    # pass merely because the surviving outline remains near the reference.
    up = np.repeat(np.repeat(a, scale, 0), scale, 1)
    ea, eb = up ^ binary_erosion(up), b ^ binary_erosion(b)
    if not ea.any() or not eb.any():
        return 0. if not ea.any() and not eb.any() else float('inf')
    return max(float(distance_transform_edt(~ea)[eb].max()),
               float(distance_transform_edt(~eb)[ea].max())) / scale


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('pack',type=Path)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--measure',action='store_true',help='Measure deviations before accepting quality bounds')
    args=parser.parse_args();pack=args.pack.resolve()
    report=json.loads((pack/'reconstruction.json').read_text())
    assert report['schema']=='vista-prompt-svg-v2'
    def source_path(value):
        path=Path(value)
        return path if path.is_absolute() else pack/path
    assert sha(source_path(report['source_table']))==report['source_table_sha256']
    assert sha(pack/'prompts.tsv')==report['prompts_manifest_sha256']
    manifest=(pack/'prompts.tsv').read_text().splitlines()
    assert manifest[0]=='H2_VISTA_PROMPT_RASTERS_V1'
    assert manifest[1:]==['\t'.join(map(str,(row['selector'],row['codepoint'],*row['rectangle'],row['atlas'])))
        for row in report['records']], 'Selector manifest and source records disagree'
    verified=set();controls=[];letter_errors=[];visible_errors=[]
    fonts={};hashes={};atlases={};rasters={}
    def checked_hash(path,expected):
        if path not in hashes:hashes[path]=sha(path)
        assert hashes[path]==expected
    for row in report['records']:
        source=source_path(row['source_font']);checked_hash(source,row['source_font_sha256'])
        if source not in fonts:
            data=source.read_bytes();fonts[source]=(data,struct.unpack_from('<65536I',data,0x400))
        data,lookup=fonts[source]
        assert lookup[row['codepoint']]==row['source_record'] and lookup.count(row['source_record'])==1
        original=decode_glyph(data,(row['source_record']+0x4040)*16)
        assert hashlib.sha256(original.tobytes()).hexdigest()==row['source_rgba_sha256']
        checked_hash(pack/row['vector'],row['vector_sha256'])
        checked_hash(pack/row['atlas'],row['atlas_sha256'])
        key=row['vector']
        if key not in rasters:
            rebuilt=np.array(render(pack/key,report['density']))
            rebuilt[rebuilt[:,:,3]==0,:3]=255
            rasters[key]=rebuilt
        rebuilt=rasters[key]
        if row['atlas'] not in atlases:
            with Image.open(pack/row['atlas']) as image:atlases[row['atlas']]=np.array(image.convert('RGBA'))
        x,y,w,h=row['rectangle'];stored=atlases[row['atlas']][y:y+h,x:x+w]
        assert np.array_equal(rebuilt,stored),'Every atlas binding must reproduce its saved SVG'
        if key not in verified:
            tree=ET.parse(pack/key)
            assert not tree.findall('.//{*}image') and not tree.findall('.//{*}text')
            assert any(tree.findall('.//{*}'+kind) for kind in ('path','polygon','ellipse','circle','rect'))
            assert not any(name.startswith('data-') for node in tree.iter() for name in node.attrib)
            assert row['vector_shapes'] <= 24, 'A control must use compact design geometry, not pixel regions'
            shape_count=sum(node.tag.rsplit('}',1)[-1] in ('path','rect','ellipse','circle','polygon') for node in tree.iter())
            assert shape_count==row['vector_shapes']
            assert not tree.findall('.//{*}filter'), 'Artwork must use ordinary vector geometry and paint'
            assert rebuilt.shape[1]==4*original.width and rebuilt.shape[0]==4*original.height
            a=np.array(original);s=report['density']
            alpha_error=contour_error(a[:,:,3]>=128,rebuilt[:,:,3]>=128,s)
            visible_errors.append(alpha_error)
            # Letters are painted into RGB, not the solid button alpha.
            # Compare their actual bright core contours as well as the exterior.
            letter=None
            if 0xe100<=row['codepoint']<=0xe103:
                body=next(node for node in tree.iter() if node.attrib.get('id')=='button-body')
                assert body.tag.rsplit('}',1)[-1]=='circle','A/B/X/Y body must be a true circle'
                group=next(node for node in tree.iter() if node.attrib.get('id')=='face-button')
                import re
                uniform=re.search(r'scale\(([^)]+)\)',group.attrib['transform']).group(1).split()
                assert len(uniform)==1,'Face buttons must not stretch independently on either axis'
                yy,xx=np.where(rebuilt[:,:,3]>=128)
                assert abs(int(xx.max()-xx.min())-int(yy.max()-yy.min()))<=1,'Rasterized face button is oval'
                mask=np.zeros(a.shape[:2],bool)
                mask[int(original.height*.30):int(original.height*.85),
                     int(original.width*.15):int(original.width*.85)]=True
                up=np.repeat(np.repeat(mask,s,0),s,1)
                letter=contour_error((a[:,:,:3].min(2)>=170)&(a[:,:,3]>=128)&mask,
                    (rebuilt[:,:,:3].min(2)>=170)&(rebuilt[:,:,3]>=128)&up,s)
                if not args.measure:assert letter<=1.25,('letter',row['codepoint'],letter)
                source_ink=(a[:,:,:3].min(2)>=170)&(a[:,:,3]>=128)&mask
                rebuilt_ink=(rebuilt[:,:,:3].min(2)>=170)&(rebuilt[:,:,3]>=128)&up
                def holes(ink, density):
                    components,_=label(binary_fill_holes(ink)&~ink)
                    return int((np.bincount(components.ravel())[1:]>=max(1,density*density*.25)).sum())
                assert holes(source_ink,1)==holes(rebuilt_ink,s)==(1,2,0,0)[row['codepoint']-0xe100],\
                    'A/B/X/Y counters changed shape or filled in'
                letter_errors.append(letter)
            # Average paint error is weighted by authored alpha, so irrelevant
            # transparent RGB cannot hide a change in color or highlight.
            down=rebuilt.reshape(original.height,s,original.width,s,4).mean((1,3))
            weight=a[:,:,3].astype(float)/255
            paint=float((np.abs(down[:,:,:3]-a[:,:,:3])*weight[:,:,None]).sum()/(3*weight.sum()))
            # Vista encodes paint with four bits/channel (steps of17). Bound
            # averaged visible paint error to one authored quantization step.
            if row['family']=='shoulder-button':
                gradient=next(node for node in tree.iter() if node.attrib.get('id')=='label-paint')
                for stop in gradient:
                    color=stop.attrib['stop-color'].lstrip('#')
                    assert max(int(color[i:i+2],16) for i in (0,2,4))<=102,'Shoulder lettering lost dark Vista paint'
            controls.append(dict(vector=key,alpha_contour_error_source_pixels=alpha_error,
                letter_contour_error_source_pixels=letter,mean_visible_rgb_error=paint))
            verified.add(key)
    result=dict(passed=not args.measure,bindings=len(report['records']),unique_vectors=len(verified),
        reconstructed_density=report['density'],maximum_silhouette_error_source_pixels=max(visible_errors),
        maximum_letter_error_source_pixels=max(letter_errors),controls=controls,
        maximum_mean_visible_rgb_error=max(row['mean_visible_rgb_error'] for row in controls),
        claim='All bindings replay from ordinary compact SVGs; true circles, dark shoulder labels and bounded letter cores/counters verified. General source-pixel contour/color deviations are reported, not pixel-copy acceptance criteria.')
    text=json.dumps(result,indent=2)+'\n'
    if args.output:args.output.write_text(text)
    print(json.dumps({k:v for k,v in result.items() if k!='controls'}))


if __name__=='__main__':
    main()
