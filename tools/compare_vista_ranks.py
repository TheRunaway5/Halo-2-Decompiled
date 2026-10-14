#!/usr/bin/env python3
"""Compare original Vista ranks, larger MCC references, and actual draft DDS art."""
import argparse
import json
from pathlib import Path
from PIL import Image,ImageDraw
from audit_rendering import file_sha


def compare(pack,mcc,output,small=False,frames=None,cell=224,horizontal=False):
    reference=json.loads((pack/'reference.json').read_text())
    report=json.loads((pack/'reconstruction.json').read_text())
    donor=json.loads((mcc/'manifest.json').read_text())
    name='ui\\global_bitmaps\\rank_icons'+('_sm' if small else '')
    bank=next(b for b in reference['owners']['mainmenu.map'] if b['name']==name)
    selected=[f for f in bank['frames'] if f['frame'] in (frames or range(43,50))]
    row_height=cell+26;art_height=round(cell*26/28)
    if horizontal and len(selected)!=1:raise ValueError('Horizontal layout requires exactly one rank frame')
    size=(cell*3,row_height) if horizontal else (cell*len(selected),row_height*3)
    sheet=Image.new('RGBA',size,(30,38,48,255));pen=ImageDraw.Draw(sheet)
    for i,frame in enumerate(selected):
        key=frame['page'];page=reference['pages'][key]
        drawing=next(r['reconstruction'] for r in report['records'] if r['cache']=='mainmenu.map' and r['name']==name and r['frame']==frame['frame'])
        src=next(r for r in donor['records'] if r['name']==f'LARGERANKICON_{frame["frame"]+1:03}')
        assert file_sha(pack/page['png'])==page['png_sha256']
        assert file_sha(mcc/src['png'])==src['png_sha256']
        assert file_sha(pack/drawing['dds'])==drawing['dds_sha256']
        rows=((pack/page['png'],'Vista original'),(mcc/src['png'],'MCC larger reference'),
              (pack/drawing['dds'],'Current SVG raster'))
        for row,(path,label) in enumerate(rows):
            y=25 if horizontal else row*row_height+25
            x=row*cell if horizontal else i*cell
            pen.text((x+4,y-16),f'Page {frame["frame"]}: {label}',fill='white')
            art=Image.open(path).convert('RGBA')
            if row==1 or small:size=(art_height,art_height)
            else:size=(cell,art_height)
            art=art.resize(size,Image.Resampling.LANCZOS)
            sheet.alpha_composite(art,(x+(cell-size[0])//2,y))
    output.parent.mkdir(parents=True,exist_ok=True)
    sheet.convert('RGB').save(output)
    return dict(output=str(output),vista_pages=len(selected),mcc_pages=len(selected),svg_dds_pages=len(selected),
                note='Vista, MCC and current DDS are shown at matching inspection sizes; MCC numerals and backgrounds are reference-only')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pack',type=Path,required=True)
    p.add_argument('--mcc-reference',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--small',action='store_true')
    p.add_argument('--frames',type=int,nargs='+',help='Vista zero-based rank frame ordinals')
    p.add_argument('--cell',type=int,default=224,help='Inspection column width')
    p.add_argument('--horizontal',action='store_true',help='Show one rank as Vista/MCC/redraw columns')
    a=p.parse_args();print(json.dumps(compare(a.pack,a.mcc_reference,a.output,a.small,a.frames,a.cell,a.horizontal),indent=2))


if __name__=='__main__':main()
