"""Render an exact-data scientific evidence figure; no generated coverage or sky color."""
from pathlib import Path
import hashlib
import json
import struct
from PIL import Image, ImageDraw, ImageFont

ROOT=Path('/home/eric/Developer/Halo-2-Decompiled')
OUT=ROOT/'analysis/multiplayer-shading/source-sky-stencil-prototype'
CONTEXT=ROOT/'analysis/multiplayer-shading/original-xemu/prt-output-audit/original20-final-gameplay.png'
ORIGINAL=ROOT/'analysis/multiplayer-shading/original20-sky-stencil-analysis/8097-after-packed-d24s8.bin'
NATIVE=OUT/'native-normal/gsout-8097-packed-d24s8.bin'
STRIP=OUT/'native-normal/vsout-strip-8097-packed-d24s8.bin'
FONT=Path('/usr/share/fonts/TTF/DejaVuSans.ttf')
BOLD=Path('/usr/share/fonts/TTF/DejaVuSans-Bold.ttf')
if not FONT.exists():
    FONT=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
    BOLD=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf')


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def font(size,bold=False):
    return ImageFont.truetype(str(BOLD if bold else FONT),size)


def run():
    report=json.loads((OUT/'native-normal/report.json').read_text())
    assert report==json.loads((OUT/'native-asan/report.json').read_text())
    assert report['validation_errors']==0 and len(report['metrics'])==17
    assert all(m['depth_differences']==m['stencil_differences']==0 for m in report['metrics'])
    raw=ORIGINAL.read_bytes()
    assert raw==NATIVE.read_bytes()==STRIP.read_bytes()
    words=struct.unpack('<307200I',raw)
    stencil=bytes(x>>24 for x in words)
    assert set(stencil)=={0,255} and stencil.count(255)==462
    original=Image.frombytes('L',(640,480),stencil)
    native=Image.frombytes('L',(640,480),bytes(x>>24 for x in struct.unpack('<307200I',NATIVE.read_bytes())))
    bounds=original.getbbox()
    assert bounds==(184,78,382,126)
    crop=(bounds[0]-16,bounds[1]-16,bounds[2]+16,bounds[3]+16)
    figure=Image.new('RGB',(1080,1990),'#f5f7fb')
    d=ImageDraw.Draw(figure)
    d.text((40,28),'Original20 sky stencil',(24,39,59),font=font(47,True))
    d.text((40,89),'Bounded retained-geometry proof',(24,39,59),font=font(32,True))
    d.text((40,136),'Stencil-only diagnostic; live writer not activated',(63,76,94),font=font(29))
    d.text((40,194),'Original final-frame context (reference only)',(24,39,59),font=font(30,True))
    context=Image.open(CONTEXT).convert('RGB')
    assert context.size==(640,480)
    figure.paste(context.resize((960,720),Image.Resampling.NEAREST),(60,239))
    d.text((40,993),'Original S8 after8097',(24,39,59),font=font(29,True))
    d.text((560,993),'Native GSOut passthrough',(24,39,59),font=font(27,True))
    for x,mask in ((40,original),(560,native)):
        figure.paste(mask.convert('RGB').resize((480,360),Image.Resampling.NEAREST),(x,1040))
    d.text((40,1421),'Actual diagnostic masks: FF white; 00 black. Raw rows unchanged.',(63,76,94),font=font(26))
    d.text((40,1471),'Nearest pixel detail; same source/native crop',(24,39,59),font=font(30,True))
    for x,mask in ((50,original),(570,native)):
        figure.paste(mask.crop(crop).convert('RGB').resize((460,160),Image.Resampling.NEAREST),(x,1521))
    d.text((40,1712),'307,200 / 307,200 D24+S8 pixels match',(24,39,59),font=font(37,True))
    d.text((40,1765),'8097: 462 changes 00->FF; 8102 / moon8110: 0 new changes',(24,39,59),font=font(27))
    d.text((40,1808),'Strip+GS also exact; normal + ASAN; SDK sync errors: 0',(24,39,59),font=font(27))
    d.text((40,1865),'462 maximum-depth pixels bound this coverage proof.',(91,64,28),font=font(28,True))
    d.text((40,1911),'Native authored vertices and sky color remain unproved.',(63,76,94),font=font(28))
    target=OUT/'phone-original-native-sky-stencil.png'
    figure.save(target)
    tool=ROOT/'tools/build_source_sky_stencil_phone.py'
    (OUT/'phone-proof-tool.py').write_bytes(tool.read_bytes())
    metadata=dict(scope='Scientific exact-data stencil figure; context is unchanged original20 final frame, not a native sky color render.',
        image=str(target.relative_to(ROOT)),image_sha256=sha(target),image_size=list(figure.size),
        inputs={str(p.relative_to(ROOT)):sha(p) for p in (CONTEXT,ORIGINAL,NATIVE,STRIP,tool)},
        fonts={str(p):sha(p) for p in (FONT,BOLD)},
        row_flip=False,mask_values=[0,255],FF_pixels=462,actual_mask_bounds_exclusive=list(bounds),
        nearest_detail_crop_exclusive=list(crop),nearest_detail_integer_scale=2,
        mask_presentation='Black00/whiteFF actual S8 bytes; no interpolated edges, mask dilation, composite sky or coverage invention.',
        original_native_raw_bytes_equal=True,all307200_D24_S8_pixels_equal=True,
        live_writer_activated=False,native_authored_vertices_proved=False,native_sky_color_proved=False,
        precision_boundary=json.loads((OUT/'source-admission.json').read_text())['geometry'])
    (OUT/'display-proof.json').write_text(json.dumps(metadata,indent=2)+'\n')
    print(target)


if __name__=='__main__':
    run()
