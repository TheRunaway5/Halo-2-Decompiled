#!/usr/bin/env python3
"""Rasterize explicitly authored Vista menu SVG families and report omissions.

Incomplete reconstruction is a review artifact. It is never represented as a
complete asset pack and never activates itself in the installed launchers.
"""
import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

from audit_rendering import file_sha
from vista_menu_svg import draw
from vista_prompt_source import write_dds
from vista_prompt_svg import render

ROOT=Path(__file__).resolve().parents[1]


def error(source,reconstructed):
    original=np.asarray(source).astype(float)
    reduced=np.asarray(reconstructed.resize(source.size,Image.Resampling.BOX)).astype(float)
    visible=(original[:,:,3]>0)|(reduced[:,:,3]>0)
    a=original[:,:,3]/255;b=reduced[:,:,3]/255
    return dict(alpha_mae=float(abs(a-b).mean()),
        premultiplied_rgb_mae=float(abs(original[:,:,:3]*a[:,:,None]-reduced[:,:,:3]*b[:,:,None])[visible].mean()) if visible.any() else 0,
        source_visible_pixels=int((a>0).sum()),redraw_visible_pixels=int((b>0).sum()))


def build(pack):
    reference=json.loads((pack/'reference.json').read_text())
    rendered,records={},[]
    for cache,banks in reference['owners'].items():
        for bank in banks:
            for frame in bank['frames']:
                key=frame['page']
                if key not in rendered:
                    page=reference['pages'][key]
                    source=Image.open(pack/page['png']).convert('RGBA')
                    density=min(4,4096//max(source.size))
                    vector='svg/'+key+'.svg'
                    drawing=draw(source,pack/vector,bank['name'],frame['frame'],density)
                    if drawing is None:
                        rendered[key]=None
                    else:
                        result=render(pack/vector,density)
                        bitmap='bitmaps/'+key+'.dds'
                        (pack/bitmap).parent.mkdir(parents=True,exist_ok=True)
                        write_dds(pack/bitmap,result)
                        png='preview/'+key+'.png'
                        (pack/png).parent.mkdir(parents=True,exist_ok=True)
                        result.save(pack/png)
                        rendered[key]=dict(vector=vector,vector_sha256=file_sha(pack/vector),
                            dds=bitmap,dds_sha256=file_sha(pack/bitmap),preview=png,
                            family=drawing.family,density=density,physical=list(result.size),
                            shape_count=sum(n.tag.rsplit('}',1)[-1] in ('path','rect','circle','ellipse','polygon') for n in drawing.root.iter()),
                            comparison=error(source,result))
                records.append(dict(cache=cache,name=bank['name'],frame=frame['frame'],page=key,
                    state='authored-needs-review' if rendered[key] else 'unfinished',
                    reconstruction=rendered[key]))
    completed={key:record for key,record in rendered.items() if record}
    report=dict(schema='halo2-vista-menu-redraw-v1',complete=False,
        reference_sha256=file_sha(pack/'reference.json'),records=records,
        policy=dict(vector_method='Explicit semantic shape recipes; no autotrace, embedded rasters, or pixel regions',
                    activation='Review only. Unfinished pages are never silently substituted or marked complete.',
                    screenshots='Original locations and camera views require reconstruction evidence before replacement.'),
        summary=dict(unique_reference_pages=len(reference['pages']),authored_unique_pages=len(completed),
                     unfinished_unique_pages=len(reference['pages'])-len(completed),
                     authored_bindings=sum(r['reconstruction'] is not None for r in records),
                     total_bindings=len(records)))
    (pack/'reconstruction.json').write_text(json.dumps(report,indent=2)+'\n')
    # Review cards compare the original at left with the actual SVG raster at
    # right. They are previews only and do not alter replacement artwork.
    items=[(key,r) for key,r in completed.items() if r['family']!='uniform-page']
    for offset in range(0,len(items),12):
        batch=items[offset:offset+12]
        sheet=Image.new('RGB',(1200,len(batch)*150),(30,38,48));pen=ImageDraw.Draw(sheet)
        for i,(key,r) in enumerate(batch):
            use=reference['pages'][key]['uses'][0]
            pen.text((10,i*150+5),use['name']+f" [{use['frame']}]",fill='white')
            for x,relative in ((10,reference['pages'][key]['png']),(610,r['preview'])):
                im=Image.open(pack/relative).convert('RGBA')
                page=reference['pages'][key]
                zoom=min(4,570/page['width'],115/page['height'])
                im=im.resize((round(page['width']*zoom),round(page['height']*zoom)),Image.Resampling.LANCZOS)
                sheet.paste(im,(x,i*150+28),im)
        sheet.save(pack/f'comparison-{offset//12+1:02d}.png')
    # Stable review link: unrelated families must not change which artwork
    # the user's already-open comparison tab displays after regeneration.
    keys=[(key,r) for key,r in items if r['family']=='virtual-keyboard-key' and
          reference['pages'][key]['uses'][0]['name'].endswith(('button_4','button_5','button_6'))]
    sheet=Image.new('RGB',(1200,len(keys)*150),(30,38,48));pen=ImageDraw.Draw(sheet)
    for i,(key,r) in enumerate(keys):
        page=reference['pages'][key];use=page['uses'][0]
        pen.text((10,i*150+5),use['name']+f" [{use['frame']}] — Vista original / vector redraw",fill='white')
        zoom=min(4,570/page['width'],115/page['height'])
        for x,relative in ((10,page['png']),(610,r['preview'])):
            im=Image.open(pack/relative).convert('RGBA').resize(
                (round(page['width']*zoom),round(page['height']*zoom)),Image.Resampling.LANCZOS)
            sheet.paste(im,(x,i*150+28),im)
    sheet.save(pack/'keyboard-comparison.png')
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pack',type=Path,default=ROOT/'assets/vista-menu-svg-4k')
    args=p.parse_args()
    print(json.dumps(build(args.pack)['summary'],indent=2))


if __name__=='__main__':main()
