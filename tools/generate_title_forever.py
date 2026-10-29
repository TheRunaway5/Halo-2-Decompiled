#!/usr/bin/env python3
"""Freeze the website's finite Arial subtitle; no runtime font is required."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import zlib

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ARIAL_SHA256 = 'c9b76220a5be42ead4733611e417cd65c5fd8aeaa33eb56576ac378a37d130a1'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--arial', type=Path, default=Path('/usr/share/fonts/microsoft/Microsoft-Fonts-main/arial.ttf'))
    parser.add_argument('--cli', type=Path, default=Path.home() / '.codex/skills/playwright/scripts/playwright_cli.sh')
    parser.add_argument('--output', type=Path, default=ROOT / 'src/ui/title_forever_artwork.inc')
    parser.add_argument('--manifest', type=Path, default=ROOT / 'resources/branding/title-forever.json')
    args = parser.parse_args()
    font = args.arial.read_bytes()
    if hashlib.sha256(font).hexdigest() != ARIAL_SHA256:
        raise RuntimeError('Expected the recorded Arial Regular 7.00 reference font')
    # The image remains inline, exactly as on the website: its baseline gap
    # contributes to the distance before the block subtitle's top margin.
    image = base64.b64encode(b'<svg xmlns="http://www.w3.org/2000/svg" width="1024" height="128"/>').decode()
    html = ('<style>@font-face{font-family:TitleArial;src:url(data:font/ttf;base64,' +
            base64.b64encode(font).decode() + ') format("truetype");font-weight:400}'
            'html,body{margin:0;background:transparent}'
            'body{font-family:TitleArial;font-size:15px;line-height:1.65}'
            '#logo{width:365px}img{width:100%;height:auto}'
            '#subtitle{display:block;text-align:center;font-size:12px;letter-spacing:8px;'
            'color:white;margin-top:12px}</style><div id="logo"><img id="image" src="data:image/svg+xml;base64,' +
            image + '"><span id="subtitle">F O R E V E R</span></div>')
    with tempfile.TemporaryDirectory(prefix='title-forever-', dir=os.environ.get('HALO2_TEST_SCRATCH')) as temp:
        scratch = Path(temp)
        session = 'forever-' + scratch.name.rsplit('-', 1)[-1]
        def cli(*argv):
            result = subprocess.run([str(args.cli), '-s=' + session, *argv], cwd=scratch,
                                    capture_output=True, text=True, timeout=90)
            if result.returncode:
                raise RuntimeError((result.stdout + result.stderr)[-4000:])
        try:
            cli('open', 'about:blank', '--idle-timeout=60000')
            reference_html = scratch / 'reference.html'
            reference_html.write_text(html)
            script = scratch / 'capture.js'
            script.write_text('async page => {\n'
                'const context = await page.context().browser().newContext({deviceScaleFactor:8,viewport:{width:400,height:160}});\n'
                'try { const reference = await context.newPage();\n'
                'await reference.goto(' + json.dumps(reference_html.as_uri()) + '); await reference.evaluate(() => document.fonts.ready);\n'
                'const metrics=await reference.evaluate(()=>{const a=document.querySelector("#image").getBoundingClientRect();'
                'const b=document.querySelector("#subtitle").getBoundingClientRect();'
                'return {logo_width:a.width,logo_height:a.height,span_width:b.width,span_height:b.height,span_gap:b.top-a.bottom,'
                'font:getComputedStyle(document.querySelector("#subtitle")).fontFamily};});\n'
                'await reference.locator("#subtitle").screenshot({path:' + json.dumps(str(scratch / 'mask.png')) + ',omitBackground:true,scale:"device"});\n'
                'return metrics;\n'
                '} finally { await context.close(); }\n}')
            # Measurement returns plain JSON; image output remains in scratch.
            result = subprocess.run([str(args.cli), '-s=' + session, '--raw', 'run-code', '--filename=' + str(script)],
                                    cwd=scratch, capture_output=True, text=True, timeout=90)
            if result.returncode:
                raise RuntimeError((result.stdout + result.stderr)[-4000:])
            metrics = None
            for index, char in enumerate(result.stdout):
                if char != '{':
                    continue
                try:
                    candidate, _ = json.JSONDecoder().raw_decode(result.stdout[index:])
                    if isinstance(candidate, dict) and 'logo_width' in candidate:
                        metrics = candidate
                        break
                except json.JSONDecodeError:
                    pass
            if metrics is None:
                raise RuntimeError('Browser metrics unavailable: ' + result.stdout[-2500:])
            mask = Image.open(scratch / 'mask.png').convert('RGBA').getchannel('A')
            bounds = mask.getbbox()
            if not bounds:
                raise RuntimeError('Empty website subtitle raster')
            bounds = (bounds[0] - 2, bounds[1] - 2, bounds[2] + 2, bounds[3] + 2)
            mask = mask.crop(bounds)
            raw = mask.tobytes()
            encoded = zlib.compress(raw, 9)
            left = bounds[0] / 8
            top = metrics['span_gap'] + bounds[1] / 8
            manifest = dict(schema='H2_WEBSITE_FOREVER_ARTWORK_V1', text='F O R E V E R',
                font='Arial Regular 7.00', font_sha256=ARIAL_SHA256, density=8,
                css=dict(font_size=12, letter_spacing=8, line_height=1.65, margin_top=12, color='#b6d9f7'),
                browser_metrics=metrics, logical_left=left, logical_top_after_logo=top,
                width=mask.width, height=mask.height, alpha_sha256=hashlib.sha256(raw).hexdigest(),
                compressed_bytes=len(encoded), font_file_included=False)
            rows = [','.join(str(v) for v in encoded[i:i+24]) for i in range(0, len(encoded), 24)]
            source = ('// Generated by tools/generate_title_forever.py; see resources/branding/title-forever.json.\n'
                f'constexpr unsigned forever_width={mask.width}, forever_height={mask.height};\n'
                f'constexpr float forever_left={left:.8f}f, forever_top={top:.8f}f;\n'
                'constexpr unsigned char forever_alpha_zlib[]{\n    ' + ',\n    '.join(rows) + '\n};\n')
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(source)
            args.manifest.parent.mkdir(parents=True, exist_ok=True)
            args.manifest.write_text(json.dumps(manifest, indent=2) + '\n')
            print(json.dumps(manifest, indent=2))
        finally:
            cli('close')


if __name__ == '__main__':
    main()
