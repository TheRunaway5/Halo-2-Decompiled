#!/usr/bin/env python3
"""Make a PlayStation companion to the port's authored Xbox SVG prompt bank.

Uses the preserved editable masters, never game font bitmaps. Intermediate
renders live in an owned temporary directory; --output retains the requested
artwork, previews and lossless atlases. Does not activate a runtime overlay.
"""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image
from vista_prompt_source import write_dds

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'resources/presentation/prompts'
SVG = 'http://www.w3.org/2000/svg'
ET.register_namespace('', SVG)
SCHEMA = 'H2_PORT_PLAYSTATION_PROMPTS_V1'
XBOX_SCHEMA = 'H2_PORT_XBOX_PROMPTS_V1'
FACE = ('cross', 'circle', 'square', 'triangle')
COLORS = ('#91baff', '#ff8c91', '#f3a0dc', '#7ee5bd')
SYMBOLS = (
    'M9.2 12.8 L22.8 26.4 M22.8 12.8 L9.2 26.4',
    'M24 20 A8 8 0 1 1 8 20 A8 8 0 1 1 24 20 Z',
    'M8.5 12.5 H23.5 V27.5 H8.5 Z',
    'M16 11 L25 27 H7 Z',
)
# Outlined labels share the original shoulder letterforms. No font dependency.
LETTERS = {
    'L': 'M0 0 H1.5 V8.5 H6 V10 H0 Z',
    'R': 'M0 0 H4.4 C6.5 0 7.2 1.2 7.2 3 C7.2 4.4 6.5 5.3 5.2 5.5 '
         'L7.7 10 H5.9 L3.5 5.8 H1.5 V10 H0 Z '
         'M1.5 1.4 V4.4 H4.1 C5.3 4.4 5.7 4 5.7 2.9 C5.7 1.9 5.2 1.4 4.1 1.4 Z',
    '1': 'M1 1.8 L3.5 0 H5 V8.5 H7 V10 H1 V8.5 H3.3 V2 L1.8 3 Z',
    '2': 'M0 2.7 C0 -.9 7.4 -.9 7.4 2.8 C7.4 4.7 5.2 6.1 2.2 8.5 '
         'H7.5 V10 H0 V8.3 C2 6.5 5.8 4.3 5.8 2.8 C5.8 .9 1.6 1 1.6 2.7 Z',
    '3': 'M0 1.6 C2.3 -.9 7.4 -.5 7.4 2.7 C7.4 4 6.7 4.7 5.8 5 '
         'C8.8 5.8 8 10.3 3.8 10.3 C2 10.3 .7 9.7 0 8.8 L1.1 7.6 '
         'C3.6 9.6 6.3 9 6.1 7 C6 5.8 4 5.8 2.7 5.8 V4.2 '
         'C7.5 4.6 6.2 -.5 1.1 2.8 Z',
}


def node(parent, tag, **attrs):
    return ET.SubElement(parent, '{' + SVG + '}' + tag,
                         {key.replace('_', '-'): str(value) for key, value in attrs.items()})


def by_id(root, identity):
    return next(item for item in root.iter() if item.get('id') == identity)


def remove(root, identities):
    for parent in root.iter():
        for child in list(parent):
            if child.get('id') in identities:
                parent.remove(child)


def paint(root, identity, colors):
    for stop, color in zip(by_id(root, identity), colors):
        stop.set('stop-color', color)


def label(parent, name, x, y, scale=1, color='#263e55'):
    group = node(parent, 'g', id='label-' + name.lower(),
                 transform=f'translate({x} {y}) scale({scale})', fill=color, fill_rule='evenodd')
    for i, letter in enumerate(name):
        node(group, 'path', d=LETTERS[letter], transform=f'translate({i * 8.5} 0)')


def control_name(row, family='playstation'):
    code = row['codepoint']
    if family == 'xbox':
        names = ('a', 'b', 'x', 'y', 'lb', 'rb', 'lt', 'rt', 'dpad-up', 'dpad-down',
                 'dpad-left', 'dpad-right', 'start', 'back', 'left-stick-press', 'right-stick-press',
                 'left-stick', 'right-stick')
        return names[code - 0xe100] if 0xe100 <= code <= 0xe111 else row['family'] + f'-{code:04x}'
    if 0xe100 <= code <= 0xe103:
        return FACE[code - 0xe100]
    names = {0xe104: 'l1', 0xe105: 'r1', 0xe106: 'l2', 0xe107: 'r2',
             0xe108: 'dpad-up', 0xe109: 'dpad-down', 0xe10a: 'dpad-left',
             0xe10b: 'dpad-right', 0xe10c: 'options', 0xe10d: 'share-create'}
    if code in names:
        return names[code]
    if code == 0xe10e:
        return 'l3'
    if code == 0xe10f:
        return 'r3'
    if code in (0xe110, 0xe12c, 0xe12d, 0xe12e):
        return 'left-stick'
    if code in (0xe111, 0xe12f, 0xe130, 0xe131):
        return 'right-stick'
    return row['family'] + f'-{code:04x}'


def convert(source, row, platform='playstation'):
    root = ET.parse(source / row['vector']).getroot()
    code, family = row['codepoint'], row['family']
    name = control_name(row, platform)
    root.find('{*}title').text = f'{platform.title()} {name}: custom controller prompt'
    root.find('{*}desc').text = ('Editable vector companion to the port-authored Xbox masters. '
        'Preserves the original canvas and control placement; uses PlayStation symbols and labels. '
        'No embedded bitmap, external font, filter or recovered Sony artwork.')
    if platform == 'xbox':
        if family == 'shoulder-button' and code == 0xe105:
            by_id(root, 'letter-B').set('transform', 'translate(8.7 0)')
        return root
    if family == 'face-button':
        remove(root, {'left-reflection'})
        index = code - 0xe100
        color = COLORS[index]
        paint(root, 'button-body-paint', ('#222939', '#343c4e', '#202636', '#30394a', '#454f62'))
        paint(root, 'button-rim-paint', ('#b6c2d4', '#131a28', '#65748c'))
        paint(root, 'top-sheen-paint', ('#c1cbdf',) * 3)
        paint(root, 'left-reflection-paint', (color,) * 3)
        remove(root, {'letter-raised-shadow', 'letter-bevel', 'letter-face'})
        group = by_id(root, 'face-button')
        # Three thin layers give the geometric symbols the same raised edge
        # treatment as the Xbox letters, retaining their open centers.
        for identity, ink, width, offset in (
                ('symbol-shadow', '#0b1324', 3.4, 'translate(.25 .65)'),
                ('symbol-bevel', color, 2.65, 'translate(0 0)'),
                ('symbol-face', color, 1.65, 'translate(0 0)')):
            node(group, 'path', id=identity, d=SYMBOLS[index], fill='none',
                 stroke=ink, stroke_width=width, stroke_linecap='round',
                 stroke_linejoin='round', transform=offset)
        by_id(root, 'symbol-bevel').set('stroke-opacity', '.45')
    elif family == 'shoulder-button':
        old = by_id(root, 'label')
        old.clear()
        old.set('id', 'label')
        old.set('transform', 'translate(33 14.2) scale(1 .85)' if code == 0xe104
                else 'translate(12.8 14.2) scale(1 .85)')
        label(old, name.upper(), 0, 0)
    elif family == 'trigger-pair':
        # Keep both source silhouettes and their active/inactive emphasis.
        # Labels are outside the luminance mask so their ink remains opaque.
        width, height = row['source_size']
        group = node(root, 'g', transform=f'scale({width / 90} {height / 38})')
        for side, x in enumerate((13.5, 59.5)):
            label(group, ('L2', 'R2')[side], x, 13, 1.1,
                  '#e4edf8' if side == code - 0xe106 else '#5c697d')
    elif family in ('legacy-metal-button', 'legacy-shoulder-letter'):
        # The compact selectors also need readable labels, including the old
        # black/white shoulder slots. Keep their original material/silhouette.
        if code in (0xe104, 0xe105):
            label(by_id(root, 'legacy-metal-button'), name.upper(), 3, 6.5, .58,
                  '#263e55' if code == 0xe105 else '#edf3fc')
        elif code in (0xe106, 0xe107):
            remove(root, {'letter-L', 'letter-R'})
            label(by_id(root, 'legacy-shoulder-letter'), name.upper(), 2.2, 5.4, .76, '#edf3fc')
        elif code in (0xe10c, 0xe10d):
            remove(root, {'direction-symbol'})
            menu_symbol(by_id(root, 'legacy-metal-button'), code, 'translate(0 0) scale(.5)', '#edf3fc')
        else:
            remove(root, {'letter-L', 'letter-R'})
            ink = {'left-stick': 'L', 'right-stick': 'R'}.get(name, name.upper())
            label(by_id(root, 'legacy-metal-button'), ink, 3 if len(ink) == 2 else 6, 6, .6, '#edf3fc')
    elif family == 'start-back-button':
        remove(root, {'arrow'})
        menu_symbol(by_id(root, 'start-back-button'), code, 'translate(.5 0)', '#405269')
    elif family == 'stick-pair':
        # E10E/F are presses; training_controls.cpp uses E110/111 for
        # movement/look. Preserve that distinction instead of labeling every
        # stick pose L3/R3. Fit the ink to each actual cap, not the canvas.
        side = 'left-stick' if name in ('l3', 'left-stick') else 'right-stick'
        cap = by_id(by_id(root, side), 'cap')
        ink = {'left-stick': 'L', 'right-stick': 'R'}.get(name, name.upper())
        size = min(float(cap.get('rx')) * 1.25 / (16 if len(ink) == 2 else 8),
                   float(cap.get('ry')) * 1.15 / 10)
        label(root, ink, float(cap.get('cx')) - (8 if len(ink) == 2 else 4) * size,
              float(cap.get('cy')) - 5 * size, size, '#f2f6ff')
    # Keyboard frames, lock and ambiguous four-pane special glyphs retain
    # their source ownership. Touchpad artwork is a separate, unbound master.
    return root


def touchpad():
    root = ET.Element('{' + SVG + '}svg', width='240', height='144', viewBox='0 0 60 36')
    node(root, 'title').text = 'PlayStation touchpad: standalone unbound prompt'
    defs = node(root, 'defs')
    for identity, colors in (('pad-face', ('#edf3fa', '#b7c7d9')),
                             ('pad-rim', ('#a0b3cb', '#586b85'))):
        gradient = node(defs, 'linearGradient', id=identity, x1=0, x2=0, y1=0, y2=1)
        for offset, color in zip((0, 1), colors):
            node(gradient, 'stop', offset=offset, stop_color=color)
    node(root, 'rect', x=1.5, y=8.5, width=57, height=25, rx=4.5,
         fill='url(#pad-face)', stroke='url(#pad-rim)', stroke_width=1)
    node(root, 'path', d='M6 10 H54 Q57 10 57 13 V15 H3 V13 Q3 10 6 10 Z',
         fill='#ffffff', fill_opacity='.5')
    for y in (15, 21, 27):
        for x in range(9, 54, 6):
            node(root, 'circle', cx=x, cy=y, r=.7, fill='#5e7694')
    return root


def menu_symbol(parent, code, transform, color):
    group = node(parent, 'g', id='options' if code == 0xe10c else 'share-create',
                 transform=transform, fill='none', stroke=color, stroke_width=1.8,
                 stroke_linecap='round', stroke_linejoin='round')
    if code == 0xe10c:
        node(group, 'path', d='M9 15 H23 M9 21 H23 M9 27 H23')
    else:
        node(group, 'path', d='M16 25 V18 M10.5 16 L7.5 13 M21.5 16 L24.5 13 M16 13 V9')
        node(group, 'rect', x=12, y=23, width=8, height=6, rx=1.4)


def save(root, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    ET.indent(root)
    ET.ElementTree(root).write(path, encoding='utf-8', xml_declaration=True)


def render(path):
    data = subprocess.check_output(['rsvg-convert', str(path)])
    return Image.open(io.BytesIO(data)).convert('RGBA')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def preview(out, source, rows, platform):
    # Self-contained SVG compositions contain the actual masters. Prefix IDs
    # and references to prevent one button's gradients affecting another.
    def embed(root, path, x, y, width=122, height=122):
        item = ET.parse(path).getroot()
        prefix = f'item-{len(root)}-'
        for part in item.iter():
            for key, value in list(part.attrib.items()):
                if key == 'id':
                    part.set(key, prefix + value)
                elif 'url(#' in value:
                    part.set(key, value.replace('url(#', 'url(#' + prefix))
        item.set('x', str(x)); item.set('y', str(y))
        item.set('width', str(width)); item.set('height', str(height))
        root.append(item)

    def canvas(width, height, title, subtitle):
        root = ET.Element('{' + SVG + '}svg', width=str(width), height=str(height),
                          viewBox=f'0 0 {width} {height}')
        node(root, 'rect', width=width, height=height, fill='#151e2d')
        node(root, 'text', x=32, y=49, fill='#edf4ff', font_family='sans-serif',
             font_size=27, font_weight=600).text = title
        node(root, 'text', x=32, y=78, fill='#92a7c3', font_family='sans-serif',
             font_size=14).text = subtitle
        return root

    examples = {row['codepoint']: row for row in rows if row['selector'] == 6}
    picks = list(range(0xe100, 0xe112)) + (['touchpad'] if platform == 'playstation' else [])
    gallery = canvas(1040, 930, platform.upper(), 'Custom button prompts')
    for i, code in enumerate(picks):
        row = examples[code] if code != 'touchpad' else dict(vector='vectors/touchpad.svg', control='touchpad')
        x, y = 32 + i % 4 * 252, 108 + i // 4 * 160
        embed(gallery, out / row['vector'], x + 30, y, 176, 116)
        caption = ({0xe10e: 'L3 / press', 0xe10f: 'R3 / press',
                   0xe110: 'Left stick / move', 0xe111: 'Right stick / look',
                   0xe10d: 'Share / Create'} if platform == 'playstation' else {}).get(code, row['control'].replace('-', ' ').title())
        node(gallery, 'text', x=x + 118, y=y + 140, fill='#b7c7dc',
             font_family='sans-serif', font_size=15, text_anchor='middle').text = caption
    save(gallery, out / 'buttons.svg')
    render(out / 'buttons.svg').save(out / 'buttons.png')

    if platform == 'xbox':
        return
    comparison = canvas(1200, 442, 'XBOX / PLAYSTATION', 'Matching custom prompt families')
    picks = list(range(0xe100, 0xe108))
    for i, code in enumerate(picks):
        row = examples[code]
        xbox = out.parent / 'prompts-xbox'
        # The caller supplies the staged Xbox bank when generating a pair.
        xbox = Path(os.environ.get('HALO2_XBOX_PROMPT_PREVIEW', str(xbox)))
        xbox_rows = json.loads((xbox / 'reconstruction.json').read_text())['records'] if xbox.is_dir() else []
        xbox_row = next((r for r in xbox_rows if r['selector'] == 6 and r['codepoint'] == code), None)
        embed(comparison, xbox / xbox_row['vector'] if xbox_row else source / row['source_vector'],
              i * 146 + 14, 112, 130, 120)
        embed(comparison, out / row['vector'], i * 146 + 14, 262, 130, 120)
        node(comparison, 'text', x=i * 146 + 79, y=414, fill='#b7c7dc',
             font_family='sans-serif', font_size=14, text_anchor='middle').text = row['control'].upper()
    save(comparison, out / 'comparison.svg')
    render(out / 'comparison.svg').save(out / 'comparison.png')


def build(source, out, platform):
    reference = json.loads((source / 'reconstruction.json').read_text())
    if platform == 'playstation':
        save(touchpad(), out / 'vectors/touchpad.svg')
    rows, images = [], {}
    for original in reference['records']:
        root = convert(source, original, platform)
        identity = hashlib.sha256(ET.tostring(root)).hexdigest()
        relative = f'vectors/{control_name(original, platform)}-{identity[:16]}.svg'
        if relative not in images:
            save(root, out / relative)
            images[relative] = render(out / relative)
        rows.append(dict(selector=original['selector'], codepoint=original['codepoint'],
                         control=control_name(original, platform), family=original['family'],
                         source_vector=original['vector'], source_vector_sha256=digest(source / original['vector']),
                         source_size=original['source_size'], raster_size=original['raster_size'],
                         vector=relative, vector_sha256=digest(out / relative)))
    table, atlases = ['H2_VISTA_PROMPT_RASTERS_V1'], {}
    for selector in range(12):
        selected = [row for row in rows if row['selector'] == selector]
        signature = tuple(row['vector'] for row in selected)
        if signature not in atlases:
            width, x, y, row_height, positions = 1024, 1, 1, 0, []
            for row in selected:
                image = images[row['vector']]
                if x + image.width + 1 > width:
                    x, y, row_height = 1, y + row_height + 2, 0
                positions.append((x, y))
                x += image.width + 2
                row_height = max(row_height, image.height)
            atlas = Image.new('RGBA', (width, y + row_height + 1), (255, 255, 255, 0))
            for row, xy in zip(selected, positions):
                atlas.paste(images[row['vector']], xy)
            relative = f'atlases/{len(atlases)}.dds'
            write_dds(out / relative, atlas)
            atlases[signature] = relative, positions
        relative, positions = atlases[signature]
        for row, (x, y) in zip(selected, positions):
            width, height = row['raster_size']
            row.update(atlas=relative, rectangle=[x, y, width, height])
            table.append('\t'.join(map(str, (selector, row['codepoint'], x, y, width, height, relative))))
    (out / 'prompts.tsv').write_text('\n'.join(table) + '\n')
    (out / 'prompt-source.tsv').write_text('H2_VISTA_PROMPTS_V1\n.\n')
    preview(out, source, rows, platform)
    report = dict(schema=SCHEMA if platform == 'playstation' else XBOX_SCHEMA, platform=platform, density=4,
                  source_manifest_sha256=digest(source / 'manifest.json'),
                  source_reconstruction_sha256=digest(source / 'reconstruction.json'),
                  bindings=len(rows), unique_vectors=len(images), atlas_count=len(atlases), records=rows,
                  supplementary_artwork=['vectors/touchpad.svg'] if platform == 'playstation' else [])
    (out / 'reconstruction.json').write_text(json.dumps(report, indent=2) + '\n')
    files = {str(path.relative_to(out)): dict(bytes=path.stat().st_size, sha256=digest(path))
             for path in sorted(out.rglob('*')) if path.is_file()}
    (out / 'manifest.json').write_text(json.dumps(dict(schema=report['schema'], files=files), indent=2) + '\n')
    return verify(source, out)


def verify(source, out):
    """Replay saved masters and independently decode DDS with Pillow."""
    report = json.loads((out / 'reconstruction.json').read_text())
    manifest = json.loads((out / 'manifest.json').read_text())
    reference = json.loads((source / 'reconstruction.json').read_text())
    assert manifest['schema'] == report['schema'] and report['schema'] in (SCHEMA, XBOX_SCHEMA)
    assert digest(source / 'manifest.json') == report['source_manifest_sha256']
    assert digest(source / 'reconstruction.json') == report['source_reconstruction_sha256']
    for name, info in manifest['files'].items():
        path = out / name
        assert path.stat().st_size == info['bytes'] and digest(path) == info['sha256'], name
    original = {(row['selector'], row['codepoint']): row for row in reference['records']}
    actual = {(row['selector'], row['codepoint']): row for row in report['records']}
    assert len(actual) == len(report['records']) and original.keys() == actual.keys()
    lines = (out / 'prompts.tsv').read_text().splitlines()
    assert lines[0] == 'H2_VISTA_PROMPT_RASTERS_V1' and len(lines) == len(actual) + 1
    expected_lines = ['\t'.join(map(str, (row['selector'], row['codepoint'], *row['rectangle'], row['atlas'])))
                      for row in report['records']]
    assert lines[1:] == expected_lines
    rasters, atlases, checked = {}, {}, 0
    for key, row in actual.items():
        prior = original[key]
        assert row['source_size'] == prior['source_size'] and row['raster_size'] == prior['raster_size']
        assert digest(source / row['source_vector']) == row['source_vector_sha256']
        path = out / row['vector']
        assert digest(path) == row['vector_sha256']
        if row['vector'] not in rasters:
            root = ET.parse(path).getroot()
            assert not any(item.tag.rsplit('}', 1)[-1] in ('image', 'text', 'filter', 'script', 'foreignObject')
                           for item in root.iter()), path
            assert not any(key.rsplit('}', 1)[-1] == 'href' for item in root.iter() for key in item.attrib), path
            rasters[row['vector']] = render(path)
        image = rasters[row['vector']]
        assert list(image.size) == row['raster_size'] and image.getchannel('A').getbbox()
        if row['atlas'] not in atlases:
            atlases[row['atlas']] = Image.open(out / row['atlas']).convert('RGBA')
        atlas = atlases[row['atlas']]
        x, y, width, height = row['rectangle']
        assert x > 0 and y > 0 and x + width < atlas.width and y + height < atlas.height
        assert np.array_equal(np.array(image), np.array(atlas.crop((x, y, x + width, y + height))))
        checked += 1
    for relative in report['supplementary_artwork']:
        image = render(out / relative)
        assert image.getchannel('A').getbbox() and image.size == (240, 144)
    return dict(passed=True, bindings=checked, vector_replays=len(rasters), atlases=len(atlases),
                supplementary_vectors=len(report['supplementary_artwork']),
                dimensions_preserved=True,
                bytes=sum(path.stat().st_size for path in out.rglob('*') if path.is_file()))


def publish(staging, destination):
    # Rebuild only a previously generated, unmodified bank. Do not overwrite
    # hand edits or unrelated files when iterating on the authoring tool.
    if destination.exists():
        manifest = json.loads((destination / 'manifest.json').read_text())
        if manifest.get('schema') not in (SCHEMA, XBOX_SCHEMA):
            raise ValueError('Destination is not an owned PlayStation prompt bank')
        expected = set(manifest['files']) | {'manifest.json'}
        found = {str(path.relative_to(destination)) for path in destination.rglob('*') if path.is_file()}
        if expected != found or any(digest(destination / name) != info['sha256']
                                   for name, info in manifest['files'].items()):
            raise ValueError('Destination has hand edits or unrelated files; select a new --output')
        # All files are authenticated generated outputs from this tool.
        for name in expected:
            (destination / name).unlink()
    shutil.copytree(staging, destination, dirs_exist_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=SOURCE)
    parser.add_argument('--platform', choices=('xbox', 'playstation'), default='playstation')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--output', type=Path)
    group.add_argument('--verify', type=Path)
    args = parser.parse_args()
    if args.verify:
        result = verify(args.source.resolve(), args.verify.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='playstation-prompts-', dir=os.environ.get('HALO2_TEST_SCRATCH')) as scratch:
            staging = Path(scratch) / 'bank'
            staging.mkdir()
            result = build(args.source.resolve(), staging, args.platform)
            publish(staging, args.output.resolve())
        result['temporary_directory_removed'] = not Path(scratch).exists()
    if os.environ.get('HALO2_TEST_SCRATCH'):
        result['test_scratch'] = os.environ['HALO2_TEST_SCRATCH']
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
