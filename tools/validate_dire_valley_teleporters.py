#!/usr/bin/env python3
"""Drive a private native A/B portal approach without editing the delivered map.

Run through tools/run_test.py. Initial birth placement is a declared test input.
Actual source movement/query/event owners execute; no successful pose, contact,
cooldown, activation event, memory backing or admission result is supplied.
"""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import re
import struct
import subprocess
import time

from gameplay_runtime_selection import selected_gameplay_runtime
from mcc_content import MccCache
from port_dire_valley import save, sha
from prepare_dire_valley_native import publish


def validate(package,mcc_root,report,runtime=None):
    repo=Path(__file__).resolve().parents[1]
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if runtime is None:runtime=repo/selected_gameplay_runtime(repo)['runtime']
    source=package/'content/halo2/Mods/dire_valley/dire_valley.map'
    authoring=json.loads((package/'authoring/scenario_authoring.json').read_text())
    metadata=MccCache(source);root=metadata.tags[metadata.u32(metadata.index+12)]['offset']
    flags=metadata.block(root+0x118,32)
    entrance=next(a for a in flags if struct.unpack_from('<h',metadata.data,a+16)[0]==7)
    exit=next(a for a in flags if struct.unpack_from('<h',metadata.data,a+16)[0]==8)
    sender=struct.unpack_from('<3f',metadata.data,entrance);destination=struct.unpack_from('<3f',metadata.data,exit)
    start=[sender[0],sender[1]-1.5,authoring['sender_fit']['opening_bounds_world'][0][2]+.001]
    inputs=scratch/'portal-approach.tsv'
    inputs.write_text(''.join(f'{i} 0 0 0 0 1.5707963267948966 0 move {int(i<30)} 0\n' for i in range(90)))
    result=dict(schema='H2_DIRE_VALLEY_NATIVE_TELEPORTER_AB_V2',passed=False,cases=[],
        original_cache_sha256=sha(source),runtime=str(runtime),runtime_sha256=sha(runtime),
        declared_start=start,declared_yaw=1.5707963267948966,forward_ticks=30,capture_tick=90,
        sender=sender,destination=destination,
        scope='One private initial start before the authored sender. Only control receiver type is disabled. Delivered starts/cache remain intact. Native rendering/input/source-world execution is observed. Traversal is accepted only if the enabled case reaches the actual exit while the control remains above. No audio or Xbox framebuffer parity claim.')
    save(report,result)
    for enabled in [False,True]:
        name='enabled' if enabled else 'disabled';c=MccCache(source);root=c.tags[c.u32(c.index+12)]['offset']
        starts=c.block(root+0x100,0x34);struct.pack_into('<I',c.data,root+0x100,1)
        struct.pack_into('<4f',c.data,starts[0],*start,1.5707963267948966)
        struct.pack_into('<h',c.data,starts[0]+16,0);struct.pack_into('<4h',c.data,starts[0]+20,12,0,0,0)
        if not enabled:
            for a in c.block(root+0x118,32):
                if struct.unpack_from('<h',c.data,a+16)[0]==8:struct.pack_into('<h',c.data,a+16,5)
        fixture=scratch/(name+'.map');fixture.write_bytes(c.data)
        content=scratch/(name+'-content')
        publish(fixture,mcc_root,content,package/'authoring/main-hud.json',scratch/(name+'-publication.json'))
        command=[str(runtime),'--multiplayer','--map',str(content/'halo2/Mods/dire_valley/dire_valley.map'),
            '--shared-maps',str(content/'halo2/h2_maps_win64_dx11'),'--local-players','1',
            '--mp-input-script',str(inputs),'--mp-capture-tick','90','--mp-reference-tick-cadence',
            '--hidden','--mute','--graphics-mode','Original','--mp-rule','10','0',
            '--viewport','1280','720','--screenshot',str(scratch/(name+'.ppm'))]
        log=scratch/(name+'.log');begin=time.monotonic();code=None;timeout=False
        try:
            with log.open('w') as f:code=subprocess.run(command,cwd=repo,stdout=f,stderr=subprocess.STDOUT,timeout=180).returncode
        except subprocess.TimeoutExpired:timeout=True
        text=log.read_text();match=re.search(r'player_position=([-\d.e]+),([-\d.e]+),([-\d.e]+)',text)
        position=[float(v) for v in match.groups()] if match else None
        raw=log.read_bytes();compressed=lzma.compress(raw)
        log_target=report.with_name(report.stem+'-'+name+'.log.xz');log_target.write_bytes(compressed)
        assert lzma.decompress(compressed)==raw
        row=dict(enabled=enabled,returncode=code,timeout=timeout,seconds=time.monotonic()-begin,
            position=position,completed='ticks=90 players=1 connected=1' in text,
            log=str(log_target),log_sha256=hashlib.sha256(raw).hexdigest(),
            log_tail=[line[:1800] for line in text.splitlines()[-5:]])
        result['cases'].append(row);save(report,result);print(json.dumps(row),flush=True)
    disabled,enabled=result['cases']
    if all(row['returncode']==0 and row['completed'] and row['position'] for row in result['cases']):
        result['passed']=(disabled['position'][2]>start[2]-.5 and
            abs(enabled['position'][2]-destination[2])<1.5 and
            sum((a-b)**2 for a,b in zip(enabled['position'][:2],destination[:2]))<4.)
    assert sha(source)==result['original_cache_sha256']
    save(report,result);return result['passed']


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for option in ['package','mcc-root','report']:parser.add_argument('--'+option,type=Path,required=True)
    parser.add_argument('--runtime',type=Path)
    raise SystemExit(0 if validate(**vars(parser.parse_args())) else 1)
