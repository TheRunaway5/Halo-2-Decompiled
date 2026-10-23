#!/usr/bin/env python3
"""Publish a pinned real Classic bake without repeating its lighting work.

Run through run_test.py. The required baked checkpoint remains untouched until
native acceptance; transient native/cache/atlas copies belong to this owner.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import time
from build_deserted_native import finalize_lightmaps
from prepare_dust2_native import publish


def sha(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def finalize(root, imported, main_hud):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert main_hud.is_file() and json.loads(main_hud.read_text())['schema']=='H2_DUST2_MAIN_GAME_HUD_V1'
    assert not (root/'content').exists() and not (root/'h2ek-tags').exists()
    baked=root/'baked-inputs';pins=json.loads((baked/'retention.json').read_text())
    assert pins['schema']=='H2_DESERTED_REQUIRED_BAKED_INPUTS_V1'
    for relative,row in pins['files'].items():
        path=baked/relative
        assert path.resolve().is_relative_to(baked.resolve())
        assert path.stat().st_size==row['bytes'] and sha(path)==row['sha256'],relative
    state=json.loads((root/'build-progress.json').read_text())
    state['publication_recovery']=dict(previous_error=state.pop('error',None),bake_repeated=False,
        pinned_inputs=str(baked),main_hud=str(main_hud),main_hud_sha256=sha(main_hud))
    state['status']='native publication from pinned real baked inputs'
    state['postprocess_scratch']=str(scratch)
    start=time.monotonic()
    def progress(name):
        state['steps'].append(dict(name=name,postprocess_elapsed_seconds=round(time.monotonic()-start,3)))
        (root/'build-progress.json').write_text(json.dumps(state,indent=2)+'\n');print(name,flush=True)
    evidence=root/'evidence'
    try:
        progress('Pinned baked cache and authoring tags verified; publishing main-game HUD')
        native=scratch/'native'
        publish(baked/'dust2_mombasa.map',None,imported,native,evidence/'native-preparation-final.json',main_hud,
            compiled_lighting=True,output_name='deserted')
        native=finalize_lightmaps(native,imported,main_hud,evidence,scratch)
        progress('Baked atlas continuity verified; native shader bank published')
        shutil.copytree(baked/'tags',root/'h2ek-tags')
        native.rename(root/'content')
        cache=root/'content/halo2/Mods/deserted/deserted.map'
        state['cache']=dict(path=str(cache),bytes=cache.stat().st_size,sha256=sha(cache))
        state['retained_authoring_tags']=dict(path=str(root/'h2ek-tags'),
            bytes=sum(p.stat().st_size for p in (root/'h2ek-tags').rglob('*') if p.is_file()))
        state['status']='compiled; native runtime review pending'
        progress('Native cache retained; real bake reused without recompilation')
    except BaseException as error:
        state['status']='incomplete';state['error']=str(error);raise
    finally:
        (root/'build-progress.json').write_text(json.dumps(state,indent=2)+'\n')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('root','imported','main-hud'):p.add_argument('--'+name,required=True,type=Path)
    finalize(**vars(p.parse_args()))
