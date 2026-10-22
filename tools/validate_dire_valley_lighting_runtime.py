#!/usr/bin/env python3
"""Replay the actual Dire Valley lighting/crash cases in owned test scratch.

Use tools/run_test.py. Completion checks are executable evidence; retained
captures still require visual review. Camera cases declare camera inputs only.
"""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import subprocess
import tempfile
import time

from port_dire_valley import save, sha


def validate(package, runtime, report, case='base', retain_capture=False):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    package=package.resolve(strict=True);runtime=runtime.resolve(strict=True)
    cache=package/'content/halo2/Mods/dire_valley/dire_valley.map'
    runtime_pin,cache_pin=sha(runtime),sha(cache)
    shared=package/'content/halo2/h2_maps_win64_dx11'
    views=2 if case=='two' else 1
    ticks=900 if case=='walk' else 30 if case in ('foliage','vehicle') else 90
    camera=None
    if case in ('foliage','vehicle'):
        filename='foliage-camera.json' if case=='foliage' else 'vehicle-camera.json'
        camera=json.loads((package/'evidence/lighting-repair'/filename).read_text())['camera']
        if len(camera)!=9:raise ValueError('Expected a declared nine-component reference camera')
    route=[(0,90,0.,1),(90,390,1.5707963267948966,1),
        (390,480,3.141592653589793,1),(480,780,-1.5707963267948966,1),(780,900,0.,0)]
    with tempfile.TemporaryDirectory(dir=scratch,prefix='dire-valley-light-') as owned:
        work=Path(owned);inputs=work/'input.tsv';frame=work/'frame.ppm';log=work/'runtime.log'
        rows=[]
        for tick in range(ticks):
            for view in range(views):
                yaw=0.;forward=int(case=='base' and tick<30)
                if case=='walk':
                    _,_,yaw,forward=next(row for row in route if row[0]<=tick<row[1])
                rows.append(f'{tick} {view} 0 0 0 {yaw} 0 move {forward} 0\n')
        inputs.write_text(''.join(rows))
        command=[str(runtime),'--multiplayer','--map',str(cache),'--shared-maps',str(shared),
            '--local-players',str(views),'--mp-rule','10','0','--graphics-mode','Original',
            '--mp-input-script',str(inputs),'--mp-capture-tick',str(ticks),
            '--mp-reference-tick-cadence','--hidden','--mute','--screenshot',str(frame)]
        if camera:command+=['--mp-reference-camera',*map(str,camera)]
        began=time.monotonic();timeout=False
        with log.open('wb') as output:
            try:code=subprocess.run(command,cwd=Path(__file__).resolve().parents[1],stdout=output,
                stderr=subprocess.STDOUT,timeout=240,env=dict(os.environ,SDL_VIDEODRIVER='x11')).returncode
            except subprocess.TimeoutExpired:code=124;timeout=True
        raw=log.read_bytes();text=raw.decode(errors='replace')
        completed=f'ticks={ticks} players={views} connected=1 peak_players={views}' in text
        record=dict(schema='H2_DIRE_VALLEY_LIGHTING_RUNTIME_V1',passed=code==0 and completed and frame.is_file(),
            case=case,runtime=str(runtime),runtime_sha256=runtime_pin,cache_sha256=cache_pin,
            command=command,code=code,timeout=timeout,seconds=time.monotonic()-began,completed=completed,
            declared_camera=camera,declared_route=route if case=='walk' else None,
            log_sha256=hashlib.sha256(raw).hexdigest(),tail=text.splitlines()[-6:],
            scope='Actual map input/source-world/render replay. Completion is not an automatic image-quality or full gameplay-parity verdict.')
        compressed=lzma.compress(raw);assert lzma.decompress(compressed)==raw
        if sha(runtime)!=runtime_pin or sha(cache)!=cache_pin:
            record.update(passed=False,inputs_changed_during_replay=True)
        report.parent.mkdir(parents=True,exist_ok=True)
        report.with_suffix('.log.xz').write_bytes(compressed)
        if retain_capture and record['passed']:
            from PIL import Image
            destination=report.with_suffix('.png');Image.open(frame).save(destination)
            record['retained_capture']=dict(path=str(destination),bytes=destination.stat().st_size,sha256=sha(destination))
        save(report,record)
    return record


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for option in ('package','runtime','report'):parser.add_argument('--'+option,type=Path,required=True)
    parser.add_argument('--case',choices=('base','foliage','vehicle','walk','two'),default='base')
    parser.add_argument('--retain-capture',action='store_true')
    result=validate(**vars(parser.parse_args()));print(json.dumps(result))
    raise SystemExit(0 if result['passed'] else 1)
