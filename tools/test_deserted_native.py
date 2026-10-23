#!/usr/bin/env python3
"""Finite native-engine render/walk review for the requested Deserted map.

Run in tools/run_test.py. Raw frames, input scripts and driver caches are owned
scratch. Selected PNG references, bounded logs and a receipt are retained.
Camera-only views are labelled separately from actual player movement.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import numpy as np
from PIL import Image


def review(root,runtime,mode='fiesta'):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    evidence=root/'evidence';cache=root/'content/halo2/Mods/deserted/deserted.map'
    shared=root/'content/halo2/h2_maps_win64_dx11'
    env=dict(os.environ,__GL_SHADER_DISK_CACHE_PATH=str(scratch/'driver-cache'),
        MESA_SHADER_CACHE_DIR=str(scratch/'mesa-cache'))
    base=[str(runtime),'--multiplayer','--map',str(cache),'--shared-maps',str(shared),
        '--local-players','1','--graphics-mode','Original','--viewport','1280','720']
    result=dict(schema='H2_DESERTED_NATIVE_REVIEW_V1',passed=False,
        cache_sha256=hashlib.sha256(cache.read_bytes()).hexdigest(),
        runtime_sha256=hashlib.sha256(runtime.read_bytes()).hexdigest(),cases=[])

    def source_publication_boundary(text):
        lower=text.lower()
        return ('required original runtime backing unavailable' in lower or
            'tu1.5 original bsp resource publication did not finish:' in lower)

    def capture(name,mode,rows,ticks,camera=None,allow_failure=False):
        script=scratch/(name+'.input');script.write_text(''.join(rows))
        image=scratch/(name+'.ppm')
        command=base+(['--custom-slayer',mode] if mode else [])+[
            '--mp-input-script',str(script),'--mp-capture-tick',str(ticks),'--screenshot',str(image),
            '--mp-reference-auxiliary-seed','305419896']
        if camera:command+=['--mp-reference-camera',*map(str,camera)]
        command+=['--mp-rule','10','0']
        with (scratch/(name+'.log')).open('w') as log:
            run=subprocess.run(command,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=150)
        text=(scratch/(name+'.log')).read_text(errors='replace')
        (evidence/(name+'.log')).write_text(text[-65536:])
        positions=re.findall(r'player_position=([-+0-9.eE]+),([-+0-9.eE]+),([-+0-9.eE]+)',text)
        row=dict(name=name,mode=mode or 'slayer',exit_code=run.returncode,ticks=ticks,
            scope='native camera inspection' if camera else 'native player input and render',
            player_position=list(map(float,positions[-1])) if positions else None,
            passed=run.returncode==0 and image.exists(),command=command,
            audio_warnings=[s for s in text.splitlines() if 'Native gait/footstep owner:' in s])
        result['cases'].append(row)
        if row['passed']:
            Image.open(image).save(evidence/(name+'.png'))
        elif not allow_failure:
            raise RuntimeError(name+' failed: '+text[-3000:])
        print(name,row['exit_code'],row['player_position'],flush=True)
        return row,text

    try:
        idle=[f'{i} 0 0 0 0 0 -0.08 move 0 0\n' for i in range(30)]
        mode='' if mode=='slayer' else mode
        spawn,text=capture('native-spawn-'+(mode or 'slayer'),mode,idle,30,allow_failure=True)
        if not spawn['passed']:
            assert source_publication_boundary(text),text[-3000:]
            result['standard_slayer_boundary']=text[-3000:]
            mode='fiesta'
            spawn,text=capture('native-spawn-fiesta',mode,idle,30)
        assert spawn['player_position'] is not None
        x0,y0,z0=spawn['player_position']
        ct=-1<x0<5.5 and 16<y0<22
        yaw=0. if ct or y0>-4 else math.pi
        walk,text=capture('native-straight-walk',mode,
            [f'{i} 0 0 0 0 {yaw} -0.08 move 1 0\n' for i in range(130)],130)
        x1,y1,z1=walk['player_position']
        moved=math.hypot(x1-x0,y1-y0)
        assert moved>1.,('Player did not traverse the authored floor',moved)
        result['actual_walk_distance_world_units']=moved
        if ct and x1-x0>1:
            speed=(x1-x0)/130
            turn=max(1,round((12.0-x0)/speed))
            north=max(1,round((24.2-y0)/speed))
            assert turn+north<900
            rows=[]
            for i in range(turn+north+20):
                angle=0 if i<turn else math.pi/2
                move=1 if i<turn+north else 0
                rows.append(f'{i} 0 0 0 0 {angle} -0.06 move {move} 0\n')
            traverse,text=capture('native-ct-to-a-walk',mode,rows,len(rows))
            x,y,z=traverse['player_position']
            assert 10.5<x<15.5 and 23.6<y<26.7 and .77<z<1.3,('CT-to-A walk did not reach A site',traverse)
            result['ct_to_a_walk_verified']=True
        for name,view in [
                ('native-ct-exit',[6.8,17.64,.45,1,0,-.08,0,0,1]),
                ('native-ct-west-passage',[.8,19.4,-.43,-1,0,-.08,0,0,1]),
                ('native-t-approach',[-3.6,-3.5,.7,0,-1,-.08,0,0,1]),
                ('native-b-tunnel-fork',[-14.2,10,.9,1,0,-.1,0,0,1]),
                ('native-b-site',[-16,21,.85,0,-1,-.12,0,0,1]),
                ('native-upper-tunnel',[-15,1,.65,0,1,-.03,0,0,1])]:
            capture(name,mode,idle,30,camera=view)
        authored_views=root/'native-review-cameras.json'
        if authored_views.exists():
            views=json.loads(authored_views.read_text())['cameras']
            assert 1<=len(views)<=24
            for view in views:
                pose=view['pose'];assert len(pose)==9 and np.isfinite(pose).all()
                capture(view['name'],mode,idle,30,camera=pose)
            result['authored_camera_review']=dict(cameras=len(views),
                source=str(authored_views),scope='Authored Blender camera poses converted to native world units; engine FOV, shaders and baked lighting reviewed independently')
        if mode:
            lighting,text=capture('native-lighting-regression-slayer','',idle,30,allow_failure=True)
            if lighting['passed'] and (evidence/'native-before-shared-collision.png').exists():
                before=np.asarray(Image.open(evidence/'native-before-shared-collision.png').convert('RGB'))
                after=np.asarray(Image.open(evidence/'native-lighting-regression-slayer.png').convert('RGB'))
                dark=float(before[620:700,800:980].mean());lit=float(after[620:700,800:980].mean())
                result['weapon_lighting_regression']=dict(before_rgb_mean=dark,after_rgb_mean=lit,
                    scope='same Slayer seed, starting pose and SMG foreground region',passed=lit>dark+4)
                assert lit>dark+4,('Weapon remains black in the direct native regression',dark,lit)
            elif lighting['passed']:
                result['weapon_lighting_regression']=dict(passed=None,
                    scope='No matching retained standard Slayer baseline for this authored geometry; current native lighting views retained')
            else:
                assert source_publication_boundary(text),text[-3000:]
                result['standard_slayer_boundary']=text[-3000:]
                result['weapon_lighting_regression']=dict(passed=False,
                    scope='Same-pose standard Slayer comparison was blocked by original BSP resource publication; Fiesta render reviewed independently')
        result['mode']=mode or 'slayer';result['time_limit_rule']={'parameter':10,'value':0}
        result['passed']=True
    finally:
        (evidence/'native-runtime-review.json').write_text(json.dumps(result,indent=2)+'\n')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',required=True,type=Path)
    p.add_argument('--runtime',required=True,type=Path)
    p.add_argument('--mode',choices=('fiesta','slayer'),default='fiesta')
    review(**vars(p.parse_args()))
