#!/usr/bin/env python3
"""Review original Vista pages beside their actual generated DDS replacements."""
import argparse
import json
import textwrap
from pathlib import Path

from PIL import Image, ImageDraw

from audit_rendering import file_sha


def compare(pack, output, baseline=None, families=(), prefix='menu'):
    reference=json.loads((pack/'reference.json').read_text())
    report=json.loads((pack/'reconstruction.json').read_text())
    assert report['reference_sha256']==file_sha(pack/'reference.json')
    previous=json.loads((baseline/'masters.json').read_text())['pages'] if baseline else {}
    rows={}
    for row in report['records']:
        art=row['reconstruction']
        if not art or art['family'].startswith('emblem-'):continue
        if families and art['family'] not in families:continue
        if row['page'] in previous and previous[row['page']]['dds_sha256']==art['dds_sha256']:continue
        rows.setdefault(row['page'],row)
    rows=sorted(rows.values(),key=lambda x:(x['reconstruction']['family'],x['name'],x['frame']))
    output.mkdir(parents=True,exist_ok=True)
    sheets=[]
    for offset in range(0,len(rows),12):
        batch=rows[offset:offset+12]
        sheet=Image.new('RGBA',(1500,210*((len(batch)+2)//3)),(30,38,48,255))
        pen=ImageDraw.Draw(sheet)
        members=[]
        for i,row in enumerate(batch):
            page=reference['pages'][row['page']];art=row['reconstruction']
            x=i%3*500;y=i//3*210
            name=row['name'].replace('ui\\screens\\game_shell\\','').replace('ui\\global_bitmaps\\','').replace('ui\\list_skins\\','list\\')
            pen.multiline_text((x+5,y+3),'\n'.join(textwrap.wrap(name,66))+
                f' [{row["frame"]}] - Vista / SVG DDS',fill='white')
            for xx,path,expected in ((0,pack/page['png'],page['png_sha256']),
                                     (250,pack/art['dds'],art['dds_sha256'])):
                assert file_sha(path)==expected,'Review source identity changed'
                image=Image.open(path).convert('RGBA')
                zoom=min(235/page['width'],150/page['height'])
                size=(max(1,round(page['width']*zoom)),max(1,round(page['height']*zoom)))
                image=image.resize(size,Image.Resampling.LANCZOS)
                sheet.alpha_composite(image,(x+xx+(250-size[0])//2,y+55+(150-size[1])//2))
            members.append(dict(page=row['page'],name=row['name'],frame=row['frame'],
                source_png_sha256=page['png_sha256'],dds_sha256=art['dds_sha256']))
        path=output/f'{prefix}-{offset//12+1:02}.png'
        sheet.convert('RGB').save(path)
        sheets.append(dict(path=path.name,sha256=file_sha(path),pages=members))
    index=dict(schema='halo2-vista-menu-dds-review-v1',unique_pages=len(rows),sheets=sheets,
        scope='Both sides use the original logical canvas; replacement pixels are read from the stored DDS, not a separate larger SVG render.')
    (output/(prefix+'-index.json')).write_text(json.dumps(index,indent=2)+'\n')
    return dict(unique_pages=len(rows),sheets=len(sheets),output=str(output))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pack',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--baseline',type=Path,help='Only include pages new or changed since this source package')
    parser.add_argument('--family',action='append',default=[])
    parser.add_argument('--prefix',default='menu')
    args=parser.parse_args()
    print(json.dumps(compare(args.pack,args.output,args.baseline,args.family,args.prefix),indent=2))


if __name__=='__main__':main()
