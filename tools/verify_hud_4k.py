#!/usr/bin/env python3
"""Independently replay every canonical SVG and check source layout/provenance."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
import numpy as np
from PIL import Image
from scipy.ndimage import distance_transform_edt
from hud_vector_trace import rasterize,decode_glyph
from hud_vector_fonts import fnv


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def dds(path):
    data=path.read_bytes();assert data[:4]==b'DDS '
    h,w=struct.unpack_from('<II',data,12)
    flags,_,bits=struct.unpack_from('<III',data,80)
    assert flags==0x41 and bits==32
    return np.frombuffer(data[128:],np.uint8).reshape(h,w,4)[:,:,[2,1,0,3]]

def deviation(source,output,channels=range(4)):
    """Half-coverage outline distance, in source pixels, channel by channel."""
    distances=[]
    h,w=source.shape[:2];scale=output.shape[1]//w
    for k in channels:
        plane=source[:,:,k]
        if plane.min()==plane.max():continue
        threshold=(int(plane.min())+int(plane.max()))/2
        ref=np.asarray(Image.fromarray(plane).resize((w*scale,h*scale),Image.Resampling.BILINEAR))>=threshold
        traced=output[:,:,k]>=threshold;changed=ref!=traced
        if not changed.any():distances.append(0);continue
        boundary=np.where(ref,distance_transform_edt(np.pad(ref,1))[1:-1,1:-1],distance_transform_edt(np.pad(~ref,1))[1:-1,1:-1])
        distances.append(float(boundary[changed].max()/scale))
    return max(distances,default=0)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('pack',type=Path);p.add_argument('--report',type=Path);a=p.parse_args()
    pack=a.pack.resolve();report=json.loads((pack/'reconstruction.json').read_text())
    assert report['schema']=='halo2-hud-vector-trace-v3'
    assert len(report['textures'])==328 and report['bitmap_banks']==70 and len(report['scoreboard'])==98
    values=0;shape=[];labels=0
    resources=pack/'source-resources';donors={}
    for line in (resources/'decode.tsv').read_text().splitlines():
        import shlex
        fields=shlex.split(line)
        if fields[2]=='65535':donors[fields[4]]=(int(fields[0]),int(fields[1]),Path(fields[5]))
    for i,page in enumerate(report['textures']+report['scoreboard']):
        svg=pack/page['vector'];target=dds(pack/page['output']);replay=np.asarray(rasterize(svg))
        assert digest(svg)==page['vector_sha256'] and digest(pack/page['output'])==page['output_sha256']
        assert np.array_equal(replay,target),f'SVG raster differs from DDS: {page["vector"]}'
        tree=ET.parse(svg).getroot();assert not tree.findall('.//{*}image'),'Embedded raster in vector'
        assert tree.findall('.//{*}path') or all(g.attrib['data-mode'] in ('constant','labels') for g in tree.findall('{*}g'))
        source_path=page.get('source')
        if page.get('source_kind')=='Xbox':
            source=dds(pack/'sources'/page['tag'].removeprefix('ui\\hud\\bitmaps\\').replace('\\','/')/f'{page["frame"]:04}.dds')
        else:
            w,h,raw=donors[source_path];source=np.frombuffer(raw.read_bytes(),np.uint8).reshape(h,w,4)
            assert digest(Path(source_path))==page['source_sha256']
        assert hashlib.sha256(source.tobytes()).hexdigest()==tree.attrib['data-source-rgba-sha256']
        assert target.shape[1]>source.shape[1] and target.shape[0]>source.shape[0]
        for channel in page.get('discrete_channels',[]):
            assert channel=='blue';s=target.shape[1]//source.shape[1]
            assert np.array_equal(target[:,:,2],source[:,:,2].repeat(s,0).repeat(s,1));labels+=1
        error=deviation(source,target,channels=(0,1,3) if page.get('discrete_channels') else range(4))
        assert error<=1.0,f'Traced visible contour drifts more than one source pixel: {page["vector"]} ({error})'
        shape.append(dict(vector=page['vector'],max_half_coverage_deviation_source_pixels=round(error,4)))
        values+=target.size
        if (i+1)%50==0:print(f'{i+1}/426 SVG texture replays verified',flush=True)
    glyphs=0;donor_glyphs=0;codes=0;font_outline_max=0
    for font in report['fonts']:
        source=Path(font['source']).read_bytes();native=(pack/'fonts'/font['name']).read_bytes()
        assert hashlib.sha256(source).hexdigest()==font['source_sha256']
        assert (pack/'fonts-source'/font['name']).read_bytes()==source
        assert native[-16:]==b'H2VT'+struct.pack('<IQ',font['density_multiplier'],fnv(source))
        assert struct.unpack_from('<I',native,0x200)[0]==0xf0000003
        restored=bytearray(native[:len(source)]);restored[0x200:0x204]=source[0x200:0x204]
        outline_data=Path(font['outline_donor']).read_bytes()
        records=set(struct.unpack_from('<65536I',source,0x400))-{0xffffffff}
        assert records==set(g['record'] for g in font['glyphs'])
        for glyph in font['glyphs']:
            at=(glyph['record']+0x4040)*16;w,h=struct.unpack_from('<HH',native,at+4)
            assert (w,h)==tuple(glyph['reconstructed_size']);svg=pack/glyph['vector']
            assert digest(svg)==glyph['vector_sha256']
            if w and h:
                offset=struct.unpack_from('<I',native,at+12)[0]
                image=rasterize(svg)
                assert native[offset:offset+w*h*4]==image.tobytes(),f'Glyph SVG raster differs: {svg}'
                outline_source=outline_data if glyph['outline_source']=='Vista' else source
                ink=np.asarray(decode_glyph(outline_source,(glyph['outline_record']+0x4040)*16))
                error=deviation(ink,np.asarray(image),channels=(3,));font_outline_max=max(font_outline_max,error)
                assert error<=1.0,f'Font ink contour drifts more than one source pixel: {svg} ({error})'
            restored[at+4:at+8]=source[at+4:at+8];restored[at+12:at+16]=source[at+12:at+16]
            glyphs+=1;codes+=len(glyph['codepoints'])
        assert restored==source,'Native vector font changes source lookup, kerning, advance, bearing, header or source stream'
        for glyph in font['donor_vectors']:
            svg=pack/glyph['vector'];assert digest(svg)==glyph['vector_sha256'];assert not ET.parse(svg).findall('.//{*}image');donor_glyphs+=1
        print(f'{font["name"]}: every source glyph, fallback mapping and vector payload verified',flush=True)
    worst=sorted(shape,key=lambda s:s['max_half_coverage_deviation_source_pixels'],reverse=True)
    result=dict(texture_pages=426,hud_pages=328,scoreboard_pages=98,fonts=len(report['fonts']),
                original_glyph_records=glyphs,donor_glyph_vectors=donor_glyphs,codepoint_mappings=codes,
                vector_raster_channel_values=values,legacy_meter_pages_with_exact_blue_labels=labels,
                embedded_rasters=0,maximum_font_ink_deviation_source_pixels=font_outline_max,visible_contour_deviation_limit_source_pixels=1.0,source_font_layout_fields='byte-identical',svg_to_native_raster='byte-identical',
                worst_half_coverage_outlines=worst[:20])
    if a.report:a.report.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))

if __name__=='__main__':main()
