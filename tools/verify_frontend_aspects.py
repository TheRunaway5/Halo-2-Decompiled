#!/usr/bin/env python3
"""Capture the real frontend in four host aspects and after a window resize.

This checks the centered original menu viewport and presentation borders. It
does not establish full-width scenery or every frontend screen's layout/flow.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image, ImageDraw
from qualify_frontend_runtime import intro_capture_witness

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary',type=Path,default=ROOT/'build-frontend/halo2_native')
parser.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/decoded-haze-aspects')
parser.add_argument('--tick',type=int,default=181)
args = parser.parse_args()
binary = args.binary.resolve()
binary_sha256 = hashlib.sha256(binary.read_bytes()).hexdigest()
output = args.output.resolve()
output.mkdir(parents=True,exist_ok=True)
environment = dict(os.environ,SDL_VIDEODRIVER='x11',VK_LAYER_VALIDATE_SYNC='1')
environment['VK_LAYER_PATH'] = str(ROOT/'.tools/validation/usr/share/vulkan/explicit_layer.d')
environment['LD_LIBRARY_PATH'] = str(ROOT/'.tools/validation/usr/lib')
cases = [('4-3',640,480,False),('16-9',1280,720,False),
         ('16-10',1280,800,False),('21-9',1680,720,False),('resize',1280,800,True)]
checks = []
for name,width,height,resize in cases:
    if hashlib.sha256(binary.read_bytes()).hexdigest() != binary_sha256:
        raise RuntimeError('frontend runtime changed between display captures')
    target = output / f'{name}.ppm'
    # Count proof frames only after the live new-map overlay clears. The
    # frozen camera/animation sample is independent of that source clock.
    command = [str(binary),'--menu','--skip-intro','--menu-tick',str(args.tick),
               '--freeze-menu','--frames','8' if resize else '3','--viewport',str(width),str(height),
               '--menu-wait-intro-clear','--hidden','--mute','--validation','--screenshot',str(target)]
    if resize:
        command.append('--resize-test')
    result = subprocess.run(command,cwd=ROOT,env=environment,capture_output=True,text=True,timeout=120)
    if hashlib.sha256(binary.read_bytes()).hexdigest() != binary_sha256:
        raise RuntimeError(f'frontend runtime changed during {name} capture')
    log = result.stdout + result.stderr
    (output / f'{name}.log').write_text(log)
    if result.returncode:
        raise RuntimeError(f'{name} frontend capture failed: {result.returncode}')
    intro_clear = intro_capture_witness(log, 8 if resize else 3)
    image = Image.open(target).convert('RGB')
    if resize:
        width,height = 1000,600
    assert image.size == (width,height), (name,image.size,(width,height))
    image.save(output / f'{name}.png')
    # Match the renderer's float32 calculation and integer scissor conversion.
    scale = min(np.float32(width)/np.float32(640),np.float32(height)/np.float32(480))
    vw,vh = np.float32(640)*scale,np.float32(480)*scale
    vx,vy = (np.float32(width)-vw)*np.float32(.5),(np.float32(height)-vh)*np.float32(.5)
    x,y,w,h = int(vx),int(vy),int(vw),int(vh)
    pixels = np.asarray(image)
    border = np.ones((height,width),dtype=bool)
    border[y:y+h,x:x+w] = False
    count = int(np.count_nonzero(pixels[border]))
    visible = int(np.count_nonzero(pixels[y:y+h,x:x+w]))
    validation_error = 'Validation Error' in log or 'VUID-' in log
    check = {'aspect':name,'binary_sha256':binary_sha256,'size':[width,height],'viewport':[float(vx),float(vy),float(vw),float(vh)],
             'scissor':[x,y,w,h],'border_pixels':int(border.sum()),'nonblack_border_channels':count,
             'nonblack_viewport_channels':visible,'waited_for_map_intro_clear':True,
             'validation_error':validation_error,'return_code':result.returncode,
             'intro_clear':intro_clear}
    checks.append(check)
    print(json.dumps(check),flush=True)
report = {'binary_sha256':binary_sha256,
          'scope':'real native frames, fixed camera/animation, centered source 4:3 menu viewport',
          'checks':checks,'passed':all(not c['nonblack_border_channels'] and
              c['nonblack_viewport_channels'] > 0 and not c['validation_error'] for c in checks),
          'matched_original_pixels':False,'extended_scene_aspect_support':False,
          'all_frontend_layout_flow_verified':False}
(output / 'validation.json').write_text(json.dumps(report,indent=2)+'\n')
gallery = Image.new('RGB',(960,778),(12,16,27))
draw = ImageDraw.Draw(gallery)
for index,(name,_,_,_) in enumerate(cases[:4]):
    source = Image.open(output / f'{name}.png').convert('RGB')
    source.thumbnail((470,350),Image.Resampling.LANCZOS)
    left,top = (index%2)*480,24+(index//2)*389
    draw.text((left+12,top-16),name.replace('-',':'),fill=(231,237,246))
    gallery.paste(source,(left+(480-source.width)//2,top+(350-source.height)//2))
gallery.save(output / 'title-aspect-grid.png')
raise SystemExit(not report['passed'])
