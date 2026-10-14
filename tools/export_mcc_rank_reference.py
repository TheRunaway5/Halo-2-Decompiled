#!/usr/bin/env python3
"""Read MCC's separate frontend rank art as reference for Vista SVG redraws.

The installed Classic rank_icons bank is still 28 x 26. This collects the
52 x 52 SMALLRANKICON and 152 x 152 LARGERANKICON families from the frontend's
mainmenuandcampaigntexturepack. It exports unchanged BC3 bytes plus inspection
PNGs; it never installs assets or embeds them in the replacement vectors.

Chunk fields were checked against the installed files and the reader at
https://github.com/Lord-Zedd/MCCTexturePackDumper/blob/master/MCCTexturePackDumper/TexturePack.cs
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct

from PIL import Image, ImageDraw
from import_mcc_hud import discover


def sha(data):
    return hashlib.sha256(data).hexdigest()


def inventory(path):
    data=path.read_bytes();at=0;result=[]
    while at<len(data):
        if len(data)-at<16:raise ValueError('Truncated texture chunk')
        uid,length,_,_=struct.unpack_from('<4I',data,at)
        end=at+16+length
        if not at<end<=len(data):raise ValueError('Texture chunk exceeds file')
        if uid==0xcdbfa090:
            if length<0x98:raise ValueError('Truncated texture definition')
            name=data[at+0x44:at+0x64].split(b'\0')[0].decode('ascii')
            fmt=struct.unpack_from('<I',data,at+0x6c)[0]
            width,height,mips,depth=struct.unpack_from('<4H',data,at+0x74)
            size,offset=struct.unpack_from('<2Q',data,at+0x88)
            result.append(dict(name=name,format=fmt,width=width,height=height,
                mips=mips,depth=depth,size=size,offset=offset,chunk_offset=at,
                definition_sha256=sha(data[at:end])))
        at=end
    return result,sha(data)


def export(root,output):
    perm=root/'mainmenuandcampaigntexturepack.perm.bin'
    temp=root/'mainmenuandcampaigntexturepack.temp.bin'
    records,perm_sha=inventory(perm)
    records=[r for r in records if r['name'].startswith(('LARGERANKICON_','SMALLRANKICON_'))]
    names={r['name'] for r in records}
    expected={f'{family}RANKICON_{n:03}' for family in ('LARGE','SMALL') for n in range(1,51)}
    if len(records)!=100 or names!=expected:raise ValueError('Unexpected MCC rank inventory')
    pixels=temp.read_bytes()
    output.mkdir(parents=True,exist_ok=False)
    for r in records:
        w,h=r['width'],r['height']
        if (r['format'],r['mips'],r['depth'])!=(3,1,1):raise ValueError('Expected a single BC3 image')
        size=((w+3)//4)*((h+3)//4)*16
        if size!=r['size'] or not 0<=r['offset']<=r['offset']+size<=len(pixels):
            raise ValueError('Rank texture payload exceeds its source')
        data=pixels[r['offset']:r['offset']+size]
        words=[124,0x81007,h,w,size,0,1]+[0]*11
        words+=[32,4,int.from_bytes(b'DXT5','little'),0,0,0,0,0,0x1000,0,0,0,0]
        dds=b'DDS '+struct.pack('<31I',*words)+data
        stem=r['name'];r['dds']=stem+'.dds';r['png']=stem+'.png'
        (output/r['dds']).write_bytes(dds)
        image=Image.open(io.BytesIO(dds)).convert('RGBA')
        if image.size!=(w,h):raise ValueError('Decoded rank extent changed')
        image.save(output/r['png'])
        r.update(payload_sha256=sha(data),dds_sha256=sha(dds),
                 png_sha256=sha((output/r['png']).read_bytes()),
                 vista_rank_frame=int(stem.rsplit('_',1)[1])-1)
    sheet=Image.new('RGBA',(1600,1000),(30,38,48,255));pen=ImageDraw.Draw(sheet)
    for i in range(50):
        r=next(r for r in records if r['name']==f'LARGERANKICON_{i+1:03}')
        x=i%10*160;y=i//10*200
        pen.text((x+4,y+4),f'MCC {i+1} / Vista page {i}',fill='white')
        sheet.alpha_composite(Image.open(output/r['png']),(x+4,y+24))
    sheet.convert('RGB').save(output/'overview.png')
    report=dict(schema='halo2-mcc-frontend-rank-reference-v1',
        source=dict(perm=str(perm.resolve()),perm_sha256=perm_sha,
                    temp=str(temp.resolve()),temp_sha256=sha(pixels)),
        format_reference='https://github.com/Lord-Zedd/MCCTexturePackDumper',
        records=records,summary=dict(rank_designs=50,large_pages=50,small_pages=50),
        policy=dict(purpose='Reference only; numbered MCC art is not installed as Vista replacements',
                    differences='MCC adds white rank numerals and changes some fills, colours and silhouettes',
                    redraw='Trace compatible details; Vista controls final layout, palette and transparency'))
    (output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,help='MCC Data/UI/texturepacks directory')
    p.add_argument('--output',type=Path,required=True,help='Fresh output directory')
    args=p.parse_args();root=args.source or discover().parent/'Data/UI/texturepacks'
    print(json.dumps(export(root,args.output)['summary'],indent=2))


if __name__=='__main__':main()
