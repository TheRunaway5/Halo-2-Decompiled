#!/usr/bin/env python3
"""Capture native Vista menus, typing, real SDL wheel events and aspect layouts."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from PIL import Image


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--resume',action='store_true')
    parser.add_argument('--case',action='append',default=[])
    parser.add_argument('--resume-binary-sha256',help='record the original binary for retained captures')
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    binary=args.binary.resolve(); output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=args.resume)
    digest=hashlib.sha256(binary.read_bytes()).hexdigest()
    retained_digest=args.resume_binary_sha256 or digest
    assert re.fullmatch('[0-9a-f]{64}',retained_digest)
    env=os.environ.copy()
    env.update(SDL_VIDEODRIVER='x11',HALO2_MENU_FOG_SEED='305419896',
        VK_LAYER_PATH=str(root/'.tools/validation/usr/share/vulkan/explicit_layer.d'),
        LD_LIBRARY_PATH=str(root/'.tools/validation/usr/lib'))
    cases=[]
    for i,name in enumerate(('player','variants','video','audio','network','about')):
        cases.append((name,19,1280,720,121,['select:'+str(i)],False))
    for name,w,h in [('4x3',960,720),('16x10',1280,800),('21x9',1720,720),('32x9',2560,720)]:
        cases.append(('settings-'+name,19,w,h,121,[],False))
    cases += [('settings-resize',19,2560,720,121,[],True),
              ('player-typing',19,1280,720,12,[],False),
              ('about-popup',19,1280,720,3,['select:5','accept'],False),
              ('network-popup',19,1280,720,3,['select:4','accept'],False),
              ('keyboard',38,1280,720,121,['select:6'],False),
              ('video-options',65792,1280,720,3,[],False),
              ('controls-wheel',65789,1280,720,3,['select:40']+['wheel-down']*4,False),
              ('settings-wheel',19,1280,720,3,['wheel-up','wheel-down'],False)]
    if args.case:
        cases=[case for case in cases if case[0] in args.case]
        assert len(cases)==len(set(args.case)), 'unknown capture case'
    def capture(case):
        name,screen,w,h,frames,actions,resize=case
        # A disposable route suppresses loading/saving live profile/video
        # preferences, including a user's previously confirmed fullscreen.
        actions=actions or ['select:0']
        assert hashlib.sha256(binary.read_bytes()).hexdigest()==digest,'binary changed during captures'
        command=[str(binary),'--menu-screen',str(screen),'--skip-intro','--freeze-menu',
            '--menu-tick','181','--menu-ui-ms','100','--menu-ui-step-ms','50',
            '--frames',str(frames),'--hidden','--mute','--validation',
            '--viewport',str(w),str(h),'--screenshot',str(output/(name+'.ppm'))]
        for action in actions: command+=['--menu-action',action]
        if resize: command+=['--resize-test']
        expected=(1000,600) if resize else (w,h)
        expected_wheel={'controls-wheel':[41,42,1,2],'settings-wheel':[5,0]}.get(name,[])
        if args.resume and (output/(name+'.png')).exists():
            with Image.open(output/(name+'.png')) as image:
                assert image.size==expected
            old=(output/(name+'.log')).read_text()
            assert not re.findall(r'VUID-|Validation Error|validation error|Fatal',old)
            old_wheel=[int(x) for x in re.findall(r'Menu wheel screen=\d+ selected=(\d+)',old)]
            if old_wheel==expected_wheel:
                print(name,'retained PASS',flush=True)
                return dict(name=name,argv=command,extent=expected,wheel_selections=old_wheel,
                            binary_sha256=retained_digest,returncode=0,validation_errors=[])
        run=subprocess.run(command,cwd=root,env=env,stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT,text=True,timeout=240)
        (output/(name+'.log')).write_text(run.stdout)
        errors=re.findall(r'VUID-|Validation Error|validation error|Fatal',run.stdout)
        assert run.returncode==0 and not errors,(name,run.returncode,errors,run.stdout[-2000:])
        with Image.open(output/(name+'.ppm')) as image:
            assert image.size==expected,(name,image.size,expected)
            image.save(output/(name+'.png'))
        wheel=[int(x) for x in re.findall(r'Menu wheel screen=\d+ selected=(\d+)',run.stdout)]
        assert wheel==expected_wheel,(name,wheel,expected_wheel)
        print(name,'PASS',expected,'wheel',wheel,flush=True)
        return dict(name=name,argv=command,extent=expected,wheel_selections=wheel,
                    binary_sha256=digest,returncode=run.returncode,validation_errors=errors)
    with ThreadPoolExecutor(max_workers=2) as pool:
        runs=list(pool.map(capture,cases))
    report=dict(binary=str(binary),sha256=digest,runs=runs,
        scope='Native Linux SDL/Vulkan captures using actual imported Vista assets. Wheel cases enter SDL_PushEvent and the normal event adapter. Not an original Windows framebuffer comparison.')
    (output/'runs.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__': main()
