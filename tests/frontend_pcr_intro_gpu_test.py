#!/usr/bin/env python3
"""Actual native PCR intro rendering with admitted MCC assets and owned frames."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--mcc-root',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    args=parser.parse_args()
    if not os.environ.get('DISPLAY'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    maps=args.mcc_root/'halo2/h2_maps_win64_dx11'
    if not (maps/'mainmenu.map').is_file():return 77
    with tempfile.TemporaryDirectory(prefix='pcr-intro-',dir=scratch) as temporary:
        output=Path(temporary)
        env=os.environ|dict(H2_TEST_ONLINE_UI='1',H2_TEST_PCR_INTRO_MCC='1')
        env.pop('H2_TEST_MENU_ASSETS',None)
        result=subprocess.run([str(args.binary),str(maps),str(maps/'mainmenu.map'),
            str(args.fixture),str(output)],env=env,timeout=75)
        if result.returncode:return result.returncode
        report=json.loads((output/'report.json').read_text())
        intro=report['original_intro']
        assert intro['asset_source']=='admitted MCC' and intro['gpu']['passed']
        assert intro['prior_bypass'] and intro['pane_does_not_restart_clock']
        map_intro=report['original_map_intro']
        assert map_intro['gpu']['passed'] and map_intro['gpu']['full_surface_opaque']
        assert map_intro['independent_clocks'] and map_intro['tab_and_prior_do_not_reset_map']
        receipt=dict(schema='H2_MCC_ORIGINAL_PCR_INTRO_GPU_1',passed=True,
            scope='Actual native Vulkan output; unchanged original timing kernel controls are separate',
            original_intro=intro,original_map_intro=map_intro,raw_frames_retained=False,retained_images=[])
        if destination:=os.environ.get('HALO2_PCR_INTRO_EVIDENCE'):
            from PIL import Image
            destination=Path(destination)
            assert destination.is_absolute()
            destination.mkdir(parents=True,exist_ok=True)
            assert not any(destination.iterdir()),'PCR intro evidence requires a fresh owned destination'
            for name in ('pcr-original-intro-0','pcr-original-intro-3000',
                         'pcr-original-intro-4000','pcr-original-prior-bypass',
                         'map-original-intro-black','map-original-intro-overlap',
                         'map-original-intro-clear','map-original-prior-clear'):
                target=destination/(name+'.png')
                with Image.open(output/(name+'.ppm')) as image:
                    image.save(target,optimize=True,compress_level=9)
                    width,height=image.size
                size=target.stat().st_size
                assert size<=1024*1024,'PCR intro PNG exceeds its explicit evidence bound'
                receipt['retained_images'].append(dict(path=str(target),bytes=size,
                    width=width,height=height,sha256=hashlib.sha256(target.read_bytes()).hexdigest()))
            assert sum(item['bytes']for item in receipt['retained_images'])<=8*1024*1024
            (destination/'evidence.json').write_text(json.dumps(receipt,indent=2)+'\n')
        print('PASS actual MCC PCR intro GPU:',json.dumps(receipt),flush=True)
    return 0


if __name__=='__main__':raise SystemExit(main())
