#!/usr/bin/env python3
"""Trace every HUD element into editable cubic vectors and rasterize for 4K.

No image generation. Every shape comes from the supplied original artwork.
Packed shader labels retain their authored values; visible contours are traced.
Physical density increases without changing manifest geometry or font metrics.
Requires Pillow, NumPy, Pycairo, libpotrace, and the native UI codec.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import numpy as np
from PIL import Image
ROOT = Path(__file__).resolve().parents[1]

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_dds(path, image):
    # Exact A8R8G8B8 shader channels, tightly packed. No lossy BC compression.
    from export_vista_menu_ui import dds_image
    rgba = np.array(image.convert('RGBA'))
    data, _ = dds_image(image.width, image.height, 0, 11,
                        rgba[:, :, [2, 1, 0, 3]].tobytes())
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix('.tmp')
    temporary.write_bytes(data)
    temporary.replace(path)  # Break any hard link to the unchanged donor.


def fnv(data):
    value=0xcbf29ce484222325
    for byte in data:value=((value^byte)*0x100000001b3)&0xffffffffffffffff
    return value

from hud_vector_trace import vectorize,decode_glyph
REUSED={}
def redraw(image,scale,path,discrete=(),font=False):
    key=(image.size,image.tobytes(),scale,tuple(discrete),font)
    if key in REUSED:
        previous,metrics=REUSED[key]
        path.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(previous,path)
        from hud_vector_trace import rasterize
        return rasterize(path),metrics
    result,metrics=vectorize(image,path,scale,discrete,font)
    REUSED[key]=path,metrics
    return result,metrics


def inventory(maps):
    from inspect_multiplayer_hud import HudCache
    banks, hashes, resources, evidence, references, legacy = {}, {}, {}, [], set(),set()
    paths = [maps / 'shared.map'] + sorted(p for p in maps.glob('*.map') if p.name != 'shared.map')
    for path in paths:
        cache = HudCache(path)
        evidence.append(dict(map=str(path), sha256=digest(path)))
        for tag in cache.tags.values():
            if not tag['offset']: continue
            if tag['group'] == 'nhdt':
                for widget in cache.hud(tag)['bitmap']:
                    if widget['bitmap']:references.add(widget['bitmap']['name'])
                    if widget['bitmap'] and widget['shader']:
                        shader=cache.shader(cache.tags[int(widget['shader']['datum'],16)])
                        if shader['template']['name'].endswith('\\widget_legacy_meter'):
                            legacy.add(widget['bitmap']['name'])
            if tag['group'] != 'bitm' or not tag['name'].startswith('ui\\hud\\'): continue
            bitmap = cache.bitmap(tag)
            pages = []
            for frame, p in enumerate(cache.block(tag['offset'] + 68, 116)):
                pointer, size = cache.u(p + 28), cache.u(p + 52)
                owner = [path, maps/'mainmenu.map', maps/'shared.map', maps/'single_player_shared.map'][pointer >> 30]
                key = (str(owner), pointer & 0x3fffffff, size)
                if key not in resources:
                    with owner.open('rb') as stream:
                        stream.seek(key[1]); resources[key] = stream.read(size)
                    if len(resources[key]) != size: raise ValueError('Truncated original HUD resource')
                raw = resources[key]
                sha = hashlib.sha256(raw).hexdigest()
                identity = (tag['name'], frame)
                if identity in hashes and hashes[identity] != sha:
                    raise ValueError(f'Map-specific HUD artwork requires separate variants: {identity}')
                hashes[identity] = sha
                page = bitmap['images'][frame].copy()
                page.update(raw=raw, owner=str(owner), pointer=hex(pointer), resource_sha256=sha)
                pages.append(page)
            signature = [(p['width'], p['height'], p['format'], p['flags']) for p in pages]
            if tag['name'] in banks:
                assert signature == banks[tag['name']]['signature'], 'Different source bank layouts'
            else:
                banks[tag['name']] = dict(signature=signature, pages=pages, source_map=str(path),
                                           sequences=bitmap['sequences'])
        cache.data.close(); cache.stream.close()
    if references - banks.keys(): raise ValueError(f'Uninventoried HUD widget references: {references - banks.keys()}')
    return banks, evidence, references, legacy


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--vista', type=Path, required=True)
    parser.add_argument('--maps', type=Path, default=ROOT/'assets/disc/maps')
    parser.add_argument('--codec', type=Path, default=ROOT/'build-hud-4k-trial/hud_4k_codec')
    parser.add_argument('--output', type=Path, default=ROOT/'assets/hud-4k-vectors')
    args = parser.parse_args()
    vista, output, maps = args.vista.resolve(), args.output.resolve(), args.maps.resolve()
    if output == vista or vista in output.parents: parser.error('Output must be separate from donor')
    output.mkdir(parents=True, exist_ok=True)
    banks, evidence, references, legacy = inventory(maps)
    donor = {}
    for line in (vista/'hud.tsv').read_text().splitlines()[1:]:
        name, frame, count, w, h, relative = line.split('\t')
        assert name in banks and len(banks[name]['pages']) == int(count)
        page = banks[name]['pages'][int(frame)]
        assert (page['width'], page['height']) == (int(w), int(h))
        donor[name, int(frame)] = relative
    # Decode every original page independently, including pages replaced by
    # Vista donors, so coverage and decoder parity can be checked separately.
    work = output/'source-resources'; work.mkdir(exist_ok=True)
    requests = []
    pages = [(name, frame, page) for name, bank in sorted(banks.items())
             for frame, page in enumerate(bank['pages'])]
    for i, (name, frame, page) in enumerate(pages):
        raw, decoded = work/f'{i:04}.bin', work/f'{i:04}.rgba'
        raw.write_bytes(page['raw'])
        requests.append(f'{page["width"]} {page["height"]} {page["format"]} {page["flags"]} {json.dumps(str(raw),ensure_ascii=False)} {json.dumps(str(decoded),ensure_ascii=False)}')
    donor_paths = set(donor.values())
    donor_paths.update(line.split('\t')[-1] for line in (vista/'scoreboard.tsv').read_text().splitlines()[1:])
    decoded_donors = {}
    for i, relative in enumerate(sorted(donor_paths)):
        source = vista/relative
        header = source.read_bytes()[:128]
        height, width = struct.unpack_from('<II', header, 12)
        decoded = work/f'donor-{i:04}.rgba'
        requests.append(f'{width} {height} 65535 0 {json.dumps(str(source),ensure_ascii=False)} {json.dumps(str(decoded),ensure_ascii=False)}')
        decoded_donors[relative] = (width, height, decoded)
    (work/'decode.tsv').write_text('\n'.join(requests)+'\n')
    subprocess.run([str(args.codec.resolve()), str(work/'decode.tsv')], check=True)
    manifest, records = ['H2UI_SAMPLING_PACK_V2'], []
    for i, (name, frame, page) in enumerate(pages):
        short = name.removeprefix('ui\\hud\\bitmaps\\').replace('\\', '/')
        original = Image.frombytes('RGBA', (page['width'], page['height']), (work/f'{i:04}.rgba').read_bytes())
        original_path = output/'sources'/short/f'{frame:04}.dds'
        write_dds(original_path, original)
        relative = f'bitmaps/hud/{name.replace(chr(92), "/")}/{frame:04}.dds'
        if (name, frame) in donor:
            source = vista/donor[name, frame]
            width, height, decoded = decoded_donors[donor[name, frame]]
            image = Image.frombytes('RGBA', (width,height), decoded.read_bytes())
            scale = 8 if short == 'hud_sensor_blip_custom' else 4
            provenance = dict(source=str(source), source_sha256=digest(source), source_kind='Vista')
        else:
            image, scale = original, 8
            provenance = dict(source=page['owner'], resource_pointer=page['pointer'],
                              resource_sha256=page['resource_sha256'], source_kind='Xbox')
        svg=Path('vectors/hud')/short/f'{frame:04}.svg'
        enlarged,metrics = redraw(image,scale,output/svg,discrete=(2,) if name in legacy else ())
        write_dds(output/relative, enlarged)
        manifest.append(f'{name}\t{frame}\t{len(banks[name]["pages"])}\t{page["width"]}\t{page["height"]}\t{relative}')
        records.append(dict(tag=name, frame=frame, original_size=image.size,
                            logical_size=original.size, reconstructed_size=enlarged.size,
                            density_multiplier=scale, output=relative,
                            output_sha256=digest(output/relative), vector=str(svg),vector_sha256=digest(output/svg),
                            discrete_channels=['blue'] if name in legacy else [], **metrics,**provenance))
        print(f'HUD {len(records)}/328 {short} {frame}: traced {image.size} -> {enlarged.size}',flush=True)
    shutil.copyfile(output/'sources/new_hud/meters/smg_meter/0000.dds', output/'sources/smg_meter.dds')
    (output/'hud.tsv').write_text('\n'.join(manifest)+'\n')
    for name in ('hud-alignment.tsv', 'scoreboard.json'):
        shutil.copyfile(vista/name, output/name)
    scoreboard_records = []
    scoreboard = (vista/'scoreboard.tsv').read_text()
    for line in scoreboard.splitlines()[1:]:
        name, frame, count, w, h, relative = line.split('\t')
        width, height, decoded = decoded_donors[relative]
        image = Image.frombytes('RGBA', (width,height), decoded.read_bytes())
        svg=Path('vectors/scoreboard')/name.replace('\\','/')/f'{int(frame):04}.svg'
        enlarged,metrics=redraw(image,2,output/svg)
        write_dds(output/relative, enlarged)
        scoreboard_records.append(dict(tag=name, frame=int(frame), source=str(vista/relative),
                                       source_sha256=digest(vista/relative), output=relative,
                                       original_size=image.size, reconstructed_size=enlarged.size,density_multiplier=2,
                                       vector=str(svg),vector_sha256=digest(output/svg),output_sha256=digest(output/relative),**metrics))
    (output/'scoreboard.tsv').write_text(scoreboard)
    from hud_vector_fonts import build_fonts
    fonts=build_fonts(maps,vista,output)
    (output/'hud-reconstruction.tsv').write_text('H2_HUD_RECONSTRUCTION_V1\nradar-size\t256\n')
    report = dict(schema='halo2-hud-vector-trace-v3', source_pack=str(vista),
                  source_manifest_sha256=digest(vista/'hud.tsv'), algorithm='Potrace cubic Bezier source tracing; independent channel coverage rasterization',
                  image_generation=False,curve_optimization_tolerance=.075,threshold_sampling_density=2, source_maps=evidence, bitmap_banks=len(banks),
                  bitmap_pages=len(records), widget_referenced_banks=sorted(references),
                  textures=records, scoreboard=scoreboard_records, fonts=fonts,
                  source_bank_metadata={name:dict(source_map=bank['source_map'], signature=bank['signature'],
                                                  sequences=bank['sequences']) for name, bank in banks.items()},
                  limits='Design-faithful cubic tracing of supplied artwork, with source-derived subpixel contours and new analytic antialiasing. This is a redraw, not recovered original vector masters or identical source pixels. Logical geometry, metrics and legacy ammo selector labels are retained. All glyphs in the active English font table are included; localized font tables are not selected by this runtime.')
    (output/'reconstruction.json').write_text(json.dumps(report, indent=2)+'\n')
    print(f'{len(records)} HUD pages in {len(banks)} banks, {len(scoreboard_records)} scoreboard pages, {len(fonts)} full fonts; all traced to editable vectors', flush=True)


if __name__ == '__main__':
    main()


