#!/usr/bin/env python3
"""Bounded actual native decoding of original and imported countdown assets."""
import argparse
import array
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import wave


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--native',type=Path,required=True)
    p.add_argument('--mcc-map',type=Path,required=True)
    p.add_argument('--xbox-map',type=Path,default=Path('assets/disc/maps/mainmenu.map'))
    p.add_argument('--report',type=Path,required=True)
    a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    native=a.native.resolve(strict=True);env=dict(os.environ)
    env['LD_LIBRARY_PATH']=str(native.parent)+os.pathsep+env.get('LD_LIBRARY_PATH','')
    rows=[]
    for owner,path in [('original',a.xbox_map),('mcc',a.mcc_map)]:
        for name in ('countdown_for_respawn','pickup_health'):
            output=scratch/(owner+'-'+name+'.wav')
            command=[str(native),'--map',str(path.resolve(strict=True)),'--decode-sound',
                     'sound\\game_sfx\\multiplayer\\'+name,'--audio-output',str(output)]
            done=subprocess.run(command,env=env,capture_output=True,text=True,timeout=45)
            row=dict(owner=owner,asset=name,returncode=done.returncode,log=(done.stdout+done.stderr)[-2500:])
            if done.returncode==0:
                with wave.open(str(output),'rb')as stream:
                    assert stream.getsampwidth()==2
                    samples=array.array('h',stream.readframes(stream.getnframes()))
                    row.update(channels=stream.getnchannels(),rate=stream.getframerate(),frames=stream.getnframes())
                row.update(peak=max(abs(v)for v in samples)/32768,
                    rms=math.sqrt(sum((v/32768)**2 for v in samples)/len(samples)),
                    wav_sha256=hashlib.sha256(output.read_bytes()).hexdigest(),bytes=output.stat().st_size)
            rows.append(row)
    report=dict(native_sha256=hashlib.sha256(native.read_bytes()).hexdigest(),assets=rows,
                actual_native_decode=True,actual_ui_trigger=False,actual_device_output=False,raw_audio_retained=False)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
