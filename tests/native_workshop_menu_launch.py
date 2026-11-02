#!/usr/bin/env python3
"""Exercise the installed launcher -> Mods -> E3 Explore Map with real X11 input.

Run through run_test.py and xvfb-run. All profiles and prepared Workshop data
remain below HALO2_TEST_SCRATCH. No finite-frame/menu-action capture route or
prepared map is supplied: the real asynchronous menu preparation owns launch.
"""
import argparse
import ctypes as C
import json
import os
from pathlib import Path
import re
import select
import shutil
import subprocess
import sys
import time

from native_split_screen_menu_startup import X11Input


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--review-unavailable',type=int,metavar='FEATURES',
                        help='First try Play Campaign and pause on its real unavailable dialog for visual review')
    args=parser.parse_args()
    root=args.root.resolve(strict=True)
    sys.path.insert(0,str(root/'tools'))
    import steam_workshop as workshop
    from gameplay_runtime_selection import selected_gameplay_runtime
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.report.resolve().is_relative_to(scratch)
    selection=selected_gameplay_runtime(root);runtime=root/selection['runtime']
    catalog=workshop.scan();ids=[m['id']for m in catalog['mods']]
    index=ids.index('3360515088');item=Path(catalog['mods'][index]['directory'])
    source=workshop.source_owned(item,'maps/earthcity_e3.map');source_pin=workshop.pin(source)
    runtime_pin=workshop.pin(runtime)
    selectors={name:workshop.pin(root/name)for name in
               ['gameplay-runtime-selection.json','frontend-runtime-selection.json']}
    env={k:v for k,v in os.environ.items()if not k.startswith(('HALO2_','H2_'))}
    for key in ['HALO2_TEST_SCRATCH','TMPDIR','TMP','TEMP','XDG_DATA_HOME','XDG_CONFIG_HOME',
                'XDG_CACHE_HOME','HALO2_PIPELINE_CACHE_DIRECTORY','MESA_SHADER_CACHE_DIR',
                '__GL_SHADER_DISK_CACHE_PATH']:
        env[key]=str(scratch/key);Path(env[key]).mkdir()
    env.update(SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy')
    stdbuf=shutil.which('stdbuf')
    if not stdbuf:raise RuntimeError('Observable native progress requires stdbuf')
    command=[stdbuf,'-oL','-eL',str(root/'Halo2-Launcher'),'--launch-game',
             '--menu-screen','6','--skip-intro','--viewport','960','720','--mute']
    report=dict(passed=False,scope=__doc__,command=command,runtime=dict(path=str(runtime),**runtime_pin),
                cache=selection['cache'],owned_scratch=str(scratch),keys=[],prepared_inputs=[],
                retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True)
    def save():args.report.write_text(json.dumps(report,indent=2)+'\n')
    log=scratch/'native-menu.log';process=None;keyboard=None;deadline=time.monotonic()+210
    def text():return log.read_text(errors='replace')if log.exists()else''
    def wait(predicate,description,seconds=45):
        end=min(deadline,time.monotonic()+seconds)
        while time.monotonic()<end:
            if predicate():return
            if process.poll()is not None:raise RuntimeError('Native launcher exited before '+description)
            time.sleep(.1)
        raise RuntimeError('Timed out awaiting '+description)
    def key(name):
        keyboard.key(name);report['keys'].append(name);save()
    def title():
        keyboard.window=None
        try:keyboard.focus()
        except RuntimeError:return''
        name=C.c_char_p()
        if not keyboard.x.XFetchName(keyboard.display,keyboard.window,C.byref(name))or not name.value:return''
        value=name.value.decode(errors='replace');keyboard.x.XFree(name);return value
    save()
    try:
        keyboard=X11Input()
        with log.open('w')as stream:
            process=subprocess.Popen(command,cwd=root,env=env,stdout=stream,stderr=subprocess.STDOUT)
            wait(lambda:'Menu ready screen=6'in text(),'main menu')
            for _ in range(5):key('Down')
            key('Return');wait(lambda:bool(re.search(r'Menu screen 60\b',text())),'Mods catalog')
            for _ in range(index):key('Down')
            key('Return');time.sleep(.5) # The native same-screen context changes retain screen60.
            key('Return');time.sleep(.5) # First mission -> authored launch choices.
            if args.review_unavailable is not None:
                from PIL import ImageGrab
                start=len(text());key('Return') # Play Campaign -> normal difficulty menu.
                wait(lambda:bool(re.search(r'Menu screen 12\b',text()[start:])),'campaign difficulty menu')
                start=len(text());key('Return') # Accept the actual highlighted difficulty.
                wait(lambda:bool(re.search(r'Menu screen 8\b',text()[start:])),'asynchronous mission preparation')
                wait(lambda:any(Path(env['HALO2_TEST_SCRATCH']).rglob('workshop-inputs.json')),
                     'production Workshop preparation receipt',150)
                time.sleep(2)
                capture=scratch/'unavailable.png'
                ImageGrab.grab(xdisplay=env['DISPLAY']).save(capture)
                report['unavailable_capture']=dict(path=str(capture),**workshop.pin(capture))
                report['expected_unavailable_features']=args.review_unavailable;save()
                print(json.dumps(dict(review_capture=str(capture),expected_features=args.review_unavailable)),flush=True)
                ready,_,_=select.select([sys.stdin],[],[],min(60,max(.1,deadline-time.monotonic())))
                if not ready:raise RuntimeError('Unavailable dialog visual review timed out')
                review=json.loads(sys.stdin.readline())
                if (review.get('title')!='UNAVAILABLE' or review.get('features')!=args.review_unavailable or
                    review.get('first')!='bitmap_predict'):
                    raise RuntimeError('Actual unavailable dialog did not match native admission')
                report['unavailable_visual_review']=review;save()
                start=len(text());key('Return') # Continue -> the original difficulty screen.
                wait(lambda:bool(re.search(r'Menu screen 12\b',text()[start:])),'difficulty return after unavailable dialog')
                start=len(text());key('Escape') # Back -> the same mission launch choices.
                wait(lambda:bool(re.search(r'Menu screen 60\b',text()[start:])),'mission launch choices after unavailable dialog')
            key('Down');key('Return') # Explore Map, through normal asynchronous preparation.
            wait(lambda:'MCC content preview'in title(),'actual E3 preview window',150)
            report['preview_window_observed']=True;report['preview_window_title']=title();save()
            end=min(deadline,time.monotonic()+5)
            while time.monotonic()<end:
                if process.poll()is not None:raise RuntimeError('Preview exited before normal window close')
                time.sleep(.1)
            keyboard.window=None;keyboard.focus();keyboard.close_window()
            report['wm_delete_window_sent']=True
            process.wait(timeout=min(15,max(.1,deadline-time.monotonic())))
        output=text();report['exit_code']=process.returncode
        frames=re.findall(r'^Rendered ([0-9]+) frames$',output,re.MULTILINE)
        report['preview_frames']=int(frames[-1])if frames else 0
        for path in Path(env['HALO2_TEST_SCRATCH']).rglob('workshop-inputs.json'):
            inputs=json.loads(path.read_text())
            provenance=json.loads(path.with_name('provenance.json').read_text())
            report['prepared_inputs'].append(dict(path=str(path),source=inputs.get('source'),
                source_matches_installed=inputs.get('source')==source_pin,
                base_matches_selected=inputs.get('base')==workshop.pin(root/selection['cache']/'provenance.json'),
                prepared_map=provenance.get('workshop',{}).get('prepared_map'),
                source_resources_unchanged=all(workshop.pin(Path(p))==v for p,v in inputs.get('resources',{}).items())))
        report.update(source_unchanged=workshop.pin(source)==source_pin,runtime_unchanged=workshop.pin(runtime)==runtime_pin,
            selections_unchanged=selectors=={name:workshop.pin(root/name)for name in selectors})
        report['passed']=bool(process.returncode==0 and report['preview_frames']>0 and report['prepared_inputs']and
            all(p['source_matches_installed']and p['base_matches_selected']and p['source_resources_unchanged']
                for p in report['prepared_inputs'])and report['source_unchanged']and report['runtime_unchanged']and
            report['selections_unchanged'])
    except Exception as error:report['error']=str(error)
    finally:
        if process is not None and process.poll()is None:
            process.terminate()
            try:process.wait(timeout=10)
            except subprocess.TimeoutExpired:process.kill();process.wait(timeout=10)
        if keyboard is not None:keyboard.close()
        report['log_tail']=text()[-24000:];save()
    print(json.dumps({k:report.get(k)for k in ['passed','error','exit_code','preview_frames']}),flush=True)
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
