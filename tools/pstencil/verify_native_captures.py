#!/usr/bin/env python3
"""Verify native capture controls and convert their unchanged pixels to PNG."""
import argparse
import json
from pathlib import Path
import re
import numpy as np
from PIL import Image


def compare(a,b):
    a,b=(np.asarray(Image.open(p)).astype(np.int16) for p in (a,b))
    return dict(darkened_channels=int((a<b).sum()),brightened_channels=int((a>b).sum()),
                changed_pixels=int(np.any(a!=b,axis=2).sum()))


def equal(a,b):return a.read_bytes()==b.read_bytes()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle',type=Path,required=True)
    parser.add_argument('--baseline',type=Path,required=True)
    parser.add_argument('--photo-baseline',type=Path,
        help='Same corrected photo tool with the new response bypassed; game baseline stays historical')
    args=parser.parse_args();base,old=args.bundle,args.baseline
    photo_old=args.photo_baseline or old
    status=json.loads((base/'capture-status.json').read_text())
    assert len(status)==9 and all(r['exit_code']==0 for r in status),status
    result=dict(baseline=str(old),photo_baseline=str(photo_old),photos={})
    for kind in ('group','visor','chief','arbiter','elite'):
        p,o=base/('armor-'+kind),photo_old/('armor-'+kind)
        point=compare(p/'pStencil.ppm',p/'pStencil-no-point-shadows.ppm')
        baked=compare(p/'pStencil.ppm',p/'pStencil-no-baked-shadow.ppm')
        spec=compare(p/'pStencil.ppm',p/'pStencil-diffuse.ppm')
        environment=compare(p/'pStencil.ppm',o/'pStencil.ppm')
        polish=compare(p/'pStencil.ppm',p/'pStencil-armor-original-specular.ppm')
        assert polish['darkened_channels']==0,(kind,polish)
        if kind in ('group','arbiter'):assert polish['brightened_channels']>0,(kind,polish)
        assert point['darkened_channels']>0 and point['brightened_channels']==0,(kind,point)
        assert baked['brightened_channels']==0,(kind,baked)
        assert spec['brightened_channels']>0 and spec['darkened_channels']==0,(kind,spec)
        assert equal(p/'Original.ppm',p/'Original-restored.ppm'),kind
        assert equal(p/'Original.ppm',o/'Original.ppm'),kind
        result['photos'][kind]=dict(point_shadow=point,baked_shadow=baked,direct_specular=spec,
            presentation_vs_baseline=environment,arbiter_polish=polish,Original_restored=True,Original_matches_baseline=True)
        for stem in ('Original','pStencil','pStencil-no-point-shadows','pStencil-no-baked-shadow'):
            Image.open(p/(stem+'.ppm')).save(p/(stem+'.png'))
    p=base/'armor-group';Image.open(p/'pStencil.png').crop((500,750,2300,1900)).save(p/'closeup.png')
    p=base/'armor-visor';Image.open(p/'pStencil.png').crop((800,600,1750,1450)).save(p/'closeup.png')
    result['visor_environment_map']=compare(p/'pStencil.ppm',p/'pStencil-visor-no-reflection.ppm')
    assert result['visor_environment_map']['changed_pixels']>0
    result['point_control']=compare(base/'game-pstencil/game.ppm',base/'game-control/game.ppm')
    assert result['point_control']['darkened_channels']>0 and result['point_control']['brightened_channels']==0
    result['Original_matches_baseline']=equal(base/'game-original/game.ppm',old/'game-original/game.ppm')
    assert result['Original_matches_baseline']
    result['simulation']={}
    for kind in ('game-original','game-pstencil','game-control'):
        text=(base/(kind+'.log')).read_text();last=next(l for l in text.splitlines() if 'Input capture tick=45' in l)
        result['simulation'][kind]={k:re.search(r'(?:^| )'+k+r'=([^ ]+)',last)[1] for k in
            ('capture_primary_final','capture_auxiliary_final','detonation_tick','detonation_position','player_position')}
    assert len({json.dumps(x,sort_keys=True) for x in result['simulation'].values()})==1
    for kind in ('game-original','game-pstencil','game-third','game-control'):
        Image.open(base/kind/'game.ppm').save(base/kind/'game.png')
    (base/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(point_control=result['point_control'],Original_matches_baseline=True,
        presentation_changed_pixels={k:v['presentation_vs_baseline']['changed_pixels'] for k,v in result['photos'].items()}),indent=2))


if __name__=='__main__':main()
