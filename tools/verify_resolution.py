#!/usr/bin/env python3
"""Capture fixed retail poses across resolutions and report native image stability.

Image differences measure the native renderer across resolutions, not agreement
with the Xbox framebuffer. Original arithmetic is checked by the differential
viewport harness and the GPU texture/viewport test separately.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import subprocess
from PIL import Image, ImageChops, ImageStat


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=Path('build-appearance/halo2_native'))
    parser.add_argument('--map', type=Path, default=Path('assets/disc/maps/01b_spacestation.map'))
    parser.add_argument('--output', type=Path, default=Path('analysis/appearance/resolution'))
    parser.add_argument('--validation', action='store_true')
    args = parser.parse_args()
    binary = args.binary.resolve()
    binary_hash = hashlib.sha256(binary.read_bytes()).hexdigest()
    shaders = binary.parent/'shaders'
    shader_hashes = {p.name:hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in sorted(shaders.glob('*.spv'))}
    args.output.mkdir(parents=True, exist_ok=True)
    sizes = [(640,480),(1280,960),(960,540),(1280,720),(1920,1080),(3840,2160)]
    results = []
    for weapon in ['battle_rifle','smg']:
        for width,height in sizes:
            name = f'{weapon}-{width}x{height}'
            capture,log = args.output/(name+'.ppm'),args.output/(name+'.log')
            command = [str(binary),'--map',str(args.map.resolve()),'--weapon',weapon,
                       '--first-person-pose','idle','0','--viewport',str(width),str(height),
                       '--hidden','--mute','--frames','3','--screenshot',str(capture.resolve())]
            if args.validation:command.append('--validation')
            with log.open('w') as output:
                run = subprocess.run(command,stdout=output,stderr=subprocess.STDOUT,timeout=60)
            text = log.read_text()
            validation = [l for l in text.splitlines() if 'Vulkan validation:' in l]
            record = {'weapon':weapon,'resolution':[width,height], 'exit_code':run.returncode,
                      'validation_messages':validation,'capture':str(capture),'log':str(log),
                      'passed':run.returncode==0 and capture.is_file() and not validation and
                               'Rendered 3 frames' in text}
            if record['passed']:
                with Image.open(capture) as frame:
                    record['passed'] &= frame.size==(width,height)
                    record['rgb_extrema'] = frame.getextrema()
                    # Check the existing workbench HUD marker's actual pixel
                    # footprint. This does not certify authored retail HUD art.
                    scale = min(width/640,height/480)
                    length = max(1,math.floor(11*scale+.5))
                    thickness = max(1,math.floor(scale+.5))
                    cx,cy = width//2,height//2
                    def marker(x,y):
                        rgb = frame.getpixel((x,y))
                        return min(rgb)>=225 and max(rgb)-min(rgb)<=2
                    row = [marker(x,cy) for x in range(cx-length,cx+length+1)]
                    column = [marker(cx,y) for y in range(cy-length,cy+length+1)]
                    def centered_span(values):
                        left = right = len(values)//2
                        if not values[left]:return 0
                        while left and values[left-1]:left-=1
                        while right+1<len(values) and values[right+1]:right+=1
                        return right-left+1
                    vertical_stroke = centered_span([marker(x,cy+length//4)
                        for x in range(cx-length,cx+length+1)])
                    horizontal_stroke = centered_span([marker(cx+length//4,y)
                        for y in range(cy-length,cy+length+1)])
                    record['workbench_marker'] = {'expected_length':length,'expected_thickness':thickness,
                        'horizontal_length':centered_span(row),'vertical_length':centered_span(column),
                        'horizontal_thickness':horizontal_stroke,'vertical_thickness':vertical_stroke}
                    record['passed'] &= centered_span(row)==length and centered_span(column)==length and \
                                        vertical_stroke==thickness and horizontal_stroke==thickness
                    frame.save(args.output/(name+'.png'))
            results.append(record)
            print(f"{name}: {'passed' if record['passed'] else 'FAILED'}",flush=True)
    comparisons = []
    for weapon in ['battle_rifle','smg']:
        for width,height in sizes:
            reference = (640,480) if width*3==height*4 else (960,540)
            if (width,height)==reference:continue
            paths = [args.output/f'{weapon}-{w}x{h}.ppm' for w,h in [reference,(width,height)]]
            if not all(any(r['weapon']==weapon and r['resolution']==[w,h] and r['passed']
                           for r in results) for w,h in [reference,(width,height)]):continue
            with Image.open(paths[0]) as low,Image.open(paths[1]) as high:
                normalized = high.resize(low.size,Image.Resampling.LANCZOS)
                difference = ImageStat.Stat(ImageChops.difference(low,normalized))
                comparisons.append({'weapon':weapon,'reference_resolution':reference,
                    'resolution':[width,height], 'mean_absolute_rgb_difference':difference.mean,
                    'rms_rgb_difference':difference.rms})
    unchanged = binary_hash==hashlib.sha256(binary.read_bytes()).hexdigest() and shader_hashes=={
        p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(shaders.glob('*.spv'))}
    report = {'binary':str(binary),'binary_sha256':binary_hash,'spirv_sha256':shader_hashes,
              'artifacts_unchanged_during_sweep':unchanged,
              'validation':args.validation,'captures':len(results),
              'passed':sum(r['passed'] for r in results),'results':results,
              'native_resolution_comparisons':comparisons,
              'scope':'fixed-pose native capture dimensions, Vulkan validity and resampled RGB stability; texture filtering changes with pixel footprint',
              'original_frame_comparison_performed':False,'all_retail_effects_implemented':False}
    (args.output/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if unchanged and all(r['passed'] for r in results) else 1


if __name__=='__main__':raise SystemExit(main())
