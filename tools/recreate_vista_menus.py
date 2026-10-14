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

# Newly authored small controls need enough storage for a 2160p presentation
# of the 480-high logical frontend. Keep the established masters unchanged.
EIGHTFOLD_CONTROLS={
    'ui\\global_bitmaps\\live_icons','ui\\global_bitmaps\\live_icons_sm',
    'ui\\list_skins\\y_menu_list\\hacky_beta_hack_dont_use',
    'ui\\list_skins\\y_menu_list\\top_item','ui\\list_skins\\variants\\lock',
    'ui\\list_skins\\game_browser\\end_caps','ui\\list_skins\\matchmaking\\player_in',
    'ui\\list_skins\\playlist_entries\\select',
    'ui\\screens\\game_shell\\pregame_lobby\\download_progress',
    'ui\\screens\\game_shell\\network_squad_browser_screen\\arrows',
    'ui\\screens\\game_shell\\load_level\\diff_border',
    'ui\\screens\\game_shell\\dialog_live_password\\password',
    'ui\\screens\\game_shell\\xbox_live\\matchmaking_progress_dialog\\progress_numbers',
    'ui\\screens\\game_shell\\xbox_live\\matchmaking_progress_dialog\\progress_dots',
    'ui\\screens\\game_shell\\xbox_live\\xbox_live_main_menu\\y_callout',
    'ui\\screens\\game_shell\\network_squad_browser_screen\\ds_icon',
    'ui\\screens\\game_shell\\difficulty\\flavor_ani',
    'ui\\screens\\game_shell\\pregame_lobby\\matchmaking2',
    'ui\\screens\\game_shell\\pregame_lobby\\item_background',
    'ui\\screens\\game_shell\\pregame_lobby\\item_background_lobby',
    'ui\\screens\\game_shell\\xbox_live\\matchmaking_progress_dialog\\done',
}


def error(source,reconstructed):
    original=np.asarray(source).astype(float)
    reduced=np.asarray(reconstructed.resize(source.size,Image.Resampling.BOX)).astype(float)
    visible=(original[:,:,3]>0)|(reduced[:,:,3]>0)
    a=original[:,:,3]/255;b=reduced[:,:,3]/255
    return dict(alpha_mae=float(abs(a-b).mean()),
        premultiplied_rgb_mae=float(abs(original[:,:,:3]*a[:,:,None]-reduced[:,:,:3]*b[:,:,None])[visible].mean()) if visible.any() else 0,
        source_visible_pixels=int((a>0).sum()),redraw_visible_pixels=int((b>0).sum()))


def build(pack,skip_emblems=False):
    reference=json.loads((pack/'reference.json').read_text())
    preserved={}
    if skip_emblems:
        old=json.loads((pack/'reconstruction.json').read_text())
        if old['reference_sha256']!=file_sha(pack/'reference.json'):
            raise ValueError('Cannot retain emblem drafts against a changed reference inventory')
        preserved={row['page']:row['reconstruction'] for row in old['records']}
    rendered,records={},[]
    for cache,banks in reference['owners'].items():
        for bank in banks:
            for frame in bank['frames']:
                key=frame['page']
                if skip_emblems and bank['name'].startswith('ui\\global_bitmaps\\emblems\\'):
                    rendered.setdefault(key,preserved.get(key))
                if key not in rendered:
                    page=reference['pages'][key]
                    source=Image.open(pack/page['png']).convert('RGBA')
                    # Resolve the ring's narrow rim and two projected wall
                    # edges above the 4.5x scale of a 2160p/480p UI canvas.
                    # Keep the denser raster scoped to these two small sprites.
                    detail=16 if bank['name'] in ('ui\\global_bitmaps\\rank_icons',
                        'ui\\global_bitmaps\\rank_icons_sm') and frame['frame']==49 else \
                        8 if bank['name'] in EIGHTFOLD_CONTROLS else 4
                    if detail==8 and np.all(np.asarray(source)==np.asarray(source)[0,0]):
                        detail=4  # Retain the shared, already-bundled empty placeholders.
                    density=min(detail,4096//max(source.size))
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
                        if getattr(drawing,'art_references',None):
                            rendered[key]['art_references']=drawing.art_references
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
    logos=[(key,r) for key,r in items if r['family']=='halo2-title-wordmark']
    if logos:
        key,record=logos[0]
        source=Image.open(pack/reference['pages'][key]['png']).convert('RGBA')
        redraw=Image.open(pack/record['preview']).convert('RGBA')
        review=Image.new('RGBA',(2048,560),(30,38,48,255));pen=ImageDraw.Draw(review)
        for row,(art,label) in enumerate(((source,'Vista original'),(redraw,'Current vector redraw'))):
            pen.text((10,row*280+2),label,fill='white')
            review.alpha_composite(art.resize((2048,256),Image.Resampling.LANCZOS),(0,row*280+18))
        review.convert('RGB').save(pack/'logo-comparison.png')
        details=(('L tail',(523,78,560,113)),('O centre',(639,53,688,78)),('2 upper notch',(845,10,887,51)))
        review=Image.new('RGBA',(1200,3*480),(30,38,48,255));pen=ImageDraw.Draw(review)
        density=record['density']
        for row,(label,box) in enumerate(details):
            pen.text((10,row*480+3),label+' - Vista / vector',fill='white')
            size=((box[2]-box[0])*10,(box[3]-box[1])*10)
            a=source.crop(box).resize(size,Image.Resampling.NEAREST)
            b=redraw.crop(tuple(v*density for v in box)).resize(size,Image.Resampling.LANCZOS)
            review.alpha_composite(a,(10,row*480+25));review.alpha_composite(b,(610,row*480+25))
        review.convert('RGB').save(pack/'logo-details-comparison.png')
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pack',type=Path,default=ROOT/'assets/vista-menu-svg-4k')
    p.add_argument('--skip-emblems',action='store_true',help='Retain existing emblem drafts and omissions unchanged')
    args=p.parse_args()
    print(json.dumps(build(args.pack,args.skip_emblems)['summary'],indent=2))


if __name__=='__main__':main()
