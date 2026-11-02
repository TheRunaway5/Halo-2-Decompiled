#!/usr/bin/env python3
"""Check observer ownership and the reported Earth gap in a bounded PC capture.

This is not original-Xbox framebuffer or full campaign qualification.
"""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import subprocess


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--capture',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--only-camera',action='store_true')
    args=ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Use tools/run_test.py')
    capture=args.capture.resolve(strict=True)
    receipt_path=capture/'capture.json'
    receipt=json.loads(receipt_path.read_text())
    retained_valid=True
    for item in receipt['retained']:
        path=Path(item['path']).resolve(strict=True)
        retained_valid &= path.is_relative_to(capture) and path.stat().st_size==item['bytes']
        retained_valid &= hashlib.sha256(path.read_bytes()).hexdigest()==item['sha256']
    trace=capture/'poses.jsonl.xz'
    with lzma.open(trace,'rt')as stream:
        rows=[json.loads(line)for line in stream]
    rows=[r for r in rows if 'source_camera_bits'in r]
    changed=[r for r in rows if r['source_camera_bits']!=r['render_camera_bits']]
    cut_window=[r for r in rows if 1400<=r['tick']<=1403]
    report=dict(passed=False,scope=__doc__,capture=str(capture),
        capture_receipt_sha256=hashlib.sha256(receipt_path.read_bytes()).hexdigest(),
        runtime_sha256=receipt['runtime_sha256'],
        trace_sha256=hashlib.sha256(trace.read_bytes()).hexdigest(),
        camera_frames=len(rows),changed_source_camera_frames=len(changed),
        mismatch_examples=changed[:4],cut_window=cut_window,
        checks=dict(completed_source_camera_preserved=bool(rows)and not changed,
                    reported_cut_window_observed=bool(cut_window),
                    retained_capture_hashes_match=bool(receipt['retained'])and retained_valid),
        original_xbox_framebuffer_qualified=False,full_campaign_qualified=False)
    if not args.only_camera:
        report['checks']['opening_target_reached']=receipt.get('declared_tick_reached',0)>=5300
        frames=list(capture.glob('*-target-5000.png'))
        report['checks']['earth_reference_present']=len(frames)==1
        if len(frames)==1:
            frame=frames[0]
            width,height=map(int,subprocess.check_output(
                ['magick','identify','-format','%w %h',str(frame)],text=True).split())
            region=f'{width//24}x{height*25//54}+{width*23//24}+{height*5//18}'
            mean=float(subprocess.check_output(['magick',str(frame),'-crop',region,
                '-colorspace','Gray','-format','%[fx:mean]','info:'],text=True))
            report['earth_right_region']=dict(path=str(frame),region=region,mean=mean,minimum=.035)
            report['checks']['earth_right_gap_absent']=mean>.035
    report['passed']=all(report['checks'].values())
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in('passed','camera_frames','changed_source_camera_frames','checks')}))
    return 0 if report['passed']else 1


if __name__=='__main__':
    raise SystemExit(main())
