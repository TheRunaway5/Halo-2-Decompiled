#!/usr/bin/env python3
"""Capture a sequential, matched Vista/trial comparison with one runtime."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import numpy as np
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/hud-4k-vector'


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    sources=dict(line.split('\t',1) for line in (ROOT/'assets/asset-sources.tsv').read_text().splitlines()[1:] if line)
    vista=Path(sources['vista']);vista=vista if vista.is_absolute() else ROOT/vista
    runtime=ROOT/'build-hud-4k-trial/halo2_native';fingerprint=sha(runtime)
    pack=ROOT/'assets/hud-4k-vectors/reconstruction.json';pack_sha=sha(pack)
    inputs=OUT/'idle-input.tsv';inputs.write_text('0 0 0 0\n')
    env=os.environ.copy();env['SDL_VIDEODRIVER']='x11'
    env['VK_LAYER_PATH']=str(ROOT/'.tools/validation/usr/share/vulkan/explicit_layer.d')
    libraries=str(ROOT/'.tools/validation/usr/lib')
    env['LD_LIBRARY_PATH']=libraries+(':'+env['LD_LIBRARY_PATH'] if env.get('LD_LIBRARY_PATH') else '')
    captures=[]
    # Xvfb auto-selection can race another auto-selected display. Start well
    # above the ordinary pool and run the pair sequentially, checking exits.
    for label in ['vista','trial']:
        screenshot=OUT/(label+'-4k.ppm');log=OUT/(label+'-4k.log')
        command=['xvfb-run','-a','-n','223','-s','-screen 0 3840x2160x24',str(ROOT/'Halo2-HUD-4K-Preview')]
        if label=='vista':command+=['--hud-ui-assets',str(vista)]
        command+=['--mp-input-script',str(inputs),'--mp-capture-tick','40','--screenshot',str(screenshot),
                  '--hidden','--validation','--mute']
        with log.open('w') as stream:subprocess.run(command,cwd=ROOT,env=env,stdout=stream,stderr=subprocess.STDOUT,check=True)
        assert sha(runtime)==fingerprint and sha(pack)==pack_sha,'Capture inputs changed between runs'
        image=Image.open(screenshot).convert('RGB');assert image.size==(3840,2160)
        png=screenshot.with_suffix('.png');image.save(png)
        text=log.read_text()
        state=next(line for line in text.splitlines() if line.startswith('Input capture tick='))
        assert 'Validation Error' not in text and 'VUID-' not in text
        captures.append(dict(label=label,command=command,log=str(log),image=str(png),sha256=sha(png),state=state))
        print(f'{label}: captured3840x2160 at tick40',flush=True)
    assert captures[0]['state']==captures[1]['state'],'Capture gameplay states differ'
    a=Image.open(captures[0]['image']);b=Image.open(captures[1]['image'])
    regions={'grenades':[180,140,720,290],'ammo':[3100,110,3760,570],
             'radar-shield':[100,1460,850,2130],'crosshair':[1700,1100,2140,1460],
             'score-timer':[3080,1660,3760,1980]}
    changed=np.any(np.array(a)!=np.array(b),axis=2);allowed=np.zeros(changed.shape,bool)
    for name,rect in regions.items():
        x0,y0,x1,y1=rect;allowed[y0:y1,x0:x1]=True
        w,h=x1-x0,y1-y0;canvas=Image.new('RGB',(w*2,h+42),(15,20,30));draw=ImageDraw.Draw(canvas)
        draw.text((12,12),'Vista source',fill='white');draw.text((w+12,12),'Vector redraw',fill='white')
        canvas.paste(a.crop(rect),(0,42));canvas.paste(b.crop(rect),(w,42));canvas.save(OUT/(name+'-comparison.png'))
    outside=int((changed&~allowed).sum());assert outside==0,'Matched world image changed outside inspected HUD regions'
    result=dict(runtime_sha256=fingerprint,pack_report_sha256=pack_sha,dimensions=a.size,captures=captures,
                changed_pixels=int(changed.sum()),hud_inspection_regions=regions,
                changed_pixels_outside_hud_inspection_regions=outside,
                claim='One matched port-render frame; vector tracing is not an original-game retail framebuffer comparison.')
    (OUT/'capture-comparison.json').write_text(json.dumps(result,indent=2)+'\n')
    print('Matched world pixels outside the inspected HUD regions are unchanged',flush=True)


if __name__=='__main__':main()
