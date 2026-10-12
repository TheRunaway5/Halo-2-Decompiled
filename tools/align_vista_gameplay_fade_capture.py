#!/usr/bin/env python3
"""Align the retained decoded exit excerpt to its original recording pixels.

The excerpt was copied from an earlier keyframe, so the requested803s seek is
not its actual first decoded frame. This uses exact low-resolution decoded
luma equality. UTC is approximate x11grab capture time, not game/QPC time.
"""
import datetime,hashlib,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CAPTURE=ROOT/'analysis/frontend/vista-session-flow-audit/fade-runtime-20261010'
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/gameplay-fade-original/capture-alignment.json'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def luma(name,seek=0):
 cmd=['ffmpeg','-v','error','-ss',str(seek),'-i',str(CAPTURE/name),'-t','20',
  '-vf','crop=180:160:1120:150,scale=1:1,format=gray','-f','rawvideo','-']
 return subprocess.check_output(cmd)
def main():
 original=luma('original-round.mkv',801);excerpt=luma('natural-exit.mkv')
 matches=[d for d in range(180)if original[:600]==excerpt[d:d+600]]
 assert len(matches)==1,matches
 lag=matches[0];n=min(len(original),len(excerpt)-lag)
 assert original[:n]==excerpt[lag:lag+n]
 start=801-lag/60
 raw_log=(CAPTURE/'recording.log').read_text();anchor=float(re.search(r'start: (\d+\.\d+)',raw_log).group(1))
 utc=lambda media:datetime.datetime.fromtimestamp(anchor+media,datetime.timezone.utc).isoformat()
 # Sampled crop excludes the central scoreboard, approximating world luma.
 samples=[dict(excerpt_seconds=t,original_seconds=start+t,approximate_utc=utc(start+t),luma=excerpt[round(t*60)])for t in [8,8.5,8.75,8.8,8.9,9,9.25,9.5,9.75,10]]
 data=dict(scope=__doc__,original_sha256=sha(CAPTURE/'original-round.mkv'),excerpt_sha256=sha(CAPTURE/'natural-exit.mkv'),fps=60,anchor_original_seconds=801,matching_lag_frames=lag,matching_frames=n,excerpt_first_original_seconds=start,x11grab_first_unix=anchor,world_crop=[1120,150,180,160],samples=samples,
  limits='Lossy sampled60Hz Wine/Cartographer video and10ms read-only process polling; no sampled engine+E4 opacity bank, original GPU trace, physical platform timing or native runtime parity.')
 OUT.write_text(json.dumps(data,indent=2)+'\n');print(json.dumps(dict(excerpt_first_original_seconds=start,matching_frames=n,samples=samples)))
if __name__=='__main__':main()
