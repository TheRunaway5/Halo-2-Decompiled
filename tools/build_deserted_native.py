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
import threading
import time

from stage_dust2_h2ek import stage
from configure_dust2_h2ek import configure
from prepare_dust2_h2ek import assemble, static_scenery
from validate_dust2_h2ek_assembly import validate
from prepare_dust2_native import publish
from deserted_collision import validate_classic


def authoring_identity(package,main_hud):
    """Bind a saved bake to the exact authored package and exported HUD."""
    package=package.resolve(strict=True)
    return dict(schema='DESERTED_BAKED_AUTHORING_INPUTS_V1',
        files={str(p.relative_to(package)):dict(bytes=p.stat().st_size,
            sha256=hashlib.sha256(p.read_bytes()).hexdigest())
            for p in sorted(package.rglob('*')) if p.is_file()},
        main_hud_sha256=hashlib.sha256(main_hud.read_bytes()).hexdigest())


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


def build(package,installed,imported,main_hud,output_root,replace_cache_sha256=None,quality='medium',prepared_wine_prefix=None,bake_timeout_seconds=7200,lightmap_workers=1,lightmap_postprocess='stitch',resume_baked_inputs=None):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert 60<=bake_timeout_seconds<=43200,'Lightmap timeout must remain finite (60 seconds to12 hours)'
    assert lightmap_workers in (1,2),'Use one ordinary bake or two isolated workers'
    assert lightmap_postprocess in ('stitch','raw')
    assert main_hud.is_file(), 'main_hud must be the exported main-hud.json file'
    assert json.loads(main_hud.read_text())['schema']=='H2_DUST2_MAIN_GAME_HUD_V1'
    assert json.loads((package/'architecture-plan.json').read_text())['name']=='Deserted'
    geometry=json.loads((output_root/'evidence/geometry-final.json').read_text())
    routes=json.loads((output_root/'evidence/routes-final.json').read_text())
    assert geometry['passed'] and routes['passed']
    assert hashlib.sha256((package/'dust2_mombasa.glb').read_bytes()).hexdigest()==geometry['model_sha256']
    source_identity=authoring_identity(package,main_hud)
    prior=output_root/'content/halo2/Mods/deserted/deserted.map'
    tag_output=output_root/'h2ek-tags'
    retained_tag_pins={str(p.relative_to(tag_output)):hashlib.sha256(p.read_bytes()).hexdigest()
        for p in tag_output.rglob('*') if p.is_file()} if tag_output.exists() else {}
    if (output_root/'content').exists():
        assert replace_cache_sha256 and hashlib.sha256(prior.read_bytes()).hexdigest()==replace_cache_sha256,'Preserve the existing published map'
    evidence=output_root/'evidence';evidence.mkdir(exist_ok=True)
    resumed_state=None
    if resume_baked_inputs is not None:
        resume_baked_inputs=resume_baked_inputs.resolve(strict=True)
        assert resume_baked_inputs==(output_root/'baked-inputs').resolve(strict=True)
        receipt=json.loads((resume_baked_inputs/'retention.json').read_text())
        assert receipt['schema']=='H2_DESERTED_REQUIRED_BAKED_INPUTS_V1'
        assert receipt.get('authoring_inputs')==source_identity, 'Saved bake does not bind the current source package and rig; preserve and explicitly qualify older checkpoints'
        assert receipt['quality']==quality
        for name,row in receipt['files'].items():
            path=(resume_baked_inputs/name).resolve(strict=True)
            assert path.is_relative_to(resume_baked_inputs) and path.stat().st_size==row['bytes']
            assert hashlib.sha256(path.read_bytes()).hexdigest()==row['sha256']
        prior_progress=output_root/'build-progress.json'
        resumed_state=json.loads(prior_progress.read_text()) if prior_progress.exists() else {}
        if prior_progress.exists():shutil.copyfile(prior_progress,evidence/('build-before-resume-'+str(time.time_ns())+'.json'))
        # The checkpoint anchor survives a failed attempt before LTMP loading.
        resumed_state['lightmap']=receipt['lightmap']
        assert resumed_state['lightmap']['sha256']
    kit=scratch/'kit';interface=scratch/'tag-interface'
    start=time.monotonic();state=dict(status='building',quality=quality,scratch=str(scratch),
        bake_timeout_seconds=bake_timeout_seconds,lightmap_workers=lightmap_workers,
        lightmap_postprocess=lightmap_postprocess,steps=[])
    state['authoring_inputs']=source_identity
    if resumed_state:
        state['resume']=dict(checkpoint=str(resume_baked_inputs),photon_rebake=False,
            original_lightmap=resumed_state['lightmap'],all_checkpoint_file_hashes_verified=True)
    (output_root/'build-progress.json').write_text(json.dumps(state,indent=2)+'\n')
    progress_lock=threading.RLock()

    def progress(name):
        with progress_lock:
            state['steps'].append(dict(name=name,elapsed_seconds=round(time.monotonic()-start,3)))
            temporary=output_root/'build-progress.json.tmp'
            temporary.write_text(json.dumps(state,indent=2)+'\n')
            temporary.replace(output_root/'build-progress.json')
        print(name,flush=True)

    def stock_error(line):
        return ('elite\\fp_body\\fp_body' in line and 'inset_lights' in line) or (
            'flak_projectile' in line and 'flak_bolt_luminous' in line)

    environment=dict(os.environ,WINEPREFIX=str(scratch/'wine'),WINEDEBUG='-all',
        WINEDLLOVERRIDES='winemenubuilder.exe=d;mscoree,mshtml=d')
    environment.pop('DISPLAY',None)
    if prepared_wine_prefix is not None:
        # A read-only snapshot of our already initialized compiler prefix avoids
        # reinstalling Wine while a separate map bake is active. The new prefix
        # has its own inode/server identity and is stopped only by this owner.
        pins={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in prepared_wine_prefix.glob('*.reg')}
        assert {'system.reg','user.reg','userdef.reg'}<=pins.keys()
        shutil.copytree(prepared_wine_prefix,scratch/'wine',symlinks=True)
        assert pins=={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in prepared_wine_prefix.glob('*.reg')}
        state['wine_initialization_reference']=dict(path=str(prepared_wine_prefix),registry_sha256=pins,
            private_copy=True,source_prefix_not_stopped_or_modified=True)

    def compiler(name,args):
        with (scratch/(name+'.log')).open('w') as log:
            command=['wine','tool_fast.exe','-batch',*args]
            result=subprocess.run(command,cwd=kit,env=environment,stdout=log,stderr=subprocess.STDOUT,
                timeout=bake_timeout_seconds if name=='lightmaps-final' else 600)
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
        scenery=static_scenery(package)
        if scenery:
            from deserted_static_scenery import install_static_wreck
            state['static_scenery']=install_static_wreck(kit,scenery['prefab_manifest'],
                interface/'tag_interface',evidence/'static-scenery-final.json')
            progress('Qualified permanent wreck source installed')
        if (package/'native-material-variants.json').exists():
            compiler('bitmaps-final',['bitmaps',r'scenarios\multi\dust2_mombasa\bitmaps'])
            for row in json.loads((package/'native-material-variants.json').read_text())['variants']:
                bitmap=kit/'tags'/(row['bitmap'].replace('\\','/')+'.bitmap')
                assert bitmap.is_file() and bitmap.stat().st_size>1000, ('Bitmap import produced no tag',str(bitmap))
        ass=kit/'data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass'
        assemble(package,ass,evidence/'assembly-final.json',6000,True,set(),{},True)
        validate(package,ass,evidence/'assembly-validation-final.json')
        progress('Private authoring inputs validated')
        if resumed_state:
            target=kit/'tags/scenarios/multi/dust2_mombasa'
            shutil.copytree(resume_baked_inputs/'tags',target,dirs_exist_ok=True)
            original_bsp=resume_baked_inputs/'original-classic-bsp.scenario_structure_bsp'
            if original_bsp.exists():
                bsps=list(target.glob('*.scenario_structure_bsp'));assert len(bsps)==1
                shutil.copyfile(original_bsp,bsps[0])
            validate_classic(kit,interface/'tag_interface',evidence/'collision-resumed-final.json')
            progress('Pinned baked tags restored; photon bake reused without changes')
        else:
            text=compiler('structure-final',['structure-new-from-ass',r'scenarios\multi\dust2_mombasa\structure\dust2_mombasa.ass','no'])
            assert 'saving imported structure' in text
            validate_classic(kit,interface/'tag_interface',evidence/'collision-final.json')
            progress('BSP compiled; baking '+quality+' lightmaps')
            if lightmap_workers==1:
                compiler('lightmaps-final',['lightmaps',r'scenarios\multi\dust2_mombasa\dust2_mombasa','dust2_mombasa',quality])
            else:
                from h2ek_lightmap_farm import bake_lightmaps_farm
                farm=bake_lightmaps_farm(kit,environment,r'scenarios\multi\dust2_mombasa\dust2_mombasa',
                    'dust2_mombasa',quality,lightmap_workers,bake_timeout_seconds,evidence/'lightmap-farm-final',
                    accepted_error=stock_error,progress=progress)
                assert farm['passed'] and farm['private_workers_removed']
                state['lightmap_farm']=dict(passed=True,workers=lightmap_workers,
                    report=str(evidence/'lightmap-farm-final/farm.json'),private_workers_removed=True)
        ltmp=kit/'tags/scenarios/multi/dust2_mombasa/dust2_mombasa_dust2_mombasa_lightmap.scenario_structure_lightmap'
        assert ltmp.is_file() and ltmp.stat().st_size>1000
        state['lightmap']=dict(bytes=ltmp.stat().st_size,sha256=hashlib.sha256(ltmp.read_bytes()).hexdigest())
        if resumed_state:assert state['lightmap']==resumed_state['lightmap']
        presentation=package/'native-sky-presentation.json'
        if presentation.exists() and 'post_bake_model_yaw_degrees' in json.loads(presentation.read_text()):
            configure(kit,package,interface/'tag_interface',evidence/'sky-presentation-final.json',
                False,True,'district',post_bake_presentation_only=True)
            assert hashlib.sha256(ltmp.read_bytes()).hexdigest()==state['lightmap']['sha256']
        checkpoint=output_root/'baked-inputs';checkpoint.mkdir(exist_ok=bool(resumed_state))
        assert authoring_identity(package,main_hud)==source_identity, 'Authoring inputs changed while the compiler was running'
        shutil.copytree(kit/'tags/scenarios/multi/dust2_mombasa',checkpoint/'tags',dirs_exist_ok=bool(resumed_state))
        def retain_baked_inputs():
            pins={str(p.relative_to(checkpoint)):dict(bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest())
                for p in checkpoint.rglob('*') if p.is_file() and p.name!='retention.json'}
            (checkpoint/'retention.json').write_text(json.dumps(dict(schema='H2_DESERTED_REQUIRED_BAKED_INPUTS_V1',
                files=pins,authoring_inputs=source_identity,quality=quality,lightmap=state['lightmap'],
                reason='Required real baked inputs until native publication and review complete'),indent=2)+'\n')
        retain_baked_inputs()
        state['baked_source_checkpoint']=str(checkpoint)
        mixed_manifest=package/'mixed-geometry-manifest.json'
        reference_authoring=None
        if mixed_manifest.exists() and json.loads(mixed_manifest.read_text()).get('collision_two_sided'):
            from deserted_collision_metadata import author_saved_references
            bsps=list((kit/'tags/scenarios/multi/dust2_mombasa').glob('*.scenario_structure_bsp'))
            assert len(bsps)==1
            shutil.copyfile(bsps[0],checkpoint/'original-classic-bsp.scenario_structure_bsp')
            reference_authoring=author_saved_references(kit,interface/'tag_interface',package,
                evidence/'collision-reference-authoring-final.json')
            assert reference_authoring['passed']
            assert hashlib.sha256(ltmp.read_bytes()).hexdigest()==state['lightmap']['sha256']
            shutil.copyfile(bsps[0],checkpoint/'tags'/bsps[0].name)
            retain_baked_inputs()
            state['collision_reference_authoring']=reference_authoring
            progress('Source-bound collision lighting references authored; baked geometry and lighting preserved')
        progress(quality.capitalize()+' lightmaps saved; building cache')
        compiler('cache-final',['build-cache-file',r'scenarios\multi\dust2_mombasa\dust2_mombasa','win64'])
        source=kit/'h2_maps_win64_dx11/dust2_mombasa.map'
        assert source.is_file() and source.stat().st_size>1000000
        shutil.copyfile(source,checkpoint/'dust2_mombasa.map')
        retain_baked_inputs()
        if reference_authoring:
            from deserted_collision_metadata import qualify_compiled_references
            state['compiled_collision_references']=qualify_compiled_references(kit,
                interface/'tag_interface',package,source,evidence/'compiled-collision-references-final.json')
            assert state['compiled_collision_references']['passed']
        progress('Real baked cache and authoring inputs pinned before native postprocessing')
        native=scratch/'native'
        publish(source,None,imported,native,evidence/'native-preparation-final.json',main_hud,
            compiled_lighting=True,output_name='deserted')
        if lightmap_postprocess=='stitch':
            native=finalize_lightmaps(native,imported,main_hud,evidence,scratch)
            progress('Baked chart seams stitched; compiled atlas continuity verified')
        else:
            progress('Raw compiled lighting retained for source-material Blender authoring; visual review pending')
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
    p.add_argument('--prepared-wine-prefix',type=Path)
    p.add_argument('--bake-timeout-seconds',type=int,default=7200)
    p.add_argument('--lightmap-workers',type=int,choices=(1,2),default=1)
    p.add_argument('--lightmap-postprocess',choices=('stitch','raw'),default='stitch')
    p.add_argument('--resume-baked-inputs',type=Path)
    build(**vars(p.parse_args()))
