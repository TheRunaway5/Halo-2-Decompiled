#!/usr/bin/env python3
"""Actual finite SDL/Vulkan SMG pickup/switch, dual fire/reload and grenade/drop.

Private X11, one tick/frame, mid-tick pose sample, dummy audio and declared
placement. Checks poses, particle/PCM submission and carried identity; no
Xbox framebuffer, audible hardware or complete original world claim.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import struct
import tempfile
import zlib
from validate_weapon_runtime_tu15 import ROOT,private_display


def capture_preview(output,raw):
    preview=output/'dual-smg-preview.png'
    data=raw.read_bytes();header=re.match(rb'P6\n(\d+) (\d+)\n255\n',data)
    assert header,'Native capture is not its expected RGB8 P6 format'
    width,height=map(int,header.groups());pixels=data[header.end():]
    assert len(pixels)==width*height*3
    rows=b''.join(b'\0'+pixels[y*width*3:(y+1)*width*3] for y in range(height))
    compressed=zlib.compress(rows);assert zlib.decompress(compressed)==rows
    def chunk(kind,payload):
        return struct.pack('>I',len(payload))+kind+payload+struct.pack('>I',zlib.crc32(kind+payload)&0xffffffff)
    preview.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',width,height,8,2,0,0,0))+
        chunk(b'IDAT',compressed)+chunk(b'IEND',b''))
    return dict(raw_capture=str(raw),preview=str(preview),
        preview_sha256=hashlib.sha256(preview.read_bytes()).hexdigest(),
        preview_scope='Lossless format conversion of the native P6 framebuffer; RGB pixels verified unchanged.')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--capture',action='store_true',help='Capture the retained dual state after reload, without grenade/drop')
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    binary=args.build.resolve()/'native_weapon_runtime_tests'
    xbe=Path.home()/'.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'
    assert hashlib.sha256(xbe.read_bytes()).hexdigest()=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    command=[str(binary),str(ROOT/'assets/disc/maps/lockout.map'),str(ROOT/'assets/disc/maps'),str(xbe)]
    if args.capture:command.append(str(args.output.resolve()/'dual-smg.ppm'))
    with private_display(args.output) as display,tempfile.TemporaryDirectory(prefix='halo2-smg-dual-') as temporary:
        env=dict(os.environ,DISPLAY=display,SDL_AUDIODRIVER='dummy',SDL_VIDEODRIVER='x11',SDL_VIDEO_DRIVER='x11',
            HALO2_ANIMATION_VALIDATION='1',XDG_CONFIG_HOME=temporary+'/config',XDG_DATA_HOME=temporary+'/data')
        env['VK_LAYER_PATH']=str(ROOT/'.tools/validation/usr/share/vulkan/explicit_layer.d')
        env['LD_LIBRARY_PATH']=':'.join((str(ROOT/'.tools/validation/usr/lib'),str(ROOT/'.tools/validation/usr/lib/x86_64-linux-gnu'),env.get('LD_LIBRARY_PATH','')))
        with (args.output/'native.log').open('w') as log:
            try:status=subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=90).returncode
            except subprocess.TimeoutExpired:status='timeout'
    log=args.output/'native.log';text=log.read_text();errors=len(re.findall(r'Validation Error|VUID-',text))
    report=dict(scope=__doc__,command=command,exit=status,
        passed=status==0 and errors==0 and 'PASS actual SDL' in text and 'Native SDL video driver: x11' in text,
        validation_errors=errors,executable_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
        xbe_sha256=hashlib.sha256(xbe.read_bytes()).hexdigest(),log_sha256=hashlib.sha256(log.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        capture=args.capture,
        observations=[line for line in text.splitlines() if line.startswith(('PASS ','SMG firing observations:',
            'SMG grenade observations:','Multiplayer frames=','Native SDL video driver:','Weapon runtime state:'))])
    if args.capture:
        capture=args.output/'dual-smg.ppm';report['passed']=report['passed'] and capture.exists()
        if capture.exists():
            report['capture_sha256']=hashlib.sha256(capture.read_bytes()).hexdigest()
            report.update(capture_preview(args.output,capture))
    (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report),flush=True)
    if not report['passed']:raise SystemExit('Actual SMG runtime probe failed; see retained log.')


if __name__=='__main__':main()
