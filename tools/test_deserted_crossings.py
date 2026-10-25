#!/usr/bin/env python3
"""Exercise Deserted's cross-map routes with real native player input.

Run through tools/run_test.py. No teleport/camera root is used for movement.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
from PIL import Image


def route_cases():
    fork=[(33,math.pi/2),(251,math.pi),(138,-math.pi/2),(34,0)]
    return [
        ('ct-junction', [(30, math.pi/2)], [1.14, 19.5], .7),
        ('mid', [(68, math.pi), (100, -math.pi/2)], [-3.56, 10.7], .8),
        ('t-spawn', [(68, math.pi), (500, -math.pi/2)], [-4., -7.], 1.5),
        ('b-crossing', [(30, math.pi/2), (251, math.pi)], [-16.2, 19.5], 1.5),
        ('upper-fork-to-t', fork+[(68,-math.pi/2),(65,math.atan2(-4.4,-.8)),
            (60,math.atan2(-4.,-1.)),(52,-math.pi/2)], [-16,-6.5], 1.6),
        # Native prefix captures place the diagonal turn at y10.919. Reach
        # the authored y12 corridor before turning east, rather than driving
        # into its visible southern wall at y11.25.
        ('lower-fork-to-mid', fork+[(35,0),(37,math.atan2(2.,1.6)),(14,math.pi/2),(95,0)], [-3.6,12], 1.6),
        ('upper-tunnel-to-b', fork+[(32,math.pi),(112,math.pi/2)], [-16.4,17.5], 1.6),
        ('b-site-interior', [(30,math.pi/2),(213,math.pi),(70,math.pi/2),(26,math.pi)], [-16.3,24.5], 1.2),
    ]


def test(root, runtime, label, case=None):
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    cache = root/'content/halo2/Mods/deserted/deserted.map'
    cases=route_cases()
    result = dict(cache_sha256=hashlib.sha256(cache.read_bytes()).hexdigest(),
        runtime_sha256=hashlib.sha256(runtime.read_bytes()).hexdigest(),
        source_glb_sha256=hashlib.sha256((root/'model/dust2_mombasa.glb').read_bytes()).hexdigest(),
        test_script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        scope='Actual native Fiesta player input from CT spawn; no repositioning', cases=[])
    env = dict(os.environ, __GL_SHADER_DISK_CACHE_PATH=str(scratch/'driver'),
        MESA_SHADER_CACHE_DIR=str(scratch/'mesa'))
    for name, segments, expected, tolerance in cases:
        if case and name not in (case if isinstance(case,list) else [case]):continue
        rows = []
        for count, yaw in segments:
            for _ in range(count):
                rows.append(f'{len(rows)} 0 0 0 0 {yaw} -0.08 move 1 0\n')
        script = scratch/(name+'.input');script.write_text(''.join(rows))
        frame = scratch/(name+'.ppm');log = scratch/(name+'.log')
        command = [str(runtime), '--multiplayer', '--map', str(cache),
            '--shared-maps', str(root/'content/halo2/h2_maps_win64_dx11'),
            '--local-players', '1', '--custom-slayer', 'fiesta', '--graphics-mode', 'Original',
            '--viewport', '1280', '720', '--mp-input-script', str(script),
            '--mp-capture-tick', str(len(rows)), '--screenshot', str(frame),
            '--mp-reference-auxiliary-seed', '305419896', '--mp-rule', '10', '0']
        with log.open('w') as stream:
            run = subprocess.run(command, cwd=root, env=env, stdout=stream,
                stderr=subprocess.STDOUT, timeout=150)
        text = log.read_text(errors='replace')
        positions = re.findall(r'player_position=([-+0-9.eE]+),([-+0-9.eE]+),([-+0-9.eE]+)', text)
        position = list(map(float, positions[-1])) if positions else None
        distance = math.dist(position[:2], expected) if position else None
        reached = (distance is not None and distance<tolerance)
        if name=='t-spawn' and position:
            reached=-6.5<position[0]<-2 and -10.5<position[1]<-6 and .8<position[2]<1.3
        passed = run.returncode == 0 and reached and position[2] > -1.5
        row = dict(name=name, position=position, expected_xy=expected, distance=distance,
            passed=passed, exit_code=run.returncode, command=command,
            input_script_sha256=hashlib.sha256(script.read_bytes()).hexdigest(),ticks=len(rows))
        result['cases'].append(row)
        print(json.dumps(row), flush=True)
        (root/'evidence'/f'{label}-{name}.log').write_text(text[-65536:])
        if frame.exists():
            Image.open(frame).save(root/'evidence'/f'{label}-{name}.png')
    assert result['cases'],'No requested native crossing cases selected'
    result['passed'] = all(c['passed'] for c in result['cases'])
    (root/'evidence'/f'{label}-crossings.json').write_text(json.dumps(result, indent=2)+'\n')
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', required=True, type=Path)
    p.add_argument('--runtime', required=True, type=Path)
    p.add_argument('--label', default='crossing')
    p.add_argument('--case',action='append',choices=('ct-junction','mid','t-spawn','b-crossing','upper-fork-to-t','lower-fork-to-mid','upper-tunnel-to-b','b-site-interior'))
    raise SystemExit(test(**vars(p.parse_args())))
