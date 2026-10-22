#!/usr/bin/env python3
"""Wait for an owned build, then run the requested finite native walk review.

Run through run_test.py. This is a dependency wait in the current build/review
session; all raw captures and caches use that enclosing scratch lifecycle.
"""
import argparse
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
    validate_cache(MccCache(root/'content/halo2/Mods/deserted/deserted.map'),
        root/'evidence/collision-cache-final.json')
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
