#!/usr/bin/env python3
"""Deliver normal controller inputs while the private reference runs continuously."""
import argparse
import json
import os
from pathlib import Path
import shutil
import re
import subprocess
import time
from capture_xbox_tu15_grenade_reference import qmp,x11_window,sha,write_json

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('session','steps','output'):p.add_argument('--'+name,type=Path,required=True)
    args=p.parse_args();steps=json.loads(args.steps.read_text());assert 0<len(steps)<=32
    for row in steps:
        assert row['key']in ('a','b','x','y','Return','Up','Down','Left','Right','e','q','w','s','d','f','i','j','k','l')
        assert 0<row.get('hold',.12)<=1 and 0<=row.get('settle',1)<=45
    out=args.output.absolute();out.mkdir(exist_ok=False);launch=json.loads((args.session/'launch.json').read_text())
    assert launch['display']!=':0'and Path('/proc',str(launch['pid'])).exists()
    rows=[];qmp(Path(launch['qmp']),[dict(execute='cont')])
    try:
        for index,row in enumerate(steps):
            x11_window(launch['display'],press=True,key=row['key'],duration=row.get('hold',.12));time.sleep(row.get('settle',1))
            record=dict(row,guest_execution_continuous=True)
            rows.append(record)
            if row.get('expect_text'):
                deadline=time.monotonic()+30;probe=Path(os.environ['HALO2_TEST_SCRATCH'])/'visible-menu.png'
                while True:
                    subprocess.run(['import','-display',launch['display'],'-window',str(x11_window(launch['display'])),str(probe)],check=True,timeout=10)
                    visible=subprocess.check_output(['tesseract',str(probe),'stdout','--psm','11'],stderr=subprocess.DEVNULL,text=True,timeout=10)
                    if re.search(row['expect_text'],visible,re.I):record['observed_menu_text']=visible;break
                    if time.monotonic()>=deadline:
                        shutil.copyfile(probe,out/f'{index:02}-unexpected-menu.png')
                        (out/f'{index:02}-unexpected-menu.txt').write_text(visible)
                        raise RuntimeError('Original visible menu did not reach '+row['expect_text'])
                    time.sleep(.2)
            if row.get('capture',False):
                path=out/f'{index:02}.png';subprocess.run(['import','-display',launch['display'],'-window',str(x11_window(launch['display'])),str(path)],check=True,timeout=10)
                record.update(image=path.name,image_sha256=sha(path))
    finally:
        qmp(Path(launch['qmp']),[dict(execute='stop')])
        write_json(out/'inputs.json',dict(steps=rows,source_launch_sha256=sha(args.session/'launch.json'),
            declared_steps_sha256=sha(args.steps),tool_sha256=sha(Path(__file__)),guest_code_patches=[],guest_memory_writes=[],guest_register_writes=[],full_weapon_parity=False))
        shutil.copyfile(Path(__file__),out/'control-source.py');shutil.copyfile(args.steps,out/'declared-steps.json')
    print(json.dumps(dict(completed_inputs=len(rows),output=str(out))),flush=True)

if __name__=='__main__':main()
