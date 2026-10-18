#!/usr/bin/env python3
"""Check isolated native material/shadow controls and preserve pixels as PNG."""
import argparse,json
from pathlib import Path
import numpy as np
from PIL import Image


def compare(a,b):
    a,b=(np.asarray(Image.open(p),dtype=np.int16) for p in (a,b))
    return dict(brightened_channels=int((a>b).sum()),darkened_channels=int((a<b).sum()),
                changed_pixels=int(np.any(a!=b,axis=2).sum()))


def verify(folder,baseline=None):
    original=folder/'illumination.ppm'
    assert original.read_bytes()==(folder/'Original-restored.ppm').read_bytes(),folder
    spec=compare(folder/'pStencil-specular.ppm',folder/'pStencil-diffuse.ppm')
    shadow=compare(folder/'pStencil-specular.ppm',folder/'pStencil-no-shadows-specular.ppm')
    independent=compare(folder/'pStencil-independent-dynamic-casters.ppm',folder/'pStencil-no-dynamic-casters.ppm')
    assert spec['brightened_channels']>0 and spec['darkened_channels']==0,(folder,spec)
    assert shadow['darkened_channels']>0 and shadow['brightened_channels']==0,(folder,shadow)
    assert independent['darkened_channels']>0 and independent['brightened_channels']==0,(folder,independent)
    assert (folder/'pStencil-specular.ppm').read_bytes()==(folder/'pStencil-independent-dynamic-casters.ppm').read_bytes(),folder
    report=dict(Original_roundtrip_exact=True,specular=spec,point_shadow=shadow,independent_caster=independent)
    if baseline:
        report['Original_matches_baseline']=original.read_bytes()==(baseline/'illumination.ppm').read_bytes()
        assert report['Original_matches_baseline'],folder
    for name in ['pStencil-specular','pStencil-diffuse','pStencil-no-shadows-specular','illumination']:
        Image.open(folder/(name+'.ppm')).save(folder/(name+'.png'))
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--bundle',type=Path,required=True)
    p.add_argument('--baseline',type=Path,required=True);a=p.parse_args();result={}
    status=json.loads((a.bundle/'vehicle-final/status.json').read_text())
    assert len(status)==6 and all(r['exit_code']==0 for r in status),status
    result['vehicles']={r['name']:verify(a.bundle/'vehicle-final'/r['name'],a.baseline/'vehicle-final'/r['name']) for r in status}
    result['weapons']={};result['weapon_fixture_limits']=[]
    for filename in ['status.json','angle-status.json','highres-status.json']:
        path=a.bundle/'weapon-final'/filename
        if not path.exists():continue
        for r in json.loads(path.read_text()):
            folder=Path(r['command'][3])
            if r['exit_code']==0:result['weapons'][folder.name]=verify(folder)
            else:result['weapon_fixture_limits'].append(dict(name=folder.name,exit_code=r['exit_code']))
    highres=json.loads((a.bundle/'weapon-final/highres-status.json').read_text())
    assert len(highres)==4 and all(r['exit_code']==0 for r in highres),highres
    (a.bundle/'dynamic-verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(vehicles=list(result['vehicles']),weapons=list(result['weapons']),
        weapon_fixture_limits=result['weapon_fixture_limits']),indent=2))


if __name__=='__main__':main()
