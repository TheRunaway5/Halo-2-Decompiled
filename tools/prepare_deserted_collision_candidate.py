#!/usr/bin/env python3
"""Compile and reject defective custom collision before an expensive bake.

Run through run_test.py. A supplied Wine prefix is an explicitly borrowed,
already running compiler owner; this helper never shuts that prefix down.
"""
import argparse
from collections import Counter
import json
import os
from pathlib import Path
import subprocess
import sys
from build_dust2_district import build
from author_deserted_collision import author
from stage_dust2_h2ek import stage
from configure_dust2_h2ek import configure
from prepare_dust2_h2ek import assemble
from audit_dust2_architecture import audit
from audit_deserted_routes import audit as routes


def check_collision(data):
    bad=[]
    for di,d in enumerate(data['instanced geometries definitions']):
        for si,surf in enumerate(d['surfaces']):
            if surf['flags']&16:continue
            first=surf['first edge'];edge=first;seen=set()
            while True:
                assert edge not in seen;seen.add(edge)
                row=d['edges'][edge];left=row['left surface']==si
                assert left or row['right surface']==si
                v=row['start vertex'] if left else row['end vertex']
                if d['vertices'][v]['first edge']==-1:bad.append([di,si,v])
                edge=row['forward edge'] if left else row['reverse edge']
                if edge==first:break
    return bad


def prepare(root, interface, wine_prefix):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    model=scratch/'model';kit=scratch/'kit';evidence=root/'deserted-streets/evidence'
    build(root/'district-rebuild/architecture-plan.json',root/'district-rebuild/reference/measured-layout.json',
        root/'redesign-candidate/model',root/'model',model)
    author(model,root/'redesign-candidate/reference-props')
    assert audit(model/'dust2_mombasa.glb',evidence/'coarse-geometry.json')==0
    assert routes(model,evidence/'coarse-routes.json')==0
    stage(Path('/home/eric/.local/share/Steam/steamapps/common/H2EK'),model,kit)
    configure(kit,model,interface,scratch/'configure.json',False,False,'district')
    ass=kit/'data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass'
    assemble(model,ass,scratch/'assembly.json',6000,True,set(),{},True)
    env=dict(os.environ,WINEPREFIX=str(wine_prefix),WINEDEBUG='-all',
        WINEDLLOVERRIDES='winemenubuilder.exe=d;mscoree,mshtml=d')
    env.pop('DISPLAY',None)
    with (scratch/'structure.log').open('w') as log:
        subprocess.run(['wine','tool_fast.exe','-batch','structure-new-from-ass',
            r'scenarios\multi\dust2_mombasa\structure\dust2_mombasa.ass','no'],
            cwd=kit,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=180,check=True)
    sys.path.insert(0,str(interface));import tag_interface as ti
    g,e,d,groups,ext,post,_=ti.get_version_args('halo2',0)
    defs=g.generate_defs(d,str(scratch/'defs'),groups,ext);tags=kit/'tags'
    data=ti.read_file(defs,str(tags),str(tags/'scenarios/multi/dust2_mombasa/dust2_mombasa.scenario_structure_bsp'),0,engine_tag=e)['Data']
    bad=check_collision(data)
    lines=(scratch/'structure.log').read_text(errors='replace').replace('\r','\n').splitlines()
    report=dict(passed=not bad,count=len(bad),bad_vertex_references=bad[:50],
        warnings=dict(Counter(s.strip() for s in lines if '### WARNING' in s)),
        plan_sha256=__import__('hashlib').sha256((model/'architecture-plan.json').read_bytes()).hexdigest())
    (evidence/'coarse-collision-probe.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report),flush=True)
    if bad:raise RuntimeError('Classic collision contains references to deleted vertices')
    destination=root/'deserted-streets/clean-model';assert not destination.exists()
    model.rename(destination)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('root','interface','wine-prefix'):p.add_argument('--'+name,required=True,type=Path)
    prepare(**vars(p.parse_args()))
