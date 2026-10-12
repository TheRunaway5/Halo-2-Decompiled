#!/usr/bin/env python3
"""Rebuild Vista control prompts as authored SVGs and fourfold RGBA rasters.

Only source-authored control and key-frame glyphs are replaced. New shapes,
outlined lettering, bevels and calibrated gradients recreate the Vista design.
No pixel-region tracing, substitute font, image generation or enlarged source
bitmap supplies the artwork.
"""
import argparse
import copy
import hashlib
import json
import shutil
from pathlib import Path
import struct
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image, ImageDraw
from vista_prompt_source import decode_glyph, write_dds
from vista_prompt_svg import draw as draw_prompt

ROOT = Path(__file__).resolve().parents[1]
CODES = list(range(0xe100, 0xe112)) + list(range(0xe12c, 0xe13d))
SCALE = 4


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--vista', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT/'assets/vista-prompts-svg-4k')
    parser.add_argument('--activate', action='store_true')
    args = parser.parse_args()
    bundled = ROOT/'resources/ui/vista-prompts-svg-4k/reference/fonts'
    if args.vista is not None:
        source = args.vista.resolve()/'fonts'
    elif (bundled/'font_table.txt').is_file():
        source = bundled
    else:
        selected = (ROOT/'assets/menu-source.tsv').read_text().splitlines()
        menu_pack = Path(selected[1].split('\t')[1])
        if not menu_pack.is_absolute():menu_pack = ROOT/menu_pack
        marker = (menu_pack/'menu-ui-source.tsv').read_text().splitlines()
        args.vista = Path(marker[1].split('\t')[1])
        if not args.vista.is_absolute():args.vista = menu_pack/args.vista
        source = args.vista.resolve()/'fonts'
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    names = (source/'font_table.txt').read_text().splitlines()
    assert len(names) == 12
    # Keep the exact reference fonts with the editable masters so regeneration
    # and provenance verification work after moving the source checkout.
    reference = out/'reference/fonts'
    reference.mkdir(parents=True, exist_ok=True)
    for name in {'font_table.txt', *(name.lower() for name in names)}:
        if (source/name).resolve() != (reference/name).resolve():
            shutil.copy2(source/name, reference/name)
    source = reference
    records, cache, atlases = [], {}, {}
    manifest = ['H2_VISTA_PROMPT_RASTERS_V1']
    for selector, name in enumerate(names):
        path = source/name.lower()
        data = path.read_bytes()
        lookup = struct.unpack_from('<65536I', data, 0x400)
        rows, images = [], []
        for code in CODES:
            index = lookup[code]
            # Some supplied fonts do not author later keyboard controls.
            if index == 0xffffffff or lookup.count(index) != 1:
                continue
            image = decode_glyph(data, (index+0x4040)*16)
            pixels = np.array(image)
            if not np.any(pixels[:, :, 3]):
                continue
            identity = hashlib.sha256(struct.pack('<II', *image.size)+pixels.tobytes()).hexdigest()
            relative = Path('vectors')/(identity+'.svg')
            if identity not in cache:
                enlarged, metrics = draw_prompt(image, out/relative, SCALE, code, name)
                rgba = np.array(enlarged)
                rgba[rgba[:, :, 3] == 0, :3] = 255
                enlarged = Image.fromarray(rgba)
                assert not ET.parse(out/relative).findall('.//{*}image')
                cache[identity] = enlarged, dict(vector=str(relative), vector_sha256=sha(out/relative),
                    source_rgba_sha256=hashlib.sha256(image.tobytes()).hexdigest(),
                    source_size=image.size, raster_size=enlarged.size, **metrics)
                (out/'preview').mkdir(exist_ok=True)
                enlarged.save(out/'preview'/(identity+'.png'))
                print(f'{len(cache)} unique prompts: {code:04X} {image.size} -> {enlarged.size}', flush=True)
            enlarged, traced = cache[identity]
            rows.append(dict(selector=selector, name=name, codepoint=code, source_record=index,
                source_font='reference/fonts/'+path.name, source_font_sha256=sha(path), **traced))
            images.append(enlarged)
        signature = tuple(row['vector_sha256'] for row in rows)
        if signature not in atlases:
            width, x, y, row_height = 1024, 1, 1, 0
            locations = []
            for image in images:
                if x+image.width+1 > width:
                    x, y, row_height = 1, y+row_height+2, 0
                assert image.width+2 <= width
                locations.append((x, y))
                x += image.width+2
                row_height = max(row_height, image.height)
            atlas = Image.new('RGBA', (width, y+row_height+1), (255,255,255,0))
            assert atlas.height <= 4096
            for image, xy in zip(images, locations):
                atlas.paste(image, xy)
            relative = Path('atlases')/(str(len(atlases))+'.dds')
            write_dds(out/relative, atlas)
            atlases[signature] = relative, locations
        relative, locations = atlases[signature]
        for row, xy, image in zip(rows, locations, images):
            row.update(atlas=str(relative), atlas_sha256=sha(out/relative), rectangle=(*xy, *image.size))
            manifest.append('\t'.join(map(str, (selector, row['codepoint'], *xy, *image.size, relative))))
        records.extend(rows)
    (out/'prompts.tsv').write_text('\n'.join(manifest)+'\n')
    report = dict(schema='vista-prompt-svg-v2', source_table='reference/fonts/font_table.txt',
        source_table_sha256=sha(source/'font_table.txt'), density=SCALE, records=records,
        unique_vectors=len(cache), atlas_count=len(atlases), prompts_manifest_sha256=sha(out/'prompts.tsv'),
        method='Hand-authored SVG control geometry and letter outlines; low-dimensional gradient palette calibrated to Vista; direct librsvg PNG rendering and lossless DDS packing',
        limits='A reference-led vector redraw, not recovered original vector masters or identical source pixels. Existing logical font metrics are retained by the runtime overlay.')
    (out/'reconstruction.json').write_text(json.dumps(report, indent=2)+'\n')
    # Show each distinct control at a readable size, beside the actual Vista
    # nearest-neighbor reference. These comparisons are separate from output.
    examples = [r for r in records if r['selector'] == 6]
    sheet = Image.new('RGB', (1200, ((len(examples)+3)//4)*190), (24,31,43))
    draw = ImageDraw.Draw(sheet)
    for i, row in enumerate(examples):
        x, y = (i%4)*300, (i//4)*190
        data = (out/row['source_font']).read_bytes()
        original = decode_glyph(data, (row['source_record']+0x4040)*16)
        rebuilt = cache[hashlib.sha256(struct.pack('<II', *original.size)+original.tobytes()).hexdigest()][0]
        factor = min(128/rebuilt.width, 125/rebuilt.height, 1)
        size = (max(1, round(rebuilt.width*factor)), max(1, round(rebuilt.height*factor)))
        original = original.resize(size, Image.Resampling.NEAREST)
        rebuilt = rebuilt.resize(size, Image.Resampling.LANCZOS)
        draw.text((x+10, y+8), f"{row['codepoint']:04X}   Vista / redrawn", fill='white')
        sheet.paste(original, (x+10, y+40), original)
        sheet.paste(rebuilt, (x+155, y+40), rebuilt)
    sheet.save(out/'comparison.png')
    # These are standalone SVG compositions of the real masters, not PNGs
    # wrapped in SVG. Prefix IDs so gradients and clips remain independent.
    gallery=ET.Element('{http://www.w3.org/2000/svg}svg',dict(width='1200',height='280',viewBox='0 0 1200 280'))
    ET.SubElement(gallery,'{http://www.w3.org/2000/svg}rect',dict(width='1200',height='280',fill='#181f2b'))
    picks=list(range(0xe100,0xe106))+[0xe10c,0xe10d]
    for i,code in enumerate(picks):
        row=next(row for row in examples if row['codepoint']==code)
        original=ET.parse(out/row['vector']).getroot()
        item=copy.deepcopy(original)
        prefix=f'control-{code:04x}-'
        for node in item.iter():
            for key,value in list(node.attrib.items()):
                if key=='id':node.set(key,prefix+value)
                elif 'url(#' in value:node.set(key,value.replace('url(#','url(#'+prefix))
        width,height=row['source_size']
        factor=min(130/width,130/height)
        item.set('x',str(i*150+(150-width*factor)/2));item.set('y',str((280-height*factor)/2))
        item.set('width',str(width*factor));item.set('height',str(height*factor))
        gallery.append(item)
    ET.indent(gallery)
    ET.ElementTree(gallery).write(out/'buttons.svg',encoding='utf-8',xml_declaration=True)
    from vista_prompt_svg import render
    render(out/'buttons.svg',1).save(out/'buttons.png')
    if args.activate:
        for registry in ('hud-source.tsv', 'menu-source.tsv'):
            lines = (ROOT/'assets'/registry).read_text().splitlines()
            pack = Path(lines[1].split('\t')[1])
            if not pack.is_absolute():pack = ROOT/pack
            temporary = pack/'prompt-source.tmp'
            temporary.write_text('H2_VISTA_PROMPTS_V1\n'+str(out)+'\n')
            temporary.replace(pack/'prompt-source.tsv')
    print(f'{len(records)} selector/control bindings, {len(cache)} unique vectors, {len(atlases)} atlases')


if __name__ == '__main__':
    main()
