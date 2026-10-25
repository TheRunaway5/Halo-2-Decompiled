#!/usr/bin/env python3
"""Catch the reported shield fade dots in Dire Valley's approved native photo.

Run through run_test.py, after a real 660x621 bridge-camera capture. The fixed
region is tied to the documented rear-ledge camera, not arbitrary screenshots.
Asset/geometry/lighting checks are separate; this observes the actual pixels.
"""
import argparse
import json
import os
from pathlib import Path
import numpy as np
from PIL import Image
from port_dire_valley import save,sha


def phase(path):
    im=Image.open(path).convert('RGB')
    if im.size!=(660,621):raise ValueError('Use the approved uncropped bridge camera at 660x621')
    a=np.asarray(im)[304:324,412:486].astype(float)
    mask=(a[:,:,0]>90)&(a[:,:,2]>60)&(a[:,:,0]>1.3*a[:,:,1])&(a[:,:,2]>1.3*a[:,:,1])
    y,x=np.where(mask)
    counts=np.bincount(((y+304)%4)*4+(x+412)%4,minlength=16)
    return dict(visible_energy_pixels=len(x),occupied_phases=int((counts>0).sum()),phase_counts=counts.tolist())


def validate(before,after,report):
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    a,b=phase(before),phase(after)
    red=a['visible_energy_pixels']>=30 and a['occupied_phases']<=4
    green=b['visible_energy_pixels']>=150 and b['occupied_phases']>=14
    r=dict(schema='H2_DIRE_VALLEY_BRIDGE_SHIELD_PIXEL_REGRESSION_V1',passed=red and green,
        known_bad_reproduced=red,corrected_native_fields=green,region=[412,304,486,324],
        before=dict(path=str(before),sha256=sha(before),**a),after=dict(path=str(after),sha256=sha(after),**b),
        symptom='Small BSP-instance fade rendered shield fields as sparse Bayer dots. Native scenery retains continuous authored energy fields.',
        scope='Exact reported bridge-camera pixel regression; full material/resource and model-placement audits remain separate.')
    save(report,r);return r


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for flag in ['before','after','report']:p.add_argument('--'+flag,type=Path,required=True)
    r=validate(**vars(p.parse_args()));print(json.dumps(r));raise SystemExit(0 if r['passed'] else 1)
