#!/usr/bin/env python3
"""Compile and retain the requested Deserted cache from its saved model.

Run through tools/run_test.py. Compiler files and Wine stay in that owner's
scratch. Only the requested native map and bounded build receipts are retained.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

from stage_dust2_h2ek import stage
from configure_dust2_h2ek import configure
from prepare_dust2_h2ek import assemble
from validate_dust2_h2ek_assembly import validate
from prepare_dust2_native import publish
from deserted_collision import validate_classic


def finalize_lightmaps(native,imported,main_hud,evidence,scratch):
    """Keep the actual bake continuous at compiled planar chart boundaries."""
    tools=Path(__file__).resolve().parent
    staging=scratch/'lighting-input';staging.mkdir();(staging/'evidence').mkdir()
    native.rename(staging/'content')
    worker=scratch/'lighting-worker';worker.mkdir()
    env=dict(os.environ,HALO2_TEST_SCRATCH=str(worker),TMPDIR=str(worker),TMP=str(worker),
        TEMP=str(worker),OPENBLAS_NUM_THREADS='1')
    shared=['--imported',str(imported),'--main-hud',str(main_hud)]
    subprocess.run([sys.executable,str(tools/'stitch_deserted_lightmaps.py'),'--root',str(staging),*shared],env=env,check=True)
    candidate=staging/'lighting-candidate'
    shutil.copyfile(staging/'evidence/collision-cache.json',evidence/'collision-cache-final.json')
    subprocess.run([sys.executable,str(tools/'solve_deserted_lightmap_seams.py'),'--root',str(candidate),*shared],env=env,check=True)
    subprocess.run([sys.executable,str(tools/'probe_deserted_atlas.py'),str(candidate),
        '--report',str(evidence/'atlas-seams-final.json')],env=env,check=True)
    for name,target in [('stitch.json','lightmap-stitch-final.json'),('seam-solve.json','lightmap-seam-solve-final.json')]:
        shutil.copyfile(candidate/'evidence'/name,evidence/target)
    shutil.copyfile(worker/'publication.json',evidence/'native-preparation-final.json')
    return candidate/'content'


def build(package,installed,imported,main_hud,output_root,replace_cache_sha256=None,quality='medium'):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert json.loads((package/'architecture-plan.json').read_text())['name']=='Deserted'
    geometry=json.loads((output_root/'evidence/geometry-final.json').read_text())
    routes=json.loads((output_root/'evidence/routes-final.json').read_text())
    assert geometry['passed'] and routes['passed']
    assert hashlib.sha256((package/'dust2_mombasa.glb').read_bytes()).hexdigest()==geometry['model_sha256']
    prior=output_root/'content/halo2/Mods/deserted/deserted.map'
    tag_output=output_root/'h2ek-tags'
    retained_tag_pins={str(p.relative_to(tag_output)):hashlib.sha256(p.read_bytes()).hexdigest()
        for p in tag_output.rglob('*') if p.is_file()} if tag_output.exists() else {}
    if (output_root/'content').exists():
        assert replace_cache_sha256 and hashlib.sha256(prior.read_bytes()).hexdigest()==replace_cache_sha256,'Preserve the existing published map'
    evidence=output_root/'evidence';evidence.mkdir(exist_ok=True)
    kit=scratch/'kit';interface=scratch/'tag-interface'
    start=time.monotonic();state=dict(status='building',quality=quality,scratch=str(scratch),steps=[])

    def progress(name):
        state['steps'].append(dict(name=name,elapsed_seconds=round(time.monotonic()-start,3)))
        (output_root/'build-progress.json').write_text(json.dumps(state,indent=2)+'\n')
        print(name,flush=True)

    def stock_error(line):
        return ('elite\\fp_body\\fp_body' in line and 'inset_lights' in line) or (
            'flak_projectile' in line and 'flak_bolt_luminous' in line)

    environment=dict(os.environ,WINEPREFIX=str(scratch/'wine'),WINEDEBUG='-all',
        WINEDLLOVERRIDES='winemenubuilder.exe=d;mscoree,mshtml=d')
    environment.pop('DISPLAY',None)

    def compiler(name,args):
        with (scratch/(name+'.log')).open('w') as log:
            command=['wine','tool_fast.exe','-batch',*args]
            result=subprocess.run(command,cwd=kit,env=environment,stdout=log,stderr=subprocess.STDOUT,
                timeout=3600 if name=='lightmaps-final' else 600)
        text=(scratch/(name+'.log')).read_text(errors='replace').replace('\r','\n')
        errors=[line for line in text.splitlines() if '### ERROR' in line]
        assert result.returncode==0,(name,result.returncode)
        assert all(stock_error(line) for line in errors),(name,errors)
        state.setdefault('compiler',[]).append(dict(name=name,command=command,stock_diagnostics=errors))
        return text

    try:
        subprocess.run(['git','clone','--quiet','https://github.com/General-101/Halo-Tag-Interface.git',str(interface)],check=True)
        subprocess.run(['git','-C',str(interface),'checkout','--quiet','7b01076b485cb851277c14defb71c8f88e214de4'],check=True)
        stage(installed,package,kit)
        configure(kit,package,interface/'tag_interface',evidence/'configure-final.json',False,False,'district')
        ass=kit/'data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass'
        assemble(package,ass,evidence/'assembly-final.json',6000,True,set(),{},True)
        validate(package,ass,evidence/'assembly-validation-final.json')
        progress('Private authoring inputs validated')
        text=compiler('structure-final',['structure-new-from-ass',r'scenarios\multi\dust2_mombasa\structure\dust2_mombasa.ass','no'])
        assert 'saving imported structure' in text
        validate_classic(kit,interface/'tag_interface',evidence/'collision-final.json')
        progress('BSP compiled; baking '+quality+' lightmaps')
        compiler('lightmaps-final',['lightmaps',r'scenarios\multi\dust2_mombasa\dust2_mombasa','dust2_mombasa',quality])
        ltmp=kit/'tags/scenarios/multi/dust2_mombasa/dust2_mombasa_dust2_mombasa_lightmap.scenario_structure_lightmap'
        assert ltmp.is_file() and ltmp.stat().st_size>1000
        state['lightmap']=dict(bytes=ltmp.stat().st_size,sha256=hashlib.sha256(ltmp.read_bytes()).hexdigest())
        progress(quality.capitalize()+' lightmaps saved; building cache')
        compiler('cache-final',['build-cache-file',r'scenarios\multi\dust2_mombasa\dust2_mombasa','win64'])
        source=kit/'h2_maps_win64_dx11/dust2_mombasa.map'
        assert source.is_file() and source.stat().st_size>1000000
        native=scratch/'native'
        publish(source,None,imported,native,evidence/'native-preparation-final.json',main_hud,
            compiled_lighting=True,output_name='deserted')
        native=finalize_lightmaps(native,imported,main_hud,evidence,scratch)
        progress('Baked chart seams stitched; compiled atlas continuity verified')
        new_tags=scratch/'new-authoring-tags'
        shutil.copytree(kit/'tags/scenarios/multi/dust2_mombasa',new_tags)
        if retained_tag_pins:
            assert retained_tag_pins=={str(p.relative_to(tag_output)):hashlib.sha256(p.read_bytes()).hexdigest()
                for p in tag_output.rglob('*') if p.is_file()},'Retained authoring tags changed during compilation'
        if (output_root/'content').exists():
            assert hashlib.sha256(prior.read_bytes()).hexdigest()==replace_cache_sha256,'Published map changed during compilation'
            (output_root/'content').rename(scratch/'previous-owned-content')
        native.rename(output_root/'content')
        # Requested Halo-compatible authoring deliverables and the required
        # baked inputs for further lighting work. No compiler/runtime copies.
        if tag_output.exists():tag_output.rename(scratch/'previous-owned-authoring-tags')
        new_tags.rename(tag_output)
        state['retained_authoring_tags']=dict(path=str(tag_output),
            bytes=sum(p.stat().st_size for p in tag_output.rglob('*') if p.is_file()),
            reason='Requested Halo-compatible map and required baked authoring inputs')
        cache=output_root/'content/halo2/Mods/deserted/deserted.map'
        state['cache']=dict(path=str(cache),bytes=cache.stat().st_size,sha256=hashlib.sha256(cache.read_bytes()).hexdigest())
        state['status']='compiled; native runtime review pending'
        progress('Deserted native cache retained')
    except BaseException as error:
        state['status']='incomplete';state['error']=str(error)
        raise
    finally:
        for log in scratch.glob('*-final.log'):
            lines=log.read_text(errors='replace').replace('\r','\n').splitlines()
            # Retain real diagnostics and the final progress tail, not megabytes
            # of repeated console progress bars.
            selected=[line for line in lines if '###' in line]+lines[-80:]
            (evidence/log.name).write_text(('\n'.join(selected)+'\n')[-65536:])
        (output_root/'build-progress.json').write_text(json.dumps(state,indent=2)+'\n')
        subprocess.run(['wineserver','-k'],env=environment,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('package','installed','imported','main-hud','output-root'):
        p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--replace-cache-sha256')
    p.add_argument('--quality',choices=('low','medium','high'),default='medium')
    build(**vars(p.parse_args()))
