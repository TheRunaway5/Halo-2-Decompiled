#!/usr/bin/env python3
"""Wait for an owned build, then run the requested finite native walk review.

Run through run_test.py. This is a dependency wait in the current build/review
session; all raw captures and caches use that enclosing scratch lifecycle.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import time
from test_deserted_crossings import test
from test_deserted_native import review
from audit_deserted_compiled import audit
from deserted_collision import validate_cache
from mcc_content import MccCache


def run(root,runtime,wait_build):
    deadline=time.monotonic()+5400
    if wait_build:
        previous=None
        while True:
            try:state=json.loads((root/'build-progress.json').read_text())
            except (FileNotFoundError,json.JSONDecodeError):state={}
            status=state.get('status','starting')
            current=(status,len(state.get('steps',[])))
            if current!=previous:
                print('Build:',status,state.get('steps',[])[-1:] or '',flush=True)
                previous=current
            if status=='compiled; native runtime review pending':break
            if status=='incomplete':raise RuntimeError(state)
            if time.monotonic()>deadline:raise TimeoutError('Owned build did not finish within the bounded review session')
            time.sleep(10)
    cache=MccCache(root/'content/halo2/Mods/deserted/deserted.map')
    resident=[t for t in cache.tags.values() if t['address'] and t['size']]
    span=max(t['address']+t['size'] for t in resident)-min(t['address'] for t in resident)
    (root/'evidence/metadata-span-final.json').write_text(json.dumps(dict(passed=span<=128*1024**2,
        resident_span=span,resident_allocated_bytes=sum(t['size'] for t in resident),limit=128*1024**2,
        scope='Local resident tag address span; native initialization additionally validates merged/shared and normalized animation allocations'),indent=2)+'\n')
    assert span<=128*1024**2, 'Native metadata span exceeds the existing 128MiB admission bound'
    variants=root/'model/native-material-variants.json'
    if variants.exists():
        bank=root/'content/native-shaders/maps/dust2_mombasa'
        lib=json.loads((bank/'library.json').read_text())
        records={(r['group'],r['name']):r for r in lib['shader_tags']}
        tags={(t['group'],t['name'].replace('/','\\')) for t in cache.tags.values()}
        checked=[]
        for row in json.loads(variants.read_text())['variants']:
            original=records[('shad',row['source_shader'].replace('/','\\'))]
            name=row['shader'].replace('/','\\');bitmap=row['bitmap'].replace('/','\\')
            assert ('shad',name) in tags and ('bitm',bitmap) in tags
            expected=copy.deepcopy(original);expected['name']=name;replaced=0
            for ref in expected['references']:
                if ref[1]==['bitm',row['source_bitmap'].replace('/','\\')]:
                    ref[1][1]=bitmap;replaced+=1
            assert replaced==3 and records[('shad',name)]==expected
            payload=bank/'payloads'/expected['payload']
            assert hashlib.sha256(payload.read_bytes()).hexdigest()==payload.stem
            checked.append(dict(shader=name,bitmap=bitmap,original_payload=expected['payload'],bitmap_references=replaced))
        (root/'evidence/native-material-variants-final.json').write_text(json.dumps(dict(passed=True,variants=checked,
            scope='Compiled bitmap/shader present; original native paver payload preserved with exactly three base-map reference changes'),indent=2)+'\n')
    validate_cache(cache,
        root/'evidence/collision-cache-final.json')
    del cache
    assert audit(root,root/'evidence/compiled-faces-final.json')==0
    print('Compiled render coverage and collision liveness passed; native walks starting',flush=True)
    crossing=test(root,runtime,'native-fixed',
        ['t-spawn','b-crossing','upper-fork-to-t','lower-fork-to-mid'])
    review(root,runtime)
    if crossing:raise RuntimeError('Native crossing failed; inspect retained poses and frames')
    print('Native crossings and render/walk review passed',flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',required=True,type=Path)
    p.add_argument('--runtime',required=True,type=Path)
    p.add_argument('--wait-build',action='store_true')
    run(**vars(p.parse_args()))
